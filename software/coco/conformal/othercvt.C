#include <stdio.h>
#include "hdr.h"
#include "projconsts.h"

static double msfn(double), tsfn(double);


global void lcc_convert(pco *pc, cco *cc)	/* Lambert conformal conic */
  { double lam, phi, n, rho0, rho;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    n = sin(PHI1*RADIANS);
    rho0 = msfn(PHI1*RADIANS) / n;
    rho = rho0 * pow(tsfn(phi) / tsfn(PHI1*RADIANS), n);
    cc -> x = rho * sin(n*lam);
    cc -> y = rho0 - rho * cos(n*lam);
  }

global void st_convert(pco *pc, cco *cc)	/* equatorial stereographic */
  { double lam, phi, coslam, sinlam, sinphi, tx, ta, stx, ctx;
    lam = (pc -> lon - THETA0) * RADIANS;
    phi = pc -> lat * RADIANS;
    coslam = cos(lam); sinlam = sin(lam); sinphi = sin(phi);
    tx = 2.0 * atan(tsfn(-phi)) - 0.5*PI;
    stx = sin(tx); ctx = cos(tx);
    ta = (4.0*ARAD) / (1.0 + ctx*coslam);
    cc -> y = ta*stx;
    cc -> x = ta*ctx*sinlam;
  }

static double msfn(double phi)
  { double sph = sin(phi), cph = cos(phi);
    return ARAD * cph / sqrt(1.0 - ESQUARED*sph*sph);
  }

static double tsfn(double phi)
  { double e = sqrt(ESQUARED);
    double esph = e*sin(phi);
    return tan(0.25*PI - 0.5*phi) * pow((1.0+esph) / (1.0-esph), 0.5*e);
  }

