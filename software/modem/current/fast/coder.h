/* segments of training sequence */
#define SEG_1	    0			/* ABAB...	 */
#define SEG_2	    (SEG_1 + 256)	/* CDCD...	 */
#define SEG_3	    (SEG_2 + 16)	/* TRN (A/C)	 */
#define SEG_4	    (SEG_3 + 256)	/* TRN (A/B/C/D) */

struct traininggen
  { traininggen(scrambler *s) { scr = s; }
    complex get(int);

private:
    scrambler *scr;
  };

struct encoder
  { encoder()	 { reset();		    }
    void reset() { setrate(rb_4800); q = 0; }
    void setrate(ushort);
    complex encode(int);
    ushort rate;

private:
    int q, t;
  };

#define TLEN	20		/* num. of columns in trellis, = 5 * constraint length */
#define NSTATES 8		/* num. of states in convolutional encoder */
#define NINPUTS 4		/* num. inputs to convolutional encoder (1 << k) */

typedef unsigned long long u64bit;

struct tnode
  { tnode *fwd[NINPUTS];	/* paths leading forwards in trellis */
    float svlen;		/* path length (metric) for surviving path terminating here */
    u64bit path;		/* path code to this point */
  };

struct decoder
  { decoder()	 { inittrellis(); reset();  }
    void reset() { setrate(rb_4800); q = 0; }	/* no need to re-init trellis; we haven't used it yet */
    void printtrellis(char*);
    void setrate(ushort);
    int decode(complex);
    complex getez();
    ushort rate;

private:
    void inittrellis();
    int viterbi(complex);
    tnode trellis[TLEN][NSTATES];
    int tindex, q;
    complex pz; /* for getez */
  };

extern c_complex ztab2[];

