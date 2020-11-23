#include "vwg.h"
#include <stdio.h>
#include <internet.h>

#define starts(s1,s2)	(strncmp(s1,s2,strlen(s2)) == 0)

global FILE *pinfile, *poutfile;
global struct ruleset *hypersyntax;
global struct dictnode *metalist, *ghead;
global struct hyperrule *toplevelhr;
global struct hypernotion *emptypn;
global uint debugbits;
global int linenum;
global bool anyerrors;

static int fdnum;
static bool sopt, fdopt;
static struct tcpuser tuser;
static char *fname;

extern initheap();		/* from heap	  */
extern initclock();		/* from common	  */
extern readgrammar();		/* from readgr	  */
extern checkmetasyntax();	/* from checkmeta */
extern factorize();		/* from factorize */
extern checkgrammar();		/* from checkgr	  */
extern parse();			/* from parse	  */


global main(argc, argv) int argc; char *argv[];
  { readcmdline(argc, argv);
    setpinpout();
    initheap(); initclock();
    anyerrors = false;
    readgrammar(fname);
    if (anyerrors) exit(2);
    linenum = -1;
    checkmetasyntax();
    if (anyerrors) exit(2);
    factorize();
    checkgrammar();
    if (anyerrors) exit(2);
    fprintf(poutfile, "OK\n"); fflush(poutfile);
    parse();
    exit(0);
  }

static readcmdline(argc, argv) int argc; char *argv[];
  { int ak = 0;
    debugbits = 0; sopt = fdopt = false;
    if (ak < argc) ak++; /* skip program name */
    while (ak < argc && argv[ak][0] == '-')
      { if (starts(argv[ak], "-db"))
	  { int ni = sscanf(&argv[ak][3], "%x", &debugbits);
	    unless (ni == 1) usage();
	    ak++;
	  }
	else if (starts(argv[ak], "-s"))
	  { int ni = sscanf(&argv[ak][2], "%x:%hd", &tuser.faddr, &tuser.fport);
	    unless (ni == 2) usage();
	    sopt = true;
	    ak++;
	  }
	else if (starts(argv[ak], "-fd"))
	  { int ni = sscanf(&argv[ak][3], "%d", &fdnum);
	    unless (ni == 1) usage();
	    fdopt = true;
	    ak++;
	  }
	else usage();
      }
    if (sopt && fdopt) giveup("can't specify both -s and -fd");
    if (ak < argc) fname = argv[ak++]; else usage();
    if (ak < argc) usage();
  }

static usage()
  { fprintf(stderr, "Usage: vwg [-db<hexbits>] [-s<host:port>] [-fd<num>] fn\n");
    exit(1);
  }

static setpinpout()
  { if (sopt)
      { int code;
	fdnum = tcp_sock();
	if (fdnum < 0) giveup("tcp_sock failed");
	tuser.lport = tuser.param = 0;
	code = tcp_connect(fdnum, &tuser);
	if (code < 0) giveup("tcp_connect failed");
      }
    if (sopt || fdopt)
      { pinfile = fdopen(fdnum, "r"); poutfile = fdopen(fdnum, "w");
	if (pinfile == NULL || poutfile == NULL) giveup("fdopen failed");
      }
    else
      { pinfile = stdin;
	poutfile = stdout;
      }
  }

static giveup(msg, p1) char *msg, *p1;
  { fprintf(stderr, "vwg: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

