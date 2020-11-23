/* Alpha document preparation system
   incremental editing module
   AJF	 January 1988 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include "alpha.h"

extern initfmtfile(), tidyfmtfile();	/* from format */
extern int formatpage();		/* from format */
extern char *lookupword();		/* from common */

extern FILE *editin, *menuin, *codeout, *messout;
extern bool pmode;
extern int minpage, maxpage, firstpage, cldeftop;
extern char *cldefs[];

extern int pvpage;		/* from format */

static struct node *filetext, *itext, *svtext, *freelist, *unmatchedlist;
static bool structured, changes;
static int pagenum, ch, column;
static char *btext, *bpara, *epara, *bcurl, *ecurl, *bcond, *econd, *elcond;
static FILE *infile;

forward bool matchlocal(), matchglobal();
forward char *collectword();
forward struct node *newnode();
forward uint pollstreams();


global initeditbook()
  { btext = lookupword("<text");
    bpara = lookupword("<PARA"); epara = lookupword(">PARA");
    bcurl = lookupword("{");     ecurl = lookupword("}");
    bcond = lookupword("<[");    econd = lookupword("]>");
    elcond = lookupword("]><[");
  }

global processfile()
  { freelist = unmatchedlist = NULL;
    changes = true; structured = false;
    initfiletext();
    initfmtfile();
    if (pmode) pm_interpret(); else sm_interpret();
    tidyfmtfile();
  }

static initfiletext()
  { struct node *top = newnode(), *tail = newnode();
    top -> typ = s_stop;    tail -> typ = s_stop;
    top -> wrd = NULL;	    tail -> wrd = NULL;
    top -> spb = 0;	    tail -> spb = 0;
    top -> bwd = NULL;	    tail -> bwd = top;
    top -> fwd = tail;	    tail -> fwd = NULL;
    filetext = itext = top;
  }

static pm_interpret()
  { infile = editin;
    nextchar();
    until (ch == EOF)
      { insertline();
	nextchar();
      }
    makelinks();
    printfile();
  }

static printfile()
  { /* format whole file for printing */
    int np, ptp, p;
    np = formatpage(filetext, -1);  /* prelim. pass to format whole file; returns num. of pages in file */
    ptp = min(maxpage-minpage, np-minpage);		    /* ptp = num. of pages to print */
    for (p=0; p < ptp; p++)
      { int pn = minpage+p;				    /* zero-relative page number */
	if (pn >= 0) formatpage(filetext, pn);		    /* 2nd arg is zero-relative */
      }
    firstpage += np; minpage -= np; maxpage -= np;	    /* adjust page params for next file, if there is one */
  }

static sm_interpret()
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

