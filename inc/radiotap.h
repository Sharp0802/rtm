#ifndef RTM_RADIOTAP_H
#define RTM_RADIOTAP_H

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <pcap.h>

#include "defs.h"

#define __pack__ __attribute__((packed)) //NOLINT
#define __field__ __attribute__((packed)) //NOLINT

#define B_STR(b) (b?"True":"False")

#define HAS(f, k) (((f)&(k))==(k))
#define HAS_F(f, k) (HAS(f,k)?1:0)

#define RT_TSFT              ((uint32_t)(1U << 0))
#define RT_FLAGS             ((uint32_t)(1U << 1))
#define RT_RATE              ((uint32_t)(1U << 2))
#define RT_CH                ((uint32_t)(1U << 3))
#define RT_FHSS              ((uint32_t)(1U << 4))
#define RT_DBM_ANT_SIGNAL    ((uint32_t)(1U << 5))
#define RT_DBM_ANT_NOISE     ((uint32_t)(1U << 6))
#define RT_LOCK_QUALITY      ((uint32_t)(1U << 7))
#define RT_TX_ATTENUATION    ((uint32_t)(1U << 8))
#define RT_DB_TX_ATTENUATION ((uint32_t)(1U << 9))
#define RT_DBM_TX_POWER      ((uint32_t)(1U << 10))
#define RT_ANT               ((uint32_t)(1U << 11))
#define RT_DB_ANT_SIGNAL     ((uint32_t)(1U << 12))
#define RT_DB_ANT_NOISE      ((uint32_t)(1U << 13))
#define RT_RX_FLAGS          ((uint32_t)(1U << 14))
#define RT_TX_FLAGS          ((uint32_t)(1U << 15))
#define RT_DATA_RETRIES      ((uint32_t)(1U << 17))
#define RT_CH_EXT            ((uint32_t)(1U << 18))
#define RT_MCS               ((uint32_t)(1U << 19))
#define RT_AMPDU             ((uint32_t)(1U << 20))
#define RT_VHT               ((uint32_t)(1U << 21))
#define RT_FRAME_TV          ((uint32_t)(1U << 22))
#define RT_HE                ((uint32_t)(1U << 23))
#define RT_HEMU              ((uint32_t)(1U << 24))
#define RT_HEMU_OTHER_USER   ((uint32_t)(1U << 25))
#define RT_0L_PSDU           ((uint32_t)(1U << 26))
#define RT_LSIG              ((uint32_t)(1U << 27))
#define RT_TLV               ((uint32_t)(1U << 28))
#define RT_RT_NS_NEXT        ((uint32_t)(1U << 29))
#define RT_VENDOR_NS_NEXT    ((uint32_t)(1U << 30))
#define RT_EXT               ((uint32_t)(1U << 31))

#define IFR_VER_PV0 0
#define IFR_VER_PV1 1

#define IFR_TO_DS          ((uint8_t)(1U << 7))
#define IFR_FROM_DS        ((uint8_t)(1U << 6))
#define IFR_MORE_FRAGMENTS ((uint8_t)(1U << 5))
#define IFR_RETRY          ((uint8_t)(1U << 4))
#define IFR_PWR_MANAGEMENT ((uint8_t)(1U << 3))
#define IFR_MORE_DATA      ((uint8_t)(1U << 2))
#define IFR_PROTECTED      ((uint8_t)(1U << 1))
#define IFR_HTC_ORDER      ((uint8_t)(1U << 0))

#define BEACON_SSID 0
#define BEACON_RSN  48


#if BYTE_ORDER == LITTLE_ENDIAN
typedef uint8_t  U1;
typedef uint16_t U2;
typedef uint32_t U4;
typedef uint64_t U8;
typedef int8_t   S1;
typedef int16_t  S2;
typedef int32_t  S4;
typedef int64_t  S8;
#else
#error "Big-Endian is not supported yet."
#endif

typedef U1 BOOL;

typedef float Mbps;
typedef S1    dBm;
typedef U2    dB;

typedef U1  MAC[6];

typedef struct
{
    U2 Frequency;
    U2 Flags;
} __field__ Channel;

typedef struct
{
    U4 Flags;
    U2 Frequency;
    U1 Channel;
    U1 MaxPower;
} __field__ XChannel;

