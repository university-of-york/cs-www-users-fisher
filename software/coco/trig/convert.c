/* compute TM projection by Fourier series method
   A.J. Fisher	 July 1995 */

#include <stdio.h>
#include <hdr.h>

#include "trighdr.h"

#define word	    int				/* or char* or ... */

#define AIRY_A	    6377563.396			/* radius of earth (semi-major axis)	     */
#define AIRY_B	    6356256.910			/* radius of earth (semi-minor axis)	     */
#define SCALE	    0.9996012717		/* scale factor on central meridian	     */
#define X0	    400000.0			/* X origin				     */
#define Y0	    (-100000.0)			/* Y origin				     */
#define M0	    5427063.815			/* meridional distance to PHI0, adj by SCALE */
#define THETA0	    (-2.0)			/* central meridian (degrees)		     */
#define PHI0	    49.0			/* "true origin" latitude                    */

#define EPS1	    1e-5			/* optimum value depends on NDERIVS & NHARMS */
#define EPS2	    1e-7			/* optimum value depends on NDERIVS & NHARMS */

#define ARAD	    (AIRY_A*SCALE)
#define PI	    3.1415926535897932384626433
#define RADIANS	    (PI / 180.0)

extern double sin(), cos(), hypot();

extern double linterm;
extern double cvec[NDERIVS][NHARMS/2];


global polar_to_os(pc, cc) struct pco *pc; struct cco *cc;
  { /* given polar coords lon, lat, compute OS coords x, y, by Fourier series method */
    double lam, phi, x, y; int k, maxp; bool cvg1;
    double powers[NDERIVS];
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    maxp = 0;  powers[maxp++] = 1.0; powers[maxp++] = lam;   /* remember powers of lam */
    x = 0.0; y = linterm*phi; cvg1 = false;
    for (k=0; k < NHARMS/2 && !cvg1; k++)
      { double asum, dsum; int i; bool cvg2;
	asum = dsum = 0.0; cvg2 = false;
	/* printf("k=%2d", k); */
	for (i=0; i < NDERIVS-1 && !cvg2; i += 2)
	  { double at, dt;
	    while (maxp < i+2)
	      { powers[maxp++] = -lam * powers[maxp-1];
		powers[maxp++] = +lam * powers[maxp-1];
	      }
	    at = powers[i] * cvec[i][k];
	    dt = powers[i+1] * cvec[i+1][k];
	    asum += at; dsum += dt;
	    if (hypot(at, dt) < EPS2) cvg2 = true;
	    /* printf(" [%14.6e %14.6e]", at, dt); */
	  }
	/* printf(" asum=%14.6e dsum=%14.6e\n", asum, dsum); */
	unless (cvg2) giveup("Failed to converge! (2)");
	y += asum * sin((2*k) * phi);
	x += dsum * cos((2*k+1) * phi);
	if (k > 0 && hypot(asum, dsum) < EPS1) cvg1 = true;	 /* first harmonic can be zero if lon == THETA0! */
      }
    unless (cvg1) giveup("Failed to converge! (1)");
    cc -> x = X0 + ARAD*x;
    cc -> y = (Y0-M0) + ARAD*y;
  }

static giveup(msg, p1) char *msg; word p1;
  { fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

