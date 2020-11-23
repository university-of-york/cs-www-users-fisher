/* G2PSG parser generator   A.J. Fisher	  September 1990 */
/* phase 1 */

#include "g2psg.h"
#include <stdio.h>
#include <setjmp.h>

#define max(a,b)	(a > b) ? a : b
#define clearfmask(m)	memset(m, 0, sizeof(struct fmask))

#define MAXRHS	  10
#define MAXGROUPS 100

#define c_def	 1	/* define any new feature specs */
#define c_mcat	 2	/* prepend "M " to category     */
#define c_starok 4	/* [f *] is legal here		*/

struct predicate
  { enum symbol h1;
    struct predicate *h2, *h3;	/* for s_and etc.		     */
    int b;			/* bit number for s_bit		     */
    int rep;			/* num. of replications		     */
    struct fmask sm;		/* leaf bits for s_star		     */
  };

struct group
  { struct fmask pm, gm;
  };

static jmp_buf recover;
static struct dict *reswdtree, *featuretree, *fspectree, *item;
static int numnonterms, numprods, numfcrs, numgroups, numfspecs, repnum, subdepth, ch;
static enum symbol symb;
static bool precnl;
static char catname[MAXNAMELEN+1];
static struct predicate *catpred;
static struct predicate *ecat, *fcat; /* empty (bottom), full (top) categories */
static int fdepth[MAXFSPECS];
static struct group groups[MAXGROUPS];

global struct clause clauses[MAXCLAUSES];
global struct dict *termtree, *nontermtree;
global struct rhsitem *startrule;
global int numterms, numclauses;
global char *freprs[MAXFSPECS];
global struct errorpos errorpos;

extern bool verbose, errors;

forward struct dict *lookupfeature(), *lookupword();
forward struct group *newgroup();
forward struct rhsitem *nonterm();
forward struct predicate *predicate(), *primary(), *dyadicop();
forward struct predicate *orop(), *andop(), *notop(), *addop();
forward struct predicate *pred0(), *pred1(), *pred2(), *category();
forward bool intersects();
forward eterm();


global readgrammar()
  { int k;
    termtree = nontermtree = featuretree = fspectree = NULL;
    startrule = NULL;
    numfspecs = numfcrs = numclauses = numgroups = numnonterms = numprods = 0;
    errorpos.fn = NULL; errorpos.ln = 0;
    strcpy(catname, "???"); /* in case it's used in an error msg */
    for (k=0; k<MAXFSPECS; k++) freprs[k] = NULL; /* for maketab */
    ecat = pred0(s_ecat); fcat = pred0(s_fcat);
    declreswds();
    subdepth = 0;
    ch = '\n'; /* for precnl */
    nextsymb();
    groupsection();
    fcrsection();
    rulessection();
    errorpos.fn = NULL; errorpos.ln = -1;
    /* enumerate terminals, filling in "indx" slot in dict nodes */
    numterms = 0;
    scantree(eterm, termtree);
    if (numterms > MAXTERMINALS) prerror(116);
    if (verbose) writestats();
  }

static eterm(z) struct dict *z;
  { z -> indx = numterms++;
  }

static declreswds()
  { reswdtree = NULL;
    dsw("FCR", s_fcr);          dsw("GAP", s_gap);
    dsw("GROUP", s_group);
  }

static dsw(name,sy) char *name; enum symbol sy;
  { struct dict *it = lookupword(name, &reswdtree, true);
    it -> indx = (int) sy;
  }

static groupsection()
  { setjmp(recover);
    while (symb == s_group)
      { struct predicate *x; struct group *z, *zm; int d, k;
	nextsymb();
	x = category(c_def + c_mcat);	    /* get the group for the "M" category */
	z = newgroup();
	d = catdepth(x);
	makemask(x, &z -> gm, d);	    /* all bits in category	*/
	makemask(x, &z -> pm, d-1);	    /* parents of leaf bits	*/
	for (k=0; k<FMASKSIZE; k++)
	  z -> gm.m[k] &= ~z -> pm.m[k];    /* leave just the leaf bits */
	zm = newgroup();		    /* a group for the non-"M" base category as well */
	for (k=0; k<FMASKSIZE/2; k++)
	  { zm -> gm.m[k] = z -> gm.m[FMASKSIZE/2 + k];
	    zm -> gm.m[FMASKSIZE/2 + k] = 0;
	    zm -> pm.m[k] = z -> pm.m[FMASKSIZE/2 + k];
	    zm -> pm.m[FMASKSIZE/2 + k] = 0;
	  }
      }
  }

