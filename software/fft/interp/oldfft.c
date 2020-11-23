/* Fast Fourier Transform   AJF	  March 1988
   Based on fbcpl version, which was based on A68 version
   This vsn rounds up sequence length to next power of 2 by interpolation: December 1992 */

#include <stdio.h>

#define global
#define forward
#define bool	    int
#define false	    0
#define true	    1
#define uchar	    unsigned char
#define uint	    unsigned int
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define seq(s1,s2)  (strcmp(s1,s2) == 0)

#define MAXDATA 16384 /* must be <= 2^15; any space above power of 2 is wasted */
#define TWOPI	6.2831853071795864769252866

#define t_unkn	0
#define t_real	1
#define t_imag	2
#define t_compl 3
#define t_angle 4
#define t_power 5

#define f_cin	1
#define f_inv	2

struct complex { double re, im; };

static struct complex czero = { 0.0, 0.0 };

static struct complex circle[MAXDATA/2];
static struct complex data[MAXDATA];
static struct complex temp[MAXDATA];

static int ndata, seqlen;
static char type;
static uint flags;

extern double sin(), cos(), atan2(), hypot();

forward struct complex cplus(), cminus(), ctimes();


global main(argc, argv) int argc; char *argv[];
  { readcmdline(argc, argv);
    readdata();
    roundup();
    initcircle();
    fft(data, temp, seqlen);
    writedata();
    exit(0);
  }

static readcmdline(argc, argv) int argc; char *argv[];
  { int k;
    type = t_unkn; flags = 0;
    for (k=1; k<argc; k++)
      { char *s = argv[k];
	if (s[0] == '-') setopts(&s[1]);
	else usage();
      }
    if (type == t_unkn) usage();
  }

static setopts(s) char *s;
  { int k = 0;
    until (s[k] == '\0')
      { int ch = s[k++];
	if (ch == 'r') type = t_real;
	else if (ch == 'i') type = t_imag;
	else if (ch == 'z') type = t_compl;
	else if (ch == 'a') type = t_angle;
	else if (ch == 'p') type = t_power;
	else if (ch == 'Z') flags |= f_cin;   /* complex input */
	else if (ch == 'I') flags |= f_inv;   /* inverse transform */
	else usage();
      }
  }

static usage()
  { fprintf(stderr, "Usage: fft -[rizapZI]\n");
    fprintf(stderr, "  rizap    output real, imaginary, (re,im), (mag,phase), power\n");
    fprintf(stderr, "  Z        input complex\n");
    fprintf(stderr, "  I        inverse transform\n");
    exit(1);
  }

static readdata()
  { bool ok; struct complex z;
    ndata = 0;
    ok = rdatum(&z);
    while (ok && ndata < MAXDATA)
      { data[ndata++] = z;
	ok = rdatum(&z);
      }
    if (ok) toomany();
    fprintf(stderr, "fft: info: ndata = %d\n", ndata);
  }

static bool rdatum(z) struct complex *z;
  { bool ok = rreal(&z -> re);
    if (ok)
      { if (flags & f_cin)
	  { ok = rreal(&z -> im);
	    unless (ok) fprintf(stderr, "fft: -Z requires an even number of input data\n");
	  }
	else z -> im = 0.0;
      }
    return ok;
  }

static bool rreal(xx) double *xx;
  { int ni = scanf("%lg", xx);
    return (ni == 1);
  }

#define ispow2(n) ((n & (n-1)) == 0)

static roundup()
  { seqlen = ndata;
    if (seqlen > 0)
      { until (ispow2(seqlen)) seqlen &= (seqlen-1);	/* knock off least sig. 1 bit */
	until (seqlen >= ndata) seqlen <<= 1;
	/* seqlen is now the least power of 2 .ge. ndata */
	fprintf(stderr, "fft: info: seqlen = %d\n", seqlen);
	if (seqlen >= MAXDATA) toomany(); /* we need one extra cell for interpolation */
	interpolate();
      }
  }

static toomany()
  { fprintf(stderr, "fft: too many data! max = %d\n", MAXDATA);
    exit(1);
  }

static interpolate()
  { int p, q, i, j;
    data[ndata] = data[0]; /* because of wrap-around */
    i = seqlen; j = ndata; p = q = seqlen*ndata;
    while (i > 0)
      { double alpha; struct complex d0, d1;
	q -= ndata; i--;
	until (p <= q && (p+seqlen) > q) { p -= seqlen; j--; }
	alpha = (q-p) / (double) seqlen;
	d0 = data[j]; d1 = data[j+1];
	data[i].re = ((1.0-alpha) * d0.re) + (alpha * d1.re);
	data[i].im = ((1.0-alpha) * d0.im) + (alpha * d1.im);
      }
  }

static initcircle()
  { int i;
    for (i=0; i < seqlen/2; i++) /* actually a semicircle */
      { double x = (TWOPI * i) / seqlen;
	circle[i].re = cos(x);
	circle[i].im = (flags & f_inv) ? sin(x) : -sin(x);
      }
  }

static fft(data, temp, n) struct complex data[], temp[]; int n;
  { if (n > 1)
      { int h = n/2;
	int i, p, t;
	for (i=0; i<h; i++)
	  { int i2 = i*2;
	    temp[i] = data[i2];		/* even */
	    temp[h+i] = data[i2+1];	/* odd	*/
	  }
	fft(&temp[0], &data[0], h);
	fft(&temp[h], &data[h], h);
	p = 0; t = seqlen/n;
	for (i=0; i<h; i++)
	  { struct complex wkt;
	    wkt = ctimes(circle[p], temp[h+i]);
	    data[i] = cplus(temp[i], wkt);
	    data[h+i] = cminus(temp[i], wkt);
	    p += t;
	  }
      }
  }

#define wrdat(x) printf("%.10g\t", x)

static writedata()
  { int n = (flags & f_cin) ? seqlen : seqlen/2;
    int i;
    for (i=0; i<n; i++)
      { struct complex *z = &data[i];
	if (type == t_real || type == t_compl) wrdat(z -> re);
	if (type == t_imag || type == t_compl) wrdat(z -> im);
	if (type == t_angle)
	  { wrdat(hypot(z -> re, z -> im));
	    wrdat(atan2(z -> re, z -> im));
	  }
	if (type == t_power) wrdat((z -> re * z -> re) + (z -> im * z -> im));
	putchar('\n');
      }
  }

static struct complex cplus(z1, z2) struct complex z1, z2;
  { struct complex z;
    z.re = z1.re + z2.re;
    z.im = z1.im + z2.im;
    return z;
  }

static struct complex cminus(z1, z2) struct complex z1, z2;
  { struct complex z;
    z.re = z1.re - z2.re;
    z.im = z1.im - z2.im;
    return z;
  }

static struct complex ctimes(z1, z2) struct complex z1, z2;
  { struct complex z;
    z.re = (z1.re * z2.re) - (z1.im * z2.im);
    z.im = (z1.im * z2.re) + (z1.re * z2.im);
    return z;
  }

