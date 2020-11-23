/* Modem for MIPS   AJF	  August 1996
   V.32 encoding / decoding routines */

#include <stdio.h>

#include "complex.h"
#include "modem.h"
#include "coder.h"

#define TLEN	8			/* num. of columns */
#define NSTATES 8			/* num. of states in convolutional encoder */
#define NINPUTS 4			/* num. inputs to convolutional encoder (1 << k) */
#define NPATHS	(2*NINPUTS)		/* paths out per node == paths in per node; "*2" for un-encoded bit */
#define INF	1e6

static c_complex ztab2[] =
  { /* 2-bit constellation (ABDC) */
    { -3.0, -1.0 }, { +1.0, -3.0 }, { -1.0, +3.0 }, { +3.0, +1.0 }, /* ABDC */
  };

static c_complex ztab4[] =
  { /* 4-bit constellation */
    { +3.0, -3.0 }, { -1.0, +1.0 }, { -3.0, +3.0 }, { +1.0, -1.0 },
    { +3.0, +1.0 }, { -1.0, -3.0 }, { -3.0, -1.0 }, { +1.0, +3.0 },
    { -1.0, +3.0 }, { +3.0, -1.0 }, { +1.0, -3.0 }, { -3.0, +1.0 },
    { -3.0, -3.0 }, { +1.0, +1.0 }, { +3.0, +3.0 }, { -1.0, -1.0 },
  };

static uchar ctab4[] =
  { 0xf, 0x1, 0x3, 0xd,
    0x5, 0x8, 0xa, 0x7,
    0x6, 0xb, 0x9, 0x4,
    0xc, 0x2, 0x0, 0xe,
  };

static uchar tfsmtab[NSTATES][NINPUTS] =
  { /* trellis encoder fsm */
    { 0, 3, 2, 1 }, { 4, 5, 7, 6 }, { 1, 2, 3, 0 }, { 7, 6, 4, 5 },
    { 2, 1, 0, 3 }, { 6, 7, 5, 4 }, { 3, 0, 1, 2 }, { 5, 4, 6, 7 },
  };

struct node
  { node *fwd[NPATHS];			/* paths leading forwards in trellis */
    node *bck[NPATHS];			/* paths leading backwards in trellis */
    float len[NPATHS];			/* and path lengths (metrics) */
    int survivor;			/* survivor (path code) */
    float svlen;			/* survivor length */
  };

static float lentab[NPATHS][NPATHS];	/* distance between points in constellation */
static int nextcol;

static void addpath(int, node*, node*), terr(int);
static bool pathfrom(node*, node*);


global void inittrellis()
  { for (int i=0; i < TLEN; i++)
      { for (int j=0; j < NSTATES; j++)
	  { node *x = &trellis[i][j];
	    for (int k=0; k < NPATHS; k++)
	      { x -> bck[k] = x -> fwd[k] = NULL;
		x -> len[k] = 0.0;
	      }
	    x -> svlen = (j == 0) ? 0.0 : INF;
	    x -> survivor = 0;
	  }
      }
    for (int i=0; i < TLEN; i++)
      { for (int j=0; j < NSTATES; j++)
	  { for (int k=0; k < NINPUTS; k++)
	      { uchar u = tfsmtab[j][k];
		node *x = &trellis[i][j], *y = &trellis[(i+1)%TLEN][u];
		addpath(2*k, x, y);
		addpath(2*k + 1, x, y);
	      }
	  }
      }
    for (int i=0; i < NPATHS; i++)
      { for (int j=0; j < NPATHS; j++)
	  { complex z = ztab4[i] - ztab4[j];
	    lentab[i][j] = hypot(z);
	  }
      }
    nextcol = 0;
  }

static void addpath(int ix, node *x, node *y)
  { /* set up a path from node x to node y */
    if (ix < 0 || ix >= NPATHS) terr(1);
    if (x -> fwd[ix] != NULL) terr(2);
    if (y -> bck[ix] != NULL) terr(3);
    x -> fwd[ix] = y; y -> bck[ix] = x;
  }

static void terr(int n)
  { giveup("Bug: trellis %d", n);
  }

inline float prx(int n) { return (float) n;		     }
inline float pry(int n) { return 0.75 * (float) (NSTATES-n); }

global void printtrellis()
  { FILE *fi = fopen("debugt.pic", "w");
    if (fi == NULL) giveup("Can't create debugt.pic");
    fprintf(fi, ".sp 0.5i\n.PS 8i\n");
    for (int i=0; i < TLEN; i++)
      { for (int j=0; j < NSTATES; j++)
	  { fprintf(fi, "\"\\(bu\" at (%g,%g)\n", prx(i), pry(j));
	  }
      }
    for (int i=0; i < TLEN-1; i++)
      { for (int j1=0; j1 < NSTATES; j1++)
	  { for (int j2=0; j2 < NSTATES; j2++)
	      { node *x = &trellis[i][j1], *y = &trellis[i+1][j2];
		if (pathfrom(x, y)) fprintf(fi, "line from (%g,%g) to (%g,%g)\n", prx(i), pry(j1), prx(i+1), pry(j2));
	      }
	  }
      }
    fprintf(fi, ".PE\n");
    fclose(fi);
  }

