struct complex
  { complex(double r, double i = 0.0) { re = r; im = i; }
    complex() { }
    double re, im;
  };

inline complex operator + (complex z1, complex z2)
  { return complex(z1.re + z2.re, z1.im + z2.im);
  }

inline complex operator - (complex z1, complex z2)
  { return complex(z1.re - z2.re, z1.im - z2.im);
  }

inline complex operator * (complex z1, complex z2)
  { return complex((z1.re * z2.re) - (z1.im * z2.im),
		   (z1.im * z2.re) + (z1.re * z2.im));
  }

inline void operator *= (complex &z, double x)
  { z.re *= x;
    z.im *= x;
  }

