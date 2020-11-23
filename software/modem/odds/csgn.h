inline float fsgn(float x)
  { return (x < 0.0) ? -1.0 :
	   (x > 0.0) ? +1.0 : 0.0;
  }

inline complex csgn(complex z)
  { return complex(fsgn(z.re), fsgn(z.im));
  }