static bool pathfrom(node *x, node *y)
  { int k=0;
    until (k >= 8 || x -> fwd[k] == y) k++;
    return (k < 8);
  }

static uchar nr_difftab[] =	/* for non-redundant (4800 bps) coding */
  { /* encoding:  indexed by Q1 Q2 I1 I2, gives Q3 Q4
       decoding:  indexed by Q1 Q2 Q3 Q4, gives I1 I2 */
    1, 0, 3, 2,	 3, 1, 2, 0,  0, 2, 1, 3,  2, 3, 0, 1,
  };

static uchar te_difftab[] =	/* for trellis encoding */
  { /* encoding:  indexed by Q1 Q2 I1 I2, gives Q3 Q4 */
    0, 1, 2, 3,	 1, 0, 3, 2,  2, 3, 1, 0,  3, 2, 0, 1,
  };

static uchar td_difftab[] =	/* for trellis decoding */
  { /* decoding:  indexed by Q1 Q2 Q3 Q4, gives I1 I2 */
    0, 1, 2, 3,	 1, 0, 3, 2,  3, 2, 0, 1,  2, 3, 1, 0,
  };

void encoder::setrate(ushort rw)
  { rate = rw & (rb_4800 | rb_7200 | rb_9600 | rb_12000 | rb_14400);
    q = t = 0;
  }

complex encoder::encode(uint b)
  { switch (rate)
      { default:
	    giveup("can't encode rate %04x", rate);

	case rb_4800:
	  { q = (q << 2) | b;
	    uint k = nr_difftab[q & 0xf];
	    q = (q & 0xc) | k;
	    return ztab2[k];
	  }

	case rb_7200:
	  { q = (q << 2) | (b >> 1);
	    uint k = te_difftab[q & 0xf];
	    q = (q & 0xc) | k;
	    uint e = ((t & 1) << 3) | (k << 1) | (b & 1);
	    t = tfsmtab[t][k];	/* advance trellis state */
	    return ztab4[e];
	  }
      }
  }

void decoder::setrate(ushort rw)
  { rate = rw & (rb_4800 | rb_7200 | rb_9600 | rb_12000 | rb_14400);
    q = 0;
  }

uint decoder::decode(complex z, complex &ez)
  { switch (rate)
      { default:
	    giveup("can't decode rate %04x", rate);

	case rb_4800:
	  { q = (q << 1) | (2.0 * z.im > -z.re);	/* b1 */
	    q = (q << 1) | (2.0 * z.re > +z.im);	/* b2 */
	    ez = ztab2[q & 3];				/* quantized version of z, one of ABDC	 */
	    return nr_difftab[q & 0xf];			/* look up dibit from most recent 4 bits */
	  }

	case rb_7200:
	  { float minl = inf;
	    for (int i=0; i < NPATHS; i++)
	      { node *x = &trellis[i];



	  { uint r;
	    /* output bits from left end of trellis */
	    node *y = &trellis[nextcol+1];


	    r = survivors[nextcol];
	    ???
	    q = (q << 1) | ((r >> 2) & 1);		/* y1 */
	    q = (q << 1) | ((r >> 1) & 1);		/* y2 */
	    ez = ztab4[r];				/* quantized version of z */
	    return (td_difftab[q & 0xf] << 1) | (r & 1);


	    /* get 4-bit constellation posn */
	    r = (r << 1) | (z.re > +2.0 || z.re < -2.0);
	    r = (r << 1) | (z.im > +2.0 || z.im < -2.0);
	    r = (r << 1) | (z.re > 0.0);
	    r = (r << 1) | (z.im > 0.0);
	    r = ctab4[r & 0xf];
	    /* advance trellis */
	    for (int j=0; j < NSTATES; j++)
	      { node *x = &trellis[nextcol][j];
		for (int k=0; k < NPATHS; k++)
		  { node *y = x -> fwd[k];			/* x .. y is the path */
		    /* (length of path) = (length of survivor from x) + (length from point k to point r) */
		    y -> len[k] = x -> svlen + lentab[k][r];
		  }
	      }
	    if (++nextcol >= TLEN) nextcol = 0;
	    for (int j=0; j < NSTATES; j++)
	      { node *x = &trellis[nextcol][j];
		float minl = INF; int mink;
		for (int k=0; k < NPATHS; k++)
		  { if (x -> len[k] < minl) { minl = x -> len[k]; mink = k; }
		  }
		x -> svlen = minl;
		x -> survivor = mink;
	      }
	    return ???;
	  }
      }
  }

