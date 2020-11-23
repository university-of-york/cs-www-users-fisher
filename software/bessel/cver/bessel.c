#include <stdio.h>
#include "bessel.h"

static int coeffs[3][MAXORDER+1];	/* coeffs for B[n-2], B[n-1], B[n] */
static struct complex roots[MAXORDER];

static struct complex cone = { 1.0, 0.0 };

extern double cmag();
extern double pow();


global main()
  { int n;
    coeffs[1][0] = 1;			/* B0 = 1   */
    coeffs[2][0] = coeffs[2][1] = 1;	/* B1 = s+1 */
    findroots(1);
    for (n=2; n <= MAXORDER; n++)
      { nextorder(n);
	findroots(n);
      }
    exit(0);
  }

static nextorder(n) int n;
  { int j;
    memcpy(coeffs[0], coeffs[1], (n-1) * sizeof(int));
    memcpy(coeffs[1], coeffs[2], n * sizeof(int));
    for (j=0; j <= n; j++)
      { coeffs[2][j] = (j < n) ? (2*n-1) * coeffs[1][j] : 0;
	if (j >= 2) coeffs[2][j] += coeffs[0][j-2];
      }
  }

static findroots(n) int n;
  { struct complex cco[MAXORDER+1]; double prod, fac;
    int i;
    for (i=0; i <= n; i++)
      { cco[i].re = (double) coeffs[2][i];
	cco[i].im = 0.0;
      }
    zroots(cco, n, roots, true);
    /* normalize */
    prod = 1.0;
    for (i=0; i < n; i++) prod *= cmag(roots[i]);
    fac = pow(prod, -1.0 / (double) n);
    for (i=0; i < n; i++)
      { roots[i].re *= fac;
	roots[i].im *= fac;
      }
    /* print only one member of each C.Conj. pair */
    if (n & 1)
      { roots[0].im = 0.0;   /* this shd be the real root */
	printroot(0);
	for (i=1; i < n-1; i += 2) printroot(i);
      }
    else
      { for (i=0; i < n-1; i += 2) printroot(i);
      }
  }

static printroot(n) int n;
  { printf("    { %15.6e, %15.6e },\n", roots[n].re, roots[n].im);
  }