static struct group *newgroup()
  { struct group *z = &groups[numgroups++];
    if (numgroups > MAXGROUPS)
      { prerror(136);
	exit(2); /* give up! */
      }
    return z;
  }

static fcrsection()
  { setjmp(recover);
    while (symb == s_fcr)
      { struct predicate *x; int n;
	nextsymb();
	x = predicate(0);
	n = x -> rep;
	if (n < 0) addclauses(x); /* no star in predicate */
	else
	  { /* replicate clauses for each instantiation of "*" */
	    int k;
	    for (k=0; k<n; k++)
	      { repnum = k;
		addclauses(x);
	      }
	  }
	numfcrs++;
      }
  }

static addclauses(x) struct predicate *x;
  { /* Nodes dominate in the order:  AND  -->  OR  -->	NOT --> ADD --> (BIT,ECAT,FCAT,STAR) */
    switch (x -> h1)
      { default:
	    prerror(261, x -> h1);
	    break;

	case s_and:
	    addclauses(x -> h2);
	    addclauses(x -> h3);
	    break;

	case s_or:	case s_not:	case s_add:
	case s_bit:	case s_ecat:	case s_fcat:	case s_star:
	  { int k;
	    struct clause *z = &clauses[numclauses++];
	    if (numclauses > MAXCLAUSES)
	      { prerror(134);
		exit(2); /* give up! */
	      }
	    clearfmask(&z -> pos); clearfmask(&z -> neg);
	    clearfmask(&z -> add.set);
	    disjunct(x, &z -> pos, &z -> neg, &z -> add.set);
	    if (numbits(&z -> add.set) > 1) prerror(126, &z -> add.set); /* not a Horn clause */
	    for (k=0; k<FMASKSIZE; k++) z -> pos.m[k] |= z -> add.set.m[k];
	    makecompl(&z -> add);
	    setorder(z);
	    break;
	  }
      }
  }

static setorder(z) struct clause *z;
  { int k;
    z -> ord = 0;
    for (k=0; k < FMASKSIZE/2; k++)
      { if (z -> add.set.m[k] != 0)
	  { /* checking mother, influencing daughter */
	    z -> ord |= o_tric;
	  }
	if (z -> add.set.m[FMASKSIZE/2+k] != 0)
	  { /* checking daughter, influencing mother */
	    z -> ord |= o_perc;
	  }
      }
    if (z -> ord == (o_tric | o_perc)) prerror(137);
  }

static disjunct(x, pm, nm, sm) struct predicate *x; struct fmask *pm, *nm, *sm;
  { switch (x -> h1)
      { default:
	    prerror(261, x -> h1);
	    break;

	case s_or:
	    disjunct(x -> h2, pm, nm, sm);
	    disjunct(x -> h3, pm, nm, sm);
	    break;

	case s_not:
	    literal(x -> h2, nm);
	    break;

	case s_add:
	    literal(x -> h2, sm);
	    break;

	case s_bit:	case s_ecat:	case s_fcat:	case s_star:
	    literal(x, pm);
	    break;
      }
  }

static rulessection()
  { setjmp(recover);
    until (symb == s_eof)
      { switch (symb)
	  { default:
		prerror((symb == s_group || symb == s_fcr) ? 127 : 120);
		unless (symb == s_eof) nextsymb();
		until (precnl || symb == s_eof) nextsymb();
		break;

	    case s_nonterm:
		idrule();
		break;
	  }
      }
  }

