using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Net.NetworkInformation;
using System.Reactive;
using System.Reactive.Linq;
using System.Threading;
using Avalonia.Threading;
using DynamicData;
using ReactiveUI;
using RTM.Extensions;
using RTM.Models;
using RTM.Repositories;


namespace RTM.ViewModels;

public class SummaryViewModel : ViewModelBase
{
    private const int FromFile      = 1;
    private const int FromInterface = 2;

    private const uint XChannelFlag = 1 << 18;

    private string?            _target;
    private ContextRepository? _repo;

    private byte   _channel;
    private string _channelText = string.Empty;
    
    private Timer _renderer;
    private Timer _tuner;

    public ObservableCollection<BeaconFrameSummary> BeaconFrames { get; } = [];
    public ObservableCollection<DataFrameSummary>   DataFrames   { get; } = [];

    public bool Alive
    {
        get => _repo?.Alive ?? false;
        set => this.RaisePropertyChanged();
    }

    public string ChannelText
    {
        get => _channelText;
        set => this.RaiseAndSetIfChanged(ref _channelText, value);
    }

    public ObservableCollection<string> Interfaces { get; } = [];


    public SummaryViewModel()
    {
        UpdateAdapters();

        _renderer = new Timer(_ =>
        {
            UpdateBeaconGrid();
            UpdateDataGrid();
        }, null, 0, 100);

        _tuner = new Timer(_ =>
        {
            ChangeChannel();
        }, null, 0, 300);
    }

    private void ChangeChannel()
    {
        if (_repo is null || _target is null || !Alive)
            return;
        
        _channel    += 5;
        _channel    %= 16;
        ChannelText =  $"CH:{_channel}";

        var       info = new ProcessStartInfo("iw", ["dev", _target, "set", "channel", _channel.ToString()]);
        using var proc = Process.Start(info);
        proc?.WaitForExit(500);
    }

    private void UpdateAdapters()
    {
        Interfaces.UpdateTo(
            NetworkInterface
               .GetAllNetworkInterfaces()
               .Select(nic => nic.Name),
            s => s);
    }

    private unsafe void UpdateBeaconGrid()
    {
        if (_repo is null)
            return;

        var lst = _repo.Cache
                       .Select(r => r.Dereference())
                       .Where(context => context.Trailer is not null &&
                                         context.Trailer->FrameType == FrameType.Beacon)
                       .GroupBy(context => context.Trailer->BeaconFrame.MACHeader.Address2)
                       .Select(group =>
                        {
                            var context = group.First();
                            var rsn     = context.Trailer->BeaconFrame.RSN;

                            var akm = rsn.V1.AuthKeyManagementList is null 
                                ? "<UNK>" 
                                : rsn.ToAuthKeyManagementString();
                            
                            return new BeaconFrameSummary(
                                group.Key.ToString(),
                                context.AntennaSignal,
                                group.Count(),
                                context.Trailer->BeaconFrame.Channel,
                                "<unknown>",
                                rsn.ToCipherSuiteString(),
                                akm,
                                context.Trailer->BeaconFrame.SSID.ToString(),
                                context.DataRate
                            );
                        });

        Dispatcher.UIThread.Invoke(() => { BeaconFrames.UpdateTo(lst, f => f); });
    }

    private unsafe void UpdateDataGrid()
    {
        if (_repo is null)
            return;

        var lst = _repo.Cache
                       .Select(context => context.Dereference())
                       .Where(context => context.Trailer is not null &&
                                         context.Trailer->FrameType == FrameType.Data)
                       .GroupBy(context =>
                        {
                            var mh = context.Trailer->DataFrame.MACHeader;
                            return new Connection(mh.Address0, mh.Address1);
                        })
                       .Select(group =>
                        {
                            var length = group.Sum(v => (long)v.Trailer->DataFrame.Length);
                            return new DataFrameSummary(group.Key.Peer0, group.Key.Peer1, group.Count(), length);
                        });

        Dispatcher.UIThread.Invoke(() => { DataFrames.UpdateTo(lst, f => f); });
    }

    public void Start()
    {
        if (_target is null)
            return;

        _repo = new ContextRepository(FromInterface, _target);

        _repo.PropertyChanged += (sender, e) =>
        {
            if (!ReferenceEquals(sender, _repo))
                return;
            if (e.PropertyName is null || !e.PropertyName.Equals(nameof(_repo.Alive)))
                return;

            Alive = _repo.Alive;
        };

        _repo.Start();
    }

    public async void Stop()
    {
        if (_repo is null)
            return;

        _repo.Stop();
        await _repo.DisposeAsync();

        UpdateAdapters();
    }


    public void ChangeTarget(string target)
    {
        _target = target;
    }
}