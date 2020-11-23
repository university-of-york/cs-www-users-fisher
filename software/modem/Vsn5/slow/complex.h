struct c_complex
  { float re, im;
  };

struct complex
  { float re, im;
    complex(float r, float i = 0.0) { re = r; im = i; }
    complex() { }					/* uninitialized complex */
    complex(c_complex z) { re = z.re; im = z.im; }	/* init from denotation */
  };

inline complex operator + (complex z1, complex z2)
  { z1.re += z2.re;
    z1.im += z2.im;
    return z1;
  }

inline complex operator * (float a, complex z)
  { z.re *= a; z.im *= a;
    return z;
  }

