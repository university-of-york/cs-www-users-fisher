/* G2PSG parser generator   A.J. Fisher	  September 1990 */
/* generate table */

#include "g2psg.h"
#include <stdio.h>

extern FILE *output;
extern struct rhsitem *startrule;
extern struct dict *termtree, *nontermtree;
extern int numterms, numclauses;
extern bool verbose;
extern char *freprs[];
extern struct clause clauses[];

static int passnum, ploc;
static int sizes[7], strptrs[5], textsize;
static int ntix[MAXNONTERMS];

forward gprods(), gstring();


global maketab()
  { passnum = 1; generate();
    passnum = 2; generate();
    if (verbose) writestats();
  }

static generate()
  { genheader();
    genterms();
    genprods();
    genntnames();
    genfnames();
    genbitmasks();
    gensymtab();
    genstringtab();
  }

static genheader()
  { ploc = 0;
    genword(0400407);	/* magic number, used for .o files on Sun  */
    genword(textsize);	/* total size of table (i.e. text segment) */
    genword(0);		/* size of data region			   */
    genword(0);		/* size of bss region			   */
    genword(sizes[5]);	/* size of symbol table			   */
    genword(0);		/* entry point				   */
    genword(0);		/* size of text relocation		   */
    genword(0);		/* size of data relocation		   */
  }

static genterms()
  { ploc = 0;
    scantree(gstring,termtree);
    genbyte(0); /* table terminator */
    until (ploc%4 == 0) genbyte(0);
    sizes[0] = ploc;
  }

static genprods()
  { ploc = 0;
    if (startrule == NULL) { if (passnum == 2) prerror(181); }
    else genword(ntix[startrule -> d -> indx]);
    scantree(gprods,nontermtree);
    sizes[1] = ploc;
  }

#define PHSIZE (2+2*FMASKSIZE+TMASKSIZE)  /* size of prod rec hdr (== prhs in parser) */
#define ITSIZE (2+2*FMASKSIZE)		  /* size of an rhs item (nonterm, term, gap) */

static gprods(z) struct dict *z;
  { struct altern *y = z -> rhs;
    ntix[z -> indx] = ploc/4;
    until (y == NULL)
      { int nr, fl, i;
	nr = y -> alen;
	fl = ploc + 4*(PHSIZE + nr*ITSIZE + 1); /* forward link to next alternative */
	genword(fl == sizes[1] ? 0 : fl/4);	/* plnk */
	genword(ntix[z -> indx]);		/* plhs */
	genfpair(&y -> acat);			/* plhf */
	gentmask(&y -> asta);			/* psta */
	for (i=0; i<nr; i++) genitem(y -> adef[i]);
	genword(x_end);
	y = y -> alnk;
      }
  }

static genitem(x) struct rhsitem *x;
  { switch (x -> sy)
      { default:
	    prerror(261, x -> sy);
	    break;

	case s_term:
	    genword(x_term);
	    genword(x -> d -> indx);
	    genzmask(); genzmask();
	    break;

	case s_gap:
	    genword(x_gap);
	    genword(0);
	    genzmask(); genzmask();
	    break;

	case s_nonterm:
	    genword(x_nonterm);
	    genword(ntix[x -> d -> indx]);
	    genfpair(&x -> rcat);
	    break;
      }
  }

static genntnames()
  { ploc = 0;
    scantree(gstring,nontermtree);
    genbyte(0); /* terminator */
    until (ploc%4 == 0) genbyte(0);
    sizes[2] = ploc;
  }

static genfnames()
  { int k;
    ploc = 0;
    for (k=0; k<MAXFSPECS; k++)
      { char *s = freprs[k];
	if (s == NULL) s = "";
	genstring(s);
      }
    until (ploc%4 == 0) genbyte(0);
    sizes[3] = ploc;
  }

static gstring(z) struct dict *z; { genstring(z -> str); }

static genbitmasks()
  { int k;
    ploc = 0;
    for (k=0; k<numclauses; k++)
      { struct clause *z = &clauses[k];
	genword(z -> ord);
	genfmask(&z -> pos);
	genfmask(&z -> neg);
	genfpair(&z -> add);
      }
    /* terminator mask (top bit set) */
    for (k=0; k < FMASKSIZE-1; k++) genword(0);
    genword(o_end); /* terminator */
    sizes[4] = ploc;
  }

static genfpair(m) struct fpair *m;
  { genfmask(&m -> set);
    genfmask(&m -> cpl);
  }

static genfmask(m) struct fmask *m;
  { int k;
    for (k=0; k<FMASKSIZE; k++) genword(m -> m[k]);
  }

static gentmask(m) struct tmask *m;
  { int k;
    for (k=0; k<TMASKSIZE; k++) genword(m -> m[k]);
  }

static genzmask()
  { int k;
    for (k=0; k<FMASKSIZE; k++) genword(0);
  }

static gensymtab()
  { int defloc, i;
    ploc = defloc = 0;
    for (i=0; i<5; i++)
      { define(strptrs[i],defloc);
	defloc += sizes[i];
      }
    textsize = defloc;
    sizes[5] = ploc;
  }

static define(ix,val) int ix,val;
  { genword(ix);      /* index into string table */
    genword(5 << 24); /* global text symbol	 */
    genword(val);     /* value			 */
  }

static genstringtab()
  { ploc = 0;
    genword(sizes[6]); /* string table size */
    strptrs[0] = ploc; genstring("_Terms");
    strptrs[1] = ploc; genstring("_Prods");
    strptrs[2] = ploc; genstring("_Nnames");
    strptrs[3] = ploc; genstring("_Fnames");
    strptrs[4] = ploc; genstring("_Clauses");
    sizes[6] = ploc;
  }

static genstring(s) char *s;
  { int k = 0;
    do genbyte(s[k]); until (s[k++] == '\0');
  }

static genword(n) int n;
  { if (passnum == 2) fwrite(&n, 4, 1, output);
    ploc += 4;
  }

static genbyte(n) int n;
  { if (passnum == 2)
      { n <<= 24;
	fwrite(&n, 1, 1, output);
      }
    ploc += 1;
  }

static writestats()
  { fprintf(stderr, "Size of terminal table:  %6d bytes\n", sizes[0]);
    fprintf(stderr, "Size of production table:%6d bytes\n", sizes[1]);
    fprintf(stderr, "Size of NT name table:   %6d bytes\n", sizes[2]);
    fprintf(stderr, "Size of FS name table:   %6d bytes\n", sizes[3]);
    fprintf(stderr, "Size of bit-mask table:  %6d bytes\n", sizes[4]);
    fprintf(stderr, "Total size:              %6d bytes\n", textsize);
  }
