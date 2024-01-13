#include "radiotap.h"
#include "x86intrin.h"

#define F_SN(n)    (1U << n)
#define F_ENTRY(f) HAS_F(flags,f),B_STR(HAS(flags,f))
#define FS_ENTRY(flags, f) HAS_F(flags,f),B_STR(HAS(flags,f))

typedef struct
{
    U2 Type;
    U2 Length;
} __field__ TLV;

static const Field s_Reserved = RESERVED;

size_t GetFieldSize(U1 bit, Pointer src)
{
    if ((1 << bit) != RT_TLV)
	return g_Fields[bit].Size;
    
    /* TLVs are aligned in 4B */
    U2 length = ((TLV*)src.V)->Length;
    if (length & 3)
	length += 4 - (length & 3);
    
    return sizeof(TLV) + length;
}

void ParseField(U1 bit, const Pointer src, Context* dst)
{
    Field field = g_Fields[bit];
    if (memcmp(&field, &s_Reserved, sizeof field) == 0)
	return;
    
    switch (1 << bit)
    {
    case RT_TSFT:
    {
	dst->TSFT = *src.U8;
	break;
    }
    
    case RT_FLAGS:
    {
	dst->Flags = *src.U1;
	break;
    }
    
    case RT_RATE:
    {
	dst->DataRate = (float)(*src.U1) * 0.5f;
	break;
    }
    
    case RT_CH:
    {
	dst->Channel = *(Channel*)src.V;
	break;
    }
    
    case RT_FHSS:
    {
	dst->FHSS = *(FHSS*)src.V;
	break;
    }
    
    case RT_DBM_ANT_SIGNAL:
    {
	dst->AntennaSignal = *src.S1;
	break;
    }
    
    case RT_DBM_ANT_NOISE:
    {
	dst->AntennaNoise = *src.S1;
	break;
    }
    
    case RT_LOCK_QUALITY:
    {
	dst->LockQuality = *src.U2;
	break;
    }
    
    case RT_TX_ATTENUATION:
    {
	dst->TXAttenuation = *src.U2;
	break;
    }
    
    case RT_DB_TX_ATTENUATION:
    {
	dst->TXAttenuationDB = *src.U2;
	break;
    }
    
    case RT_DBM_TX_POWER:
    {
	dst->TXPower = *src.S1;
	break;
    }
    
    case RT_ANT:
    {
	dst->Antenna = *src.U1;
	break;
    }
    
    case RT_DB_ANT_SIGNAL:
    {
	dst->AntennaSignalDB = *src.U1;
	break;
    }
    
    case RT_DB_ANT_NOISE:
    {
	dst->AntennaNoiseDB = *src.U1;
	break;
    }
    
    case RT_RX_FLAGS:
    {
	dst->RXFlags = *src.U2;
	break;
    }
    
    case RT_TX_FLAGS:
    {
	dst->TXFlags = *src.U2;
	break;
    }
    
    case RT_DATA_RETRIES:
    {
	dst->Retries = *src.U1;
	break;
    }
    
    case RT_CH_EXT:
    {
	dst->XChannel = *(XChannel*)src.V;
	break;
    }
    
    case RT_MCS:
    {
	dst->MCS = *(MCS*)src.V;
	break;
    }
    
    case RT_AMPDU:
    {
	dst->AMPDU = *(AMPDU*)src.V;
	break;
    }
    
    case RT_VHT:
    {
	dst->VHT = *(VHT*)src.V;
	break;
    }
    
    case RT_FRAME_TV:
    {
	dst->Timestamp = *(Timestamp*)src.V;
	break;
    }
    
    case RT_HE:
    {
	dst->HE_SIG_A = 1;
	break;
    }
    
    case RT_HEMU:
    {
	dst->HE_SIG_A = 1;
	dst->HE_SIG_B = 1;
	break;
    }
    
    case RT_HEMU_OTHER_USER:
    {
	dst->HE_SIG_B = 1;
	break;
    }
    
    case RT_0L_PSDU:
    {
	dst->EmptyPSDU = *src.U1;
	break;
    }
    
    case RT_LSIG:
    {
	dst->LSIG = *(LSIG*)src.V;
	break;
    }
    
    case RT_TLV:
    {
	dst->TLV = 1;
    }
    
    case RT_RT_NS_NEXT:
    {
	/* (default) */
	break;
    }
    
    case RT_VENDOR_NS_NEXT:
    {
	dst->VendorNamespace = 1;
	break;
    }
	
    }
}

