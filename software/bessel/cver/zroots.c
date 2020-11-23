/* Laguerre root-finder */

#include <stdio.h>
#include "bessel.h"

#define MAXIT	   80
#define EPSS	   1e-14

static struct complex czero = { 0.0, 0.0 };
static struct complex cone  = { 1.0, 0.0 };
static struct complex ctwo  = { 2.0, 0.0 };

extern struct complex cadd(), csub(), cmul(), cdiv(), csqrt(), cexp();
extern double cmag();
extern double log(), fabs(), drand48();

forward double randstart();


global zroots(a, n, roots, polish) struct complex a[]; int n; struct complex roots[]; bool polish;
  { /* Numerical Recipes p. 374 */
    int j;
    struct complex ad[MAXORDER+1];
    memcpy(ad, a, (n+1) * sizeof(struct complex));
    for (j=n; j >= 1; j--)
      { struct complex x; bool ok;
	do
	  { x.re = randstart(); x.im = randstart();
	    ok = laguer(ad, j, &x);
	  }
	until (ok);
	roots[j-1] = x;
	deflate(ad, j, x);
      }
    if (polish)
      { for (j=0; j < n; j++)
	  { bool ok = laguer(a, n, &roots[j]);
	    unless (ok)
	      { fprintf(stderr, "Polish failed! root %d\n", j);
		exit(1);
	      }
	  }
      }
    sortroots(roots, n);
  }

static bool laguer(a, n, x) struct complex a[]; int n; struct complex *x;
  { /* Numerical Recipes p. 373 */
    struct complex cn; int iter;
    cn.re = (double) n; cn.im = 0.0;
    for (iter=0; iter < MAXIT; iter++)
      { struct complex dx, b, d, f, g, h, sq, gp, gm, g2;
	double abx, abp, abm, err; int j;
	b = a[n];
	err = cmag(b);
	d = f = czero;
	abx = cmag(*x);
	for (j = n-1; j >= 0; j--)
	  { f = cadd(cmul(*x, f), d);
	    d = cadd(cmul(*x, d), b);
	    b = cadd(cmul(*x, b), a[j]);
	    err = cmag(b) + abx*err;
	  }
	err *= EPSS;
	if (cmag(b) <= err) return true;
	g = cdiv(d, b);
	g2 = cmul(g, g);
	h = csub(g2, cmul(ctwo, cdiv(f, b)));
	sq = csqrt(cmul(csub(cn, cone), csub(cmul(cn, h), g2)));
	gp = cadd(g, sq);
	gm = csub(g, sq);
	abp = cmag(gp);
	abm = cmag(gm);
	if (abp < abm) gp = gm;
	if (abp > 0.0 || abm > 0.0) dx = cdiv(cn, gp);
	else
	  { struct complex w;
	    w.re = log(1.0 + abx);
	    w.im = (double) (iter+1);
	    dx = cexp(w);
	  }
	*x = csub(*x, dx);
      }
    return false; /* too many iterations */
  }

static double randstart()
  { return -1.0 + 2.0 * drand48();	/* range -1 .. +1 */
  }

static deflate(ad, n, x) struct complex ad[]; int n; struct complex x;
  { /* fwd deflation: divide poly "ad" by (x-r) */
    struct complex b, c; int j;
    b = ad[n];
    for (j = n-1; j >= 0; j--)
      { c = ad[j];
	ad[j] = b;
	b = cadd(cmul(x, b), c);
      }
  }

static sortroots(roots, n) struct complex roots[]; int n;
  { int j, js; bool again;
    /* sort roots in order of increasing abs imag part */
    do
      { again = false;
	for (j=0; j < n-1; j++)
	  { if (fabs(roots[j].im) > fabs(roots[j+1].im))
	      { swap(&roots[j], &roots[j+1]);
		again = true;
	      }
	  }
      }
    while (again);
    /* ensure conjugate pairs are listed neg, pos */
    js = (n & 1) ? 1 : 0;
    for (j=js; j < n-1; j += 2)
      { if (roots[j].im > roots[j+1].im) swap(&roots[j], &roots[j+1]);
      }
  }

static swap(rr1, rr2) struct complex *rr1, *rr2;
  { struct complex r;
    r = *rr1; *rr1 = *rr2; *rr2 = r;
  }

