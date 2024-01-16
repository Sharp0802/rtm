using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using RTM.Primitives;

namespace RTM.Models;

public sealed class ContextCache : 
    IEnumerable<Ref<Context>>, 
    IDisposable,
    IAsyncDisposable
{
    private record CacheEntry(
        DateTime     Timestamp,
        Ref<Context> Context
    );


    private readonly ConcurrentQueue<CacheEntry> _queue = new();
    private readonly CancellationTokenSource     _cts   = new();
    private readonly Task                        _runner;
    
    public TimeSpan Timeout { get; set; } = TimeSpan.FromSeconds(10);

    public ContextCache()
    {
        _runner = Task.Factory.StartNew(() => Flush(_cts.Token));
    }
    

    public void Dispose()
    {
        _cts.Cancel();
        _runner.GetAwaiter().GetResult();
    }

    public async ValueTask DisposeAsync()
    {
        await _cts.CancelAsync();
        await _runner;
    }


    private void Flush(CancellationToken token)
    {
        try
        {
            while (!token.IsCancellationRequested)
            {
                CacheEntry? entry;
                while (!_queue.TryPeek(out entry) && !token.IsCancellationRequested) 
                    Task.Delay(10, token).Wait(token);

                if (entry is null)
                    return;
        
                var diff = DateTime.Now - entry.Timestamp;
                if (diff <= Timeout)
                    continue;
            
                Task.Delay(diff - Timeout, token).Wait(token);

                _queue.TryDequeue(out _);
                unsafe
                {
                    Native.ReleaseContext(entry.Context);
                }
            }
        }
        catch (TaskCanceledException)
        {
        }
    }


    public void Add(Ref<Context> item)
    {
        _queue.Enqueue(new CacheEntry(DateTime.Now, item));
    }

    public void Clear()
    {
        foreach (var (_, context) in _queue)
        {
            unsafe
            {
                Native.ReleaseContext(context);
            }
        }

        _queue.Clear();
    }
    
    
    public IEnumerator<Ref<Context>> GetEnumerator()
    {
        return _queue.ToArray().Select(p => p.Context).GetEnumerator();
    }

    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
}