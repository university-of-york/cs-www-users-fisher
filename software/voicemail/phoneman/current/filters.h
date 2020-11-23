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
  };

