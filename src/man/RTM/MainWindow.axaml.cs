using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Net.NetworkInformation;
using System.Reactive.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Threading;
using MsBox.Avalonia;

namespace RTM;

public class Ref<T> where T : unmanaged
{
    public T Value;
}

public record BeaconSummary(
    string Station,
    dBm    Power,
    uint   Count,
    ushort Channel,
    string Encryption,
    string Cipher,
    string Auth,
    string SSID,
    Mbps   DataRate
);

public record DataSummary(
    string Peer0,
    string Peer1,
    uint   Count,
    nuint  Length
);

public enum CipherSuites
{
    CCMP   = 0x04AC0F00,
    TKIP   = 0x02AC0F00,
    WEP40  = 0x01AC0F00,
    WEP104 = 0x05AC0F00
}

public enum AuthKeyManagementSuites
{
    IEEE8021x   = 0x01AC0F00,
    PSK         = 0x02AC0F00,
    FTOver8021x = 0x03AC0F00
}

public class BeaconContext
{
    public unsafe Context* Context { get; }
    public        uint     Count   { get; }

    public unsafe BeaconContext(Context* context, uint count)
    {
        Context = context;
        Count   = count;
    }
}

public partial class MainWindow : Window
{
    private const string FileSelectEntry = "From PCAP dump";

    private const int FromFile      = 1;
    private const int FromInterface = 2;

    private Task?     _runner;
    private Ref<byte> _token = new();

    private static MainWindow? _instance;

    private readonly ConcurrentDictionary<MAC, BeaconContext> _beacons = new();

    private readonly ConcurrentDictionary<Connection, (uint Count, nuint Length)> _data = new();


    public MainWindow()
    {
        if (Interlocked.CompareExchange(ref _instance, this, null) is not null)
            throw new InvalidOperationException();

        InitializeComponent();
        unsafe
        {
            Title = $"RTM (rtm) {Marshal.PtrToStringUTF8((IntPtr)Native.GetVersionString())}";
        }

        UpdateStates();

        var renderer = new Timer(_ =>
        {
            Dispatcher.UIThread.Post(() =>
            {
                RenderBeaconGrid();
                RenderDataGrid();
            });
        }, null, TimeSpan.Zero, TimeSpan.FromSeconds(0.5));
        Unloaded += (_, __) => renderer.Dispose();
    }


    private unsafe BeaconContext UpdateBeacon(Context* context)
    {
        Console.WriteLine("update b");
        
        return _beacons.AddOrUpdate(
            context->Trailer->BeaconFrame.MACHeader.Address2,
            new BeaconContext(context, 1),
            (_, value) =>
            {
                Native.ReleaseContext(value.Context);
                return new BeaconContext(context, value.Count + 1);
            });
    }

    private unsafe void ClearBeacon()
    {
        foreach (var context in _beacons.Values)
            Native.ReleaseContext(context.Context);
        _beacons.Clear();
    }

    private unsafe (uint Count, nuint Length) UpdateData(Context* context)
    {
        Console.WriteLine("update d");
        
        return _data.AddOrUpdate(
            new Connection(
                context->Trailer->DataFrame.MACHeader.Address1,
                context->Trailer->DataFrame.MACHeader.Address0),
            (1, context->Trailer->DataFrame.Length),
            (_, value) => (value.Count + 1, value.Length + context->Trailer->DataFrame.Length)
        );
    }

