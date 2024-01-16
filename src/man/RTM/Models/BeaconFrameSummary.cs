namespace RTM.Models;

public record BeaconFrameSummary(
    string Station,
    dBm    Power,
    int    Count,
    ushort Channel,
    string Encryption,
    string Cipher,
    string Auth,
    string SSID,
    Mbps   DataRate
);