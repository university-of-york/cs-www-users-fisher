#include "vwg.h"
#include "states.h"
#include <stdio.h>

/* parse tree node types */
#define ft_cycle    0
#define ft_down	    1
#define ft_across   2
#define ft_or	    3
#define NUMFTSYMS   4

struct tree
  { struct tree *hlnk;		/* for linking trees in hash table */
    int sy;			/* ft_xxx      */
    struct tree *x1, *x2;
    struct hypernotion *hn;	/* for ft_down */
  };

static struct tree *cyclenode;
static struct tree *forest[NUMFTSYMS][HASHSIZE];
static bool xoption;		/* for printftree */

extern FILE *poutfile;

extern bool eqhn();		/* from hnmatch */
extern int hashval();		/* from common	*/
extern dots();			/* form common	*/

forward struct tree *makeft(), *ftdyad(), *ftnode();


global struct tree *makeftree(st) struct state *st;
  { /* Convert Earley-style factorised "tree" into a more manageable binary parse tree */
    int i, j;
    for (i=0; i < NUMFTSYMS; i++)
      for (j = 0; j < HASHSIZE; j++) forest[i][j] = NULL;
    cyclenode = ftnode(ft_cycle, NULL, NULL);
    return makeft(st, NULL);
  }

static struct tree *makeft(stc, stp) struct state *stc, *stp;
  { /* stc is child state, stp is parent state */
    struct tree *ft;
    if (stc == NULL) ft = NULL;
    else if (stc -> mark) ft = cyclenode;
    else
      { struct statelist *x;
	stc -> mark = true; /* to trap cycles */
	ft = NULL;
	for (x = stc -> treep; x != NULL; x = x -> link)
	  { struct tree *ft1 = makeft(x -> left, NULL);
	    struct tree *ft2 = makeft(x -> down, stc);
	    struct tree *ft3 = ftdyad(ft_across, ft1, ft2);
	    unless (ft == ft3) ft = ftdyad(ft_or, ft, ft3);	/* weed out spurious ambiguity */
	  }
	stc -> mark = false;
      }
    unless (stp == NULL)
      { struct hyperalt *rhs = stp -> pval -> rhs;
	int j = stp -> jval - 1;
	ft = ftnode(ft_down, ft, NULL, rhs -> rdef[j]);
      }
    return ft;
  }

static struct tree *ftdyad(sy, x1, x2) int sy; struct tree *x1, *x2;
  { return (x1 == NULL) ? x2 : (x2 == NULL) ? x1 : ftnode(sy, x1, x2);
  }

static struct tree *ftnode(sy, x1, x2, hn) int sy; struct tree *x1, *x2; struct hypernotion *hn;
  { int hv = hashval(2, x1, x2);
    struct tree **head = &forest[sy][hv];
    struct tree *x = *head;
    until (x == NULL || eqtree(x, sy, x1, x2, hn)) x = x -> hlnk;
    if (x == NULL)
      { x = heap(1, struct tree);
	x -> sy = sy;
	x -> x1 = x1;
	x -> x2 = x2;
	x -> hn = hn;
	x -> hlnk = *head; *head = x;
      }
    return x;
  }

static bool eqtree(x, sy, x1, x2, hn) struct tree *x; int sy; struct tree *x1, *x2; struct hypernotion *hn;
  { if (x -> sy != sy) return false;
    switch (sy)
      { default:
	    return true;

	case ft_or:	case ft_across:
	    return (x -> x1 == x1 && x -> x2 == x2);

	case ft_down:
	    return (x -> x1 == x1 && eqhn(x -> hn, hn));
      }
  }

global printftree(x, xopt) struct tree *x; bool xopt;
  { xoption = xopt;
    if (x == NULL) fprintf(poutfile, "No parse.\n");
    else
      { putc('\n', poutfile);
	prftree(x, 0);
	putc('\n', poutfile);
      }
  }

static prftree(x, k) struct tree *x; int k;
  { switch (x -> sy)
      { default:
	    dots(k, poutfile); fprintf(poutfile, "BUG!\n");
	    break;

	case ft_or:
	    dots(k, poutfile); fprintf(poutfile, "OR\n");
	    if (xoption)
	      { prftree(x -> x1, k+2);
		prftree(x -> x2, k+2);
	      }
	    else
	      { int n = 0;
		pralts(x, k+2, &n);
	      }
	    break;

	case ft_across:
	    if (xoption)
	      { dots(k, poutfile); fprintf(poutfile, "ACR\n");
		prftree(x -> x1, k+2);
		prftree(x -> x2, k+2);
	      }
	    else
	      { prftree(x -> x1, k);
		prftree(x -> x2, k);
	      }
	    break;

	case ft_down:
	    dots(k, poutfile); fprintf(poutfile, "%h\n", x -> hn);
	    unless (x -> x1 == NULL) prftree(x -> x1, k+2);
	    break;

	case ft_cycle:
	    dots(k, poutfile); fprintf(poutfile, "*RECURSIVE*\n");
	    break;
      }
  }

static pralts(x, k, nn) struct tree *x; int k; int *nn;
  { if (x -> sy == ft_or)
      { pralts(x -> x1, k, nn);
	pralts(x -> x2, k, nn);
      }
    else
      { dots(k, poutfile); fprintf(poutfile, "(%d)\n", ++(*nn));
	prftree(x, k+2);
      }
  }

