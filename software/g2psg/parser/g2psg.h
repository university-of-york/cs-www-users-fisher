/* G2PSG parser generator   A.J. Fisher	  September 1990 */
/* header file */

#define global
#define forward
#define uint unsigned int
#define bool unsigned int
#define false ((bool) 0)
#define true ((bool) 1)
#define unless(x) if (!(x))
#define until(x) while (!(x))
#define heap(n,t) (t*) cheap((n)*sizeof(t))

#define MAXNAMELEN   80
#define MAXNONTERMS  100
#define MAXCLAUSES   100

#define FMASKSIZE    16	  /* must agree with parser.c */	/* N.B. must be even */
#define TMASKSIZE    15	  /* must agree with parser.c */

#define MAXFSPECS    FMASKSIZE*32
#define MAXTERMINALS (TMASKSIZE*32 - 1) /* save top bit for stopper */

typedef (*proc)();

forward char *cheap();

enum symbol
  { s_add, s_and, s_bit, s_bus, s_cat1, s_colon, s_comma, s_ecat, s_fcat,
    s_eof, s_equals, s_fcr, s_feature, s_gap, s_group,
    s_implies, s_lbrac, s_nonterm, s_not, s_or, s_precedes, s_rbrac,
    s_star, s_sub, s_term
  };

struct errorpos /* for error messages */
  { char *fn; /* filename */
    int ln;   /* line number */
  };

struct fmask { uint m[FMASKSIZE]; };
struct tmask { uint m[TMASKSIZE]; };

struct fpair
  { struct fmask set;
    struct fmask cpl;
  };

struct clause /* clause of fcr list */
  { uint ord;		/* top-down or bottom-up		 */
    struct fmask pos;	/* positive literals to check		 */
    struct fmask neg;	/* negative literals to check		 */
    struct fpair add;	/* positive literal to add (at most one) */
  };

struct dict /* dictionary node; one for each NT */
  { struct dict *llk, *rlk;
    struct altern *rhs; /* list of alternatives RHSs */
    int indx;
    char *str;
  };

struct rhsitem
  { enum symbol sy; /* s_term, s_nonterm, s_gap */
    struct dict *d;
    struct fpair rcat;
  };

struct memo /* memo list node (for computing starters) */
  { struct memo *mlnk;
    struct fmask fval;
    struct tmask tval;
  };

struct altern /* alternative node (one particular RHS) */
  { struct altern *alnk;
    uint amrk;
    int alen;			/* length of rhs		      */
    struct rhsitem **adef;	/* rhs definition (array of rhsitems) */
    struct fpair acat;		/* lhs features			      */
    struct tmask asta;
    struct errorpos alep;
    struct memo *amem;
  };

/* symbols in production record (used by parser & maketab) */
#define x_nonterm   0
#define x_term	    1
#define x_gap	    2
#define x_end	    3

/* propagation types (used by parser & maketab) */
#define o_tric	    1
#define o_perc	    2
#define o_end	    4

