namespace RTM.Models;

public record DataFrameSummary(
    MAC   Peer0,
    MAC   Peer1,
    int   Count,
    long  Length
);