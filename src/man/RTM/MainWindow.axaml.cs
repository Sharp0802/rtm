using System;
using System.Runtime.InteropServices;
using Avalonia.Controls;
using RTM.ViewModels;

namespace RTM;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        
        unsafe
        {
            Title = $"RTM (rtm) {Marshal.PtrToStringUTF8((IntPtr)Native.GetVersionString())}";
        }
    }

    private void OnSelectionChanged(object? _, SelectionChangedEventArgs e)
    {
        if (e.AddedItems.Count <= 0 ||
            e.AddedItems[0] is not string target)
            return;

        ((MainWindowViewModel?)DataContext)?.SummaryViewModel.ChangeTarget(target);
    }
}