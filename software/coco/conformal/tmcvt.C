#include <stdio.h>
#include "hdr.h"
#include "projconsts.h"

#define POLY0(x,a0)		(a0)
#define POLY1(x,a1,a0)		(POLY0(x,a1) * x + a0)
#define POLY2(x,a2,a1,a0)	(POLY1(x,a2,a1) * x + a0)
#define POLY3(x,a3,a2,a1,a0)	(POLY2(x,a3,a2,a1) * x + a0)

static double meridist(double);


global void tm_convert(pco *pc, cco *cc)	/* transverse Mercator */
  { /* given lon, lat, compute OS coords x, y, by series (PROJ) method */
    double lam, phi, sph, cph, tph, tph2, lcph, lsph, lcph2, nu, eta2, poly;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    sph = sin(phi); cph = cos(phi); tph = sph/cph; tph2 = tph*tph;
    lcph = lam*cph; lsph = lam*sph; lcph2 = lcph*lcph;
    nu = ARAD / sqrt(1.0 - ESQUARED*sph*sph); /* R of C perp to meridian */
    eta2 = (ESQUARED*cph*cph) / (1.0-ESQUARED);		/* eta^2 = (nu/rho)-1	   */

    poly = POLY3(lcph2, POLY3(tph2, -1, 179, -479, 61) / 5040,
			(POLY2(tph2, 1, -18, 5) + eta2 * POLY1(tph2, -58, 14)) / 120,
			(1 - tph2 + eta2) / 6,
			1);

    cc -> x = X0 + lcph*nu*poly;

    poly = POLY3(lcph2, POLY3(tph2, -1, 543, -3111, 1385) / 40320,
			(POLY2(tph2, 1, -58, 61) + eta2 * POLY1(tph2, -330, 270)) / 720,
			(5 - tph2 + eta2 * (4*eta2 + 9)) / 24,
			0.5);

    cc -> y = Y0 + meridist(phi) - meridist(PHI0*RADIANS) + lsph*lcph*nu*poly;
  };

/* meridional distance for ellipsoid
   8th degree - accurate to < 1e-5 meters when used in conjunction
   with typical major axis values */

#define EPS3	6.0905227287964770e-10
#define EPS2	1.0820180907122924e-07
#define EPS1	2.0853669848390807e-05
#define EPS0	5.0008134269452229e-03
#define EPSK	9.9833027342682179e-01

static double meridist(double phi)
  { /* meridional distance to latitude phi */
    double sph, cph, cs, ss, p;
    sph = sin(phi); cph = cos(phi);
    cs = cph*sph; ss = sph*sph;
    p = POLY3(ss, EPS3, EPS2, EPS1, EPS0);
    return ARAD * (EPSK*phi - p*cs);
  }

