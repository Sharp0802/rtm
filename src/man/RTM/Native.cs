using System.Runtime.InteropServices;
using RTM.Models;

namespace RTM;

internal unsafe partial class Native
{
    private const string Library = "librtm.so";

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void Callback(Context* context);

    [LibraryImport(Library)]
    internal static partial byte* GetVersionString();

    [LibraryImport(Library)]
    internal static partial byte* GetLastErrorString();

    [LibraryImport(Library)]
    internal static partial int Run(
        int                                        mode,
        byte*                                      target,
        byte*                                      token,
        delegate* unmanaged[Cdecl]<Context*, void> callback
    );

    [LibraryImport(Library)]
    internal static partial void ReleaseContext(Context* context);
    
    [LibraryImport(Library)]
    internal static partial void InspectRadiotap(Context* src);
}