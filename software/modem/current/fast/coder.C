/* Modem for MIPS   AJF	  August 1996
   V.32 encoding / decoding routines */

#include <stdio.h>

#include <complex.h>
#include <scramble.h>

#include "modem.h"
#include "coder.h"

#define INF 1e6

global c_complex ztab2[] =
  { /* 2-bit constellation (ABDC) */
    /* also used by v32rx, v32tx */
    { -3.0, -1.0 }, { +1.0, -3.0 }, { -1.0, +3.0 }, { +3.0, +1.0 }, /* ABDC */
  };

static c_complex ztab4[] =
  { /* 4-bit constellation */
    { +3.0, -3.0 }, { -1.0, +1.0 }, { -3.0, +3.0 }, { +1.0, -1.0 },
    { +3.0, +1.0 }, { -1.0, -3.0 }, { -3.0, -1.0 }, { +1.0, +3.0 },
    { -1.0, +3.0 }, { +3.0, -1.0 }, { +1.0, -3.0 }, { -3.0, +1.0 },
    { -3.0, -3.0 }, { +1.0, +1.0 }, { +3.0, +3.0 }, { -1.0, -1.0 },
  };

static uchar tfsmtab[NSTATES][NINPUTS] =
  { /* trellis encoder fsm */
    { 0, 3, 2, 1 }, { 4, 5, 7, 6 }, { 1, 2, 3, 0 }, { 7, 6, 4, 5 },
    { 2, 1, 0, 3 }, { 6, 7, 5, 4 }, { 3, 0, 1, 2 }, { 5, 4, 6, 7 },
  };

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


complex traininggen::get(int bc)
  { int k;
    if (bc < SEG_2)
      { /* segment 1 : ABAB... */
	k = (bc & 1) ? 1 : 0;
      }
    else if (bc < SEG_3)
      { /* segment 2 : CDCD... */
	k = (bc & 1) ? 2 : 3;
      }
    else
      { /* segment 3 & 4 : scrambled sequence */
	int b1 = scr -> fwd(1);
	int b2 = scr -> fwd(1);
	if (bc < SEG_4) b2 = b1;
	k = (b1 << 1) | b2;
      }
    return ztab2[k];	/* one of ABDC */
  }

void encoder::setrate(ushort rw)
  { rate = rw & (rb_4800 | rb_7200 | rb_9600 | rb_12000 | rb_14400);
    t = 0;
  }

complex encoder::encode(int b)
  { switch (rate)
      { default:
	    giveup("can't encode rate %04x", rate);

	case rb_4800:
	  { q = (q << 2) | b;
	    int k = nr_difftab[q & 0xf];
	    q = (q & 0xc) | k;
	    return ztab2[k];
	  }

	case rb_7200:
	  { q = (q << 2) | (b >> 1);
	    int k = te_difftab[q & 0xf];
	    q = (q & 0xc) | k;
	    int e = ((t & 1) << 3) | (k << 1) | (b & 1);
	    t = tfsmtab[t][k];	/* advance trellis state */
	    return ztab4[e];
	  }
      }
  }

void decoder::inittrellis()	/* private */
  { for (int i = 0; i < TLEN; i++)
      { for (int j = 0; j < NSTATES; j++)
	  { tnode *x = &trellis[i][j];
	    x -> path = 0;	/* 64 bits */
	    x -> svlen = (j == 0) ? 0.0 : INF;
	    for (int k = 0; k < NINPUTS; k++)
	      { int u = tfsmtab[j][k];
		tnode *y = &trellis[(i+1) % TLEN][u];
		x -> fwd[k] = y;    /* set up a path from node x to node y */
	      }
	  }
      }
    tindex = 0;
  }

