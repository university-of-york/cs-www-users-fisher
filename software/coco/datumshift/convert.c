/* Routines for shifting datum
   A.J. Fisher	 September 1995 */

#include <stdio.h>
#include <hdr.h>

#define PI	    3.1415926535897932384626433
#define RADIANS	    (PI / 180.0)

struct datum
  { char *name;		/* name of datum		    */
    double A, B;	/* equatorial, polar semi-diameters */
    double DX, DY, DZ;	/* ECEF shifts, datum minus WGS84   */
  };

static struct datum datumlist[] =
  { { "WGS84",  6378137.0,   6356752.3142451795,    0.0,    0.0,    0.0 },      /* World Geodetic Standard 1984     */
    { "WGS72",  6378135.0,   6356752.3142451795,    0.0,    0.0,    4.5 },      /* World Geodetic Standard 1972     */
    { "NAD27",  6378206.4,   6356583.8,             8.0, -160.0, -176.0 },      /* North American Datum 1927        */
    { "OSGB36", 6377563.396, 6356256.910,        -375.0,  111.0, -431.0 },      /* Ordnance Survey of Great Britain */
    { NULL,	0.0,	     0.0,		    0.0,    0.0,    0.0 },	/* terminator			    */
  };

extern double sin(), cos(), sqrt(), pow();

forward struct datum *lookupdatum();


global datumshift(pc1, pc2, sys1, sys2) struct triple *pc1, *pc2; char *sys1, *sys2;
  { struct datum *dat1, *dat2;
    double delta_a, delta_f, delta_x, delta_y, delta_z, ba1, ba2, esq1;
    double lam, phi, sphi, cphi, slam, clam, rn, rm, dlam, dphi, dhgt;

    dat1 = lookupdatum(sys1);
    dat2 = lookupdatum(sys2);
    delta_a = dat2 -> A - dat1 -> A;
    ba1 = dat1 -> B / dat1 -> A;
    ba2 = dat2 -> B / dat2 -> A;
    delta_f = ba1 - ba2;
    esq1 = 1.0 - ba1*ba1;
    delta_x = dat2 -> DX - dat1 -> DX;
    delta_y = dat2 -> DY - dat1 -> DY;
    delta_z = dat2 -> DZ - dat1 -> DZ;

    lam = pc1 -> lon * RADIANS;
    phi = pc1 -> lat * RADIANS;
    sphi = sin(phi); cphi = cos(phi); slam = sin(lam); clam = cos(lam);
    rn = dat1 -> A / sqrt(1.0 - esq1*sphi*sphi);			/* R of C perp to meridian */
    rm = dat1 -> A * (1.0 - esq1) / pow(1.0 - esq1*sphi*sphi, 1.5);	/* R of C along meridian   */

    dlam = ( - delta_x * slam + delta_y * clam ) / ((rn + pc1 -> hgt) * cphi);

    dphi = ( - delta_x * sphi*clam - delta_y * sphi*slam + delta_z * cphi
	     + delta_a * rn*esq1 * sphi*cphi / dat1 -> A
	     + delta_f * (rm/ba1 + rn*ba1) * sphi*cphi ) / (rm + pc1 -> hgt);

    dhgt = delta_x * cphi*clam + delta_y * cphi*slam + delta_z * sphi
	 - delta_a * (dat1 -> A / rn)
	 + delta_f * ba1*rn * sphi*sphi;

    pc2 -> lon = (lam + dlam) / RADIANS;
    pc2 -> lat = (phi + dphi) / RADIANS;
    pc2 -> hgt = pc1 -> hgt + dhgt;
  }

static struct datum *lookupdatum(sys) char *sys;
  { int k = 0;
    until (datumlist[k].name == NULL || seq(datumlist[k].name, sys)) k++;
    if (datumlist[k].name == NULL)
      { fprintf(stderr, "datumshift: unknown datum %s\n", sys);
	exit(1);
      }
    return &datumlist[k];
  }

