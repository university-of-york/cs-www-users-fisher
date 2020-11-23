/* gencode - use "-l" output from mkfilter to generate C code
   to implement digital filter
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   April 1995 */

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "mkfilter.h"

global char *progname;

static char cmdline[MAXSTRING+1];
static double xcoeffs[MAXPZ+1];
static double ycoeffs[MAXPZ+1];
static int margin, nxfacs, npoles, nzeros;
static double pbgain;
static enum { none, ansic, xyc, fifi } language;

static void usage(), readcmdline(char*[]);
static void compilefifi(), prshift(char*, int);
static void compileclear();
static void comp_fir(), comp_iir(), pr_shiftdown(char*, int);
static void pr_xcoeffs(), pr_ycoeffs(), pr_xpart(char*), prxfac(double), pr_ypart(char*), prnl();
static void giveup(char*, int = 0, int = 0);


global int main(int argc, char *argv[])
  { readcmdline(argv);
    readdata(cmdline, pbgain, nzeros, xcoeffs, npoles, ycoeffs);
    if (language == fifi) compilefifi(); else compileclear();
    return 0;
  }

static void readcmdline(char *argv[])
  { language = none;
    int ap = 0;
    progname = (argv[ap] != NULL) ? argv[ap++] : "???";
    until (argv[ap] == NULL)
      { char *s = argv[ap++];
	if (seq(s, "-ansic")) language = ansic;
	else if (seq(s, "-xyc")) language = xyc;
	else if (seq(s, "-f")) language = fifi;
	else usage();
      }
    if (language == none) language = ansic;
  }

static void usage()
  { fprintf(stderr, "Gencode version %s from <fisher@minster.york.ac.uk>\n", VERSION);
    fprintf(stderr, "Usage: gencode [-ansic | -xyc | -f]\n");
    exit(1);
  }

static void compilefifi()
  { bool mvavg = false;
    if (npoles == nzeros)
      { int n = 0;
	while (n < npoles && ycoeffs[n] == 0.0 && xcoeffs[n] == 1.0) n++;
	if (n >= npoles) mvavg = true;
      }
    margin = 11;
    printf("static float _fstepf_$(filter *fi, float x)\n");
    printf("  { /* %s */\n", cmdline);
    printf("    float *v = fi -> v;\n");
    if (mvavg)
      { printf("    float &sum = fi -> sum; int &ptr = fi -> ptr;\n");
	printf("    sum += (x - v[ptr]);\n");
	printf("    v[ptr] = x;\n");
	printf("    if (++ptr == %d) ptr = 0;\n", npoles+1);
	printf("    return %18.10e * sum;\n", 1.0/pbgain);
      }
    else
      { prshift("v", npoles);
	printf("    v[%d] =    (%18.10e * x)", npoles, 1.0/pbgain);
	pr_ypart("v");
	printf(";\n");
	printf("    return ");
	pr_xpart("v");
	printf(";\n");
      }
    printf("  }\n\n");
    printf("static fspec _fspecs_$ = { %d, %d, _fstepf_$ };\n\n", nzeros, npoles);
  }

static void prshift(char *vs, int n)
  { if (n > 0)
      { for (int i=0; i < n; i++)
	  { if (i%4 == 0) printf("    ");
	    printf("%s[%d] = %s[%d]; ", vs, i, vs, i+1);
	    if (i%4 == 3) putchar('\n');
	  }
	unless (n%4 == 0) putchar('\n');
      }
  }

static void compileclear()
  { if (language == xyc)
      { printf("%15.8e", pbgain);
	for (int i=0; i <= nzeros; i++) printf("\t%15.8e", xcoeffs[i]);
	for (int i=0; i <= npoles; i++) printf("\t%15.8e", ycoeffs[i]);
	putchar('\n');
      }
    else
      { printf("/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher\n");
	printf("   Command line: %s */\n\n", cmdline);
	int n = 0;
	while (n < npoles && ycoeffs[n] == 0.0) n++;
	if (n >= npoles) comp_fir(); else comp_iir();
      }
  }