void decoder::printtrellis(char *fn)
  { FILE *fi = fopen(fn, "w");
    if (fi == NULL) giveup("Can't create %s", fn);
    for (int j = 0; j < NSTATES; j++)
      { tnode *x = &trellis[tindex][j];
	fprintf(fi, "%d: %10.3f ", j, x -> svlen);
	u64bit path = x -> path;
	for (int i = 0; i < TLEN; i++)
	  { int d = (path >> (TLEN-1)*3) & 7;
	    putc('0' + d, fi);
	    path <<= 3;
	  }
	putc('\n', fi);
      }
    fclose(fi);
  }

void decoder::setrate(ushort rw)
  { rate = rw & (rb_4800 | rb_7200 | rb_9600 | rb_12000 | rb_14400);
  }

int decoder::decode(complex z)
  { switch (rate)
      { default:
	    giveup("can't decode rate %04x", rate);

	case rb_4800:
	    q = (q << 1) | (2.0 * z.im > -z.re);	    /* b1 */
	    q = (q << 1) | (2.0 * z.re > +z.im);	    /* b2 */
	    return nr_difftab[q & 0xf];			    /* look up dibit from most recent 4 bits */

	case rb_7200:
	  { pz = z;	/* remember for getez */
	    int r = viterbi(z);
	    /* differentially decode and return result (3 bits) */
	    q = (q << 2) | (r >> 1);			    /* shift in y1, y2 from past r */
	    return (td_difftab[q & 0xf] << 1) | (r & 1);    /* diff. decode & return result */
	  }
      }
  }

int decoder::viterbi(complex z)			/* private */
  { int ncol = (tindex+1) % TLEN;		/* index of next column */
    float metric[NSTATES];			/* dist^2 from z to closest point in j'th sub-constellation */
    uchar botbit[NSTATES];			/* unencoded bit */
    int jx = 0;
    for (int j = 0; j < NSTATES; j++)
      { float p0 = power(z - ztab4[jx++]);
	float p1 = power(z - ztab4[jx++]);
	if (p0 < p1) { metric[j] = p0; botbit[j] = 0; }
	else { metric[j] = p1; botbit[j] = 1; }
	trellis[ncol][j].svlen = INF;
      }
    for (int j = 0; j < NSTATES; j++)
      { int t = (j & 1) << 2;			/* t bit */
	tnode *x = &trellis[tindex][j];
	float svlen = (x -> svlen) * 0.99;	/* length of survivor from x, reduced by a bit to ensure stability */
	for (int k = 0; k < NINPUTS; k++)
	  { tnode *y = x -> fwd[k];		/* x .. y is the path */
	    float plen = svlen + metric[t+k];	/* work out total path length */
	    if (plen < y -> svlen)
	      { y -> svlen = plen;		/* install as survivor */
		y -> path = (x -> path << 3) | (k << 1) | botbit[t+k];
	      }
	  }
      }
    tindex = ncol;				/* move to next column */
    tnode *x = &trellis[tindex][0];		/* choose an arbitrary node at rh end */
    return (x -> path >> (TLEN-1)*3) & 7;	/* return bits about to drop off lh end */
  }

static uchar ctab4[] =
  { 0xf, 0x1, 0x3, 0xd,
    0x5, 0x8, 0xa, 0x7,
    0x6, 0xb, 0x9, 0x4,
    0xc, 0x2, 0x0, 0xe,
  };

complex decoder::getez()
  { switch (rate)
      { default:
	    giveup("can't decode rate %04x", rate);

	case rb_4800:
	    return ztab2[q & 3];		/* return quantized version of z, one of ABDC	*/

	case rb_7200:
	  { int r = 0;
	    r = (r << 1) | (pz.re > +2.0 || pz.re < -2.0);
	    r = (r << 1) | (pz.im > +2.0 || pz.im < -2.0);
	    r = (r << 1) | (pz.re > 0.0);
	    r = (r << 1) | (pz.im > 0.0);
	    r = ctab4[r];			/* get 4-bit constellation posn */
	    return ztab4[r];			/* return quantized version of z */
	  }
      }
  }