static idrule()
  { struct errorpos ep; struct rhsitem *nt; struct altern *a;
    ep = errorpos;
    nt = nonterm();
    if (startrule == NULL) startrule = nt;
    if (nt -> d == startrule -> d)
      { if (numbits(&nt -> rcat.set) > 0) prerror(106, &nt -> rcat.set);
	unless (nt -> d -> rhs == NULL) prerror(123);
      }
    checkfor(s_colon, 104);
    a = heap(1, struct altern);
    numprods++;
    if (symb == s_nonterm)
      { /* ID rule */
	struct rhsitem *v[MAXRHS];
	int nr = 0;
	v[nr++] = nonterm();
	while (nr < MAXRHS && symb == s_comma)
	  { nextsymb();
	    v[nr++] = nonterm();
	  }
	if (symb == s_comma) prerror(110);
	a -> adef = heap(nr, struct rhsitem *);
	memcpy(a -> adef, v, nr * sizeof(struct rhsitem *));
	a -> alen = nr;
      }
    else if (symb == s_term || symb == s_gap)
      { /* lexical rule or gap rule */
	struct rhsitem *x = heap(1, struct rhsitem);
	x -> sy = symb;
	x -> d = item;
	nextsymb();
	a -> adef = heap(1, struct rhsitem *);
	a -> adef[0] = x;
	a -> alen = 1;
      }
    else prerror(109);
    a -> amrk = 0;
    a -> acat = nt -> rcat; /* struct assignment (struct fpair) */
    a -> alep = ep;
    a -> amem = NULL;
    a -> alnk = nt -> d -> rhs;
    nt -> d -> rhs = a; /* link in the new alternative */
  }

static struct rhsitem *nonterm()
  { struct dict *it; struct rhsitem *z; struct predicate *x; int d;
    it = item;
    checkfor(s_nonterm, 107);
    x = category(0);
    z = heap(1, struct rhsitem);
    z -> sy = s_nonterm;
    d = catdepth(x);
    makemask(x, &z -> rcat.set, d);
    makecompl(&z -> rcat);
    z -> d = it;
    return z;
  }

static struct predicate *predicate(p) int p;
  { struct predicate *x; enum symbol op; int pr;
    x = primary();
    op = symb; pr = priof(op);
    while (pr > p)
      { struct predicate *y;
	nextsymb();
	y = predicate(pr);
	x = dyadicop(op, x, y);
	op = symb; pr = priof(op);
      }
    return x;
  }

static int priof(op) enum symbol op;
  { switch (op)
      { default:	return -1;
	case s_implies: return 1;
	case s_equals:	return 1;
	case s_or:	return 2;
	case s_and:	return 3;
      }
  }

static struct predicate *primary()
  { switch (symb)
      { default:
	    prerror(121);
	    return ecat;

	case s_lbrac:
	  { struct predicate *x;
	    nextsymb();
	    x = predicate(0);
	    checkfor(s_rbrac, 122);
	    return x;
	  }

	case s_not:
	    nextsymb();
	    return notop(primary());

	case s_add:
	    nextsymb();
	    return addop(primary());

	case s_sub:
	    return category(c_starok);
      }
  }

static struct predicate *dyadicop(op, x, y) enum symbol op; struct predicate *x, *y;
  { switch (op)
      { default:
	    prerror(261, op);
	    return ecat;

	case s_implies:
	    return orop(notop(x), y);

	case s_equals:
	    return andop(dyadicop(s_implies,x,y), dyadicop(s_implies,y,x));

	case s_or:
	    return orop(x, y);

	case s_and:
	    return andop(x, y);
      }
  }

static struct predicate *category(opts) uint opts;
  { int k = 0, d = 0;
    catpred = ecat;
    if (opts & c_mcat) { catname[k++] = 'M'; d++; }
    getcat(k, d, opts);
    return catpred;
  }

static getcat(k, d, opts) int k, d; uint opts;
  { if (symb == s_sub)
      { nextsymb();
	unless (symb == s_bus)
	  { featurespec(k, d, opts);
	    while (symb == s_comma)
	      { nextsymb();
		featurespec(k, d, opts);
	      }
	  }
	checkfor(s_bus, 115);
      }
  }

