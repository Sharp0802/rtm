using System.Runtime.InteropServices;
using System.Text;

namespace RTM;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public unsafe struct CipherSuiteOUI
{
    public fixed U1 Value[3];
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public unsafe struct SSID
{
    public fixed U1 Value[32];

    public override string ToString()
    {
        fixed (U1* p = Value)
        {
            int size;
            for (size = 0; size < 32 && p[size] != 0; ++size)
            {
            }

            return Encoding.UTF8.GetString(p, size);
        }
    }
}

[StructLayout(LayoutKind.Explicit, Pack = 1)]
public unsafe struct RSN
{
    [FieldOffset(0)]
    public U2 Version;

    [FieldOffset(2)]
    public RSN_1 V1;
}
