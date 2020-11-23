struct correlator
  { correlator() { reset();	  }
    void reset() { z0 = z1 = 0.0; }
    complex fstep(complex);

private:
    complex z0, z1, z2;
  };

inline complex correlator::fstep(complex z)
  { z0 = z1; z1 = z2; z2 = z;
    return z2 * cconj(z0);
  }

