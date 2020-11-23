/* gencode - use "-l" output from mkfilter to generate C code
   to implement digital filter
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   April 1995 */

#include <stdio.h>
#include "mkfilter.h"

static char cmdline[256];
static int xcoeffs[MAXPOLES+1];
static double ycoeffs[MAXPOLES+1];
static int npoles;


global main(argc, argv) int argc; char *argv[];
  { unless (argc == 1) usage();
    readdata();
    compilecode();
    exit(0);
  }

static usage()
  { fprintf(stderr, "Usage: gencode\n");
    exit(1);
  }

static readdata()
  { int ni, i;
    ni = scanf("%[^\n]\n", cmdline);
    unless (ni == 1) formaterror(1);
    ni = scanf("%d\n", &npoles);
    unless (ni == 1 && npoles >= 0 && npoles <= MAXPOLES) formaterror(2);
    for (i=0; i <= npoles; i++)
      { ni = scanf(" %d %lg\n", &xcoeffs[i], &ycoeffs[i]);
	unless (ni == 2) formaterror(3);
      }
    unless (ycoeffs[npoles] == -1.0) formaterror(4);
  }

static formaterror(n) int n;
  { giveup("input format error (%d)", n);
  }

static compilecode()
  { int i;
    printf("/* Digital filter designed by mkfilter/gencode   A.J. Fisher\n");
    printf("   Command line: %s */\n\n", cmdline);
    printf("#define NPOLES %d\n\n", npoles);
    printf("/* the following relies on memcpy's working with overlapping areas (which it does) */\n");
    printf("#define shiftdown(v,n) memcpy(&v[0], &v[1], (n) * sizeof(float));\n\n");
    printf("static float xvals[NPOLES+1], yvals[NPOLES+1];\n\n");
    printf("static float ycoeffs[] =\n  {");
    for (i=0; i < npoles; i++)
      { if (i > 0 && i%5 == 0) printf("\n   ");
	printf(" %+0.10f,", ycoeffs[i]);
      }
    printf("\n  };\n\n");
    printf("static filterloop()\n");
    printf("  { for (;;)\n");
    printf("      { shiftdown(xvals, NPOLES);\n");
    printf("        xvals[NPOLES] = <i>next input value</i>;\n");
    printf("        filterstep(xvals, yvals, ycoeffs);\n");
    printf("        <i>next output value</i> = yvals[NPOLES];\n");
    printf("      }\n");
    printf("  }\n\n");
    printf("static filterstep(xv, yv, yco) float xv[], yv[], yco[];\n");
    printf("  { shiftdown(yv, NPOLES);\n");
    printf("    yv[%d] =", npoles);
    for (i=0; i < (npoles+1)/2; i++)
      { int j = npoles-i;
	int xi = xcoeffs[i], xj = xcoeffs[npoles-i];
	/* they should be paired */
	if (xi == xj)
	  { unless (xi == 0)
	      { prxfac(xi);
		printf("(xv[%d] + xv[%d])", i, j);
	      }
	  }
	else if (xi == -xj && xi > 0)
	  { prxfac(xi);
	    printf("(xv[%d] - xv[%d])", i, j);
	  }
	else if (xi == -xj && xi < 0)
	  { prxfac(xj);
	    printf("(xv[%d] - xv[%d])", j, i);
	  }
	else giveup("Bug: x[i] != x[j] (i=%d, j=%d)", i, j);
      }
    if ((npoles+1) & 1)
      { int j = npoles/2;
	int xj = xcoeffs[j];
	unless (xj == 0)
	  { prxfac(xj);
	    printf("xv[%d]", j);
	  }
      }
    for (i=0; i < npoles; i++)
      { if (i%4 == 0)
	  { int j;
	    putchar('\n'); for (j=0; j < 11; j++) putchar(' ');
	  }
	printf(" + (yco[%d] * yv[%d])", i, i);
      }
    printf(";\n");
    printf("  }\n\n");
  }

static prxfac(x) int x;
  { if (x > 0) printf(" + ");
    if (x < 0) { printf(" - "); x = -x; }
    unless (x == 1) printf("%d * ", x);
  }

static giveup(msg, p1, p2) char *msg; word p1, p2;
  { fprintf(stderr, "gencode: ");
    fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    exit(1);
  }

