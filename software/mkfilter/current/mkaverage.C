/* mkaverage -- design moving-average FIR filter
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   July 1998 */

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "mkfilter.h"

static bool lflag;
static int numcoeffs;

static void readcmdline(char**);
static void usage();
static void printresults(char**), printcmdline(char**);
static void giveup(char*, int = 0);


global int main(int argc, char *argv[])
  { readcmdline(argv);
    if (lflag) printresults(argv); else printf("OK!\n");
    return 0;
  }

static void readcmdline(char *argv[])
  { int ap = 0, n = 0;
    unless (argv[ap] == NULL) ap++;	/* skip program name */
    unless (argv[ap] != NULL && seq(argv[ap++], "-Av")) usage();
    lflag = false;
    until (argv[ap] == NULL)
      { char *s = argv[ap++];
	if (seq(s, "-l")) lflag = true;
	else if (n == 0) { numcoeffs = atoi(s); n++; }
	else usage();
      }
    unless (n == 1) usage();
    /* (num.zeros) = (num.coeffs) - 1 */
    if (numcoeffs < 1 || numcoeffs > MAXPZ+1) giveup("filter length must be in range 1 .. %d", MAXPZ+1);
  }

static void usage()
  { fprintf(stderr, "Mkaverage V.%s from <fisher@minster.york.ac.uk>\n", VERSION);
    fprintf(stderr, "Usage: mkaverage -Av [-l] <len>\n");
    exit(1);
  }

static void printresults(char *argv[])
  { int i;
    printcmdline(argv);
    printf("G  = %d\n", numcoeffs);
    printf("NZ = %d\n", numcoeffs-1);
    for (i = 0; i < numcoeffs; i++) printf("1\n");
    printf("NP = %d\n", numcoeffs-1);
    for (i = 0; i < numcoeffs-1; i++) printf("0\n");
    printf("-1\n");
  }

static void printcmdline(char *argv[])
  { int k = 0;
    until (argv[k] == NULL)
      { if (k > 0) putchar(' ');
	fputs(argv[k++], stdout);
      }
    putchar('\n');
 }

static void giveup(char *msg, int p1)
  { fprintf(stderr, "mkaverage: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

