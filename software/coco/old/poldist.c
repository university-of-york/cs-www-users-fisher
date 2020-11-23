/* Given lat/long of 2 points on surface of Earth, compute distance between them.
   ** Spherical Earth ** (so point-by-point corrections are required if this is to be useful)
   A.J. Fisher	 April 1995 */

#include <stdio.h>

#define forward
#define global
#define bool	    int
#define false	    0
#define true	    1
#define unless(x)   if(!(x))

#define GMEANRAD    6366901.239	   /* geometric mean radius of earth	       */
#define CEE_VACUO   299792500.0	   /* speed of light in vacuo		       */
#define CEE	    299691158.6	   /* real speed of light!		       */

#define PI	    3.1415926535897932384626433
#define RADIANS	    (PI / 180.0)

extern double cos(), sqrt(), atan2();

forward double cvtangle(), computedist();

struct pco { double lat, lon };	    /* polar coords (latitude, longitude)  */


global main(argc, argv) int argc; char *argv[];
  { for (;;)
      { struct pco pco1, pco2; double dist;
	printf("Point 1:\n"); getlatlong(&pco1);
	printf("Point 2:\n"); getlatlong(&pco2);
	dist = computedist(pco1, pco2);
	printf("Distance %11.4f us\n", dist);
      }
  }

static getlatlong(pc) struct pco *pc;
  { char buf[256];
    printf("   Latitude:  "); fflush(stdout);
    getstr(buf); pc -> lat = cvtangle(buf, "NS");
    printf("   Longitude: "); fflush(stdout);
    getstr(buf); pc -> lon = cvtangle(buf, "EW");
  }

static getstr(buf) char *buf;
  { char *x = gets(buf);
    if (x == NULL) { putchar('\n'); exit(0); }
  }

static double cvtangle(s, dir) char *s, *dir;
  { int deg, min; char chs[2]; double sec, ang;
    int ni = sscanf(s, "%d%d%lf%1s", &deg, &min, &sec, chs);
    unless (ni == 4 && (chs[0] == dir[0] || chs[0] == dir[1]))
      { fprintf(stderr, "Format error: %s\n", s);
	exit(1);
      }
    ang = deg + min/60.0 + sec/3600.0;
    if (chs[0] == dir[1]) ang = -ang;
    return ang;
  }

static double computedist(pc1, pc2) struct pco pc1, pc2;
  { double th1 = pc1.lon, ph1 = pc1.lat;
    double th2 = pc2.lon, ph2 = pc2.lat;
    double cdifph, csumph, cdifth, cdvtx, sdvtx;
    cdifph = cos(RADIANS * (ph1 - ph2));
    csumph = cos(RADIANS * (ph1 + ph2));
    cdifth = cos(RADIANS * (th1 - th2));
    cdvtx = 0.5 * (cdifph * (1.0+cdifth) - csumph * (1.0-cdifth));
    sdvtx = sqrt((1.0+cdvtx) * (1.0-cdvtx));
    return (GMEANRAD * 1e6 / CEE) * atan2(sdvtx, cdvtx);    /* result in us */
  }

