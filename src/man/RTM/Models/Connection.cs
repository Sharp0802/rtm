using System;

namespace RTM.Models;

public readonly record struct Connection(
    MAC Peer0,
    MAC Peer1)
{
    public bool Equals(Connection? other)
    {
        return other is not null &&
               ((other.Value.Peer0.Equals(Peer0) && other.Value.Peer1.Equals(Peer1)) ||
                (other.Value.Peer0.Equals(Peer1) && other.Value.Peer0.Equals(Peer1)));
    }

    public override int GetHashCode()
    {
        return HashCode.Combine(Peer0, Peer1);
    }
}
