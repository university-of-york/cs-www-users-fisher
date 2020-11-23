/* Fast Fourier Transform   A.J. Fisher	  March 1988
   Based on fbcpl version, which was based on Algol 68 version
   Cvted from C to C++	 November 1996 */

#include <stdio.h>
#include "complex.h"

#define global
#define forward
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

#define MAXDATA	    (1 << 18)	/* was 65536 */
#define TWOPI	    6.2831853071795864769252866
#define ALPHA	    0.54	/* for Hamming window */

#define t_unkn	0
#define t_real	1
#define t_imag	2
#define t_compl 3
#define t_angle 4
#define t_power 5

#define f_cin	1
#define f_inv	2
#define f_win	4

typedef unsigned char uchar;
typedef unsigned int uint;

static complex circle[MAXDATA/2 + 1];	/* extra slot needed for window computation */
static complex data[MAXDATA];
static complex temp[MAXDATA];

static int ndata, seqlen;
static char type;
static uint flags;

extern "C"
  { double sin(double), cos(double), atan2(double, double), hypot(double, double);
  };

static void readcmdline(int, char**), setopts(char*), usage(), readdata();
static bool rdatum(complex&), rreal(double&);
static void roundup(), initcircle(), windowdata();
static double window(int);
static void fft(complex*, complex*, int);
static void writedata(), wreal(double);

inline bool ispow2(uint n)     { return (n & (n-1)) == 0;      }
inline double power(complex z) { return z.re*z.re + z.im*z.im; }
inline double hypot(complex z) { return hypot(z.im, z.re);     }
inline double atan2(complex z) { return atan2(z.im, z.re);     }


global void main(int argc, char **argv)
  { readcmdline(argc, argv);
    readdata();
    roundup();
    initcircle();
    if (flags & f_win) windowdata();
    fft(data, temp, seqlen);
    writedata();
    exit(0);
  }

static void readcmdline(int argc, char **argv)
  { type = t_unkn; flags = 0;
    for (int k = 1; k < argc; k++)
      { char *s = argv[k];
	if (s[0] == '-') setopts(&s[1]);
	else usage();
      }
    if (type == t_unkn) usage();
  }

static void setopts(char *s)
  { int k = 0;
    until (s[k] == '\0')
      { switch (s[k++])
	  { default:
		usage();

	    case 'r':
		type = t_real;
		break;

	    case 'i':
		type = t_imag;
		break;

	    case 'z':
		type = t_compl;
		break;

	    case 'a':
		type = t_angle;
		break;

	    case 'p':
		type = t_power;
		break;

	    case 'Z':
		flags |= f_cin;	  /* complex input */
		break;

	    case 'I':
		flags |= f_inv;	  /* inverse transform */
		break;

	    case 'w':
		flags |= f_win;	  /* window the input */
		break;
	  }
      }
  }

static void usage()
  { fprintf(stderr, "Usage: fft -[rizapZIw]\n");
    fprintf(stderr, "  rizap    output real, imaginary, (re,im), (mag,phase), power\n");
    fprintf(stderr, "  Z        input complex\n");
    fprintf(stderr, "  I        inverse transform\n");
    fprintf(stderr, "  w        window the input\n");
    exit(1);
  }

static void readdata()
  { ndata = 0;
    complex z;
    bool ok = rdatum(z);
    while (ok && ndata < MAXDATA)
      { data[ndata++] = z;
	ok = rdatum(z);
      }
    if (ok)
      { fprintf(stderr, "fft: too many data! max = %d\n", MAXDATA);
	exit(1);
      }
    fprintf(stderr, "fft: info: ndata = %d\n", ndata);
  }

static bool rdatum(complex &z)
  { bool ok = rreal(z.re);
    if (ok)
      { if (flags & f_cin)
	  { ok = rreal(z.im);
	    unless (ok) fprintf(stderr, "fft: -Z requires an even number of input data\n");
	  }
	else z.im = 0.0;
      }
    return ok;
  }

static bool rreal(double &x)
  { int ni = scanf("%lg", &x);
    return (ni == 1);
  }

static void roundup()
  { seqlen = ndata;
    if (seqlen > 0)
      { until (ispow2(seqlen)) seqlen &= (seqlen-1);	/* knock off least sig. 1 bit */
	until (seqlen >= ndata) seqlen <<= 1;
	/* seqlen is now the least power of 2 .ge. ndata */
	fprintf(stderr, "fft: info: seqlen = %d\n", seqlen);
	while (ndata < seqlen) data[ndata++] = 0.0;
      }
  }

static void initcircle()
  { for (int i = 0; i <= seqlen/2; i++) /* actually a semicircle; n.b. extra slot for window computation */
      { double x = (TWOPI * i) / seqlen;
	circle[i].re = cos(x);
	circle[i].im = (flags & f_inv) ? sin(x) : -sin(x);
      }
  }

static void windowdata()
  { for (int i = 0; i < seqlen; i++) data[i] *= window(i);
  }

static double window(int j)
  { /* Hamming window */
    double win = (j < seqlen/2) ? circle[seqlen/2 - j].re : circle[j - seqlen/2].re;
    return ALPHA + (1.0-ALPHA) * win;
  }

static void fft(complex *data, complex *temp, int n)
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

static void writedata()
  { int n = (flags & f_cin) ? seqlen : seqlen/2;
    for (int i = 0; i < n; i++)
      { complex z = data[i];
	if (type == t_real || type == t_compl) wreal(z.re);
	if (type == t_imag || type == t_compl) wreal(z.im);
	if (type == t_angle) { wreal(hypot(z)); wreal(atan2(z)); }
	if (type == t_power) wreal(power(z));
	putchar('\n');
      }
  }

static void wreal(double x)
  { printf("%.10g\t", x);
  }

