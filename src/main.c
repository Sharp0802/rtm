#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <pcap.h>
#include <stdarg.h>

#include "config.h"
#include "radiotap.h"
#include "ui/ui.h"

#define ARGS_FILE    (1)
#define ARGS_IF      (2)

#define CHECK_ARG__(arg, ch) (strcmp(arg,ch)==0)
#define CHECK_ARG(arg, ch) (CHECK_ARG__(arg,ch)||CHECK_ARG__(arg,"-"ch)||CHECK_ARG__(arg,"/"ch))

#define RED "\x1b[31m"
#define RST "\x1b[0m"

#define EXCEPT()                     \
if (setjmp(s_Frame))                 \
{                                    \
    printf(RED "%s\n" RST, s_Error); \
    goto EXIT;                       \
}

#define EXCEPTX(fn)                  \
if (setjmp(s_Frame))                 \
{                                    \
    printf(RED "%s\n" RST, s_Error); \
    fn();                            \
    goto EXIT;                       \
}


static jmp_buf s_Frame;
static char    s_Error[BUFSIZ];

__attribute__((always_inline))
inline static void Version(void)
{
    puts(
	    "RTM (rtm) " VERSION "                                                      \n"
	    "Copyright (C) 2023 Yeong-won Seo                                           \n"
	    "This is free software; see the source for copying conditions.  There is NO \n"
	    "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE."
    );
}

__attribute__((always_inline))
inline static void Help(void)
{
    puts(
	    "+-------------+-----------------------------+\n"
	    "| SYNTAX      | rtm (<param> <arg>?)+       |\n"
	    "| SAMPLE      | rtm i wlan1                 |\n"
	    "+-------------+-----------------------------+\n"
	    "| PARAM       | DESC                        |\n"
	    "|  ifname, i  | A name of monitor interface |\n"
	    "|  file, f    | A pcap-dump file path       |\n"
	    "|  version, v | Print version information   |\n"
	    "|  help, h    | Print this message          |\n"
	    "+-------------+-----------------------------+"
    );
}

__attribute__((always_inline))
inline static void SyntaxError(void)
{
    puts(RED "invalid syntax" RST);
    Help();
}

__attribute__((noreturn, format(printf, 1, 2)))
void FastFailF(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_Error, sizeof s_Error, fmt, ap);
    va_end(ap);
    longjmp(s_Frame, 1);
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
    snprintf(s_Error, sizeof s_Error, "\ninterrupted (%d)", sig);
    longjmp(s_Frame, 1);
}


void run(int mode, const char* target)
{
    static char err[PCAP_ERRBUF_SIZE];
    
    pcap_t            * dev;
    struct pcap_pkthdr* hdr;
    const char        * pkt;
    
    dev = 0;
    
    EXCEPT()
    
    switch (mode) // NOLINT(*-multiway-paths-covered)
    {
    case ARGS_IF:
	dev = pcap_open_live(target, BUFSIZ, 1, 1, err);
	break;
    case ARGS_FILE:
	dev = pcap_open_offline(target, err);
	break;
    }
    if (!dev)
	FastFailF("pcap_open_live(): %s", err);
    
    EXCEPT()
    
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
	HandleContext(ctx);
	ReleaseContext(ctx);
    }

EXIT:
    if (dev)
    	pcap_close(dev);
}

int main(int argc, char* argv[])
{
    char* target;
    int mode;
    
    int i;
    int pmod;
    
    EXCEPTX(SyntaxError)
    
    target = 0;
    mode   = 0;
    for (i = 1, pmod = 0; i < argc; ++i)
    {
	if (pmod)
	{
	    if (mode)
	    {
		FastFailF("conflicted target modifier: '%s'", argv[i]);
	    }
	    
	    target = argv[i];
	    mode   = pmod;
	    pmod   = 0;
	}
	else if (CHECK_ARG(argv[i], "f"))
	{
	    pmod = ARGS_FILE;
	}
	else if (CHECK_ARG(argv[i], "i"))
	{
	    pmod = ARGS_IF;
	}
	else if (CHECK_ARG(argv[i], "v"))
	{
	    Version();
	    return 0;
	}
	else if (CHECK_ARG(argv[i], "h"))
	{
	    Help();
	    return 0;
	}
	else
	{
	    FastFailF("unexpected token: '%s'", argv[i]);
	}
    }
    if (!target)
	FastFail("no target modifier");
    
    signal(SIGINT, Interrupt);
    signal(SIGTERM, Interrupt);
#ifndef _WIN32
    signal(SIGKILL, Interrupt);
    signal(SIGQUIT, Interrupt);
    signal(SIGSTOP, Interrupt);
#endif
    
    run(mode, target);

EXIT:
    return 0;
}