static featurespec(k, d, opts) int k, d; uint opts;
  { struct dict *it = NULL;
    while (symb == s_feature)
      { int len = strlen(item -> str);
	if (k+len+1 > MAXNAMELEN)
	  { catname[k] = '\0';
	    prerror(105, catname);
	  }
	else
	  { if (k > 0) catname[k++] = ' ';
	    strcpy(&catname[k], item -> str);
	    k += len;
	    catname[k] = '\0';
	    unless (catname[0] == 'M' && catname[1] == '\0') /* feature "M" by itself doesn't get a bit */
	      { struct predicate *x;
		it = lookupfeature(catname, d, opts);
		x = heap(1, struct predicate);
		x -> h1 = s_bit; x -> b = it -> indx; x -> rep = -1;
		catpred = andop(catpred, x);
	      }
	    d++;
	  }
	nextsymb();
      }
    if ((opts & c_starok) && (symb == s_star))
      { if (it == NULL) prerror(124); /* [*] and [M *] are is illegal */
	else
	  { struct predicate *x = heap(1, struct predicate);
	    int k;
	    x -> h1 = s_star;
	    clearfmask(&x -> sm);
	    for (k=0; k<numgroups; k++)
	      { struct group *z = &groups[k];
		if (testbit(it -> indx, &z -> pm))
		  { int j;
		    for (j=0; j<FMASKSIZE; j++) x -> sm.m[j] |= z -> gm.m[j];
		  }
	      }
	    x -> rep = numbits(&x -> sm);
	    /* fprintf(stderr, "*** subst for [%s *] : %m\n", catname, &x -> sm); */    /* debug */
	    if (x -> rep == 0) prerror(128, catname); /* can't find group; can happen if GROUP stmt is missing */
	    catpred = andop(catpred, x);
	  }
	nextsymb();
      }
    else getcat(k, d, opts); /* nested category, if any */
  }

static struct predicate *andop(x, y) struct predicate *x, *y;
  { return (x -> h1 == s_fcat || y -> h1 == s_fcat) ? fcat :
	   (x -> h1 == s_ecat) ? y :
	   (y -> h1 == s_ecat) ? x :
	   pred2(s_and, x, y);
  }

static struct predicate *orop(x, y) struct predicate *x, *y;
  { return (x -> h1 == s_and) ? andop(orop(x -> h2, y), orop(x -> h3, y)) :
	   (y -> h1 == s_and) ? andop(orop(x, y -> h2), orop(x, y -> h3)) :
	   (x -> h1 == s_ecat || y -> h1 == s_ecat) ? ecat :
	   (x -> h1 == s_fcat) ? y :
	   (y -> h1 == s_fcat) ? x :
	   pred2(s_or, x, y);
  }

static struct predicate *notop(x) struct predicate *x;
  { return (x -> h1 == s_and) ? orop(notop(x -> h2), notop(x -> h3)) :
	   (x -> h1 == s_or) ? andop(notop(x -> h2), notop(x -> h3)) :
	   (x -> h1 == s_not) ? x -> h2 :
	   (x -> h1 == s_ecat) ? fcat :
	   (x -> h1 == s_fcat) ? ecat :
	   pred1(s_not, x);
  }

static struct predicate *addop(x) struct predicate *x;
  { return (x -> h1 == s_and) ? andop(addop(x -> h2), addop(x -> h3)) :
	   (x -> h1 == s_or) ? orop(addop(x -> h2), addop(x -> h3)) :
	   (x -> h1 == s_not) ? notop(addop(x -> h2)) :
	   (x -> h1 == s_add) ? x :
	   (x -> h1 == s_ecat) ? ecat :
	   (x -> h1 == s_fcat) ? fcat :
	   pred1(s_add, x);
  }

static struct predicate *pred0(op) enum symbol op;
  { struct predicate *z = heap(1, struct predicate);
    z -> h1 = op;
    z -> rep = -1;
    return z;
  }

static struct predicate *pred1(op, x) enum symbol op; struct predicate *x;
  { struct predicate *z = heap(1, struct predicate);
    z -> h1 = op;
    z -> h2 = x;
    z -> rep = x -> rep;
    return z;
  }

static struct predicate *pred2(op, x, y) enum symbol op; struct predicate *x, *y;
  { int xr = x -> rep, yr = y -> rep;
    struct predicate *z = heap(1, struct predicate);
    z -> h1 = op;
    z -> h2 = x;
    z -> h3 = y;
    if (xr >= 0 && yr >= 0 && xr != yr) prerror(130, catname);
    z -> rep = max(xr, yr);
    return z;
  }

