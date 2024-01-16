global using Mbps = float;
global using dBm = sbyte;
global using dB = ushort;
using System;
using System.Runtime.InteropServices;

namespace RTM.Models;

public enum CipherSuites : uint
{
    CCMP   = 0x04AC0F00,
    TKIP   = 0x02AC0F00,
    WEP40  = 0x01AC0F00,
    WEP104 = 0x05AC0F00
}

public enum AuthKeyManagementSuites : uint
{
    IEEE8021X   = 0x01AC0F00,
    PSK         = 0x02AC0F00,
    FTOver8021X = 0x03AC0F00
}

public enum FrameType : byte
{
    None,
    Beacon,
    Data
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public unsafe struct CipherSuiteOUI
{
    public fixed byte Value[3];
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public unsafe struct MAC : IEquatable<MAC>
{
    public fixed byte Value[6];

    public override string ToString()
    {
        return $"{Value[0]:X2}:{Value[1]:X2}:{Value[2]:X2}:{Value[3]:X2}:{Value[4]:X2}:{Value[5]:X2}";
    }

    public bool Equals(MAC other)
    {
        for (var i = 0; i < 6; ++i)
            if (Value[i] != other.Value[i])
                return false;
        return true;
    }

    public override bool Equals(object? obj)
    {
        return obj is MAC other && Equals(other);
    }

    public override int GetHashCode()
    {
        int r;
        fixed (byte* p = Value)
            r = Marshal.ReadInt32((IntPtr)p, 2);
        return r;
    }
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct MACHeader
{
    public ushort FrameControl;
    public ushort Duration;
    public MAC    Address0;
    public MAC    Address1;
    public MAC    Address2;
    public ushort SequenceControl;
}

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public unsafe struct SSID
{
    public fixed byte Value[32];

    public override string ToString()
    {
        fixed (byte* p = Value)
            return Marshal.PtrToStringUTF8((IntPtr)p)!;
    }
}

[StructLayout(LayoutKind.Explicit, Pack = 1)]
public struct RSN
{
    [FieldOffset(0)]
    public ushort Version;

    [FieldOffset(2)]
    public RSN_1 V1;
}

[StructLayout(LayoutKind.Explicit, Pack = 1)]
public struct Timestamp
{
    [FieldOffset(0)]
    public ulong Timestamp_;

    [FieldOffset(8)]
    public ushort Precision;

    [FieldOffset(10)]
    public byte Unit;

    [FieldOffset(10)]
    public byte Position;

    [FieldOffset(11)]
    public byte Flags;
}

[StructLayout(LayoutKind.Explicit, Pack = 1)]
public struct ContextTrailer
{
    [FieldOffset(0)]
    public FrameType FrameType;

    [FieldOffset(1)]
    public BeaconFrame BeaconFrame;

    [FieldOffset(1)]
    public DataFrame DataFrame;
}