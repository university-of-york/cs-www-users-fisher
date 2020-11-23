/* G2PSG parser generator   A.J. Fisher	  September 1990 */
/* check grammar is connected, make starters */

#include "g2psg.h"
#include <stdio.h>

#define m_seen 1

extern struct rhsitem *startrule;
extern struct dict *nontermtree;
extern struct errorpos errorpos;

static bool again;

forward bool anystarters(), groupsok();
forward check();


global transstarters()
  { unless (startrule == NULL) mark(startrule);
    scantree(check,nontermtree);
  }

static mark(x) struct rhsitem *x;
  { if (x -> sy == s_nonterm)
      { struct dict *z = x -> d;
	struct altern *y = z -> rhs;
	bool def = false, any = false;
	struct errorpos ep; ep = errorpos;
	until (y == NULL)
	  { struct fpair *rf = &x -> rcat;   /* rhs features */
	    struct fpair *lf = &y -> acat;   /* lhs features */
	    struct fpair f;
	    errorpos = y -> alep;
	    any = true;
	    if (groupsok(lf, rf, &f))
	      { def = true;
		unless (y -> amrk & m_seen)
		  { int i;
		    y -> amrk |= m_seen;
		    for (i=0; i < (y -> alen); i++) mark(y -> adef[i]);
		  }
	      }
	    y = y -> alnk;
	  }
	errorpos = ep;
	unless (any) prerror(151, z -> str);			 /* NT not defined   */
	if (any && !def) prerror(152, z -> str, &x -> rcat.set); /* feature mismatch */
      }
  }

static check(z) struct dict *z;
  { struct altern *y = z -> rhs;
    struct errorpos ep; ep = errorpos;
    until (y == NULL)
      { errorpos = y -> alep;
	memset(&y -> asta, 0, sizeof(struct tmask));
	unless (y -> amrk & m_seen) prerror(153); /* not connected */
	do { again = false; findstarters(y, &y -> acat, &y -> asta); } while (again);
	unless (anystarters(&y -> asta)) prerror(154); /* blind alley, no starters */
	y = y -> alnk;
      }
    errorpos = ep;
  }

static bool anystarters(tm) struct tmask *tm;
  { int k = 0;
    while (k < TMASKSIZE && tm -> m[k] == 0) k++;
    return (k < TMASKSIZE);
  }

#define meq(f1,f2) (memcmp(f1, f2, sizeof(struct fmask)) == 0) /* are two fmasks identical? */

static findstarters(y, f, tm) struct altern *y; struct fpair *f; struct tmask *tm;
  { /* find starters of NT y when called with features f
       or result into tm */
    int k;
    struct memo *ml = y -> amem; /* memo list head */
    until (ml == NULL || meq(&ml -> fval, &f -> set)) ml = ml -> mlnk;
    if (ml == NULL)
      { struct rhsitem *x = y -> adef[0]; /* leftmost constituent */
	ml = heap(1, struct memo);
	ml -> fval = f -> set; /* struct assignment */
	memset(&ml -> tval, 0, sizeof(struct tmask));
	ml -> mlnk = y -> amem;
	y -> amem = ml; /* link it into memo list */
	if (x -> sy == s_term) setbit(x -> d -> indx, ml -> tval.m);
	if (x -> sy == s_gap) memset(&ml -> tval, -1, sizeof(struct tmask)); /* set all bits */
	if (x -> sy == s_nonterm) descend(x, &ml -> tval); /* follow leftmost branch */
      }
    for (k=0; k < TMASKSIZE; k++)
      { uint m = ml -> tval.m[k];
	if ((m & ~tm -> m[k]) != 0) again = true; /* another pass is required */
	tm -> m[k] |= m;
      }
  }

static descend(x, tm) struct rhsitem *x; struct tmask *tm;
  { struct dict *z = x -> d;
    struct altern *y = z -> rhs;
    until (y == NULL)
      { struct fpair *rf = &x -> rcat;	 /* rhs features */
	struct fpair *lf = &y -> acat;	 /* lhs features */
	struct fpair af;		      /* augmented features */
	if (groupsok(lf, rf, &af)) findstarters(y, &af, tm);
	y = y -> alnk;
      }
  }

static bool groupsok(f1, f2, af) struct fpair *f1, *f2, *af;
  { /* or's f1 with f2, putting result in f,
       checking group complement masks, returns T/F */
    int k = 0;
    while (k < FMASKSIZE && (f1 -> set.m[k] & f2 -> cpl.m[k]) == 0 && (f2 -> set.m[k] & f1 -> cpl.m[k]) == 0)
      { af -> set.m[k] = f1 -> set.m[k] | f2 -> set.m[k];
	af -> cpl.m[k] = f1 -> cpl.m[k] | f2 -> cpl.m[k];
	k++;
      }
    return (k >= FMASKSIZE);
  }