static int catdepth(x) struct predicate *x;
  { switch (x -> h1)
      { default:
	    prerror(261, x -> h1);
	    return 0;

	case s_and:
	  { int d1 = catdepth(x -> h2);
	    int d2 = catdepth(x -> h3);
	    return max(d1, d2);
	  }

	case s_bit:
	    return fdepth[x -> b];

	case s_ecat:
	    return 0;
      }
  }

static makemask(x, m, d) struct predicate *x; struct fmask *m; int d;
  { clearfmask(m);
    setmask(x, m, d);
  }

static setmask(x, m, d) struct predicate *x; struct fmask *m; int d;
  { switch (x -> h1)
      { default:
	    prerror(261, x -> h1);
	    break;

	case s_and:
	    setmask(x -> h2, m, d);
	    setmask(x -> h3, m, d);
	    break;

	case s_bit:
	    if (fdepth[x -> b] <= d) literal(x, m);
	    break;

	case s_ecat:
	    break;
      }
  }

static literal(x, m) struct predicate *x; struct fmask *m;
  { switch (x -> h1)
      { default:
	    prerror(261, x -> h1);
	    break;

	case s_not:	case s_add:
	    prerror(135); /* misuse of ~ or + e.g. ~+[...] */
	    break;

	case s_ecat:	case s_fcat:
	    prerror(129); /* misuse of [] */
	    break;

	case s_bit:
	    setbit(x -> b, m -> m);
	    break;

	case s_star:
	  { /* add a single bit, determined by "repnum", to m */
	    struct fmask *sm = &x -> sm;
	    int k = 0, j = 0; bool done = false;
	    while (k < FMASKSIZE && !done)
	      { uint w = sm -> m[k];
		while (w != 0 && !done)
		  { uint bit = w & ~(w-1);	/* isolate a single bit */
		    w &= ~bit;			/* clear the bit from w */
		    if (j == repnum) { m -> m[k] |= bit; done = true; }
		    j++;
		  }
		k++;
	      }
	    break;
	  }
      }
  }

static makecompl(x) struct fpair *x;
  { int k;
    clearfmask(&x -> cpl);
    for (k=0; k<numgroups; k++)
      { struct group *z = &groups[k];
	if (intersects(&x -> set, &z -> gm))
	  { int j;
	    for (j=0; j<FMASKSIZE; j++)
	      x -> cpl.m[j] |= (z -> gm.m[j] & ~x -> set.m[j]);
	  }
      }
    if (intersects(&x -> set, &x -> cpl)) prerror(133, &x -> set);
  }

static bool intersects(m1, m2) struct fmask *m1, *m2;
  { int k = 0;
    while (k < FMASKSIZE && (m1 -> m[k] & m2 -> m[k]) == 0) k++;
    return (k < FMASKSIZE);
  }

static int numbits(m) struct fmask *m;
  { int k = 0, nb = 0;
    while (k < FMASKSIZE)
      { uint w = m -> m[k];
	until (w == 0) { w &= (w-1); nb++; }
	k++;
      }
    return nb;
  }

static checkfor(s, err) enum symbol s; int err;
  { unless (symb == s)
      { prerror(err);
	unless (symb == s_eof) nextsymb();
	until (precnl || symb == s_eof) nextsymb();
	subdepth = 0;
	_longjmp(recover);
      }
    nextsymb();
  }

