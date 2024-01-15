// Copyright 2023 Yeong-won Seo
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

using System;

namespace RTM;

public record Connection(
    MAC Peer0,
    MAC Peer1)
{
    public virtual bool Equals(Connection? other)
    {
        return other is not null &&
               ((other.Peer0.Equals(Peer0) && other.Peer1.Equals(Peer1)) ||
                (other.Peer0.Equals(Peer1) && other.Peer0.Equals(Peer1)));
    }

    public override int GetHashCode()
    {
        return HashCode.Combine(Peer0, Peer1);
    }
}