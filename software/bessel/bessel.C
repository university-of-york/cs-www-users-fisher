#include <stdio.h>
#include <new.h>
#include <math.h>

#include "complex.h"
#include "bessel.h"

#define MAXITS	  100
#define EPS	  1e-14

static void noroom(), nextorder(int**, int), findroots(int*, int), normalize(complex*, int);
static double evalprod(complex*, int);
static void printroots(complex*, int), prroot(complex);


global void main()
  { set_new_handler(noroom);
    static int **coeffs = new int*[3];	/* coeffs for B[n-2], B[n-1], B[n] */
    for (int i = 0; i < 3; i++) coeffs[i] = new int[MAXORDER+1];
    coeffs[1][0] = 1;			/* B0 = 1   */
    coeffs[2][0] = coeffs[2][1] = 1;	/* B1 = s+1 */
    findroots(coeffs[2], 1);
    for (int n = 2; n <= MAXORDER; n++)
      { nextorder(coeffs, n);
	findroots(coeffs[2], n);
      }
    exit(0);
  }

static void noroom()
  { fprintf(stderr, "No room!\n");
    exit(1);
  }

static void nextorder(int **coeffs, int n)
  { memcpy(coeffs[0], coeffs[1], (n-1) * sizeof(int));
    memcpy(coeffs[1], coeffs[2], n * sizeof(int));
    for (int j = 0; j <= n; j++)
      { coeffs[2][j] = (j < n) ? (2*n-1) * coeffs[1][j] : 0;
	if (j >= 2) coeffs[2][j] += coeffs[0][j-2];
      }
  }

static void findroots(int *ico, int n)
  { complex cco[MAXORDER+1], roots[MAXORDER];
    for (int i = 0; i <= n; i++) cco[i] = complex(ico[i]);
    zroots(cco, n, roots, true);
    if (n & 1) roots[0].im = 0.0;   /* this shd be the real root */
    normalize(roots, n);
    printroots(roots, n);
  }

static void normalize(complex *roots, int n)
  { for (int i = 0; i < MAXITS; i++)
      { double r = evalprod(roots, n);
	double fac = r / M_SQRT2;
	double eps = fabs(fac - 1.0);
	// fprintf(stderr, "??? n=%d r=%15.6e fac=%15.6e eps=%15.6e\n", n, r, fac, eps);
	if (eps < EPS) return;
	for (int j = 0; j < n; j++) roots[j] *= fac;
      }
    fprintf(stderr, "n=%d Newton failed to converge\n", n);
  }

static double evalprod(complex *roots, int n)
  { complex jay = complex(0.0, 1.0), prod = 1.0;
    for (int i = 0; i < n; i++) prod *= (jay/roots[i] - 1.0);
    return hypot(prod);
  }

static void printroots(complex *roots, int n)
  { /* print only one member of each C.Conj. pair */
    if (n & 1)
      { prroot(roots[0]);
	for (int i = 1; i < n-1; i += 2) prroot(roots[i]);
      }
    else
      { for (int i = 0; i < n-1; i += 2) prroot(roots[i]);
      }
  }

static void prroot(complex z)
  { printf("    { %20.11e, %20.11e},\n", z.re, z.im);
  }