static nextsymb()
  { precnl = false;
l:  switch (ch)
      { default:
	    prerror(103, ch);
	    rch();
	    goto l;

	case ' ':   case '\t':
	    rch();
	    goto l;

	case '\n':
	    precnl = true;
	    rch();
	    goto l;

	case EOF:
	    symb = s_eof;
	    break;

	case '{':
	    pragmat();
	    goto l;

	case '\'':  case '"':
	    terminal();
	    break;

	case 'a':   case 'b':	case 'c':   case 'd':	case 'e':   case 'f':
	case 'g':   case 'h':	case 'i':   case 'j':	case 'k':   case 'l':
	case 'm':   case 'n':	case 'o':   case 'p':	case 'q':   case 'r':
	case 's':   case 't':	case 'u':   case 'v':	case 'w':   case 'x':
	case 'y':   case 'z':
	case 'A':   case 'B':	case 'C':   case 'D':	case 'E':   case 'F':
	case 'G':   case 'H':	case 'I':   case 'J':	case 'K':   case 'L':
	case 'M':   case 'N':	case 'O':   case 'P':	case 'Q':   case 'R':
	case 'S':   case 'T':	case 'U':   case 'V':	case 'W':   case 'X':
	case 'Y':   case 'Z':
	case '0':   case '1':	case '2':   case '3':	case '4':   case '5':
	case '6':   case '7':	case '8':   case '9':
	    name();
	    break;

	case '+':
	    if (subdepth > 0) name();
	    else
	      { symb = s_add;
		rch();
	      }
	    break;

	case '-':
	    if (subdepth <= 0) prerror(132);
	    name();
	    break;

	case '*':
	    symb = s_star;
	    rch();
	    break;

	case ':':
	    symb = s_colon;
	    rch();
	    break;

	case ',':
	    symb = s_comma;
	    rch();
	    break;

	case '[':
	    symb = s_sub;
	    subdepth++;
	    rch();
	    break;

	case ']':
	    if (subdepth <= 0)
	      { prerror(125);
		rch();
		goto l;
	      }
	    symb = s_bus;
	    subdepth--;
	    rch();
	    break;

	case '(':
	    symb = s_lbrac;
	    rch();
	    break;

	case ')':
	    symb = s_rbrac;
	    rch();
	    break;

	case '&':
	    symb = s_and;
	    rch();
	    break;

	case '#':
	    symb = s_or;
	    rch();
	    break;

	case '=':
	    symb = s_implies; /* => */
	    rch(); checkch('>', 102);
	    break;

	case '<':
	    rch();
	    if (ch == '=')
	      { symb = s_equals; /* <=> */
		rch(); checkch('>', 102);
	      }
	    else symb = s_precedes; /* < */
	    break;

	case '~':
	    symb = s_not;
	    rch();
	    break;
      }
  }

static pragmat()
  { rch(); checkch('$', 108);
    switch (ch)
      { default:
	    prerror(119,ch);
	    break;

	case 'E':
	    /* record error passed on by metagee */
	    errors = true;
	    rch();
	    break;

	case 'F':
	  { /* record current filename and line number */
	    char v[MAXNAMELEN+1]; int nc = 0, ln = 0;
	    rch();
	    while (ch == ' ' || ch == '\t') rch();
	    until (nc >= MAXNAMELEN || ch == ' ' || ch == '\t' || ch == '}' || ch == EOF)
	      { v[nc++] = ch;
		rch();
	      }
	    v[nc++] = '\0';
	    errorpos.fn = heap(nc, char);
	    strcpy(errorpos.fn, v);
	    while (ch == ' ' || ch == '\t') rch();
	    while (ch >= '0' && ch <= '9')
	      { ln = ln*10 + (ch-'0');
		rch();
	      }
	    errorpos.ln = ln;
	    break;
	  }
      }
    while (ch == ' ' || ch == '\t') rch();
    checkch('}', 108);
  }

static terminal()
  { char v[MAXNAMELEN+1]; int nc = 0;
    int qu = ch;
    rch();
    while (nc < MAXNAMELEN && !(ch == qu || ch == '\n' || ch == EOF))
      { unless (ch >= ' ' && ch <= '~') prerror(111,ch);
	v[nc++] = ch;
	rch();
      }
    checkch(qu, 117);
    if (nc == 0) prerror(114);
    v[nc++] = '\0';
    item = lookupword(v, &termtree, true);
    symb = s_term;
  }

#define namechar(c) \
    ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))

static name()
  { char v[MAXNAMELEN+1]; int nc = 0;
    struct dict *it;
    if (ch == '+' || ch == '-')
      { v[nc++] = ch;
	rch();
      }
    else
      { while (nc < MAXNAMELEN && namechar(ch))
	  { v[nc++] = ch;
	    rch();
	  }
	if (namechar(ch)) prerror(101);
      }
    v[nc++] = '\0';
    it = lookupword(v, &reswdtree, false);
    if (it != NULL) symb = (enum symbol) it -> indx;
    else if (subdepth > 0)
      { symb = s_feature;
	item = lookupword(v, &featuretree, true);
      }
    else
      { symb = s_nonterm;
	item = lookupword(v, &nontermtree, true);
	if (item -> indx < 0)
	  { /* a new non-terminal */
	    item -> indx = numnonterms++;
	    if (numnonterms > MAXNONTERMS)
	      { prerror(131);
		exit(2); /* give up! */
	      }
	  }
      }
  }