void InspectContextTrivial(const Context* src, U1 bit)
{
    if (~src->Present & (1 << bit))
	return;
    
    Field field = g_Fields[bit];
    if (memcmp(&field, &s_Reserved, sizeof field) == 0)
	return;
    
    switch (1 << bit)
    {
    case RT_TSFT:
    {
	printf("TSFT: %lu\n", src->TSFT);
	break;
    }
    
    case RT_FLAGS:
    {
	U4 flags = src->Flags;
	
	printf(
		"Flags: 0x%02X\n"
		"  .... ...%u = CFP           : %s\n"
		"  .... ..%u. = Preamble      : %s\n"
		"  .... .%u.. = WEP           : %s\n"
		"  .... %u... = Fragmentation : %s\n"
		"  ...%u .... = FCS at end    : %s\n"
		"  ..%u. .... = Data Pad      : %s\n"
		"  .%u.. .... = Bad FCS       : %s\n"
		"  %u... .... = Short GI      : %s\n",
		flags,
		F_ENTRY(F_SN(0)),
		F_ENTRY(F_SN(1)),
		F_ENTRY(F_SN(2)),
		F_ENTRY(F_SN(3)),
		F_ENTRY(F_SN(4)),
		F_ENTRY(F_SN(5)),
		F_ENTRY(F_SN(6)),
		F_ENTRY(F_SN(7))
	);
	
	break;
    }
    
    case RT_RATE:
    {
	printf("Data Rate: %.1f Mbps\n", src->DataRate);
	break;
    }
    
    case RT_CH:
    {
	Channel ch = src->Channel;
	
	printf(
		"Channel frequency: %hu MHz\n"
		"Channel flags: 0x%04hX\n"
		"  .... .... .... ...%u = 700 MHz spectrum                                  : %s\n"
		"  .... .... .... ..%u. = 600 MHz spectrum                                  : %s\n"
		"  .... .... .... .%u.. = 500 MHz spectrum                                  : %s\n"
		"  .... .... ...%u .... = Turbo                                             : %s\n"
		"  .... .... ..%u. .... = Complementary Code Keying (CCK)                   : %s\n"
		"  .... .... .%u.. .... = Orthogonal Frequency-Division Multiplexing (OFDM) : %s\n"
		"  .... .... %u... .... = 2 GHz spectrum                                    : %s\n"
		"  .... ...%u .... .... = 5GHz spectrum                                     : %s\n"
		"  .... ..%u. .... .... = Passive                                           : %s\n"
		"  .... .%u.. .... .... = Dynamic CCK-OFDM                                  : %s\n"
		"  .... %u... .... .... = Gaussian Frequency Shift Keying (GFSK)            : %s\n"
		"  ...%u .... .... .... = GSM (900MHz)                                      : %s\n"
		"  ..%u. .... .... .... = Static Turbo                                      : %s\n"
		"  .%u.. .... .... .... = Half Rate Channel (10MHz Channel Width)           : %s\n"
		"  %u... .... .... .... = Quarter Rate Channel (5MHz Channel Width)         : %s\n",
		ch.Frequency,
		ch.Flags,
		FS_ENTRY(ch.Flags, F_SN(0)),
		FS_ENTRY(ch.Flags, F_SN(1)),
		FS_ENTRY(ch.Flags, F_SN(2)),
		FS_ENTRY(ch.Flags, F_SN(4)),
		FS_ENTRY(ch.Flags, F_SN(5)),
		FS_ENTRY(ch.Flags, F_SN(6)),
		FS_ENTRY(ch.Flags, F_SN(7)),
		FS_ENTRY(ch.Flags, F_SN(8)),
		FS_ENTRY(ch.Flags, F_SN(9)),
		FS_ENTRY(ch.Flags, F_SN(10)),
		FS_ENTRY(ch.Flags, F_SN(11)),
		FS_ENTRY(ch.Flags, F_SN(12)),
		FS_ENTRY(ch.Flags, F_SN(13)),
		FS_ENTRY(ch.Flags, F_SN(14)),
		FS_ENTRY(ch.Flags, F_SN(15))
	);
	
	break;
    }
    
    case RT_FHSS:
    {
	FHSS fhss = src->FHSS;
	
	printf(
		"Hop Set: %hhu\n"
		"Hop Pattern: %hhu\n",
		fhss.HopSet,
		fhss.HopPattern
	);
	
	break;
    }
    
    case RT_DBM_ANT_SIGNAL:
    {
	S1 signal = src->AntennaSignal;
	
	printf(
		"Antenna Signal: %hhd dBm\n",
		signal
	);
	
	break;
    }
    
    case RT_DBM_ANT_NOISE:
    {
	S1 noise = src->AntennaNoise;
	
	printf(
		"Antenna Noise: %hhd dBm\n",
		noise
	);
	
	break;
    }
    
    case RT_LOCK_QUALITY:
    {
	U2 quality = src->LockQuality;
	
	printf(
		"Barker Code Lock Quality: %hu\n",
		quality
	);
	
	break;
    }
    
    case RT_TX_ATTENUATION:
    {
	U2 att = src->TXAttenuation;
	
	printf(
		"TX Attenuation: %hu",
		att
	);
	
	break;
    }
    
    case RT_DB_TX_ATTENUATION:
    {
	U2 att = src->TXAttenuationDB;
	
	printf(
		"TX Attenuation (dB): %hu dB\n",
		att
	);
	
	break;
    }
    
    case RT_DBM_TX_POWER:
    {
	S1 pwr = src->TXPower;
	
	printf(
		"TX Power: %hhd dBm\n",
		pwr
	);
	
	break;
    }
    
    case RT_ANT:
    {
	U1 ant = src->Antenna;
	
	printf(
		"Antenna: %hhu\n",
		ant
	);
	
	break;
    }
    
    case RT_DB_ANT_SIGNAL:
    {
	U1 signal = src->AntennaSignalDB;
	
	printf(
		"Antenna Signal (dB): %hhu dB\n",
		signal
	);
	
	break;
    }
    
    case RT_DB_ANT_NOISE:
    {
	U1 noise = src->AntennaNoiseDB;
	
	printf(
		"Antenna Noise (dB): %hhu dB\n",
		noise
	);
	
	break;
    }
    
    case RT_RX_FLAGS:
    {
	U2 flags = src->RXFlags;
	
	printf(
		"RX flags: 0x%04X\n"
		"  .... .... .... ..%hu. = Bad PLCP : %s\n",
		flags,
		F_ENTRY(1 << 1)
	);
	
	break;
    }
    
    case RT_TX_FLAGS:
    {
	U2 flags = src->TXFlags;
	
	printf(
		"TX flags: 0x%04X\n"
		"  .... .... .... ...%u = Excessive retries                     : %s\n"
		"  .... .... .... ..%u. = CTS-to-self protection                : %s\n"
		"  .... .... .... .%u.. = RTS/CTS handshake                     : %s\n"
		"  .... .... .... %u... = Not-expected ACK & no-retry on no-ACK : %s\n"
		"  .... .... ...%u .... = Pre-configured sequence number        : %s\n"
		"  .... .... ..%u. .... = Not-Reordered                         : %s\n",
		flags,
		F_ENTRY(1 << 0),
		F_ENTRY(1 << 1),
		F_ENTRY(1 << 2),
		F_ENTRY(1 << 3),
		F_ENTRY(1 << 4),
		F_ENTRY(1 << 5)
	);
	
	break;
    }
    
    case RT_DATA_RETRIES:
    {
	U1 retries = src->Retries;
	
	printf(
		"Data Retries: %hhu\n",
		retries
	);
	
	break;
    }
    
    case RT_CH_EXT:
    {
	XChannel xch = src->XChannel;
	
	printf(
		"XChannel:\n"
		"  Flags     : 0x%08X\n"
		"    .... .... .... .... .... .... ...%u .... = Turbo                                             : %s\n"
		"    .... .... .... .... .... .... ..%u. .... = CCK Modulation                                    : %s\n"
		"    .... .... .... .... .... .... .%u.. .... = Orthogonal Frequency-Division Multiplexing (OFDM) : %s\n"
		"    .... .... .... .... .... .... %u... .... = 2 GHz spectrum                                    : %s\n"
		"    .... .... .... .... .... ...%u .... .... = 5GHz spectrum                                     : %s\n"
		"    .... .... .... .... .... ..%u. .... .... = Passive                                           : %s\n"
		"    .... .... .... .... .... .%u.. .... .... = Dynamic CCK-OFDM                                  : %s\n"
		"    .... .... .... .... .... %u... .... .... = Gaussian Frequency Shift Keying (GFSK)            : %s\n"
		"    .... .... .... .... ...%u .... .... .... = GSM (900MHz)                                      : %s\n"
		"    .... .... .... .... ..%u. .... .... .... = Static Turbo                                      : %s\n"
		"    .... .... .... .... .%u.. .... .... .... = Half Rate Channel (10MHz Channel Width)           : %s\n"
		"    .... .... .... .... %u... .... .... .... = Quarter Rate Channel (5MHz Channel Width)         : %s\n"
		"    .... .... .... ...%u .... .... .... .... = Channel FrameType HT/20                                : %s\n"
		"    .... .... .... ..%u. .... .... .... .... = Channel FrameType HT/40+                               : %s\n"
		"    .... .... .... .%u.. .... .... .... .... = Channel FrameType HT/40-                               : %s\n"
		"  Frequency : %hu MHz\n"
		"  Channel   : %hhu\n"
		"  Max Power : %hhu\n",
		xch.Flags,
		FS_ENTRY(xch.Flags, 1 << 4),
		FS_ENTRY(xch.Flags, 1 << 5),
		FS_ENTRY(xch.Flags, 1 << 6),
		FS_ENTRY(xch.Flags, 1 << 7),
		FS_ENTRY(xch.Flags, 1 << 8),
		FS_ENTRY(xch.Flags, 1 << 9),
		FS_ENTRY(xch.Flags, 1 << 10),
		FS_ENTRY(xch.Flags, 1 << 11),
		FS_ENTRY(xch.Flags, 1 << 12),
		FS_ENTRY(xch.Flags, 1 << 13),
		FS_ENTRY(xch.Flags, 1 << 14),
		FS_ENTRY(xch.Flags, 1 << 15),
		FS_ENTRY(xch.Flags, 1 << 16),
		FS_ENTRY(xch.Flags, 1 << 17),
		FS_ENTRY(xch.Flags, 1 << 18),
		xch.Frequency,
		xch.Channel,
		xch.MaxPower
	);
	
	break;
    }
    
    case RT_MCS:
    {
	MCS mcs = src->MCS;
	
	U1 bandwidth = mcs.Flags & 0x03;
	U1 nSTBC     = (mcs.Flags & 0x60) >> 5;
	
	printf(
		"MCS information\n"
		"  Known                  : 0x%02X\n"
		"    .... ...%u = Bandwidth                                           : %s\n"
		"    .... ..%u. = MCS index known                                     : %s\n"
		"    .... .%u.. = Guard interval                                      : %s\n"
		"    .... %u... = HT format                                           : %s\n"
		"    ...%u .... = FEC type                                            : %s\n"
		"    ..%u. .... = STBC known                                          : %s\n"
		"    .%u.. .... = Number of extension spatial streams known           : %s\n"
		"    %u... .... = Bit 1 (MSB) of Number of extension spartial streams : %s\n"
		"  Flags                  : 0x%02X\n"
		"    .... ..%u%u = Bandwidth      { 0:20, 1:40, 2:20L, 3:20U }         : %hhu\n"
		"    .... .%u.. = Guard interval { 0:long-GI, 1:short-GI }            : %hhu\n"
		"    .... %u... = HT format      { 0:mixed, 1:greenfield }            : %hhu\n"
		"    ...%u .... = FEC type       { 0:BCC, 1:LDPC }                    : %hhu\n"
		"    .%u%u. .... = Number of STBC streams                              : %hhu\n"
		"    %u... .... = Bit 0 (LSB) of Number of extension spartial streams : %hhu\n"
		"  MCS (IEEE802.11n-2009) : %hhu\n",
		
		/* KNOWN */
		mcs.Known,
		FS_ENTRY(mcs.Known, 1 << 0),
		FS_ENTRY(mcs.Known, 1 << 1),
		FS_ENTRY(mcs.Known, 1 << 2),
		FS_ENTRY(mcs.Known, 1 << 3),
		FS_ENTRY(mcs.Known, 1 << 4),
		FS_ENTRY(mcs.Known, 1 << 5),
		FS_ENTRY(mcs.Known, 1 << 6),
		FS_ENTRY(mcs.Known, 1 << 7),
		
		/* FLAGS */
		mcs.Flags,
		
		bandwidth >> 1,
		bandwidth & 1,
		bandwidth,
		
		HAS_F(mcs.Flags, 0x04),
		HAS_F(mcs.Flags, 0x04),
		
		HAS_F(mcs.Flags, 0x08),
		HAS_F(mcs.Flags, 0x08),
		
		HAS_F(mcs.Flags, 0x10),
		HAS_F(mcs.Flags, 0x10),
		
		nSTBC >> 1,
		nSTBC & 1,
		nSTBC,
		
		HAS_F(mcs.Flags, 0x80),
		HAS_F(mcs.Flags, 0x80),
		
		/* MCS */
		mcs.MCS_
	);
	
	break;
    }
    
    case RT_AMPDU:
    {
	AMPDU ampdu = src->AMPDU;
	
	printf(
		"A-MPDU status\n"
		"  Reference Number: %u\n"
		"  Flags           : 0x%04hX\n"
		"    .... ...%u = Driver 0-length subframe : %s\n"
		"    .... ..%u. = MACHeader 0-length subframe  : %s\n"
		"    .... .%u.. = Last subframe is known   : %s\n"
		"    .... %u... = This is last subframe    : %s\n"
		"    ...%u .... = delimiter CRC error      : %s\n"
		"    ..%u. .... = delimiter CRC valid      : %s\n"
		"    .%u.. .... = EOF                      : %s\n"
		"    %u... .... = EOF known                : %s\n"
		"  delimiter CRC   : 0x%02hhX\n",
		ampdu.Reference,
		ampdu.Flags,
		FS_ENTRY(ampdu.Flags, 1 << 0),
		FS_ENTRY(ampdu.Flags, 1 << 1),
		FS_ENTRY(ampdu.Flags, 1 << 2),
		FS_ENTRY(ampdu.Flags, 1 << 3),
		FS_ENTRY(ampdu.Flags, 1 << 4),
		FS_ENTRY(ampdu.Flags, 1 << 5),
		FS_ENTRY(ampdu.Flags, 1 << 6),
		FS_ENTRY(ampdu.Flags, 1 << 7),
		ampdu.CRC
	);
	
	break;
    }
    
    case RT_VHT:
    {
	VHT vht = src->VHT;
	U1 total;
	U1 i;
	
	if (vht.Bandwidth > 10)
	    total = 160;
	else if (vht.Bandwidth > 3)
	    total = 80;
	else if (vht.Bandwidth > 0)
	    total = 40;
	else
	    total = 20;
	
	printf(
		"VHT\n"
		"  Known       : 0x%04X\n"
		"    .... .... .... ...%u = STBC known                         : %s\n"
		"    .... .... .... ..%u. = TXOP_PS_NOT_ALLOWED known          : %s\n"
		"    .... .... .... .%u.. = Guard interval                     : %s\n"
		"    .... .... .... %u... = Short GI NSYM disambiguation known : %s\n"
		"    .... .... ...%u .... = LDPC extra OFDM symbol known       : %s\n"
		"    .... .... ..%u. .... = Beamformed known/applicable        : %s\n"
		"    .... .... .%u.. .... = Bandwidth known                    : %s\n"
		"    .... .... %u... .... = Group ID known                     : %s\n"
		"    .... ...%u .... .... = Partial AID known/applicable       : %s\n"
		"  Flags       : 0x%02X\n"
		"    .... ...%u = STBC                         : %s\n"
		"    .... ..%u. = TXOP_PS_NOT_ALLOWED          : %s\n"
		"    .... .%u.. = Guard interval               : %s\n"
		"    .... %u... = Short GI NSYM disambiguation : %s\n"
		"    ...%u .... = LDPC Extra OFDM symbol       : %s\n"
		"    ..%u. .... = Beamformed                   : %s\n"
		"  Bandwidth   : %hhu [%hhu MHz]\n",
		vht.Known,
		FS_ENTRY(vht.Known, 1 << 0),
		FS_ENTRY(vht.Known, 1 << 1),
		FS_ENTRY(vht.Known, 1 << 2),
		FS_ENTRY(vht.Known, 1 << 3),
		FS_ENTRY(vht.Known, 1 << 4),
		FS_ENTRY(vht.Known, 1 << 5),
		FS_ENTRY(vht.Known, 1 << 6),
		FS_ENTRY(vht.Known, 1 << 7),
		FS_ENTRY(vht.Known, 1 << 8),
		vht.Flags,
		FS_ENTRY(vht.Flags, 1 << 0),
		FS_ENTRY(vht.Flags, 1 << 1),
		FS_ENTRY(vht.Flags, 1 << 2),
		FS_ENTRY(vht.Flags, 1 << 3),
		FS_ENTRY(vht.Flags, 1 << 4),
		FS_ENTRY(vht.Flags, 1 << 5),
		vht.Bandwidth,
		total
	);
	
	for (i = 0; i < 4; ++i)
	{
	    U1 mcsNss = vht.MCSNss[i];
	    
	    U1 nss = mcsNss & 0x0F;
	    U1 mcs = mcsNss & 0xF0;
	    
	    if (!nss)
	    {
		printf(
			"  MCS NSS.%hhu   : (not present)\n",
			i
		);
	    }
	    else
	    {
		printf(
			"  MCS NSS.%hhu   : \n"
			"    Number of Spatial Streams : %hhu\n"
			"    MCS rate index            : %hhu\n",
			i,
			nss,
			mcs
		);
	    }
	}
	
	U4 usr = log2u(vht.Coding);
	
	printf(
		"  User        : %u\n"
		"  Group       : %hhu\n"
		"  Partial AID : %hu\n",
		usr,
		vht.GroupID,
		vht.PartialAID
	);
	
	break;
    }
    
    case RT_FRAME_TV:
    {
	Timestamp tv = src->Timestamp;
	
	const char* unit;
	switch (tv.Unit & 0x0F)
	{
	case 0:
	    unit = "ms";
	    break;
	case 1:
	    unit = "us";
	    break;
	case 2:
	    unit = "ns";
	    break;
	
	default:
	    unit = "(undefined unit)";
	    break;
	}
	
	const char* sample;
	switch (tv.Position >> 4)
	{
	case 0:
	    sample = "first bit of MPDU";
	    break;
	case 1:
	    sample = "signal acquisition at start of PLCP";
	    break;
	case 2:
	    sample = "end of PPDU";
	    break;
	case 3:
	    sample = "end of MPDU";
	    break;
	case 15:
	    sample = "vendor defined";
	    break;
	default:
	    sample = "(reserved)";
	    break;
	}
	
	printf(
		"Timestamp\n"
		"  Timestamp         : %lu %s\n"
		"  Precision         : %hu %s\n"
		"  Sampling Position : %s\n"
		"  Flags             : 0x%02hhX\n"
		"    .... ...%u = Counter        : %s\n"
		"    .... ..%u. = Accuracy Known : %s\n",
		tv.Timestamp, unit,
		tv.Precision, unit,
		sample,
		tv.Flags,
		FS_ENTRY(tv.Flags, 1 << 0),
		FS_ENTRY(tv.Flags, 1 << 1)
	);
	
	break;
    }
    
    case RT_HE:
    {
	printf("HE-SIG-A (present, optimized-out)\n");
	break;
    }
    
    case RT_HEMU:
    {
	printf("HE-SIG-A/B (present, optimized-out)\n");
	break;
    }
    
    case RT_HEMU_OTHER_USER:
    {
	printf("HE-SIG-B (present, optimized-out)\n");
	break;
    }
    
    case RT_0L_PSDU:
    {
	U1 type = src->EmptyPSDU;
	
	const char* str;
	switch (type)
	{
	case 0:
	    str = "Sounding PPDU";
	    break;
	case 1:
	    str = "Data not captured (e.g. multi-user PPDU)";
	    break;
	default:
	    str = "Vendor-specific";
	    break;
	}
	
	printf(
		"0-Length-PSDU: %hhu %s\n",
		type,
		str
	);
	
	break;
    }
    
    case RT_LSIG:
    {
	LSIG lsig = src->LSIG;
	
	const char* data1;
	switch (lsig.data1)
	{
	case 1:
	    data1 = "Rate known";
	    break;
	
	case 2:
	    data1 = "Length known";
	    break;
	
	default:
	    data1 = "???";
	    break;
	}
	
	printf(
		"L-SIG (%s)\n"
		"  Rate   : %hu\n"
		"  Length : %hu\n",
		data1,
		lsig.data2 & 0x000f,
		lsig.data2 >> 4
	);
	
	break;
    }
    
    case RT_TLV:
    {
	printf("TLVs (present, optimized-out)");
	break;
    }
    
    case RT_RT_NS_NEXT:
    {
	/* it's default. shouldn't print-out. */
	break;
    }
    
    case RT_VENDOR_NS_NEXT:
    {
	printf("Vendor Namespace (present, optimized-out)");
	break;
    }
	
    }
}

void InspectRadiotap(const Context* src)
{
    size_t c, i;
    U1 f;
    const Context* tmp;
    
    puts("== MACHeader");
    
    for (c = 0, tmp = src; tmp; tmp = tmp->Next, c++)
    {
    }
    
    for (i = 0, tmp = src; i < c; tmp = tmp->Next, ++i)
    {
	printf("==== 0x%08X %lu/%lu\n", tmp->Present, i + 1, c);
	for (f = 0; f < 32; ++f)
	    InspectContextTrivial(tmp, f);
    }
}
