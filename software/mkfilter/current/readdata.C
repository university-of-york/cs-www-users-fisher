/* mkfilter -- given n, compute recurrence relation
   to implement Butterworth, Bessel or Chebyshev filter of order n
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   September 1992 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "mkfilter.h"
#include "complex.h"

static void rdcoeffs(char, int&, double*);
static int rdint(char*);
static double rddouble(char*);
static void getline(char*, int&, int, char*), rdline(char*), formaterror(int);


global void readdata(char *cmdline, double &pbgain, int &nzeros, double *xcoeffs, int &npoles, double *ycoeffs)
  { rdline(cmdline);
    pbgain = rddouble("G  = ");
    rdcoeffs('Z', nzeros, xcoeffs);
    rdcoeffs('P', npoles, ycoeffs);
    unless (ycoeffs[npoles] == -1.0) formaterror(1);
    if (nzeros > npoles) formaterror(2);
  }

static void rdcoeffs(char c, int &n, double *coeffs)
  { char fmt[6]; strcpy(fmt, "N? = "); fmt[1] = c;
    n = rdint(fmt);
    unless (n >= 0 && n <= MAXPZ) formaterror(3);
    for (int i = 0; i <= n; i++) coeffs[i] = rddouble("");
  }

static int rdint(char *exp)
  { char vec[MAXSTRING+1]; int p;
    getline(vec, p, 4, exp);
    return atoi(&vec[p]);
  }

static double rddouble(char *exp)
  { char vec[MAXSTRING+1]; int p;
    getline(vec, p, 5, exp);
    return atof(&vec[p]);
  }

static void getline(char *vec, int &p, int e, char *exp)
  { rdline(vec);
    p = strlen(exp);
    unless (memcmp(vec, exp, p) == 0) formaterror(e);
    while (vec[p] == ' ' || vec[p] == '\t') p++;
    unless ((vec[p] >= '0' && vec[p] <= '9') ||
	    (vec[p] == '+') || (vec[p] == '-') || (vec[p] == '.')) formaterror(e);
  }

static void rdline(char vec[])
  { int n = 0;
    int ch = getchar();
    until (n >= MAXSTRING || ch == '\n' || ch < 0)
      { vec[n++] = ch;
	ch = getchar();
      }
    if (n == 0 && ch < 0) formaterror(6);
    vec[n] = '\0';
    until (ch == '\n' || ch < 0) ch = getchar();
  }

static void formaterror(int n)
  { fprintf(stderr, "%s: input format error (%d)\n", progname, n);
    exit(1);
  }