static struct dict *lookupfeature(s, d, opts) char *s; int d; uint opts;
  { struct dict *it = lookupword(s, &fspectree, opts & c_def);
    if (it == NULL)
      { prerror(112, s);
	it = lookupword(s, &fspectree, true);
      }
    if (it -> indx < 0)
      { if (s[0] == 'M' && s[1] == ' ')
	  { struct dict *xit = lookupfeature(&s[2], d-1, opts); /* recurse */
	    it -> indx = MAXFSPECS/2 + xit -> indx;
	    if (it -> indx >= MAXFSPECS-1) /* reserve top bit for stopper */
	      { prerror(113);
		exit(2); /* give up! */
	      }
	  }
	else
	  { it -> indx = numfspecs++;
	    if (it -> indx >= MAXFSPECS/2)
	      { prerror(113);
		exit(2); /* give up! */
	      }
	  }
	freprs[it -> indx] = it -> str;
	fdepth[it -> indx] = d;
	/* fprintf(stderr, "*** depth of \"%s\" = %d\n", it -> str, d); /* debug */
      }
    return it;
  }

static struct dict *lookupword(v, m, ins) char *v; struct dict **m; bool ins;
  { bool found = false;
    until (found || *m == NULL)
      { int k = strcmp(v, (*m) -> str);
	/* N.B. must order "abc" before "ab" so parser lex. search will work! */
	if (k < 0) m = &(*m) -> rlk;
	if (k > 0) m = &(*m) -> llk;
	if (k == 0) found = true;
      }
    if (!found && ins)
      { struct dict *t = heap(1, struct dict);
	t -> llk = t -> rlk = NULL;
	t -> rhs = NULL;
	t -> indx = -1;
	t -> str = heap(strlen(v)+1, char);
	strcpy(t -> str, v);
	*m = t;
      }
    return *m;
  }

static checkch(c, err) char c; int err; { if (ch == c) rch(); else prerror(err); }

static rch()
  { if (ch == '\n' && errorpos.ln >= 0) errorpos.ln++;
    ch = getchar();
  }

static writestats()
  { writesizes();
    writegroups();
    writefcrs();
  }

static writesizes()
  { fprintf(stderr, "Number of productions:      %6d\n", numprods);
    fprintf(stderr, "Number of terminals:        %6d (%d)\n", numterms, MAXTERMINALS);
    fprintf(stderr, "Number of non-terminals:    %6d (%d)\n", numnonterms, MAXNONTERMS);
    fprintf(stderr, "Number of feature spec bits:%6d (%d)\n", numfspecs, MAXFSPECS/2);
    fprintf(stderr, "Number of feature groups:   %6d (%d)\n", numgroups, MAXGROUPS);
    fprintf(stderr, "Number of FCRs:             %6d\n", numfcrs);
    fprintf(stderr, "Number of FCR clauses:      %6d (%d)\n", numclauses, MAXCLAUSES);
    fprintf(stderr, "\n");
  }

static writegroups()
  { int k;
    for (k=0; k<numgroups; k++)
      { struct group *z = &groups[k];
	fprintf(stderr, "GROUP %m\n", &z -> pm);
	fprintf(stderr, "  --> %m\n", &z -> gm);
	fprintf(stderr, "\n");
      }
  }

static writefcrs()
  { int k;
    for (k=0; k<numclauses; k++)
      { struct clause *z = &clauses[k];
	fprintf(stderr, "FCR ord =");
	if (z -> ord & o_tric) fprintf(stderr, " tric");
	if (z -> ord & o_perc) fprintf(stderr, " perc");
	fprintf(stderr, "\n");
	fprintf(stderr, "    pos     %m\n", &z -> pos);
	fprintf(stderr, "    neg     %m\n", &z -> neg);
	fprintf(stderr, "    add.set %m\n", &z -> add.set);
	fprintf(stderr, "    add.cpl %m\n", &z -> add.cpl);
	fprintf(stderr, "\n");
      }
  }

