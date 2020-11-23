/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include "jtp.h"
#include "jcodes.h"
#include "tcodes.h"

static void cstaticfields(Class*, tprog*);
static void cmethods(Class*, tprog*);
static void listjcode(form*, word*, uchar*), listtcode(tprog*);
static int getwd(uchar*, int&);


global tprog *compileclass(Class *cl)
  { tprog *tp = new tprog;
    cmethods(cl, tp);
    cstaticfields(cl, tp);
    return tp;
  }

static void cstaticfields(Class *cl, tprog *tp)
  { formvec *fx;
    bool any = false;
    fx = cl -> fields;
    for (int i = 0; i < (fx -> num); i++)
      { form *x = fx -> vec[i];
	if (x -> acc & acc_static)
	  { unless (any)
	      { tp -> gen(t_psect, m_num, 1);	/* data segment */
		any = true;
	      }
	    tp -> gen(t_label, m_name, mkname(x -> fn));
	    tp -> gen(t_blkb, m_num, 4 * objectsize(x -> rtype));
	  }
      }
    if (any)
      { tp -> gen(t_psect, m_num, 0);		/* text segment */
	tp -> gen(t_end);
      }
  }

static void cmethods(Class *cl, tprog *tp)
  { formvec *mx = cl -> methods;
    for (int i = 0; i < (mx -> num); i++)
      { form *x = mx -> vec[i];
	if (x -> code != NULL)
	  { findlabels(x);
	    if (options & opt_v) listjcode(x, cl -> constpool, cl -> consttags);
	    tprog *y = new tprog, *z = new tprog;
	    translate(x, y, cl -> constpool, cl -> consttags);
	    if (options & opt_v) listtcode(y);	//
	    optimize(y, z);	/* code rearrangement */
	    peephole(z);	/* peephole optimization in-place */
	    if (options & opt_v) listtcode(z);	//
	    tp -> append(z);	/* append to output */
	    delete y; delete z;
	  }
      }
  }

static void listjcode(form *x, word *cpool, uchar *ctags)
  { /* list Java bytecode for method */
    fullname *fn = x -> fn;
    printf("***\tEntry\t\tcl=%s id=%s sig=%s\n", fn -> cl,  fn -> id, fn -> sig);
    uchar *cp = x -> code;
    until (cp[0] == j_end)
      { int lab = labelat(cp);
	if (lab > 0) printf("L%d", lab);
	jlist(cp, cpool, ctags);
	cp += jlength(cp);
      }
    putchar('\n');
  }

static void listtcode(tprog *x)
  { /* list Transputer code for method */
    for (int i = 0; i < (x -> clen); i++) tlist(x -> code[i]);
    putchar('\n');
  }

