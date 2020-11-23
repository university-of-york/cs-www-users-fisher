struct c_complex
  { double re, im;
  };

struct complex
  { double re, im;
    complex(double r, double i = 0.0) { re = r; im = i; }
    complex() { }					/* uninitialized complex */
    complex(c_complex z) { re = z.re; im = z.im; }	/* init from denotation */
  };

extern complex csqrt(complex), cexp(complex), expj(double);	    /* from complex.C */

inline double hypot(complex z)
  { return hypot(z.im, z.re);
  }

inline double atan2(complex z)
  { return atan2(z.im, z.re);
  }

inline complex cconj(complex z)
  { z.im = -z.im;
    return z;
  }

extern complex operator * (complex, complex);

inline complex operator * (complex z, double a)
  { z.re *= a; z.im *= a;
    return z;
  }

inline void operator *= (complex &z, double a)
  { z.re *= a; z.im *= a;
  }

inline void operator *= (complex &z1, complex z2)
  { z1 = z1 * z2;
  }

extern complex operator / (complex, complex);

inline complex operator / (complex z, double a)
  { z.re /= a; z.im /= a;
    return z;
  }

inline void operator /= (complex &z, double a)
  { z = z / a;
  }

inline complex operator + (complex z1, complex z2)
  { z1.re += z2.re;
    z1.im += z2.im;
    return z1;
  }

inline complex operator - (complex z1, complex z2)
  { z1.re -= z2.re;
    z1.im -= z2.im;
    return z1;
  }

inline void operator -= (complex &z1, complex z2)
  { z1.re -= z2.re;
    z1.im -= z2.im;
  }

inline complex operator - (complex z)
  { return 0.0 - z;
  }

inline bool operator == (complex z1, complex z2)
  { return (z1.re == z2.re) && (z1.im == z2.im);
  }

inline complex sqr(complex z)
  { return z*z;
  }

