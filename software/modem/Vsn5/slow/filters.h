/* Modem for MIPS   AJF	  January 1995
   Filter header file */

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

