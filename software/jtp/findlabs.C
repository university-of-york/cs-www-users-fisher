/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include <strings.h>
#include "jtp.h"
#include "jcodes.h"

#define MAXLABELS 1000

struct label
  { uchar *ptr;
    int lab;
  };

static label labels[MAXLABELS];
static int numlabels;

static void checklab(uchar*), addlabel(uchar*);
static int clabs(label*, label*);

typedef int (*cfunc)(label*, label*);

extern "C"
  { /* binary search */
    label *bsearch(label*, label*, int, int, cfunc);
  };


global void findlabels(form *x)
  { /* identify label points */
    numlabels = 0;
    uchar *cp = x -> code;
    until (cp[0] == j_end)
      { checklab(cp);
	cp += jlength(cp);
      }
    addlabel(cp);   /* stopper */
  }

static void checklab(uchar *cp)
  { switch (cp[0])
      { case j_ifeq:		case j_ifnull:		case j_iflt:		case j_ifle:		case j_ifne:
	case j_ifnonnull:	case j_ifgt:		case j_ifge:
	case j_ificmpeq:	case j_ificmpne:	case j_ificmplt:	case j_ificmpgt:
	case j_ificmple:	case j_ificmpge:	case j_ifacmpeq:	case j_ifacmpne:
	case j_goto:		case j_jsr:
	  { int n = (cp[1] << 8) | cp[2];
	    if (n & 0x8000) n |= 0xffff0000;
	    addlabel(&cp[n]);
	    break;
	  }

	case j_gotow:		case j_jsrw:
	  { int n;
	    for (int i = 0; i < 4; i++) n = (n << 8) | cp[i+1];
	    addlabel(&cp[n]);
	  }

	case j_tableswitch:	case j_lookupswitch:
	  { int len = 1;
	    while ((int) (cp+len) & 3) len++;	/* skip padding after opcode */
	    if (cp[0] == j_tableswitch)
	      { tswitch *sw = (tswitch*) &cp[len];
		addlabel(&cp[sw -> dflt]);
		int ni = (sw -> hi) - (sw -> lo) + 1;
		for (int i = 0; i < ni; i++)
		  { int n = sw -> vec[i];
		    addlabel(&cp[n]);
		  }
	      }
	    else
	      { lswitch *sw = (lswitch*) &cp[len];
		addlabel(&cp[sw -> dflt]);
		int np = sw -> np;
		for (int i = 0; i < np; i++)
		  { int n = sw -> vec[2*i+1];
		    addlabel(&cp[n]);
		  }
	      }
	    break;
	  }
      }
  }

static void addlabel(uchar *ptr)
  { /* insertion sort */
    int n = 0;
    while (n < numlabels && labels[n].ptr < ptr) n++;
    if (n >= numlabels || labels[n].ptr > ptr)
      { /* it's a new label */
	if (numlabels >= MAXLABELS) giveup("too many labels in Java bytecode!");
	memmove(&labels[n+1], &labels[n], (numlabels-n) * sizeof(label));
	numlabels++;
	labels[n].ptr = ptr;
	labels[n].lab = ++labno;
      }
  }

global int labelat(uchar *ptr)
  { label key = { ptr, 0 };
    label *x = bsearch(&key, labels, numlabels, sizeof(label), clabs);
    return (x != NULL) ? x -> lab : -1;
  }

static int clabs(label *x1, label *x2)
  { uchar *p1 = x1 -> ptr, *p2 = x2 -> ptr;
    return (p1 < p2) ? -1 :
	   (p1 > p2) ? +1 : 0;
  }

