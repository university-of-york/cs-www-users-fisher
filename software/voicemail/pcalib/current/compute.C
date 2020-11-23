/* pcalib - interactive calibration tool for AJF's LTU
   A.J. Fisher	 April 1996 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

#include "header.h"
#include "gfxlib.h"

static int seqlen;
static complex tempvec[MAXSEQLEN], circle[MAXSEQLEN/2];

static fft(complex[], complex[], int);


global void initcompute(int slen)
  { seqlen = slen;
    for (int i = 0; i < seqlen/2; i++)
      { double x = (TWOPI * i) / seqlen;
	circle[i] = expj(-x);
      }
  }

global void compute(complex tdvec[], complex fdvec[])
  { for (int k = 0; k < seqlen; k++) fdvec[k] = tdvec[k];
    fft(fdvec, tempvec, seqlen);	/* forward fft */
  }

static fft(complex data[], complex temp[], int n)
  { if (n > 1)
      { int h = n/2;
	for (int i = 0; i < h; i++)
	  { int i2 = i*2;
	    temp[i] = data[i2];		/* even */
	    temp[h+i] = data[i2+1];	/* odd	*/
	  }
	fft(&temp[0], &data[0], h);
	fft(&temp[h], &data[h], h);
	int p = 0, t = seqlen/n;
	for (int i = 0; i < h; i++)
	  { complex wkt = circle[p] * temp[h+i];
	    data[i] = temp[i] + wkt;
	    data[h+i] = temp[i] - wkt;
	    p += t;
	  }
      }
  }

global int computedelay(complex tdvec[])
  { double maxh = -1.0; int maxi;
    for (int i=0; i < seqlen; i++)
      { double h = cmag(tdvec[i]);
	if (h > maxh) { maxh = h; maxi = i; }
      }
    return maxi;
  }

