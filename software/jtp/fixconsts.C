/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include <string.h>
#include "jtp.h"
#include "tcodes.h"

#define MAXCONSTANTS 1000	/* was 200 */

struct constant
  { int m;
    word *cp;
    int lab;
  };

typedef int (*cfunc)(constant*, constant*);

extern "C"
  { /* binary search */
    constant *bsearch(constant*, constant*, int, int, cfunc);
  };

static constant constants[MAXCONSTANTS];
static int numconstants;

static void addconstant(int, int, word*);
static int labforconst(int, word*), cconsts(constant*, constant*);


global void fixconstants(tprog *tp)
  { int clen = tp -> clen;	    /* length before constants are "fixed" */
    tp -> gen(t_psect, m_num, 1);   /* put contants in data segment */
    numconstants = 0;
    for (int i = 0; i < clen; i++)
      { tinstr *ti = tp -> code[i];
	switch (ti -> m1)
	  { case m_astr:
	      { int lab = ++labno;
		char *s = ti -> p1.s;
		tp -> gen(t_word, m_num, strlen(s));	/* length precedes characters */
		tp -> gen(t_label, m_lab, lab);
		tp -> gen(t_ascii, m_str, s);
		ti -> m1 = m_lab; ti -> p1.n = lab;
		break;
	      }

	    case m_afloat:  case m_adouble:
	      { int m = (ti -> m1 == m_adouble) ? m_double : m_float;
		int lab = labforconst(m, ti -> p1.w);	/* seen this constant before? */
		if (lab < 0)
		  { lab = ++labno;
		    addconstant(lab, m, ti -> p1.w);
		    tp -> gen(t_label, m_lab, lab);
		    tp -> gen(t_word, m, ti -> p1);
		  }
		ti -> m1 = m_lab; ti -> p1.n = lab;
		break;
	      }
	  }
      }
    tp -> gen(t_psect, m_num, 0);   /* back to text segment */
  }

static void addconstant(int lab, int m, word *cp)
  { /* insertion sort */
    constant key = { m, cp, lab };
    int n = 0;
    while (n < numconstants && cconsts(&constants[n], &key) < 0) n++;
    if (numconstants >= MAXCONSTANTS) giveup("too many constants!");
    memmove(&constants[n+1], &constants[n], (numconstants-n) * sizeof(constant));
    numconstants++;
    constants[n] = key;	    /* copy the struct */
  }

static int labforconst(int m, word *cp)
  { constant key = { m, cp, 0 };
    constant *x = bsearch(&key, constants, numconstants, sizeof(constant), cconsts);
    return (x != NULL) ? x -> lab : -1;
  }

static int cconsts(constant *c1, constant *c2)
  { if (c1 -> m < c2 -> m) return -1;
    if (c1 -> m > c2 -> m) return +1;
    if (c1 -> cp[0].n < c2 -> cp[0].n) return -1;
    if (c1 -> cp[0].n > c2 -> cp[0].n) return +1;
    if (c1 -> m == m_double)
      { if (c1 -> cp[1].n < c2 -> cp[1].n) return -1;
	if (c1 -> cp[1].n > c2 -> cp[1].n) return +1;
      }
    return 0;
  }

