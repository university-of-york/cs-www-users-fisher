#include <stdio.h>

#define global

#define PI	    3.14159265358979323846
#define SEQLEN	    1024

struct complex
  { double re, im;
  };

extern "C" double sin(double), cos(double), hypot(double, double);

static complex circle[SEQLEN/2];

static void prshapes(int), prshape(double, int), rotate(complex*);
static double cosfac(double, double), sqr(double), sinc(double);
static void initcircle(), fft(complex*), subfft(complex*, complex*, int);

static complex operator + (complex, complex),
	       operator - (complex, complex),
	       operator * (complex, complex);


global main()
  { initcircle();
    prshapes(0);	/* time domain */
    printf(".bp\n"); prshapes(1);
    printf(".bp\n"); prshapes(2);
    printf(".bp\n"); prshapes(3);
    exit(0);
  }

static void prshapes(int d)
  { printf(".sp 1i\n.G1 8i\nticks bot off\n");
    prshape(0.0, d); prshape(0.5, d); prshape(1.0, d);
    printf(".G2\n");
  }

static void prshape(double beta, int d)
  { complex vec[SEQLEN];
    for (int i = 0; i < SEQLEN; i++)
      { double x = 8.0*PI * (double) (i-SEQLEN/2) / (double) SEQLEN;
	if (d > 0) x *= 48.0;
	vec[i].re = cosfac(beta, x) * sinc(x);
	vec[i].im = 0.0;
      }
    if (d > 0)
      { rotate(vec); fft(vec); rotate(vec);
	if (d == 2) for (int i = 0; i < SEQLEN; i++) { double x = vec[i].re; vec[i].re = vec[i].im; vec[i].im = x; }
	if (d == 3) for (int i = 0; i < SEQLEN; i++) vec[i].re = hypot(vec[i].re, vec[i].im);
      }
    printf("new solid\n");
    for (int i = 0; i < SEQLEN; i++) printf("%d %g\n", i-SEQLEN/2, vec[i].re);
  }

static void rotate(complex *vec)
  { for (int i = 0; i < SEQLEN/2; i++)
      { complex z = vec[i];
	vec[i] = vec[SEQLEN/2 + i];
	vec[SEQLEN/2 + i] = z;
      }
  }

static double cosfac(double beta, double x)
  { double bx = beta * x;
    double top = cos(bx), bot = 1.0 - sqr(2.0 * bx / PI);
    return (top == 0.0 && bot == 0.0) ? (PI/4.0) :	/* degenerate case */
	   top / bot;
  }

static double sqr(double x)
  { return x*x;
  }

static double sinc(double x)
  { return (x == 0.0) ? 1.0 :	/* degenerate case */
	   sin(x) / x;
  }

static void initcircle()
  { for (int i = 0; i < SEQLEN/2; i++) /* actually a semicircle */
      { double x = (2.0*PI*i) / SEQLEN;
	circle[i].re = cos(x);
	circle[i].im = -sin(x);
      }
  }

static void fft(complex *data)
  { complex temp[SEQLEN];
    subfft(data, temp, SEQLEN);
  }

static void subfft(complex *data, complex *temp, int n)
  { if (n > 1)
      { int h = n/2; int i;
	for (i=0; i<h; i++)
	  { temp[i] = data[2*i];     /* even */
	    temp[h+i] = data[2*i+1]; /* odd  */
	  }
	subfft(&temp[0], &data[0], h);
	subfft(&temp[h], &data[h], h);
	int p = 0; int t = SEQLEN/n;
	for (i=0; i<h; i++)
	  { complex wkt = circle[p] * temp[h+i];
	    data[i] = temp[i] + wkt;
	    data[h+i] = temp[i] - wkt;
	    p += t;
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
    z.re = (z1.re * z2.re) - (z1.im * z2.im);
    z.im = (z1.im * z2.re) + (z1.re * z2.im);
    return z;
  }

