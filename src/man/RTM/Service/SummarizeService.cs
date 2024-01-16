using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Threading;
using MsBox.Avalonia;
using RTM.Models;
using RTM.ViewModels;

namespace RTM.Service;

public enum SummarizeServiceState
{
    Suspended,
    Running
}

public class SummarizeService
{
    private const int FromFile      = 1;
    private const int FromInterface = 2;
    
    
    private static readonly Lazy<SummarizeService> LazyInstance = new();

    public static SummarizeService Instance => LazyInstance.Value;

    
    private SummarizeServiceState _state = SummarizeServiceState.Suspended;
    private string?               _target;
    private Task?                 _runner;
    private byte                  _token;

    public SummaryViewModel? ViewModel { get; set; }
    
    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    private static unsafe void Callback(Context* context)
    {
        if (context->Trailer is null)
            return;
        
        switch (context->Trailer->FrameType)
        {
            case FrameType.Beacon:
                Instance.ViewModel?.UpdateBeacon(context);
                break;
            case FrameType.Data:
                Instance.ViewModel?.UpdateData(context);
                break;
        }
    }

    public void Start()
    {
        Console.WriteLine("Start requested");
        
        _state = SummarizeServiceState.Running;
        _runner = Task.Factory.StartNew(() =>
        {
            //ViewModel!.Clear();
            
            _token = 1;
            
            var mode = _target!.Equals(MainWindow.FileSelectEntry, StringComparison.Ordinal) 
                ? FromFile 
                : FromInterface;
            
            var target = new byte[Encoding.UTF8.GetByteCount(_target) + 1];
            target[Encoding.UTF8.GetBytes(_target, target)] = 0;

            unsafe
            {
                int ret;
                fixed (byte* token = &_token)
                fixed (byte* p = target)
                    ret = Native.Run(mode, p, token, &Callback);   

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
        });

        Dispatcher.UIThread.Post(() =>
        {
            MainWindow.Instance.StartButton.IsEnabled       = false;
            MainWindow.Instance.StopButton.IsEnabled        = true;
            MainWindow.Instance.InterfaceSelector.IsEnabled = false; 
        });
    }

    public void Stop()
    {
        Console.WriteLine("Stop requested");
        
        _state = SummarizeServiceState.Suspended;
        _token = 0;

        Dispatcher.UIThread.Post(() =>
        {
            MainWindow.Instance.StartButton.IsEnabled       = true;
            MainWindow.Instance.StopButton.IsEnabled        = false;
            MainWindow.Instance.InterfaceSelector.IsEnabled = true;
        });
    }

    public void ChangeTarget(string target)
    {
        Console.WriteLine("Selection changed 2");
        _target = target.Equals(MainWindow.FileSelectEntry, StringComparison.Ordinal) 
            ? MainWindow.Instance.FileNameField.Text 
            : target;
    }
}
