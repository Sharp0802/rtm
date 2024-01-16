using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Threading;
using MsBox.Avalonia;
using RTM.Models;

namespace RTM.Repositories;

public sealed class ContextRepository : IDisposable, IAsyncDisposable, INotifyPropertyChanged
{
    private static ContextRepository? _instance;

    private          bool   _alive;
    private          byte   _token;
    private readonly int    _mode;
    private readonly string _target;

    public ContextRepository(int mode, string target)
    {
        _instance = this;

        _mode   = mode;
        _target = target;
    }

    public bool Alive
    {
        get => _alive;
        set => SetField(ref _alive, value);
    }

    public ContextCache Cache { get; } = new();


    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    private static unsafe void Callback(Context* context, void* _)
    {
#if DEBUG
        if (context->Trailer is not null && context->Trailer->FrameType == FrameType.Beacon)
            Native.InspectRadiotap(context);
#endif
        _instance?.Cache.Add(context);
    }

    public void Start()
    {
        Alive = true;
        Task.Factory.StartNew(() =>
        {
            Cache.Clear();

            _token = 1;

            var tar = new byte[Encoding.UTF8.GetByteCount(_target) + 1];
            tar[Encoding.UTF8.GetBytes(_target, tar)] = 0;

            unsafe
            {
                int ret;
                fixed (byte* token = &_token)
                fixed (byte* p = tar)
                    ret = Native.Run(_mode, p, token, &Callback, null);

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
                       .ShowAsync();
                });

                Stop();
            }
        }).ConfigureAwait(false);
    }

    public void Stop()
    {
        Alive  = false;
        _token = 0;
    }


    public void Dispose()
    {
        Stop();
        Cache.Dispose();
    }

    public async ValueTask DisposeAsync()
    {
        await Cache.DisposeAsync();
    }


    public event PropertyChangedEventHandler? PropertyChanged;

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    private bool SetField<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value))
            return false;
        field = value;
        OnPropertyChanged(propertyName);
        return true;
    }
}