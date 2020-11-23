/* Routines for complex arithmetic */

#include "bessel.h"

static struct complex czero = { 0.0, 0.0 };

extern double exp(), sin(), cos(), hypot(), sqrt();

forward double Xsqrt();


global double cmag(z) struct complex z;
  { return hypot(z.re, z.im);
  }

global struct complex csqrt(x) struct complex x;
  { struct complex z;
    double r = hypot(x.im, x.re);
    z.re = Xsqrt(0.5 * (r + x.re));
    z.im = Xsqrt(0.5 * (r - x.re));
    if (x.im < 0.0) z.im = -z.im;
    return z;
  }

static double Xsqrt(x) double x;
  { /* because of deficiencies in hypot on Sparc, it's possible for arg of Xsqrt to be small and -ve,
       which logically it can't be (since r >= |x.re|).	 Take it as 0. */
    return (x >= 0.0) ? sqrt(x) : 0.0;
  }

global struct complex cexp(x) struct complex x;
  { struct complex z; double r;
    r = exp(x.re);
    z.re = r * cos(x.im);
    z.im = r * sin(x.im);
    return z;
  }

global struct complex cconj(x) struct complex x;
  { struct complex z;
    z.re = x.re;
    z.im = -x.im;
    return z;
  }

global struct complex cadd(x, y) struct complex x, y;
  { struct complex z;
    z.re = x.re + y.re;
    z.im = x.im + y.im;
    return z;
  }

global struct complex csub(x, y) struct complex x, y;
  { struct complex z;
    z.re = x.re - y.re;
    z.im = x.im - y.im;
    return z;
  }

global struct complex cmul(x, y) struct complex x, y;
  { struct complex z;
    z.re = (x.re*y.re - x.im*y.im);
    z.im = (x.im*y.re + x.re*y.im);
    return z;
  }

global struct complex cdiv(x, y) struct complex x, y;
  { struct complex z;
    double mag = y.re*y.re + y.im*y.im;
    z.re = (x.re*y.re + x.im*y.im) / mag;
    z.im = (x.im*y.re - x.re*y.im) / mag;
    return z;
  }

