#include "radiotap.h"
#include "errno.h"


extern size_t GetFieldSize(U1 bit, Pointer src);

extern void ParseField(U1 bit, Pointer src, Context* dst);

static void ParseRSN(Pointer mp, RSN* rsn)
{
    U2 i;
    
    switch (rsn->Version = *mp.U2++)
    {
    
    case 1:
    {
	RSN_1 v;
	
	memcpy(v.GroupCipherSuite, mp.U4++, 4);
	
	v.PairwiseCipherSuiteCount = *mp.U2++;
	v.PairwiseCipherSuiteList = malloc(v.PairwiseCipherSuiteCount * sizeof *v.PairwiseCipherSuiteList);
	for (i = 0; i < v.PairwiseCipherSuiteCount; ++i)
	    memcpy(v.PairwiseCipherSuiteList + i, mp.U4++, sizeof *v.PairwiseCipherSuiteList);
	
	v.AuthKeyManagementSuiteCount = *mp.U2++;
	v.AuthKeyManagementList = malloc(v.AuthKeyManagementSuiteCount * sizeof *v.AuthKeyManagementList);
	for (i = 0; i < v.AuthKeyManagementSuiteCount; ++i)
	    memcpy(v.AuthKeyManagementList + i, mp.U4++, sizeof *v.AuthKeyManagementList);
	
	v.Capabilities = *mp.U2++;
	
	rsn->V1 = v;
	
	break;
    }
    
    }
}

static void ReleaseRSN(RSN* rsn)
{
    switch (rsn->Version)
    {
    case 1:
    {
	free(rsn->V1.PairwiseCipherSuiteList);
	free(rsn->V1.AuthKeyManagementList);
	break;
    }
    }
}

static void ParseBeacon(const void* src, size_t nb, Pointer mp, MACHeader mac, ContextTrailer* dst)
{
    /* fixed params */
    U8 __attribute__((unused)) tv       = *mp.U8++;
    U2 __attribute__((unused)) interval = *mp.U2++;
    U2 __attribute__((unused)) compat   = *mp.U2++;
    
    U1 type;
    U1 len;
    
    RSN rsn;
    
    Pointer lmp;
    memset(&lmp, 0, sizeof lmp);
    
    dst->FrameType = CT_BEACON;
    memset(&dst->BeaconFrame, 0, sizeof dst->BeaconFrame);
    dst->BeaconFrame.MACHeader = mac;
    
    /* tagged params */
    for (; (mp.U1 - (const U1*)src) < (ssize_t)(nb - 4);)
    {
	type = *mp.U1++;
	len  = *mp.U1++;
	lmp.V = mp.V;
	mp.U1 += len;
	
	switch (type)
	{
	case BEACON_SSID:
	{
	    memcpy(dst->BeaconFrame.SSID, (const char*)lmp.U1, len);
            dst->BeaconFrame.SSID[len] = 0;
	    break;
	}

    case BEACON_DS:
    {
        dst->BeaconFrame.Channel = *lmp.U1;
        break;
    }
	
	case BEACON_RSN:
	{
	    ParseRSN(lmp, &rsn);
        dst->BeaconFrame.RSN = rsn;
	    break;
	}
	
	default:
	    break;
	}
    }
}

static void ParseData(const void* src, size_t nb, Pointer mp, MACHeader mac, ContextTrailer* dst)
{
    dst->FrameType = CT_DATA;
    memset(&dst->DataFrame, 0, sizeof dst->DataFrame);
    dst->DataFrame.MACHeader = mac;
    
    dst->DataFrame.Length = nb - (mp.U1 - (const U1*)src);
}

__attribute__((hot))
Context* ParseContext(const void* src, size_t nb)
{
    static __thread uint32_t flags[BUFSIZ];
    static __thread uint32_t nFlag;
    Pointer                  mp;
    Radiotap                 rt;
    MACHeader                ifr;
    Field                    field;
    size_t                   misaligned;
    void (* parser)(const void*, size_t, Pointer, MACHeader, ContextTrailer*);
    U4 i;
    U1 f;
    
    Context* root;
    Context* context;
    
    if (sizeof rt > nb)
    {
        errno = E2BIG;
        goto FAULT;
    }
    
    /*
     * union with different field Size is undefined.
     * explicit memory initialization is required.
     */
    memset(&mp, 0, sizeof mp);
    mp.V = src;
    
    rt = *(const Radiotap*)mp.V;
    
    if (rt.Length > nb)
    {
        errno = E2BIG;
        goto FAULT;
    }
    
    /* revision must be set to 0 */
    if (rt.Revision)
	return 0;
    
    nFlag = 0;
    mp.U1 += 4;
    
    /* parse presents */
    do
    {
	flags[nFlag++] = *mp.U4;
    } while (HAS(*mp.U4++, RT_EXT));
    
    /* parse fields */
    context = root = calloc(1, sizeof *context);
    
    for (i = 0; i < nFlag; ++i)
    {
	if (i > 0)
	    context = (context->Next = calloc(1, sizeof *context));
	
	context->Present = flags[i];
	
	for (f = 0; f < 32; ++f)
	{
	    if (~flags[i] & (1 << f))
		continue;
	    
	    field = g_Fields[f];
	    
	    /* align address */
	    if (field.Alignment)
	    {
		misaligned = (mp.U1 - (const U1*)src) % field.Alignment;
		if (misaligned)
		    mp.U1 += field.Alignment - misaligned;
	    }
	    
	    /* fast-fail BOF */
	    if ((mp.U1 - (const U1*)src) > (ssize_t)nb)
	    {
		ReleaseContext(root);
                errno = EFAULT;
		goto FAULT;
	    }
	    
	    ParseField(f, mp, context);
	    
	    /* handle variable-length fields (TLVs) */
	    mp.U1 += (field.Size == (size_t)-1)
		     ? GetFieldSize(f, mp)
		     : field.Size;
	}
    }
    
    if (rt.Length + sizeof ifr > nb)
    {
        errno = E2BIG;
        goto FAULT;
    }
    
    /* reset address (radiotap) */
    mp.V = src;
    mp.U1 += rt.Length;
    
    /* reset address (mac-header) */
    ifr = *(MACHeader*)mp.V;
    mp.U1 += sizeof ifr;
    
    /* ignore CCMP parameters */
    
    /* parse inner frame */
    U1 type    = (ifr.Types >> 2) & 3;
    U1 subtype = ifr.Types >> 4;
    
    if (type == 0 && subtype == 0x1b)
	parser = ParseData;
    else if (type == 0 && subtype == 8)
	parser = ParseBeacon;
    else
    {
	parser = 0;
	root->Trailer = 0;
    }
    
    if (parser)
    {
	root->Trailer = malloc(sizeof *root->Trailer);
	parser(src, nb, mp, ifr, root->Trailer);
    }
    
    errno = 0;
    return root;

FAULT:
    root = 0;
    return root;
}

void ReleaseTrailer(ContextTrailer* trailer)
{
    if (!trailer)
	return;
    
    switch (trailer->FrameType)
    {
    case CT_NONE:
	break;
    
    case CT_BEACON:
	ReleaseRSN(&trailer->BeaconFrame.RSN);
	break;
	
    case CT_DATA:
	break;
    }
    
    free(trailer);
}

EXPORT void ReleaseContext(Context* context)
{
    if (!context)
	return;
    
    ReleaseTrailer(context->Trailer);
    
    Context* tmp;
    do
    {
	tmp = context->Next;
	free(context);
    } while ((context = tmp));
}