static processchar()
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
	    itext = filetext;
	    break;

	case 'm':
	  { /* mark path from current line to root (starting at a "real" word, not bol) */
	    struct node *tx = itext;
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

static batchchanges()
  { /* Editor is waiting for kbd input */
    if (changes)
      { makelinks();
	unless (structured)
	  { formatpage(filetext, -1);	/* prelim. pass to format entire file */
	    structured = true;
	  }
	formatpage(filetext, pagenum);
      }
    changes = false;
    fflush(codeout); fflush(messout);
  }

static makelinks()
  { linkup(matchlocal);
    linkup(matchglobal);
    reportunmatched();
  }

static linkup(p) proc p;
  { bool again;
    do
      { struct node *x = unmatchedlist;
	again = false;
	until (x == NULL)
	  { if (x -> typ == s_ubegin) again |= p(x);
	    x = x -> unm;
	  }
      }
    while (again);
  }

#define bracmatch(s1,s2) seq(&s1[1], &s2[1])

static bool matchlocal(xb) struct node *xb;
  { struct node *x = xb -> fwd;
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

static bool matchglobal(xb) struct node *xb;
  { struct node *x = xb;
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

static reportunmatched()
  { struct node *x = unmatchedlist;
    until (x == NULL)
      { fprintf(messout, "\"%s\" is unmatched\n", x -> wrd);
	x = x -> unm;
      }
  }

static insertline()
  { struct node *x;
    int nsp; bool cond = true;
    until (itext -> typ == s_eol || itext -> typ == s_stop) itext = itext -> fwd;
    insertword(s_bol, NULL, 0);
    x = itext; /* beginning of line */
    column = 0;
    nsp = countspaces();
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
	  { enum nodetype s;
	    if (w == bcurl) w = bpara; /* translate "{" to "<PARA" */
	    if (w == ecurl) w = epara; /* translate "}" to ">PARA" */
	    s = (w[0] == '<') ? s_ubegin : (w[0] == '>') ? s_uend : s_word;
	    insertword(s, w, nsp);
	  }
	nsp = countspaces();
      }
    insertword(s_eol, NULL, 0);
    itext = x; /* s_bol of inserted line */
  }

#define nextch() { if (n < MAXSTRLEN) v[n++] = ch; \
		   unless (ch == '\n' || ch == EOF) nextchar(); }

static char *collectword()
  { char v[MAXSTRLEN+1];
    int n = 0;
    until (ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF)
      { /* "esc c1 c2" counts as one char */
	while (ch == '\033') { nextch(); nextch(); }
	nextch();
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

static insertword(s, w, nsp) enum nodetype s; char *w; int nsp;
  { linknode(s, w, nsp);
    if (structured) markpath(); /* mark path to root */
    if (s == s_ubegin || s == s_uend) unmatched(itext);
  }

static deleteword()
  { markpath(); /* mark path to root */
    if (itext -> wrd == btext)
      { /* deleting <text; re-open all global spans */
	struct node *x = itext;
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

static demote(xb, xe) struct node *xb, *xe;
  { unless (xb == NULL)
      { unmatched(xb);
	xb -> typ = s_ubegin;
      }
    unless (xe == NULL)
      { unmatched(xe);
	xe -> typ = s_uend;
      }
  }

static unmatched(x) struct node *x;
  { struct node *z = unmatchedlist;
    until (z == NULL || z == x) z = z -> unm;
    if (z == NULL)
      { x -> unm = unmatchedlist;
	unmatchedlist = x;
      }
    else fprintf(messout, "bug: duplicate in unmatched list \"%s\"\n", x -> wrd);
  }

static promote(xb, xe) struct node *xb, *xe;
  { unless (xb == NULL)
      { rematched(xb);
	xb -> typ = s_begin; xb -> slk = xe;
      }
    unless (xe == NULL)
      { rematched(xe);
	xe -> typ = s_end; xe -> slk = xb;
      }
  }

static rematched(x) struct node *x;
  { struct node **zz = &unmatchedlist;
    struct node *z = *zz;
    until (z == NULL || z == x)
      { zz = &z -> unm;
	z = *zz;
      }
    if (z == NULL) fprintf(messout, "bug: in rematched \"%s\"\n",x -> wrd);
    else *zz = z -> unm;
  }

static markpath()
  { /* mark path to root for re-formatting */
    struct node *x = itext;
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

static linknode(s, w, nsp) enum nodetype s; char *w; int nsp;
  { struct node *b = itext, *f = itext -> fwd;
    struct node *x = newnode();
    x -> typ = s; x -> wrd = w; x -> spb = nsp;
    x -> bwd = b; x -> fwd = f;
    unless (b == NULL) b -> fwd = x;
    unless (f == NULL) f -> bwd = x;
    itext = x;
  }

static unlinknode(x) struct node *x;
  { struct node *b = x -> bwd, *f = x -> fwd;
    unless (b == NULL) b -> fwd = f;
    unless (f == NULL) f -> bwd = b;
    /* link through bwd, leave fwd untouched */
    x -> bwd = freelist; freelist = x;
  }

static struct node *newnode()
  { struct node *x;
    if (freelist != NULL)
      { x = freelist;
	freelist = freelist -> bwd;
      }
    else x = heap(1, struct node);
    x -> spn = -1; x -> edt = true;
    return x;
  }

#define filebit(f) (1 << (f -> _file))

static nextchar()
  { while (infile == NULL)
      { /* accept input from either stream; poll editin first for efficiency */
	if (charwaiting(editin)) infile = editin;
	else if (charwaiting(menuin)) infile = menuin;
	else pollstreams(filebit(editin) | filebit(menuin), true);  /* block until input arrives */
      }
    ch = getc(infile);
  }

static bool charwaiting(f) FILE *f;
  { /* test if char available, without blocking */
    return (f -> _cnt > 0) || (pollstreams(filebit(f), false) != 0);
  }

static uint pollstreams(ib, block) uint ib; bool block;
  { struct timeval tv; uint ob; int nb;
    tv.tv_sec = 0L; tv.tv_usec = 0L;
    ob = ib;
    nb = select(BPW, &ob, NULL, NULL, (block ? NULL : &tv));
    if (nb < 0)
      { unless (errno == EINTR) a_giveup("bug: select failed (%d)", errno);
	ob = 0;
      }
    return ob;
  }

