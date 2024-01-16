using System;
using RTM.Models;

namespace RTM.Extensions;

[Flags]
public enum CipherSuiteSymbol
{
    None   = 0x0000,
    WEP40  = 0x0001,
    TKIP   = 0x0002,
    WRAP   = 0x0004,
    WEP104 = 0x0008,
    WPA2   = 0x0010
}

[Flags]
public enum AuthKeyManagementSymbol
{
    None = 0x0000,
    SAE  = 0x0001,
    MGT  = 0x0002,
    CMAC = 0x0004,
    PSK  = 0x0008,
    OWE  = 0x0010,
    FT   = 0x0020,
}

public static class CipherSuiteExtension
{
    private static AuthKeyManagementSymbol ReadAKM(this RSN rsn)
    {
        unsafe
        {
            var flags = AuthKeyManagementSymbol.None;
            for (var i = 0; i < rsn.V1.AuthKeyManagementSuiteCount; ++i)
                flags |= rsn.V1.AuthKeyManagementList[i].Type switch
                {
                    1       => AuthKeyManagementSymbol.MGT,
                    2       => AuthKeyManagementSymbol.PSK,
                    3       => AuthKeyManagementSymbol.FT,
                    6 or 13 => AuthKeyManagementSymbol.CMAC,
                    8       => AuthKeyManagementSymbol.SAE,
                    18      => AuthKeyManagementSymbol.OWE,
                    _       => AuthKeyManagementSymbol.None
                };
            return flags;
        }
    }

    public static string ToCipherSuiteString(this RSN rsn)
    {
        var flags = CipherSuiteSymbol.None;
        for (var i = 0; i < rsn.V1.PairwiseCipherSuiteCount; ++i)
            flags |= rsn.V1.GroupCipherSuite.Type switch
            {
                1                       => CipherSuiteSymbol.WEP40,
                2                       => CipherSuiteSymbol.TKIP,
                3                       => CipherSuiteSymbol.WRAP,
                5                       => CipherSuiteSymbol.WEP104,
                4 or 8 or 9 or 11 or 12 => CipherSuiteSymbol.WPA2,
                _                       => CipherSuiteSymbol.None
            };

        if ((flags & CipherSuiteSymbol.WPA2) != 0)
        {
            var akm = ReadAKM(rsn);
            return (akm & AuthKeyManagementSymbol.SAE) != 0 ||
                   (akm & AuthKeyManagementSymbol.OWE) != 0
                ? "WPA3"
                : "WPA2";
        }
        else if (flags != CipherSuiteSymbol.None)
        {
            return flags.ToString(); // TODO
        }
        else
        {
            return "<UNK>";
        }
    }

    public static string ToAuthKeyManagementString(this RSN rsn)
    {
        var flags = ReadAKM(rsn);
        return flags.ToString(); // TODO
    }
}