/* pcalib - interactive calibration tool for AJF's LTU
   Complex arithmetic routines
   A.J. Fisher	 April 1996 */

#include "header.h"


global double cmag(complex z)
  { return hypot(z.im, z.re);
  }

global double carg(complex z)
  { return atan2(z.im, z.re);
  }

global double creal(complex z)
  { return z.re;
  }

global double cimag(complex z)
  { return z.im;
  }

global complex cconj(complex z)
  { z.im = -z.im;
    return z;
  }

global complex expj(double th)
  { return complex(cos(th), sin(th));
  }

global complex operator + (complex z1, complex z2)
  { z1.re += z2.re;
    z1.im += z2.im;
    return z1;
  }

global complex operator - (complex z1, complex z2)
  { z1.re -= z2.re;
    z1.im -= z2.im;
    return z1;
  }

global complex operator * (complex z1, complex z2)
  { return complex((z1.re * z2.re) - (z1.im * z2.im),
		   (z1.im * z2.re) + (z1.re * z2.im));
  }

global complex operator / (complex z1, complex z2)
  { double mag = (z2.re * z2.re) + (z2.im * z2.im);
    return complex((z1.re * z2.re + z1.im * z2.im) / mag,
		   (z1.im * z2.re - z1.re * z2.im) / mag);
  }

