/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 July 1997 */

#include <stdio.h>
#include "jtp.h"
#include "tcodes.h"
#include "cftags.h"

#define b_doing	 1
#define b_done	 2
#define b_clinit 4

#define MAXDEPTH 20

static tprog *program;
static char *clinit;
static Class *classvec[MAXDEPTH];
static int cldepth;

static void initclasses(), initclass(Class*), initbyname(char*), genmtabs();
static void gen(int, int = m_none, word = 0);


global void gentail(tprog *tp)
  { program = tp;
    clinit = lookupstring("<clinit>");
    gen(t_label, m_name, lookupstring("$clinits"));
    initclasses();
    gen(t_ret);
    gen(t_psect, m_num, 1);	/* need word alignment for method tables */
    genmtabs();
    gen(t_psect, m_num, 2);	/* special psect for $end */
    gen(t_label, m_name, lookupstring("$end"));
    gen(t_psect, m_num, 0);	/* back to text segment */
    gen(t_end);
  }

static void initclasses()
  { for (int i = 0; i < numitems; i++)
      { Item *it = &items[i];
	if (it -> ty == ty_class)
	  { Class *cl = it -> cl;
	    cl -> bits = 0;
	    formvec *mx = cl -> methods;
	    for (int j = 0; j < (mx -> num); j++)
	      { form *x = mx -> vec[j];
		if (x -> fn -> id == clinit) cl -> bits |= b_clinit;
	      }
	  }
      }
    cldepth = 0;
    initbyname("Heap");         /* must init this first! */
    for (int i = 0; i < numitems; i++)
      { Item *it = &items[i];
	if (it -> ty == ty_class) initclass(it -> cl);
      }
  }

static void initclass(Class *cl)
  { unless (cl -> bits & b_done)
      { if (cldepth >= MAXDEPTH) giveup("static init of classes nested too deeply!");
	classvec[cldepth++] = cl;
	if (cl -> bits & b_doing)
	  { warn("cyclic static init of classes:");
	    for (int i = 0; i < cldepth; i++)
	      { if (i > 0) fprintf(stderr, " ==> ");
		fputs(classvec[i] -> name, stderr);
		if (classvec[i] -> bits & b_clinit) putc('*', stderr);
	      }
	    putc('\n', stderr);
	  }
	else
	  { cl -> bits |= b_doing;
	    /* init classes called by this class *before* initing this class */
	    int nc = cl -> constpool[0].n;
	    for (int j = 1; j < nc; j++)
	      { if (cl -> consttags[j] == Constant_Class)
		  { char *cn = cl -> constpool[j].s;
		    unless (cn == cl -> name || cn == cl -> super) initbyname(cn);
		  }
	      }
	    /* next init superclass (after called classes: subtle!) */
	    unless (cl -> super == NULL) initbyname(cl -> super);
	    /* finally init this class */
	    if (cl -> bits & b_clinit)
	      { char buf[MAXSTRING+1]; sprintf(buf, "%s::<clinit>()", cl -> name);
		gen(t_call, m_name, lookupstring(buf));
	      }
	    cl -> bits &= ~b_doing;
	  }
	cldepth--;
      }
    cl -> bits |= b_done;
  }

static void initbyname(char *name)
  { Class *cl = lookupclass(name);
    unless (cl == NULL) initclass(cl);
  }

static void genmtabs()
  { for (int i = 0; i < numitems; i++)
      { Item *it = &items[i];
	if (it -> ty == ty_class)
	  { Class *cl = it -> cl;
	    unless (cl -> acc & acc_abstract)
	      { char buf[MAXSTRING+1]; sprintf(buf, "%s::<mtab>", cl -> name);
		gen(t_label, m_name, lookupstring(buf));
		formvec *mtab = cl -> mtab;
		for (int j = 0; j < mtab -> num; j++)
		  gen(t_word, m_name, mkname(mtab -> vec[j] -> fn));
	      }
	  }
      }
  }

static void gen(int op, int m, word v)
  { program -> gen(op, m, v);
  }

