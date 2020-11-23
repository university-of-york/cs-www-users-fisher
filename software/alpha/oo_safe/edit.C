/* Alpha document preparation system
   incremental editing module
   AJF	 January 1988 */

#include <errno.h>

#include "alpha.h"

typedef bool (*bfunc)(node*);

static node *freelist = NULL;

static node *texttop, *texttail, *itext, *svtext, *unmatchedlist;
static bool structured, changes;
static int pagenum, ch, column;
static char *btext, *bpara, *epara, *bcurl, *ecurl, *bcond, *econd, *elcond;
static FILE *infile;

static void initfiletext(), tidyfiletext(), pm_interpret(), printfile(), sm_interpret();
static void processchar(), batchchanges(), makelinks(), linkup(bfunc);
static bool matchlocal(node*), matchglobal(node*);
static void reportunmatched(), insertline();
static char *collectword();
static int countspaces();
static void insertword(nodetype, char*, int);
static void deleteword();
static void demote(node*, node*), unmatched(node*), promote(node*, node*), rematched(node*);
static void markpath(), linknode(node*), unlinknode(node*);
static void nextchar();
static uint pollstreams(uint, bool);

static struct initedit
  { initedit();
  }
initedit;

initedit::initedit()
  { /* called on program entry */
    btext = lookupword("<text");
    bpara = lookupword("<PARA"); epara = lookupword(">PARA");
    bcurl = lookupword("{");     ecurl = lookupword("}");
    bcond = lookupword("<[");    econd = lookupword("]>");
    elcond = lookupword("]><[");
  }

global void processfile()
  { unmatchedlist = NULL;
    changes = true; structured = false;
    initfiletext();
    initfmtfile();
    if (pmode) pm_interpret(); else sm_interpret();
    tidyfmtfile();
    tidyfiletext();
  }

static void initfiletext()
  { texttop = new node(s_stop, NULL, 0);
    texttail = new node(s_stop, NULL, 0);
    texttop -> bwd = NULL;	texttail -> bwd = texttop;
    texttop -> fwd = texttail;	texttail -> fwd = NULL;
    itext = texttop;
  }

static void tidyfiletext()
  { /* return the whole of filetext chain to freelist (in one swell foop) */
    texttop -> bwd = freelist;
    freelist = texttail;
  }

static void pm_interpret()
  { infile = editin;
    nextchar();
    until (ch == EOF)
      { insertline();
	nextchar();
      }
    makelinks();
    printfile();
  }

static void printfile()
  { /* format whole file for printing */
    int np = formatpage(texttop, -1);  /* prelim. pass to format whole file; returns num. of pages in file */
    int ptp = min(maxpage-minpage, np-minpage);		    /* ptp = num. of pages to print */
    for (int p=0; p < ptp; p++)
      { int pn = minpage+p;				    /* zero-relative page number */
	if (pn >= 0) formatpage(texttop, pn);		   /* 2nd arg is zero-relative */
      }
    firstpage += np; minpage -= np; maxpage -= np;	    /* adjust page params for next file, if there is one */
  }

static void sm_interpret()
  { pagenum = 0;
    infile = NULL;  /* get next char from either editin or menuin */
    nextchar();
    until (ch == EOF)
      { until (ch == 'W' || ch == EOF)
	  { processchar();
	    nextchar();
	  }
	batchchanges();
	if (ch == 'W')
	  { infile = NULL;
	    nextchar();
	  }
      }
  }

static void processchar()
  { switch (ch)
      { default:
	    fprintf(messout, "bug: unknown delta char /%c/\n", ch);
	    break;

	case 'B':
	    /* backward one line */
	    do itext = itext -> bwd; until (itext -> typ == s_bol || itext -> typ == s_stop);
	    break;

	case 'D':
	    /* delete a line */
	    until (itext -> typ == s_eol) deleteword();
	    deleteword();
	    break;

	case 'F':
	    /* forward one line */
	    do itext = itext -> fwd; until (itext -> typ == s_bol || itext -> typ == s_stop);
	    break;

	case 'I':
	    /* insert a line of words */
	    nextchar();
	    insertline();
	    break;

	case 'P':
	  { /* specify page number for previewing */
	    int npn = pagenum;
	    nextchar();
	    if (ch == '+') npn++;
	    else if (ch == '-') npn--;
	    else
	      { int n = 0;
		while (ch >= '0' && ch <= '9')
		  { n = (n*10) + (ch-'0');
		    nextchar();
		  }
		npn = n - firstpage;
	      }
	    if (npn < 0) npn = 0;
	    unless (npn == pagenum)
	      { pagenum = npn;
		changes = true;
	      }
	    break;
	  }

	case 'R':
	    /* restore place */
	    itext = svtext;
	    break;

	case 'S':
	    /* save place */
	    svtext = itext;
	    break;

	case 'T':
	    /* top of file */
	    itext = texttop;
	    break;

	case 'm':
	  { /* mark path from current line to root (starting at a "real" word, not bol) */
	    node *tx = itext;
	    until (itext -> typ == s_word || itext -> typ == s_stop) itext = itext -> fwd;
	    markpath();
	    itext = tx;
	    break;
	  }

	case 'p':
	    /* preview last page edited */
	    if (pvpage >= 0 && pvpage != pagenum)
	      { pagenum = pvpage;
		changes = true;
	      }
	    break;
      }
  }

