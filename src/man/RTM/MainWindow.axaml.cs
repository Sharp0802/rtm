using System;
using System.Linq;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using Avalonia.Controls;
using Avalonia.Interactivity;
using RTM.Models;
using RTM.ViewModels;

namespace RTM;

public unsafe class BeaconContext(Context* context, uint count)
{
    public Context* Context { get; } = context;
    public uint     Count   { get; } = count;
}

public partial class MainWindow : Window
{
    public const string FileSelectEntry = "From PCAP dump";
    
    public static MainWindow Instance { get; private set; }

    public MainWindow()
    {
        Instance = this;
        
        InitializeComponent();
        Design.SetDataContext(this, new MainWindowViewModel());
        
        unsafe
        {
            Title = $"RTM (rtm) {Marshal.PtrToStringUTF8((IntPtr)Native.GetVersionString())}";
        }

        UpdateAdapters();
        
        
        StartButton.IsEnabled       = true;
        StopButton.IsEnabled        = false;
        InterfaceSelector.IsEnabled = true;
    }

    private void UpdateAdapters()
    {
        InterfaceSelector.ItemsSource = NetworkInterface
                                       .GetAllNetworkInterfaces()
                                       .Select(nic => nic.Name)
                                       .Prepend(FileSelectEntry);
    }

    private void OnSelectionChanged(object? _, SelectionChangedEventArgs e)
    {
        if (e.AddedItems.Count <= 0 ||
            e.AddedItems[0] is not string target)
            return;

        FileNameField.IsEnabled = target.Equals(FileSelectEntry, StringComparison.Ordinal);

        var vm = (MainWindowViewModel)Design.GetDataContext(this);
        vm.SummaryViewModel.ChangeTarget(target);
    }

    private void StartButton_OnClick(object? sender, RoutedEventArgs e)
    {
        var vm = (MainWindowViewModel)Design.GetDataContext(this);
        vm.SummaryViewModel.Start();
    }

    private void StopButton_OnClick(object? sender, RoutedEventArgs e)
    {
        var vm = (MainWindowViewModel)Design.GetDataContext(this);
        vm.SummaryViewModel.Stop();
    }
}