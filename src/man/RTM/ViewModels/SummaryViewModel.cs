using System;
using System.Collections.Concurrent;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Threading;
using System.Windows.Input;
using Avalonia.Controls;
using Avalonia.Threading;
using ReactiveUI;
using RTM.Models;
using RTM.Service;

namespace RTM.ViewModels;

public record BeaconFrameSummary(
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

public record DataFrameSummary(
    MAC   Peer0,
    MAC   Peer1,
    uint  Count,
    nuint Length
);

public class SummaryViewModel : ViewModelBase
{
    private readonly ConcurrentDictionary<MAC, BeaconContext>                     _beacons = new();
    private readonly ConcurrentDictionary<Connection, (uint Count, nuint Length)> _data    = new();

    private ObservableCollection<BeaconFrameSummary> _observableBeacons = new();
    private ObservableCollection<DataFrameSummary>   _observableData    = new();

    private Timer _renderer;
    

    public ObservableCollection<BeaconFrameSummary> BeaconFrames
    {
        get => _observableBeacons;
        set => this.RaiseAndSetIfChanged(ref _observableBeacons, value);
    }

    public ObservableCollection<DataFrameSummary> DataFrames
    {
        get => _observableData;
        set => this.RaiseAndSetIfChanged(ref _observableData, value);
    }


    public SummaryViewModel()
    {
        _renderer = new Timer(_ => Render(), null, TimeSpan.Zero, TimeSpan.FromMilliseconds(100));
    }

    ~SummaryViewModel()
    {
        _renderer.Dispose();
    }


    private void Render()
    {
        Dispatcher.UIThread.Post(() =>
        {
            RenderBeaconFrame();
            RenderDataFrame(); 
        });
    }

    public void Start()
    {
        SummarizeService.Instance.Start();
    }

    public void Stop()
    {
        SummarizeService.Instance.Stop();
    }

    public void ChangeTarget(string target)
    {
        SummarizeService.Instance.ChangeTarget(target);
    }

    private void RenderBeaconFrame()
    {
        var lst = _beacons.Values.Select(v =>
        {
            const string unknown = "<unknown>";
            
            unsafe
            {
                if (v.Context is null || v.Context->Trailer is null)
                    return null;

                var rsn = v.Context->Trailer->BeaconFrame.RSN;
                
                CipherSuites?            gcs;
                AuthKeyManagementSuites? akm;
                
                switch (rsn.Version)
                {
                    case 1:
                        gcs = *(CipherSuites*)rsn.V1.GroupCipherSuite;
                        akm = *(AuthKeyManagementSuites*)rsn.V1.AuthKeyManagementList;
                        break;

                    default:
                        gcs = null;
                        akm = null;
                        break;
                }

                return new BeaconFrameSummary(
                    v.Context->Trailer->BeaconFrame.MACHeader.Address2.ToString(),
                    v.Context->AntennaSignal,
                    v.Count,
                    v.Context->Channel.Frequency,
                    unknown,
                    gcs?.ToString() ?? unknown,
                    akm?.ToString() ?? unknown,
                    v.Context->Trailer->BeaconFrame.SSID.ToString(),
                    v.Context->DataRate
                );
            }
        }).Where(v => v is not null).ToObservable().ToListObservable();
        Console.WriteLine(lst.Count);
        MainWindow.Instance.BeaconGrid.ItemsSource = lst;
    }

    private void RenderDataFrame()
    {
        MainWindow.Instance.DataGrid.ItemsSource = _data.Select(pair =>
            new DataFrameSummary(
                pair.Key.Peer0,
                pair.Key.Peer1,
                pair.Value.Count,
                pair.Value.Length
            )
        ).ToObservable().ToListObservable();
    }

    public unsafe BeaconContext UpdateBeacon(Context* context)
    {
        Console.WriteLine("aaa");
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

    public unsafe (uint Count, nuint Length) UpdateData(Context* context)
    {
        return _data.AddOrUpdate(
            new Connection(
                context->Trailer->DataFrame.MACHeader.Address1,
                context->Trailer->DataFrame.MACHeader.Address0),
            (1, context->Trailer->DataFrame.Length),
            (_, value) => (value.Count + 1, value.Length + context->Trailer->DataFrame.Length)
        );
    }

    private void ClearData()
    {
        _data.Clear();
    }

    public void Clear()
    {
        ClearBeacon();
        ClearData();
    }
}