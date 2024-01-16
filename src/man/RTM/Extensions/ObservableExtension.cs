using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using DynamicData;

namespace RTM.Extensions;

public static class ObservableExtension
{
    public static void UpdateTo<TTarget, TKey>(
        this ObservableCollection<TTarget> target, 
        IEnumerable<TTarget>               to,
        Func<TTarget, TKey>                selector)
    {
        var arr = to as TTarget[] ?? to.ToArray();
        target.AddRange(arr.ExceptBy(target.Select(selector), selector));
        target.RemoveMany(target.ExceptBy(arr.Select(selector), selector));
    }
}