static void batchchanges()
  { /* Editor is waiting for kbd input */
    if (changes)
      { makelinks();
	unless (structured)
	  { formatpage(texttop, -1);   /* prelim. pass to format entire file */
	    structured = true;
	  }
	formatpage(texttop, pagenum);
      }
    changes = false;
    fflush(codeout); fflush(messout);
  }

static void makelinks()
  { linkup(matchlocal);
    linkup(matchglobal);
    reportunmatched();
  }

static void linkup(bfunc p)
  { bool again;
    do
      { node *x = unmatchedlist;
	again = false;
	until (x == NULL)
	  { if (x -> typ == s_ubegin) again |= p(x);
	    x = x -> unm;
	  }
      }
    while (again);
  }

inline bool bracmatch(char *s1, char *s2) { return seq(&s1[1], &s2[1]); }

static bool matchlocal(node *xb)
  { node *x = xb -> fwd;
    bool ok;
    until (x == NULL || x -> typ == s_ubegin || x -> typ == s_uend || x -> typ == s_end)
      { if (x -> typ == s_begin && x -> slk != NULL) x = x -> slk; /* skip inner local span */
	x = x -> fwd;
      }
    if (x != NULL && (x -> typ == s_uend || x -> typ == s_end) && bracmatch(xb -> wrd, x -> wrd))
      { if (x -> typ == s_end) demote(x -> slk, x); /* re-open previously matched span */
	promote(xb, x);
	ok = true;
      }
    else ok = false;
    return ok;
  }

static bool matchglobal(node *xb)
  { node *x = xb;
    bool ok;
    until (x == NULL || x -> wrd == btext)
      { if (x -> typ == s_begin && x -> slk != NULL) x = x -> slk; /* skip inner local span */
	x = x -> fwd;
      }
    if (x != NULL)
      { promote(xb, NULL);
	ok = true;
      }
    else ok = false;
    return ok;
  }

static void reportunmatched()
  { node *x = unmatchedlist;
    until (x == NULL)
      { fprintf(messout, "\"%s\" is unmatched\n", x -> wrd);
	x = x -> unm;
      }
  }

static void insertline()
  { until (itext -> typ == s_eol || itext -> typ == s_stop) itext = itext -> fwd;
    insertword(s_bol, NULL, 0);
    node *x = itext; /* beginning of line */
    column = 0;
    int nsp = countspaces();
    bool cond = true;
    until (ch == '\n' || ch == EOF)
      { char *w = collectword();
	if (w == bcond)
	  { char *w1; int k = 0;
	    countspaces(); /* skip spaces */
	    w1 = collectword();
	    while (k < cldeftop && cldefs[k] != w1) k++;
	    cond = (k < cldeftop);
	  }
	else if (w == econd) cond = true;
	else if (w == elcond) cond = !cond;
	else if (cond)
	  { if (w == bcurl) w = bpara; /* translate "{" to "<PARA" */
	    if (w == ecurl) w = epara; /* translate "}" to ">PARA" */
	    nodetype s = (w[0] == '<') ? s_ubegin : (w[0] == '>') ? s_uend : s_word;
	    insertword(s, w, nsp);
	  }
	nsp = countspaces();
      }
    insertword(s_eol, NULL, 0);
    itext = x; /* s_bol of inserted line */
  }

inline void nextch(char v[], int &n)
  { if (n < MAXSTRLEN) v[n++] = ch;
    unless (ch == '\n' || ch == EOF) nextchar();
  }

static char *collectword()
  { char v[MAXSTRLEN+1];
    int n = 0;
    until (ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF)
      { /* "esc c1 c2" counts as one char */
	while (ch == '\033') { nextch(v, n); nextch(v, n); }
	nextch(v, n);
	column++;
      }
    v[n++] = '\0';
    return lookupword(v);
  }

static int countspaces()
  { int n = 0;
    while (ch == ' ' || ch == '\t')
      { int sp = (ch == '\t') ? (8 - column%8) : 1;   /* cols. to move */
	n += sp; column += sp;
	nextchar();
      }
    return n;
  }

