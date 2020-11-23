#include <stdio.h>
#include "hdr.h"

#define MAXDATA	 1024	/* power of 2 */

static complex czero = { 0.0, 0.0 };
static int seqlen;

static complex wpow(int), expj(double);
static complex operator * (complex, complex);


global void chirp_fft(complex data[], int n, int sgn)
  { if (n > MAXDATA) giveup("Bug: too big in chirp_fft");
    seqlen = n;
    static complex yvec[2*MAXDATA], vvec[2*MAXDATA], gvec[2*MAXDATA]; int i;
    for (i=0; i < n; i++) yvec[i] = wpow(-(i*i)) * data[i];
    for (i=n; i < 2*n; i++) yvec[i] = czero;
    dofft(yvec, 2*n, sgn);
    for (i=0; i < n; i++) vvec[i] = wpow(+(i*i));
    for (i=n; i < 2*n; i++)
      { int k = 2*n - i;
	vvec[i] = wpow(+(k*k));
      }
    dofft(vvec, 2*n, sgn);
    for (i=0; i < 2*n; i++) gvec[i] = vvec[i] * yvec[i];
    dofft(gvec, 2*n, -sgn);
    for (i=0; i < n; i++) data[i] = wpow(-(i*i)) * gvec[i];
  }

static complex wpow(int n)
  { return expj(n * (PI/seqlen));
  }

static complex expj(double theta)
  { complex z;
    z.re = cos(theta);
    z.im = sin(theta);
    return z;
  }

static complex operator * (complex z1, complex z2)
  { complex z;
    if (fabs(z2.re) < 1e-12 && fabs(z2.im) < 1e-12) z = czero;
    else if (z1.re == 1.0 && z1.im == 0.0) z = z2;
    else if (z1.re == 0.0 && z1.im == -1.0) { z.re = z2.im; z.im = -z2.re; }
    else if (z1.re == 0.0 && z1.im == +1.0) { z.re = -z2.im; z.im = z2.re; }
    else
      { z.re = (z1.re * z2.re) - (z1.im * z2.im);
	z.im = (z1.im * z2.re) + (z1.re * z2.im);
	/* mulcount++; */
      }
    return z;
  }

