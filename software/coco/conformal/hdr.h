#define global
#define bool	    int
#define false	    0
#define true	    1
#define ushort	    unsigned short

#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define PI	    3.1415926535897932384626433
#define TWOPI	    (2.0 * PI)
#define RADIANS	    (PI / 180.0)

#define MAXDATA	    16384
#define NHARMS	    16
#define NDERIVS	    20

struct complex { double re, im; };
struct pco { double lon, lat; };	/* polar coords (latitude, longitude)	*/
struct cco { double x, y; };		/* Cartesian coords (easting, northing) */

struct derivatives
  { double linterm;
    double avec[NDERIVS][NHARMS+3], bvec[NDERIVS][NHARMS+3]; /* padding reqd for convolution */
  };

extern void makederivs(char*, derivatives*);			/* from mkderivs.C */
extern void dofft(complex[], int, int, int, int);		/* from fft.C	   */
extern void tm_convert(pco*, cco*);				/* from tmcvt.C	   */
extern void lcc_convert(pco*, cco*), st_convert(pco*, cco*);	/* from othercvt.C */
extern void initstats(), printstats();				/* from common.C   */
extern void printline(pco*, cco*, cco*);			/* from common.C   */
extern void giveup(char* ...);					/* from common.C   */

extern "C"
  { double sin(double), cos(double), tan(double), atan(double), sqrt(double), fabs(double);
    double hypot(double, double), pow(double, double);
    int atoi(char*);
  };

