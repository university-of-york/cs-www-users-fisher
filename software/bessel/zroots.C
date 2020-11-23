/* Laguerre root-finder */

#include <stdio.h>
#include <math.h>

#include "complex.h"
#include "bessel.h"

#define MAXIT	80
#define EPSS	1e-14

static bool laguer(complex[], int, complex&);
static double randstart();
static void deflate(complex[], int, complex);
static void sortroots(complex[], int);
static void swap(complex&, complex&);


global void zroots(complex a[], int n, complex roots[], bool polish)
  { /* Numerical Recipes p. 374 */
    complex ad[MAXORDER+1];
    memcpy(ad, a, (n+1) * sizeof(complex));
    for (int j = n; j >= 1; j--)
      { complex x; bool ok;
	do
	  { x = complex(randstart(), randstart());	/* random starting point */
	    ok = laguer(ad, j, x);
	  }
	until (ok);
	roots[j-1] = x;
	deflate(ad, j, x);
      }
    if (polish)
      { for (int j = 0; j < n; j++)
	  { bool ok = laguer(a, n, roots[j]);
	    unless (ok)
	      { fprintf(stderr, "Polish failed! root %d\n", j);
		exit(1);
	      }
	  }
      }
    sortroots(roots, n);
  }

static bool laguer(complex a[], int n, complex &x)
  { /* Numerical Recipes p. 373 */
    complex cn = complex(n);
    for (int iter = 0; iter < MAXIT; iter++)
      { complex dx, b, d, f, g, h, sq, gp, gm, g2;
	double abx, abp, abm, err;
	b = a[n];
	err = hypot(b);
	d = f = 0.0;
	abx = hypot(x);
	for (int j = n-1; j >= 0; j--)
	  { f = x*f + d;
	    d = x*d + b;
	    b = x*b + a[j];
	    err = hypot(b) + abx*err;
	  }
	err *= EPSS;
	if (hypot(b) <= err) return true;
	g = d/b;
	g2 = g*g;
	h = g2 - 2.0*(f/b);
	sq = csqrt(((cn-1.0)*((cn*h)-g2)));
	gp = g + sq;
	gm = g - sq;
	abp = hypot(gp);
	abm = hypot(gm);
	if (abp < abm) gp = gm;
	if (abp > 0.0 || abm > 0.0) dx = cn/gp;
	else
	  { complex w = complex(log(1.0 + abx), (double) (iter+1));
	    dx = cexp(w);
	  }
	x -= dx;
      }
    return false; /* too many iterations */
  }

static double randstart()
  { return -1.0 + 2.0 * drand48();	/* range -1 .. +1 */
  }

static void deflate(complex ad[], int n, complex x)
  { /* fwd deflation: divide poly "ad" by (x-r) */
    complex b = ad[n];
    for (int j = n-1; j >= 0; j--)
      { complex c = ad[j];
	ad[j] = b;
	b = x*b + c;
      }
  }

static void sortroots(complex roots[], int n)
  { /* sort roots in order of increasing abs imag part */
    bool again;
    do
      { again = false;
	for (int j = 0; j < n-1; j++)
	  { if (fabs(roots[j].im) > fabs(roots[j+1].im))
	      { swap(roots[j], roots[j+1]);
		again = true;
	      }
	  }
      }
    while (again);
    /* ensure conjugate pairs are listed neg, pos */
    int js = (n & 1) ? 1 : 0;
    for (int j = js; j < n-1; j += 2)
      { if (roots[j].im > roots[j+1].im) swap(roots[j], roots[j+1]);
      }
  }

static void swap(complex &r1, complex &r2)
  { complex r;
    r = r1; r1 = r2; r2 = r;
  }

