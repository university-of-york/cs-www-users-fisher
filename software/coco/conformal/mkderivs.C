#include <stdio.h>
#include "hdr.h"

#define C1	1.0050365011826925e+00	    /* 1.0 + 3.0*C3			    */
#define C3	1.6788337275641298e-03	    /* 0.25 * (ESQUARED / (1.0 - ESQUARED)) */

static void readderivs(char*, derivatives*);
static void fmterror(char*);
static void convolve(derivatives*);


global void makederivs(char *fn, derivatives *derivs)
  { readderivs(fn, derivs);
    convolve(derivs);
  }

static void readderivs(char *fn, derivatives *derivs)
  { int k, ni;
    double *av = derivs -> avec[0], *bv = derivs -> bvec[0];
    FILE *fi = fopen(fn, "r");
    if (fi == NULL) giveup("Can't open %s", fn);
    ni = fscanf(fi, "%lf\n", &derivs -> linterm);
    unless (ni == 1) fmterror(fn);
    for (k = 0; k < NHARMS; k++) av[k] = bv[k] = 0.0;
    for (k = 0; k < NHARMS && !feof(fi); k++)
      { ni = fscanf(fi, "%lf %lf\n", &bv[k], &av[k]);
	unless (ni == 2) fmterror(fn);
      }
    unless (feof(fi)) giveup("Too many harmonics in file %s", fn);
    fclose(fi);
    fprintf(stderr, "%d harmonics read.\n", k);
  }

static void fmterror(char *fn)
  { giveup("Format error in file %s", fn);
  }

#define aval(n)	  (((n) > 0) ? (n)*av[n] : -(n)*av[-(n)])	/* n * a[n] */
#define bval(n)	  (((n) > 0) ? (n)*bv[n] : +(n)*bv[-(n)])	/* n * b[n] */

static void convolve(derivatives *derivs)
  { int j, k;
    double *av = derivs -> avec[0], *bv = derivs -> bvec[0];
    for (j=1; j < NDERIVS; j++)
      { double *av1 = derivs -> avec[j], *bv1 = derivs -> bvec[j];
	double fac = 1.0 / (double) (2*j);
	/* add zero padding for convolution */
	for (k=0; k < 3; k++) av[NHARMS+k] = bv[NHARMS+k] = 0.0;
	/* convolve */
	for (k=0; k < NHARMS; k++)
	  { av1[k] = -fac * (C1 * (bval(k-1) + bval(k+1)) + C3 * (bval(k-3) + bval(k+3)));
	    bv1[k] = fac * (C1 * (aval(k-1) + aval(k+1)) + C3 * (aval(k-3) + aval(k+3)));
	  }
	bv1[0] *= 0.5;
	if (j == 1)
	  { bv1[1] += C1 * derivs -> linterm;
	    bv1[3] += C3 * derivs -> linterm;
	  }
	av = av1; bv = bv1;
      }
  }

