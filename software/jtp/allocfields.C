/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include "jtp.h"

#define acc_overridden 0x8000

static void alloc(Class*);
static bool methmatch(form*, form*);

static char *init_id = lookupstring("<init>");


global void allocfields()
  { /* allocate field offsets and determine size of classes */
    for (int i = 0; i < numitems; i++)
      { Item *it = &items[i];
	if (it -> ty == ty_class)
	  { Class *cl = it -> cl;
	    cl -> size = -1;
	    cl -> refbm = 0;
	  }
      }
    for (int i = 0; i < numitems; i++)
      { Item *it = &items[i];
	if (it -> ty == ty_class) alloc(it -> cl);
      }
    for (int i = 0; i < numitems; i++)
      { Item *it = &items[i];
	if (it -> ty == ty_class)
	  { Class *cl = it -> cl;
	    formvec *mx = cl -> methods;
	    for (int j = 0; j < mx -> num; j++)
	      { form *x = mx -> vec[j];
		unless (x -> acc & acc_overridden) x -> acc |= acc_final;
	      }
	  }
      }
  }

static void alloc(Class *cl)
  { if (cl -> size < 0)
      { cl -> size = 0;
	cl -> mtab = new formvec();
	unless (cl -> super == NULL)
	  { Class *scl = lookupclass(cl -> super);
	    if (scl != NULL)
	      { alloc(scl);
		cl -> size = scl -> size;
		cl -> refbm = scl -> refbm;
		delete cl -> mtab;
		cl -> mtab = scl -> mtab -> clone();
	      }
	    else warn("can't find superclass `%s' of `%s'", cl -> super, cl -> name);
	  }
	formvec *fx;
	fx = cl -> fields;
	for (int i = 0; i < (fx -> num); i++)
	  { form *x = fx -> vec[i];
	    unless (x -> acc & acc_static)
	      { x -> addr = cl -> size;
		cl -> size += objectsize(x -> rtype);
		if (x -> rtype == 'L' || x -> rtype == 'R')
		  { if (x -> addr < 31) (cl -> refbm) |= (1 << (x -> addr));	/* top bit reserved for "array of objects" */
		    else warn("sorry, reference occurs past word 30 in class `%s'", cl -> name);
		  }
	      }
	  }
	fx = cl -> methods;
	formvec *mtab = cl -> mtab;
	for (int i = 0; i < (fx -> num); i++)
	  { form *x = fx -> vec[i];
	    unless (x -> fn -> id == init_id)	/* constructors don't go in method table */
	      { int j = 0;
		while (j < mtab -> num && !methmatch(x, mtab -> vec[j])) j++;
		x -> addr = j;
		if (j < mtab -> num)
		  { form *y = mtab -> vec[j];
		    y -> acc |= acc_overridden;
		    if (y -> acc & acc_final) warn("method `%s' overrides final method `%s'", mkname(x -> fn), mkname(y -> fn));
		    if (x -> acc & acc_static) warn("static method `%s' overrides `%s'", mkname(x -> fn), mkname(y -> fn));
		    mtab -> vec[j] = x;
		  }
		else
		  { unless (x -> acc & acc_static) mtab -> add(x);
		  }
	      }
	  }
      }
  }

static bool methmatch(form *x1, form *x2)
  { return (!(x1 -> acc & acc_private) &&
	    (x1 -> fn -> id == x2 -> fn -> id) && (x1 -> fn -> sig == x2 -> fn -> sig));
  }

