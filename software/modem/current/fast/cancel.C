/* Modem for MIPS   AJF	  October 1996
   Echo canceller routines */

#include <stdio.h>
#include <math.h>
#include <string.h>	/* memset */

#include <complex.h>
#include <myaudio.h>

#include "modem.h"
#include "cancel.h"


void canceller::reset()
  { memset(coeffs, 0, ncs * sizeof(complex));
    memset(in, 0, size * sizeof(complex));
    next = 0;
  }

void canceller::insert(complex z)
  { /* circular buffer */
    in[next] = z;
    if (++next >= size) next = 0;
  }

complex canceller::get()
  { /* get predicted echo value */
    complex z = 0.0;
    int j = 0;
    int k = size + ebeg - TRDELAY;
    while (j < ncs)
      { int p = (next+k) & (size-1);
	z += coeffs[j] * in[p];
	j++; k += SYMBLEN/2;
      }
    return z;
  }

void canceller::update(complex eps)
  { complex deps = (delta / (float) ncs) * eps;
    int j = 0;
    int k = size + ebeg - TRDELAY;
    while (j < ncs)
      { int p = (next+k) & (size-1);
	coeffs[j] += deps * cconj(in[p]);
	j++; k += SYMBLEN/2;
      }
  }

inline float hypot(complex z)
  { return hypot(z.im, z.re);
  }

inline float atan2(complex z)
  { return atan2(z.im, z.re);
  }

void canceller::print(char *fn)
  { FILE *fi = fopen(fn, "w");
    if (fi != NULL)
      { int spc = SYMBLEN/2;
	fprintf(fi, ".sp 0.5i\n");
	fprintf(fi, ".G1 8i\n");
	fprintf(fi, "new solid\n");
	for (int j=0; j < ncs; j++) fprintf(fi, "%4d %g\n", ebeg + (j*spc), coeffs[j].re);
	fprintf(fi, ".G2\n.bp\n");
	fprintf(fi, ".sp 0.5i\n.G1 8i\n");
	fprintf(fi, "new solid\n");
	for (int j=0; j < ncs; j++) fprintf(fi, "%4d %g\n", ebeg + (j*spc), coeffs[j].im);
	fprintf(fi, ".G2\n.bp\n");
	fprintf(fi, ".sp 0.5i\n.G1 8i\n");
	fprintf(fi, "new solid\n");
	for (int j=0; j < ncs; j++) fprintf(fi, "%4d %g\n", ebeg + (j*spc), hypot(coeffs[j]));
	fprintf(fi, ".G2\n.bp\n");
	fprintf(fi, ".sp 0.5i\n.G1 8i\n");
	fprintf(fi, "new solid\n");
	for (int j=0; j < ncs; j++) fprintf(fi, "%4d %g\n", ebeg + (j*spc), atan2(coeffs[j]));
	fprintf(fi, ".G2\n.bp\n");
	fprintf(fi, ".sp 0.5i\n");
	for (int j=0; j < ncs; j++) fprintf(fi, "{ %10.6f, %10.6f },\n", coeffs[j].re, coeffs[j].im);
	fclose(fi);
      }
    else fprintf(stderr, "can't create %s\n", fn);
  }

