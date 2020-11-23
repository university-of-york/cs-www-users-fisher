struct c_complex
  { float re, im;
  };

struct complex
  { float re, im;
    complex(float r, float i = 0.0) { re = r; im = i; }
    complex() { }					/* uninitialized complex */
    complex(c_complex z) { re = z.re; im = z.im; }	/* init from denotation */
  };

inline complex cconj(complex z)
  { z.im = -z.im;
    return z;
  }

inline float power(complex z)
  { return z.re*z.re + z.im*z.im;
  }

inline complex operator - (complex z)
  { z.re = -z.re; z.im = -z.im;
    return z;
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

inline complex operator * (float a, complex z)
  { z.re *= a; z.im *= a;
    return z;
  }

inline complex operator * (complex z, float a)
  { z.re *= a; z.im *= a;
    return z;
  }

inline complex operator * (complex z1, complex z2)
  { return complex(z1.re*z2.re - z1.im*z2.im,
		   z1.re*z2.im + z1.im*z2.re);
  }

inline complex operator / (complex z, float a)
  { z.re /= a; z.im /= a;
    return z;
  }

inline complex operator / (complex z1, complex z2)
  { return (z1 * cconj(z2)) / power(z2);
  }

inline void operator += (complex &z1, complex z2)
  { z1.re += z2.re;
    z1.im += z2.im;
  }

inline void operator -= (complex &z1, complex z2)
  { z1.re -= z2.re;
    z1.im -= z2.im;
  }

inline void operator *= (complex &z1, complex z2)
  { z1 = z1 * z2;
  }

inline void operator /= (complex &z, float a)
  { z.re /= a; z.im /= a;
  }