static void comp_fir()
  { printf("#define NZEROS %d\n", nzeros);
    printf("#define GAIN   %15.9e\n\n", pbgain);
    printf("static float xv[NZEROS+1];\n\n");
    pr_xcoeffs();
    printf("static void filterloop()\n");
    printf("  { for (;;)\n");
    printf("      { float sum; int i;\n");
    printf("        for (i = 0; i < NZEROS; i++) xv[i] = xv[i+1];\n");
    printf("        xv[NZEROS] = `next input value' / GAIN;\n");
    printf("        sum = 0.0;\n");
    printf("        for (i = 0; i <= NZEROS; i++) sum += (xcoeffs[i] * xv[i]);\n");
    printf("        `next output value' = sum;\n");
    printf("      }\n");
    printf("  }\n\n");
  }

static void comp_iir()
  { printf("#define NZEROS %d\n", nzeros);
    printf("#define NPOLES %d\n", npoles);
    printf("#define GAIN   %15.9e\n\n", pbgain);
    printf("static float xv[NZEROS+1], yv[NPOLES+1];\n\n");
    margin = 20;
    printf("static void filterloop()\n");
    printf("  { for (;;)\n");
    printf("      { "); pr_shiftdown("xv", nzeros); putchar('\n');
    printf("        xv[%d] = `next input value' / GAIN;\n", nzeros);
    printf("        "); pr_shiftdown("yv", npoles); putchar('\n');
    printf("        yv[%d] =", npoles);
    pr_xpart("xv"); pr_ypart("yv"); printf(";\n");
    printf("        `next output value' = yv[%d];\n", npoles);
    printf("      }\n");
    printf("  }\n\n");
  }

static void pr_shiftdown(char *vs, int n)
  { for (int i = 0; i < n; i++) printf("%s[%d] = %s[%d]; ", vs, i, vs, i+1);
  }

static void pr_xcoeffs()
  { printf("static float xcoeffs[] =\n  {");
    for (int i=0; i <= nzeros; i++)
      { if (i > 0 && i%4 == 0) printf("\n   ");
	printf(" %+0.10f,", xcoeffs[i]);
      }
    printf("\n  };\n\n");
  }

static void pr_ycoeffs()
  { printf("static float ycoeffs[] =\n  {");
    for (int i=0; i < npoles; i++)
      { if (i > 0 && i%4 == 0) printf("\n   ");
	printf(" %+0.10f,", ycoeffs[i]);
      }
    printf("\n  };\n\n");
  }

static void pr_xpart(char *vs)
  { /* output contribution from X vec */
    nxfacs = 0;
    for (int i=0; i < (nzeros+1)/2; i++)
      { int j = nzeros-i;
	double xi = xcoeffs[i], xj = xcoeffs[j];
	/* they should be paired, except for allpass resonator */
	if (xi == xj)
	  { unless (xi == 0.0) { prxfac(xi); printf("(%s[%d] + %s[%d])", vs, i, vs, j); }
	  }
	else if (xi == -xj)
	  { if (xi > 0.0) { prxfac(xi); printf("(%s[%d] - %s[%d])", vs, i, vs, j); }
	    else { prxfac(xj); printf("(%s[%d] - %s[%d])", vs, j, vs, i); }
	  }
	else
	  { unless (xi == 0.0) { prxfac(xi); printf("%s[%d]", vs, i); }
	    unless (xj == 0.0) { prxfac(xj); printf("%s[%d]", vs, j); }
	  }
      }
    if ((nzeros+1) & 1)
      { int j = nzeros/2;
	double xj = xcoeffs[j];
	unless (xj == 0.0) { prxfac(xj); printf("%s[%d]", vs, j); }
      }
  }

static void prxfac(double x)
  { if (nxfacs > 0 && nxfacs%3 == 0) prnl();
    if (x > 0.0) printf((nxfacs > 0) ? " + " : "   ");
    if (x < 0.0) { printf(" - "); x = -x; }
    unless (x == 1.0)
      { double f = fmod(x, 1.0);
	char *fmt = (f < EPS || f > 1.0-EPS) ? "%g" : "%14.10f";
	printf(fmt, x); printf(" * ");
      }
    nxfacs++;
  }

static void pr_ypart(char *vs)
  { /* output contribution from Y vec */
    for (int i=0; i < npoles; i++)
      { if (i%2 == 0) prnl();
	printf(" + (%14.10f * %s[%d])", ycoeffs[i], vs, i);
      }
  }

static void prnl()
  { putchar('\n');
    for (int j = 0; j < margin; j++) putchar(' ');
  }

static void giveup(char *msg, int p1, int p2)
  { fprintf(stderr, "gencode: ");
    fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    exit(1);
  }

