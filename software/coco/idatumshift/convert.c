/* Shift datum from WGS 72 to OSGB 36 using fixed-point arithmetic
   AJF	 September 1995 */

#define DEBUG

#include <hdr.h>

#define PI	    3.1415926535897932384626433
#define TWOPI	    (2.0 * PI)

#define A1	    6378135.0			/* WGS equatorial radius		   */
#define RN1	    6392857.5750870025		/* WGS R of C perp to meridian at 2.W 56.N */
#define RM1	    6379414.7384705329		/* WGS R of C along meridian at 2.W 56.N   */

#define DELTA_A	    (-571.604)			/* OSGB - WGS				*/
#define BA1	    0.996647501855194269	/* WGS					*/
#define BA2	    0.996659149478096383	/* OSGB					*/
#define DELTA_F	    (BA1 - BA2)			/* OSGB flattening minus WGS flattening */
#define ESQ1	    6.693757045800536e-3	/* WGS					*/

#define DELTA_X	    (-375.0)			/* OSGB - WGS				*/
#define DELTA_Y	    111.0			/* OSGB - WGS				*/
#define DELTA_Z	    (-435.5)			/* OSGB - WGS				*/

#define FAC1	    (DELTA_X / RN1)
#define FAC2	    (DELTA_Y / RN1)

#define FAC3	    (DELTA_X / RM1)
#define FAC4	    (DELTA_Y / RM1)
#define FAC5	    (DELTA_Z / RM1)
#define FAC6	    (((DELTA_A * RN1*ESQ1 / A1) + (DELTA_F * (RM1/BA1 + RN1*BA1))) / RM1)

#define FXSCALE	    (1 << 28)
#define EXTRASCALE  (1 << 19)

#define GFAC1	    (FAC1 / TWOPI * EXTRASCALE)
#define GFAC2	    (FAC2 / TWOPI * EXTRASCALE)
#define GFAC3	    (FAC3 / TWOPI * EXTRASCALE)
#define GFAC4	    (FAC4 / TWOPI * EXTRASCALE)
#define GFAC5	    (FAC5 / TWOPI * EXTRASCALE)
#define GFAC6	    (FAC6 / TWOPI * EXTRASCALE)

#define IFAC1	    ifix(GFAC1 * FXSCALE)
#define IFAC2	    ifix(GFAC2 * FXSCALE)
#define IFAC3	    ifix(GFAC3 * FXSCALE)
#define IFAC4	    ifix(GFAC4 * FXSCALE)
#define IFAC5	    ifix(GFAC5 * FXSCALE)
#define IFAC6	    ifix(GFAC6 * FXSCALE)

extern double sin(), cos();

struct ipco { int lon, lat };	  /* polar coords (latitude, longitude)	  */


global datumshift(pc1, pc2) struct triple *pc1, *pc2;
  { struct ipco ipco1, ipco2;
#ifdef DEBUG
    printf("GFAC1 = %.12f   IFAC1 = %11d\n", GFAC1, IFAC1);
    printf("GFAC2 = %.12f   IFAC2 = %11d\n", GFAC2, IFAC2);
    printf("GFAC3 = %.12f   IFAC3 = %11d\n", GFAC3, IFAC3);
    printf("GFAC4 = %.12f   IFAC4 = %11d\n", GFAC4, IFAC4);
    printf("GFAC5 = %.12f   IFAC5 = %11d\n", GFAC5, IFAC5);
    printf("GFAC6 = %.12f   IFAC6 = %11d\n", GFAC6, IFAC6);
    exit(0);
#endif
    ipco1.lon = ifix((FXSCALE/360.0) * pc1 -> lon);
    ipco1.lat = ifix((FXSCALE/360.0) * pc1 -> lat);
    iconvert(&ipco1, &ipco2);
    pc2 -> lon = (double) ipco2.lon * (360.0/FXSCALE);
    pc2 -> lat = (double) ipco2.lat * (360.0/FXSCALE);
    pc2 -> hgt = 0.0;
  }

static iconvert(ipc1, ipc2) struct ipco *ipc1, *ipc2;
  { int lam, phi, sphi, cphi, slam, clam, dlam, dphi;
    int spcl, spsl, spcp;

    lam = ipc1 -> lon;	/* FXSCALE represents 2pi radians */
    phi = ipc1 -> lat;	/* FXSCALE represents 2pi radians */
    sphi = isin(phi); cphi = icos(phi); slam = isin(lam); clam = icos(lam);

    dlam = muldiv(-IFAC1, slam, cphi) + muldiv(IFAC2, clam, cphi);

    spcl = muldiv(sphi, clam, FXSCALE);
    spsl = muldiv(sphi, slam, FXSCALE);
    spcp = muldiv(sphi, cphi, FXSCALE);

    dphi = muldiv(-IFAC3, spcl, FXSCALE) + muldiv(-IFAC4, spsl, FXSCALE) +
	   muldiv(IFAC5, cphi, FXSCALE) + muldiv(IFAC6, spcp, FXSCALE);

    ipc2 -> lon = lam + dlam/EXTRASCALE;
    ipc2 -> lat = phi + dphi/EXTRASCALE;
  }

static int isin(ix) int ix;
  { return ifix(FXSCALE * sin((double) ix * TWOPI / (double) FXSCALE));
  }

static int icos(ix) int ix;
  { return ifix(FXSCALE * cos((double) ix * TWOPI / (double) FXSCALE));
  }

static int muldiv(a, b, c) int a, b, c;
  { double da = (double) a, db = (double) b, dc = (double) c;
    return ifix(da * db / dc);
  }

static int ifix(x) double x;
  { return (x > 0.0) ? (int) (x+0.5) :
	   (x < 0.0) ? (int) (x-0.5) : 0;
  }

