/* Fast Fourier Transform   AJF	  March 1988
   Based on fbcpl version, which was based on A68 version
   Mixed radix: December 1992 */

#include <math.h>
#include <stdio.h>

#define global
#define bool	    int
#define false	    0
#define true	    1
#define uchar	    unsigned char
#define uint	    unsigned int
#define unless(x)   if(!(x))

#define seq(s1,s2)  (strcmp(s1,s2) == 0)

#define MAXDATA 20000
#define LOGMAXD 15   /* log2 of maxdata, rounded up */

#define TWOPI	6.2831853071795864769252866

#define t_unkn	0
#define t_real	1
#define t_imag	2
#define t_compl 3
#define t_power 4

#define f_ac	1
#define f_cin	2

struct complex { double re, im; };

static struct complex circle[MAXDATA];
static struct complex data[MAXDATA+1]; /* extra cell needed by readdata */
static struct complex temp[MAXDATA];
static uchar radixtab[LOGMAXD];

static int ndata;
static char type;
static uint flags;

struct complex cplus(), ctimes();


global main(argc, argv) int argc; char *argv[];
  { readcmdline(argc, argv);
    readdata();
    if (ndata > 0)
      { initcircle();
	factorize();
	fft(data, temp, ndata, 0);
	if (flags & f_ac) { data[0].re = 0.0; data[0].im = 0.0; }
	writedata();
      }
    exit(0);
  }

static readcmdline(argc, argv) int argc; char *argv[];
  { int k;
    type = t_unkn; flags = 0;
    for (k=1; k<argc; k++)
      { char *s = argv[k];
	if (seq(s,"-r")) type = t_real;
	else if (seq(s,"-i")) type = t_imag;
	else if (seq(s,"-z")) type = t_compl;
	else if (seq(s,"-p")) type = t_power;
	else if (seq(s,"-ac")) flags |= f_ac;   /* set 0 Hz component to zero before output */
	else if (seq(s,"-Z")) flags |= f_cin;   /* complex input */
	else usage();
      }
    if (type == t_unkn) usage();
  }

static usage()
  { fprintf(stderr, "Usage: fft -rizp [-Z] [-ac]\n");
    exit(1);
  }

static readdata()
  { bool ok;
    ndata = 0;
    ok = rdatum(&data[ndata]);
    while (ok && ndata < MAXDATA)
      { ndata++;
	ok = rdatum(&data[ndata]);
      }
    if (ok)
      { fprintf(stderr, "fft: too many data! max = %d\n", MAXDATA);
	exit(1);
      }
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
  { int k = scanf(" %lf", xx);
    return (k == 1);
  }

static initcircle()
  { int i;
    for (i=0; i<ndata; i++)
      { double x = (TWOPI * i) / ndata;
	circle[i].re = cos(x); circle[i].im = -sin(x);
      }
  }

static factorize()
  { int n = ndata, d = 0; int i;
    while (n > 1)
      { int p; bool found = false;
	for (p=2; p <= 7 && !found; p = (p+1) | 1) /* for p = 2, 3, 5, 7 */
	  { if (n%p == 0)
	      { radixtab[d++] = p;
		n /= p;
		found = true;
	      }
	  }
	unless (found)
	  { fprintf(stderr, "fft: ndata=%d: can't factorize!\n");
	    exit(1);
	  }
      }
    fprintf(stderr, "fft: for info: ndata=%d, factors are", ndata);
    for (i=0; i<d; i++) fprintf(stderr, " %d", radixtab[i]);
    fputc('\n', stderr);
  }

static fft(data, temp, n, d) struct complex data[], temp[]; int n, d;
  { if (n > 1)
      { int r = radixtab[d];
	int h = n/r;		/* we know r divides n ...			*/
	int t = ndata/n;	/* ... and n divides ndata; circle[x*t] is W^x	*/
	int i;
	for (i=0; i<h; i++)
	  { int ir = i*r; int j;
	    for (j=0; j<r; j++)
	      { int jh = j*h;
		temp[jh + i] = data[ir + j];
	      }
	  }
	for (i=0; i<r; i++)
	  { int ih = i*h;
	    fft(&temp[ih], &data[ih], h, d+1);
	  }
	for (i=0; i<h; i++)
	  { int j;
	    for (j=0; j<r; j++)
	      { int jh = j*h; int k;
		data[jh + i] = temp[i];
		for (k=1; k<r; k++)
		  { int e = k*(jh + i)*t;
		    int kh = k*h;
		    data[jh + i] = cplus(data[jh + i], ctimes(circle[e%ndata], temp[kh + i]));
		  }
	      }
	  }
      }
  }

static writedata()
  { int n = (flags & f_cin) ? ndata : ndata/2;
    int i;
    for (i=0; i<n; i++)
      { if (type == t_real || type == t_compl) printf("%10g ", data[i].re);
	if (type == t_imag || type == t_compl) printf("%10g ", data[i].im);
	if (type == t_power)
	  { struct complex z;
	    z = data[i];
	    printf("%10g ", z.re * z.re + z.im * z.im);
	  }
	putchar('\n');
      }
  }

static struct complex cplus(z1, z2) struct complex z1, z2;
  { struct complex z;
    z.re = z1.re + z2.re;
    z.im = z1.im + z2.im;
    return z;
  }

static struct complex ctimes(z1, z2) struct complex z1, z2;
  { struct complex z;
    z.re = (z1.re * z2.re) - (z1.im * z2.im);
    z.im = (z1.im * z2.re) + (z1.re * z2.im);
    return z;
  }

