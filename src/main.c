#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <pcap.h>

#include "radiotap.h"

#define RED "\x1b[31m"
#define RST "\x1b[0m"


static jmp_buf s_Frame;
static char s_Error[BUFSIZ];


void Help(void)
{
    puts(
	    RED "invalid syntax" RST "\n"
	    "syntax: rtm <ifname>\n"
	    "sample: rtm wlan1"
    );
}


__attribute__((noreturn))
void FastFail(const char* msg)
{
    strncpy(s_Error, msg, sizeof s_Error);
    longjmp(s_Frame, 1);
}

__attribute__((noreturn))
void Interrupt(int sig)
{
    puts("e");
    snprintf(s_Error, sizeof s_Error, "interrupted (%d)", sig);
    longjmp(s_Frame, 1);
}



int main(int argc, char* argv[])
{
    static char err[PCAP_ERRBUF_SIZE];
    
    pcap_t* dev;
    struct pcap_pkthdr* hdr;
    const char* pkt;
    
    if (argc != 2)
    {
	Help();
	return 1;
    }
    
    dev = pcap_open_live(argv[1], BUFSIZ, 1, 1, err);
    if (!dev)
    {
	printf(RED "pcap_open_live(): %s" RST, err);
	return 1;
    }
    
    if (setjmp(s_Frame))
    {
	printf(RED "%s\n" RST, s_Error);
	goto EXIT;
    }
    
    signal(SIGINT, Interrupt);
    signal(SIGKILL, Interrupt);
    signal(SIGQUIT, Interrupt);
    signal(SIGSTOP, Interrupt);
    signal(SIGTERM, Interrupt);
    
    while (1)
    {
	switch (pcap_next_ex(dev, &hdr, (const u_char**)&pkt))
	{
	case 0:
	    continue;
	case PCAP_ERROR:
	case PCAP_ERROR_BREAK:
	    FastFail(pcap_geterr(dev));
	}
	
	Context* ctx = ParseContext(pkt, hdr->caplen);
	// TODO : print
	//InspectRadiotap(ctx);
	ReleaseContext(ctx);
    }

EXIT:
    pcap_close(dev);
    return 0;
}
