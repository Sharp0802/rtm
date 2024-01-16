using System.Runtime.InteropServices;
using RTM.Models;

namespace RTM;

internal unsafe partial class Native
{
    private const string Library = "librtm.so";

    [LibraryImport(Library)]
    internal static partial byte* GetVersionString();

    [LibraryImport(Library)]
    internal static partial byte* GetLastErrorString();

    [LibraryImport(Library)]
    internal static partial int Run(
        int                                               mode,
        byte*                                             target,
        byte*                                             token,
        delegate* unmanaged[Cdecl]<Context*, void*, void> callback,
        void*                                             arg
    );

    [LibraryImport(Library)]
    internal static partial void ReleaseContext(Context* context);

    [LibraryImport(Library)]
    internal static partial void InspectRadiotap(Context* src);
}