static void insertword(nodetype s, char *w, int nsp)
  { node *x = new node(s, w, nsp);
    linknode(x);
    if (structured) markpath(); /* mark path to root */
    if (s == s_ubegin || s == s_uend) unmatched(itext);
  }

static void deleteword()
  { markpath(); /* mark path to root */
    if (itext -> wrd == btext)
      { /* deleting <text; re-open all global spans */
	node *x = itext;
	until (x == NULL)
	  { if (x -> typ == s_begin && x -> slk == NULL) demote(x, NULL);
	    x = x -> bwd;
	  }
      }
    if (itext -> typ == s_begin) demote(itext, itext -> slk);
    if (itext -> typ == s_end) demote(itext -> slk, itext);
    if (itext -> typ == s_ubegin || itext -> typ == s_uend) rematched(itext); /* remove entry from unmatched list */
    unlinknode(itext); itext = itext -> fwd;
  }

static void demote(node *xb, node *xe)
  { unless (xb == NULL)
      { unmatched(xb);
	xb -> typ = s_ubegin;
      }
    unless (xe == NULL)
      { unmatched(xe);
	xe -> typ = s_uend;
      }
  }

static void unmatched(node *x)
  { node *z = unmatchedlist;
    until (z == NULL || z == x) z = z -> unm;
    if (z == NULL)
      { x -> unm = unmatchedlist;
	unmatchedlist = x;
      }
    else fprintf(messout, "bug: duplicate in unmatched list \"%s\"\n", x -> wrd);
  }

static void promote(node *xb, node *xe)
  { unless (xb == NULL)
      { rematched(xb);
	xb -> typ = s_begin; xb -> slk = xe;
      }
    unless (xe == NULL)
      { rematched(xe);
	xe -> typ = s_end; xe -> slk = xb;
      }
  }

static void rematched(node *x)
  { node **zz = &unmatchedlist;
    node *z = *zz;
    until (z == NULL || z == x)
      { zz = &z -> unm;
	z = *zz;
      }
    if (z == NULL) fprintf(messout, "bug: in rematched \"%s\"\n",x -> wrd);
    else *zz = z -> unm;
  }

static void markpath()
  { /* mark path to root for re-formatting */
    node *x = itext;
    x -> edt = true;	/* mark this word as "edited" */
    until (x == NULL)
      { x -> spn = -1;	/* mark for re-formatting */
	do
	  { if (x -> typ == s_end) x = x -> slk; /* skip nested span */
	    x = x -> bwd;
	  }
	until (x == NULL || x -> typ == s_begin);
      }
    unless (changes)
      { unless (pmode) putc('\014', messout);
	changes = true;
      }
  }

static void linknode(node *x)
  { node *b = itext, *f = itext -> fwd;
    x -> bwd = b; x -> fwd = f;
    unless (b == NULL) b -> fwd = x;
    unless (f == NULL) f -> bwd = x;
    itext = x;
  }

static void unlinknode(node *x)
  { node *b = x -> bwd, *f = x -> fwd;
    unless (b == NULL) b -> fwd = f;
    unless (f == NULL) f -> bwd = b;
    delete x;
  }

global node::node(nodetype t, char *w, int s)
  { /* constructor */
    typ = t; wrd = w; spb = s;
    spn = -1; edt = true;
  }

global void* node::operator new(int nb)
  { /* called by "new node" */
    node *x;
    if (freelist != NULL)
      { /* allocate from free list */
	x = freelist;
	freelist = freelist -> bwd;
      }
    else
      { /* allocate using malloc */
	x = ::new node;
      }
    return x;
  }

global void node::operator delete(void *p)
  { node *x = (node*) p;
    /* link through bwd, leave fwd untouched */
    x -> bwd = freelist; freelist = x;
  }

inline uint filebit(FILE *f) { return 1 << (f -> _file); }

inline bool charwaiting(FILE *f)
  { /* test if char available, without blocking */
    return (f -> _cnt > 0) || (pollstreams(filebit(f), false) != 0);
  }

static void nextchar()
  { while (infile == NULL)
      { /* accept input from either stream; poll editin first for efficiency */
	if (charwaiting(editin)) infile = editin;
	else if (charwaiting(menuin)) infile = menuin;
	else pollstreams(filebit(editin) | filebit(menuin), true);  /* block until input arrives */
      }
    ch = getc(infile);
  }

static uint pollstreams(uint bits, bool block)
  { static int tmo[] = { 0, 0 };
    int nb = select(BPW, &bits, NULL, NULL, (block ? NULL : tmo));
    if (nb < 0)
      { unless (errno == EINTR) giveup("bug: select failed (%d)", errno);
	bits = 0;
      }
    return bits;
  }

