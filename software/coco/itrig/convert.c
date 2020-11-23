/* compute TM projection by Fourier series method
   A.J. Fisher	 July 1995 */

#include <stdio.h>
#include <hdr.h>

#include "trighdr.h"

#define word	    int				/* or char* or ... */

#define PI	    3.1415926535897932384626433
#define TWOPI	    (2.0 * PI)

#define X0	    400000			/* X origin				     */
#define Y0	    (-100000)			/* Y origin				     */
#define M0	    5427064			/* meridional distance to PHI0, adj by SCALE */
#define THETA0	    (-1491308)			/* central meridian (-2 deg)		     */
#define ARAD	    6375020			/* AIRY_A * SCALE			     */

#define IEPS1	    2684   /* 1e-5 * FXSCALE */ /* optimum value depends on NDERIVS & NHARMS */
#define IEPS2	    27	   /* 1e-7 * FXSCALE */ /* optimum value depends on NDERIVS & NHARMS */

extern double sin(), cos(), hypot();

extern int linterm;
extern int cvec[NDERIVS][NHARMS/2];

struct ipco { int lon, lat };	  /* polar coords (latitude, longitude)	  */
struct icco { int x, y };	  /* Cartesian coords (easting, northing) */


global polar_to_os(pc, cc) struct pco *pc; struct cco *cc;
  { /* given polar coords lon, lat, compute OS coords x, y, by Fourier series method */
    struct ipco ipco; struct icco icco;
    ipco.lon = ifix((FXSCALE/360.0) * pc -> lon);
    ipco.lat = ifix((FXSCALE/360.0) * pc -> lat);
    iconvert(&ipco, &icco);
    cc -> x = (double) icco.x;
    cc -> y = (double) icco.y;
  }

static int iconvert(ipc, icc) struct ipco *ipc; struct icco *icc;
  { int lam, phi, x, y, k, maxp; bool cvg1;
    int powers[NDERIVS];
    lam = muldiv(ITWOPI, ipc -> lon - THETA0, FXSCALE); /* FXSCALE represents 1 radian	  */
    phi = ipc -> lat;					/* FXSCALE represents 2pi radians */
    maxp = 0;  powers[maxp++] = FXSCALE; powers[maxp++] = lam;	 /* remember powers of lam */
    x = 0; y = muldiv(linterm, phi, FXSCALE); cvg1 = false;
    for (k=0; k < NHARMS/2 && !cvg1; k++)
      { int asum, dsum, i; bool cvg2;
	asum = dsum = 0; cvg2 = false;
	/* printf("k=%2d", k); */
	for (i=0; i < NDERIVS-1 && !cvg2; i += 2)
	  { int at, dt;
	    while (maxp < i+2)
	      { powers[maxp++] = muldiv(-lam, powers[maxp-1], FXSCALE);
		powers[maxp++] = muldiv(+lam, powers[maxp-1], FXSCALE);
	      }
	    at = muldiv(powers[i], cvec[i][k], FXSCALE);
	    dt = muldiv(powers[i+1], cvec[i+1][k], FXSCALE);
	    asum += at; dsum += dt;
	    if (ihypot(at, dt) < IEPS2) cvg2 = true;
	    /* printf(" [%d %d]", at, dt); */
	  }
	/* printf(" asum=%d dsum=%d\n", asum, dsum); */
	unless (cvg2) giveup("Failed to converge! (2)");
	y += muldiv(asum, isin((2*k) * phi), FXSCALE);
	x += muldiv(dsum, icos((2*k+1) * phi), FXSCALE);
	if (k > 0 && ihypot(asum, dsum) < IEPS1) cvg1 = true;	   /* first harmonic can be zero if lon == THETA0! */
      }
    unless (cvg1) giveup("Failed to converge! (1)");
    icc -> x = X0 + muldiv(x, ARAD, FXSCALE);
    icc -> y = (Y0-M0) + muldiv(y, ARAD, FXSCALE);
  }

static int isin(ix) int ix;
  { return ifix(FXSCALE * sin((double) ix * TWOPI / (double) FXSCALE));
  }

static int icos(ix) int ix;
  { return ifix(FXSCALE * cos((double) ix * TWOPI / (double) FXSCALE));
  }

static int ihypot(ix, iy) int ix, iy;
  { return ifix(hypot((double) ix, (double) iy));
  }

static int muldiv(a, b, c) int a, b, c;
  { double da = (double) a, db = (double) b, dc = (double) c;
    return ifix(da * db / dc);
  }

static int ifix(x) double x;
  { return (x > 0.0) ? (int) (x+0.5) :
	   (x < 0.0) ? (int) (x-0.5) : 0;
  }

static giveup(msg, p1) char *msg; word p1;
  { fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

