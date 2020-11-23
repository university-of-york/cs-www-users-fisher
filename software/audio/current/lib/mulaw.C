/* Mu-law companding routines	A.J. Fisher   January 1996 */

#include "fishaudio.h"
#include "private.h"

static int *expandtable;

static uchar interpolate(int);

struct initmu
  { initmu();
    ~initmu();
  };

static initmu initmu;

initmu::initmu()
  { /* make mu-law expanding table */
    expandtable = new int[129];	    /* extra for stopper */
    for (int n=0; n < 128; n++)
      { double x = (pow(256.0, (double) n / 127.0) - 1.0) / 255.0;	/* result in range 0 .. 1 */
	int ix = (int) (x * 32767.0 + 0.5);				/* in range 0 .. 32767	  */
	expandtable[n] = ix;
      }
    expandtable[128] = 99999;	    /* stopper */
  }

initmu::~initmu()
  { delete expandtable;
  }

global int mu_expand(uchar val)
  { /* expand 8 bit ==> 16 bit */
    return (val >= 128) ? expandtable[255-val] : -(expandtable[127-val]+1);
  }

global uchar mu_compress(int val)
  { /* compress 16 bit ==> 8 bit */
    if (val < -32768 || val > +32767) giveup("value out of range in mu_compress: %d", val);
    return (val >= 0) ? 255 - interpolate(val) :
			127 - interpolate(-(val+1));
  }

static uchar interpolate(int val)
  { /* binary chop */
    int p1 = 0, p2 = 128; int pm;
    for (int i=0; i<8; i++)
      { pm = (p1+p2)/2;
	if (val > expandtable[pm]) p1 = pm; else p2 = pm;
      }
    return pm;
  }

