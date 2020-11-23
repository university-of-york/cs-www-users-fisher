#include <stdio.h>
#include <math.h>

#include "private.h"
#include "complex.h"
#include "debug.h"

#define MAXTICKS 300

struct db_tick { char ch; int ptr; };

static void findrange(float*, int, float&, float&);

debugger::debugger(int nr, int nc)
  { if (nr > 4) nr = 4;
    nrows = nr; ncols = nc;
    for (int i = 0; i < nrows; i++) vec[i] = new float[ncols];
    ticks = new db_tick[MAXTICKS];
    reset();
  }

void debugger::insert(float f1, float f2, float f3, float f4)
  { if (ptr < ncols)
      { if (nrows >= 1) vec[0][ptr] = f1;
	if (nrows >= 2) vec[1][ptr] = f2;
	if (nrows >= 3) vec[2][ptr] = f3;
	if (nrows >= 4) vec[3][ptr] = f4;
	ptr++;
      }
  }

void debugger::tick(char ch)
  { if (nticks < MAXTICKS)
      { ticks[nticks].ch = ch;
	ticks[nticks].ptr = ptr;
	nticks++;
      }
  }

debugger::~debugger()
  { for (int i = 0; i < nrows; i++) delete vec[i];
    delete ticks;
  }

void debugger::print(char *fn)
  { FILE *fi = fopen(fn, "w");
    if (fi != NULL)
      { for (int pn = 0; pn < nrows; pn++)
	  { float ym, dy;
	    findrange(vec[pn], ptr, ym, dy);
	    int n = 0, nxt = 0;
	    while (n < ptr)
	      { int n1 = n + 500;
		if (n1 > ptr) n1 = ptr;
		fprintf(fi, ".sp 0.5i\n");
		fprintf(fi, "pn=%d; %d ... %d\n", pn, n, n1-1);
		fprintf(fi, ".G1 8i\n");
		fprintf(fi, "ticks bot out 0.1 from %d to %d by 100 \"%%g\"\n", n, n+500);
		fprintf(fi, "ticks bot out 0.05 from %d to %d by 10 \"\"\n", n, n+500);
		fprintf(fi, "ticks left from %g to %g by %g\n", -ym, +ym, dy);
		if (nxt < nticks && ticks[nxt].ptr >= n && ticks[nxt].ptr <= n1)
		  { int tn = 0;
		    fprintf(fi, "ticks bot in 0.1 at ");
		    while (nxt < nticks && ticks[nxt].ptr <= n1)
		      { if (tn++ > 0) fprintf(fi, ", ");
			fprintf(fi, "%d \"%c\"", ticks[nxt].ptr, ticks[nxt].ch);
			nxt++;
		      }
		    putc('\n', fi);
		    if (ticks[nxt-1].ptr == n1) nxt--;	/* include last tick again on next page */
		  }
		fprintf(fi, "new solid\n");
		for (int i = n; i < n1; i++) fprintf(fi, "%d %g\n", i, vec[pn][i]);
		fprintf(fi, ".G2\n.bp\n");
		n = n1;
	      }
	  }
	fclose(fi);
      }
    else fprintf(stderr, "can't create %s\n", fn);
  }

static void findrange(float *vec, int num, float &ym, float &dy)
  { float ymax = -1.0;
    for (int i=0; i < num; i++)
      { float y = fabsf(vec[i]);
	if (y > ymax) ymax = y;
      }
    if (ymax > 0.0)
      { int m = (int) (100.0 + log10f(ymax)) - 100;	/* round downwards */
	float ym1 = powf(10.0, m); int nt = 10;
	ym = ym1;
	while (ymax > ym)
	  { ym = 2.0*ym1;
	    if (ymax > ym) { ym = 3.0*ym1; nt = 6; }
	    if (ymax > ym) { ym = 5.0*ym1; nt = 10; }
	    if (ymax > ym) { ym1 *= 10.0; ym = ym1; }
	  }
	dy = ym/nt;
      }
    else ym = 0.0;
  }