    private unsafe void RenderBeaconGrid()
    {
        var lst = _beacons.Values.Select(v =>
        {
            if (v.Context is null)
                return null;
            
            var rsn = v.Context->Trailer->BeaconFrame.RSN;

            CipherSuites            gcs;
            AuthKeyManagementSuites akm;
            switch (rsn.Version)
            {
                case 1:
                    gcs = *(CipherSuites*)rsn.V1.GroupCipherSuite;
                    akm = *(AuthKeyManagementSuites*)rsn.V1.AuthKeyManagementList;
                    break;

                default:
                    return null;
            }

            return new BeaconSummary(
                v.Context->Trailer->BeaconFrame.MACHeader.Address2.ToString(),
                v.Context->AntennaSignal,
                v.Count,
                v.Context->Channel.Frequency,
                "<unknown>",
                gcs.ToString(),
                akm.ToString(),
                v.Context->Trailer->BeaconFrame.SSID.ToString(),
                v.Context->DataRate
            );
        }).Where(v => v is not null).ToObservable().ToListObservable();

        Console.WriteLine(lst.Count);
        
        BeaconGrid.ItemsSource = lst;
    }

    private void RenderDataGrid()
    {
        var lst = _data.Select(pair => new DataSummary(
            pair.Key.Peer0.ToString(),
            pair.Key.Peer1.ToString(),
            pair.Value.Count,
            pair.Value.Length
        )).ToObservable().ToListObservable();
        
        Console.WriteLine(lst.Count);
        
        DataGrid.ItemsSource = lst;
    }


    private static IEnumerable<string> EnumerateAdapters()
    {
        return NetworkInterface.GetAllNetworkInterfaces().Select(nic => nic.Name);
    }

    private void UpdateStates()
    {
        InterfaceSelector.ItemsSource = EnumerateAdapters().Append(FileSelectEntry);
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    private static unsafe void Callback(Context* context)
    {
        if (context->Trailer is null)
            return;

        //Native.InspectRadiotap(context);

        switch (context->Trailer->FrameType)
        {
            case FrameType.Beacon:
                _instance!.UpdateBeacon(context);
                break;
            case FrameType.Data:
                _instance!.UpdateData(context);
                break;
        }
    }

    private void Run(int mode, string target)
    {
        unsafe
        {
            ClearBeacon();
            _data.Clear();

            _token.Value = 1;

            var ifname = Encoding.UTF8.GetBytes(target).AsSpan();
            var ret = Native.Run(
                mode,
                (byte*)Unsafe.AsPointer(ref ifname.GetPinnableReference()),
                (byte*)Unsafe.AsPointer(ref _token),
                &Callback);

            Dispatcher.UIThread.Post(() =>
            {
                MessageBoxManager
                   .GetMessageBoxStandard(
                        ret == 0
                            ? "Info"
                            : "Error",
                        ret == 0
                            ? "Service exited (0)"
                            : Marshal.PtrToStringUTF8((IntPtr)Native.GetLastErrorString()) ?? "<empty>")
                   .ShowAsPopupAsync(this);
            });
        }
    }

    private Task RunAsync(int mode, string target)
    {
        return Task.Factory.StartNew(() => Run(mode, target));
    }

    private void OnSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (e.AddedItems.Count <= 0 ||
            e.AddedItems[0] is not string target)
            return;

        FileNameField.IsEnabled = target.Equals(FileSelectEntry, StringComparison.Ordinal);
    }

    private void Start(object? sender, RoutedEventArgs e)
    {
        if (InterfaceSelector.SelectedItem is not string target)
        {
            MessageBoxManager
               .GetMessageBoxStandard(
                    "Warning",
                    "A network interface not selected")
               .ShowAsPopupAsync(this);

            return;
        }

        var mode = FromInterface;
        if (target.Equals(FileSelectEntry, StringComparison.Ordinal))
        {
            mode   = FromFile;
            target = FileNameField.Text ?? string.Empty;
        }

        _runner = RunAsync(mode, target).ContinueWith(_ =>
            Dispatcher.UIThread.Post(() =>
                Stop(this, new RoutedEventArgs())
            )
        );

        InterfaceSelector.IsEnabled = false;
        StartButton.IsEnabled       = false;
        StopButton.IsEnabled        = true;
    }

    private void Stop(object? sender, RoutedEventArgs e)
    {
        InterfaceSelector.IsEnabled = true;
        StartButton.IsEnabled       = true;
        StopButton.IsEnabled        = false;
        _token.Value                = 0;
        UpdateStates();
    }
}