typedef struct
{
    U1 HopSet;
    U1 HopPattern;
} __field__ FHSS;

typedef struct
{
    U1 Known;
    U1 Flags;
    U1 MCS_;
} __field__ MCS;

typedef struct
{
    U4 Reference;
    U2 Flags;
    U1 CRC;
    U1 Reserved;
} __field__ AMPDU;

typedef struct
{
    U2 Known;
    U1 Flags;
    U1 Bandwidth;
    U1 MCSNss[4];
    U1 Coding;
    U1 GroupID;
    U2 PartialAID;
} __field__ VHT;

typedef struct
{
    U8 Timestamp;
    U2 Precision;
    union
    {
        U1 Unit;
        U1 Position;
    };
    U1 Flags;
} __field__ Timestamp;

typedef struct
{
    U2 data1;
    U2 data2;
} __field__ LSIG;


typedef struct
{
    U1 Revision;
    U1 Pad;
    U2 Length;
    U4 Present;
} __pack__  Radiotap;

typedef struct
{
    union
    {
        U2 FrameControl;
        struct
        {
            U1 Types;
            U1 Flags;
        };
    };
    U2  Duration;
    MAC Address[3];
    U2  SequenceControl;
} __pack__  MACHeader;

typedef U1 CipherSuiteOUI[3];

typedef struct
{
    CipherSuiteOUI OUI;
    U1             Type;
} __pack__ CipherSuite;

typedef union
{
    const void* V;
    
    const U1* U1;
    const U2* U2;
    const U4* U4;
    const U8* U8;
    
    const S1* S1;
    const S2* S2;
    const S4* S4;
    const S8* S8;
}          Pointer;

typedef struct
{
    size_t Size;
    size_t Alignment;
} __pack__ Field;

typedef enum : U1
{
    CT_NONE,
    CT_BEACON,
    CT_DATA
}          FrameType;

typedef struct
{
    U1 GroupCipherSuite[4];
    
    U2 PairwiseCipherSuiteCount;
    CipherSuite* PairwiseCipherSuiteList;
    
    U2 AuthKeyManagementSuiteCount;
    CipherSuite* AuthKeyManagementList;
    
    U2 Capabilities;
} __pack__ RSN_1;

typedef struct
{
    U2 Version;
    union
    {
        RSN_1 V1;
    };
} __pack__ RSN;

typedef char SSID[32];

typedef struct
{
    MACHeader MACHeader;
    SSID SSID;
    RSN  RSN;
} __pack__   BeaconFrame;

typedef struct
{
    MACHeader MACHeader;
    size_t Length;
} __pack__   DataFrame;

typedef struct
{
    FrameType FrameType;
    union
    {
        BeaconFrame BeaconFrame;
        DataFrame   DataFrame;
    };
} __pack__   ContextTrailer;

typedef struct tag__ctx_t
{
    struct tag__ctx_t* Next;
    
    U4 Present;
    
    AMPDU     AMPDU;
    U1        Antenna;
    dBm       AntennaNoise;
    dB        AntennaNoiseDB;
    dBm       AntennaSignal;
    dB        AntennaSignalDB;
    Channel   Channel;
    Mbps      DataRate;
    U1        EmptyPSDU;
    FHSS      FHSS;
    U4        Flags;
    BOOL      HE_SIG_A;
    BOOL      HE_SIG_B;
    U2        LockQuality;
    LSIG      LSIG;
    MCS       MCS;
    U1        Retries;
    U2        RXFlags;
    U8        TSFT;
    U2        TXAttenuation;
    dB        TXAttenuationDB;
    U2        TXFlags;
    dBm       TXPower;
    Timestamp Timestamp;
    BOOL      TLV;
    BOOL      VendorNamespace;
    VHT       VHT;
    XChannel  XChannel;
    
    ContextTrailer* Trailer;
} __pack__ Context;

#define RESERVED { 0, 0 }

extern const Field g_Fields[32];


Context* ParseContext(const void* src, size_t nb);

EXPORT void ReleaseContext(Context* context);

void ReleaseTrailer(ContextTrailer* trailer);

/* TODO: merge into managed assembly */
void InspectRadiotap(const Context* src);

#endif
