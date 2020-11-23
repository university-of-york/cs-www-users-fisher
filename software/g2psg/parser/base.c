/* G2PSG parser generator   A.J. Fisher	  September 1990 */
/* root module */

#include "g2psg.h"
#include <stdio.h>

static uint hpbase;

extern uint sbrk();

global FILE *output;
global bool verbose, errors;


global main(argc,argv) int argc; char *argv[];
  { uint hpbot = sbrk(0);
    hpbase = hpbot;
    verbose = errors = false;
    openfiles(argv);
    reportheap("before RG");
    readgrammar();
    reportheap("after RG ");
    unless (errors)
      { transstarters();
	reportheap("after TS ");
	maketab();
	reportheap("after MT ");
      }
    fclose(output);
    hpbase = hpbot;
    reportheap("total    ");
    exit(errors ? 2 : 0);
  }

static openfiles(argv) char *argv[];
  { int k = 0; char *fn;
    if (argv[k] != NULL) k++; /* skip program name */
    fn = argv[k++];
    while (fn != NULL && fn[0] == '-')
      { if (fn[1] == 'v') verbose = true;
	else usage();
	fn = argv[k++];
      }
    if (fn == NULL) usage();
    output = fopen(fn,"w");
    if (output == NULL)
      { fprintf(stderr, "g2psg: can't open %s\n", fn);
	exit(1);
      }
  }

static usage()
  { fprintf(stderr, "Usage: g2psg [-v] objfn\n");
    exit(1);
  }

static reportheap(s) char *s;
  { uint top = sbrk(0);
    if (verbose) fprintf(stderr, "Heap used:  (%s)  %8x-%8x (%8d bytes)\n", s, hpbase, top-1, top-hpbase);
    hpbase = top;
  }
