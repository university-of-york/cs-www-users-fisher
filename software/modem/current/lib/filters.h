struct filter;

typedef float (*fstepfunc)(filter*, float);

struct fspec
  { int nz, np;
    fstepfunc fsf;
  };

struct filter
  { filter(fspec*);
    ~filter();
    float fstep(float x) { return fs -> fsf(this, x); }
    fspec *fs; float *v;
    float sum; int ptr;	    /* mvg avg filters only */
  };

struct cfilter
  { cfilter(fspec *fs)
      { ref = new filter(fs);
	imf = new filter(fs);
      }
    ~cfilter()
      { delete ref; delete imf;
      }
    complex fstep(complex z) { return complex(ref -> fstep(z.re), imf -> fstep(z.im)); }

private:
    filter *ref, *imf;
  };

