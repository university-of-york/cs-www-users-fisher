#include <stdio.h>
#include "hdr.h"

static complex circle[MAXDATA/2];
static int seqlen, mulcount;

static complex czero = { 0.0, 0.0 };
static complex cone  = { 1.0, 0.0 };

static void initcircle(int);
static void fft1(complex[], complex[], int), fft2(complex[], complex[], int);
static complex operator + (complex, complex), operator - (complex, complex), operator * (complex, complex);
static complex operator - (complex);


global void dofft(complex data[], int n, int sgn, int offs, int nw)
  { static complex temp[MAXDATA+1]; int i, j;
    if (n < 0 || n > MAXDATA || (n & (n-1)) != 0) giveup("Bug: n in dofft");
    if (n%nw != 0) giveup("Bug: nw in dofft");
    mulcount = 0; seqlen = n; offs &= (n-1);
    initcircle(sgn);
    for (i=0; i < n; i++)
      { int k = (i*offs) & (n-1);
	complex wk = (k >= n/2) ? -circle[k - n/2] : circle[k];
	temp[i] = wk * data[i];
      }
    int sep = n/nw, p = 0;
    for (i=0; i < sep; i++)
      for (j=0; j < nw; j++) data[p++] = temp[j*sep + i];
    p = 0;
    for (i=0; i < sep; i++)
      { fft2(&data[p], &temp[p], nw);
	p += nw;
      }
    for (j=0; j < nw; j++)
      { complex sum = czero;
	for (i=0; i < sep; i++)
	  { int k = i*j;
	    complex wk = (k >= n/2) ? -circle[k - n/2] : circle[k];
	    sum = sum + (wk * data[i*nw + j]);
	  }
	for (i=0; i < sep; i++) temp[i*nw + j] = sum;
      }
    if (sgn > 0)
      { for (i=0; i < n; i++)
	  { temp[i].re /= (double) n;
	    temp[i].im /= (double) n;
	  }
      }
    for (i=0; i < n; i++) data[(offs+i) & (n-1)] = temp[i];
    fprintf(stderr, "*** %d complex multiplies\n", mulcount);
#ifdef DEBUG
    printf("??? n=%d sgn=%d offs=%d nw=%d\n", n, sgn, offs, nw);
    for (i=0; i < n; i++) printf("%4d: %14.4e %14.4e\n", i, data[i].re, data[i].im);
    exit(0);
#endif
  }

static void initcircle(int sgn)
  { for (int i = 0; i < seqlen/2; i++) /* actually a semicircle */
      { double x = (TWOPI * i) / seqlen;
	circle[i].re = cos(x);
	circle[i].im = sgn * sin(x);
      }
    /* set up certain points "exactly" so ctimes optimization works */
    circle[0] = cone;
    circle[seqlen/4].re = 0.0;
    circle[seqlen/4].im = (double) sgn;
  }

static void fft1(complex data[], complex temp[], int n) /* D-I-T (shuffle before) */
  { if (n > 1)
      { int h = n/2; int i;
	for (i=0; i < h; i++)
	  { int i2 = i*2;
	    temp[i] = data[i2];		/* even */
	    temp[h+i] = data[i2+1];	/* odd	*/
	  }
	fft1(&temp[0], &data[0], h);
	fft1(&temp[h], &data[h], h);
	int p = 0, t = seqlen/n;
	for (i=0; i < h; i++)
	  { complex wkt = circle[p] * temp[h+i];
	    data[i] = temp[i] + wkt;
	    data[h+i] = temp[i] - wkt;
	    p += t;
	  }
      }
  }

static void fft2(complex data[], complex temp[], int n) /* D-I-F (shuffle after) */
  { if (n > 1)
      { int h = n/2; int i;
	int p = 0, t = seqlen/n;
	for (i=0; i < h; i++)
	  { temp[i] = data[i] + data[h+i];
	    temp[h+i] = circle[p] * (data[i] - data[h+i]);
	    p += t;
	  }
	fft2(&temp[0], &data[0], h);
	fft2(&temp[h], &data[h], h);
	for (i=0; i < h; i++)
	  { int i2 = i*2;
	    data[i2] = temp[i];		/* even */
	    data[i2+1] = temp[h+i];	/* odd	*/
	  }
      }
  }

static complex operator + (complex z1, complex z2)
  { complex z;
    z.re = z1.re + z2.re;
    z.im = z1.im + z2.im;
    return z;
  }

static complex operator - (complex z1, complex z2)
  { complex z;
    z.re = z1.re - z2.re;
    z.im = z1.im - z2.im;
    return z;
  }

static complex operator * (complex z1, complex z2)
  { complex z;
    if (fabs(z2.re) < 1e-12 && fabs(z2.im) < 1e-12) z = czero;
    else if (z1.re == +1.0 && z1.im == 0.0) z = z2;
    else if (z1.re == -1.0 && z1.im == 0.0) z = -z2;
    else if (z1.re == 0.0 && z1.im == +1.0) { z.re = -z2.im; z.im = z2.re; }
    else if (z1.re == 0.0 && z1.im == -1.0) { z.re = z2.im; z.im = -z2.re; }
    else
      { z.re = (z1.re * z2.re) - (z1.im * z2.im);
	z.im = (z1.im * z2.re) + (z1.re * z2.im);
	mulcount++;
      }
    return z;
  }

static complex operator - (complex z)
  { z.re = -z.re; z.im = -z.im;
    return z;
  }

