/* Alpha document preparation system
   formatting module
   AJF	 December 1987 */

#include "alpha.h"
#include "gfxlib.h"

#include <stdio.h>
#include <setjmp.h>

#define MAXCONTEXT   160    /* max. depth of nesting of begin-end brackets		   */
#define MAXSYMS	     100    /* max. symbols (begin brackets, backslash directives, macros) */
#define MAXTABS	     30	    /* max. num. of tab stops					   */
#define MAXTLITS     100    /* max. num. of transliterations				   */

#define ENCHAR	     0261   /* en dash in Alpha encoding */

#define f_centre     0x01   /* <ce in force				*/
#define f_fill	     0x02   /* <fi in force				*/
#define f_justify    0x04   /* <ju in force				*/
#define f_whitespace 0x08   /* <ws in force				*/
#define f_eqn	     0x10   /* <eqn in force				*/
#define f_eqnce	     0x20   /* <eqnce in force				*/
#define f_reformat   0x40   /* begin bracket edited; reformat all below */

enum direct
  { /* < > directives */
    d_bit, d_def, d_text, d_null, d_ft, d_ls, d_lth, d_in, d_rin, d_tabs, d_tr,
    d_head, d_foot, d_fn, d_use,
    /* backslash directives */
    b_comment, b_glue, b_horiz, b_vert, b_tab, b_fix, b_tabfix,
    b_line, b_need, b_page, b_arg, b_tr, b_box, b_indent
  };

struct symtabent
  { char *str;
    enum direct dval;	/* d_ / b_ code		    */
    uint bval;		/* for <ce / <-ce etc. only */
    struct node *xval;	/* for macros		    */
  };

struct tlit
  { int len;	/* length of sequence to transliterate			  */
    char *tv;	/* sequence to transliterate				  */
    int u;	/* single char in translit font, or (LIGBIT | [c1:c2:c3]) */
  };

extern ps_outproc();					    /* from psout    */
extern scr_outproc();					    /* from scrout   */
extern int tabtranslit(), symtranslit(), boxtranslit();	    /* from translit */
extern inittranslit(), tidytranslit();			    /* from translit */
extern initfonts(), splitfontname();			    /* from common   */
extern bool setnumber();				    /* from common   */
extern char *lookupword();				    /* from common   */
extern bool testbit();					    /* from common   */
extern setbit();					    /* from common   */

extern FILE *messout;			/* from alpha	  */
extern int firstpage;			/* from alpha	  */
extern bool pmode;			/* from alpha	  */
extern char *stylefn;			/* from readstyle */
extern struct fishfont *fontinfo[];	/* from common	  */

global proc outproc;			/* used by common */

extern int
    /* style variables, defined in style file, from readstyle */
    bodywidth, leftmargin, mingluewidth,
    linesperpage, linesperbody, topmargin, headposn, footposn,
    vertspacing, indexspacing, indexmag;

global int fonts[NUMFP];	/* table of indices into fontinfo for R,I,B fonts (also used by common) */
global int pvpage;		/* used by edit	  */
global bool anotherpass;	/* used by scrout */

static jmp_buf abort[MAXCONTEXT];
static struct symtabent symtab[MAXSYMS];
static int context, symtop, dirbase, macrobase, numtlits, minline, maxline, linenum, linesskipped;
static int vertoffset, linewid, gluecount, supergluecount, linespacing, linethickness;
static int indent, nextindent, rightindent, currpage;
static int currfont, wordcount, generation;
static int eqnposition, topeqnwidth, boteqnwidth, eqntabs[3];
static char *dashword, *dotsword, *dirname;
static char *leqnword, *reqnword, *overword, *lparword, *rparword, *lbracword, *rbracword, *integword;
static bool continflag, firstfile;
static uint tlitbits[FONTSIZE/BPW], escapable[FONTSIZE/BPW], flags;
static struct node *ftext, *nextarg;
static int oline[MAXSTRLEN+1];
static int tabstops[MAXTABS];
static struct tlit tlittab[MAXTLITS];

forward struct node *collectarg();


global initfmtbook()
  { /* note messout, codeout, style vars are not set up yet... */
    int i; char *esc;
    outproc = pmode ? ps_outproc : scr_outproc;
    declsyswords();
    dashword = lookupword("-");
    dotsword = lookupword("...");
    leqnword = lookupword("{{");
    reqnword = lookupword("}}");
    overword = lookupword("over");
    lparword = lookupword("lpar"); rparword = lookupword("rpar");
    integword = lookupword("integ");
    lbracword = lookupword("lbrac"); rbracword = lookupword("rbrac");
    for (i=0; i < FONTSIZE/BPW; i++) escapable[i] = 0;
    esc = "<>{}].\\";   /* escapable chars */
    for (i=0; esc[i] != '\0'; i++) setbit(escapable, esc[i]);
    initfonts();
    firstfile = true;
  }

static declsyswords()
  { /* declare backslash directives */
    symtop = 0;
    declare("\\;", b_comment);           declare("\\g", b_glue);
    declare("\\h", b_horiz);             declare("\\v", b_vert);
    declare("\\t", b_tab);               declare("\\f", b_fix);
    declare("\\tf", b_tabfix);           declare("\\line", b_line);
    declare("\\need", b_need);           declare("\\page", b_page);
    declare("\\arg", b_arg);             declare("\\tr", b_tr);
    declare("\\box", b_box);             declare("\\in", b_indent);
    /* declare begin-end directives */
    dirbase = symtop;
    declare("<ce", d_bit, f_centre);     declare("<-ce", d_bit, f_centre);
    declare("<fi", d_bit, f_fill);       declare("<-fi", d_bit, f_fill);
    declare("<ju", d_bit, f_justify);    declare("<-ju", d_bit, f_justify);
    declare("<ws", d_bit, f_whitespace); declare("<-ws", d_bit, f_whitespace);
    declare("<eqn", d_bit, f_eqn);       declare("<-eqn", d_bit, f_eqn);
    declare("<eqnce", d_bit, f_eqnce);   declare("<-eqnce", d_bit, f_eqnce);
    declare("<def", d_def);              declare("<text", d_text);
    declare("<", d_null);                declare("<ft", d_ft);
    declare("<ls", d_ls);                declare("<lth", d_lth);
    declare("<in", d_in);                declare("<rin", d_rin);
    declare("<tabs", d_tabs);            declare("<tr", d_tr);
    declare("<head", d_head);            declare("<foot", d_foot);
    declare("<fn", d_fn);                declare("<use", d_use);
    macrobase = symtop;
  }

global initfmtfile()
  { int i;
    if (firstfile)
      { /* do book-level initialization now messout, codeout, style vars are set up */
	outproc(o_bobook);
	for (i=0; i < NUMTFONTS; i++)
	  { int tf = lookupfont("C", 12); /* transliteration font */
	    if (tf < 0) a_giveup("can't find transliteration font!"); /* give up */
	    fontinfo[tf] -> short_name = NULL; /* zap font name so it won't be found */
	  }
	firstfile = false;
      }
    outproc(o_bofile);
    inittranslit();
    for (i=0; i < FONTSIZE/BPW; i++) tlitbits[i] = 0;
    currpage = currfont = -1;
    generation = 0;
    symtop = macrobase; /* delete down to "permanent" symbols */
  }

global tidyfmtfile()
  { tidytranslit();	/* remove temp. files */
    outproc(o_eofile);
  }

global tidyfmtbook()
  { outproc(o_eobook);
  }

global int formatpage(x, pn) struct node *x; int pn;
  { /* pn < 0 means format whole file; o/wise pn is zero-relative */
    bool cls = false;
    if (pn >= 0 && pn != currpage)
      { currpage = pn;
	clearscreen();
	cls = true;
      }
    outproc(o_clcbar);			   /* clear changebars */
    minline = pn*linesperbody;		   /* first line to display */
    maxline = minline+(linesperbody-1);	   /* last line to display  */
    pvpage = -1;
    dopass(x);
    while (anotherpass)
      { clearscreen();
	dopass(x);
      }
    skiplines(linesperbody-1); /* clear rest of page */
    if (cls) outproc(o_eopage, firstpage+currpage);
    return (linenum+linesperbody-1) / linesperbody;	/* return num. of pages */
  }

static dopass(x) struct node *x;
  { int i;
    for (i=0; i<MAXTABS; i++) tabstops[i] = (i+1)*36000; /* half-inch intervals */
    linenum = linewid = gluecount = supergluecount = numtlits = 0;
    oline[0] = 0;
    eqnposition = topeqnwidth = boteqnwidth = 0;
    for (i=0; i<3; i++) eqntabs[i] = 0;
    linesskipped = linesperbody;
    vertoffset = topmargin;
    linespacing = 1;
    linethickness = 0; /* means "use thinnest possible" */
    context = indent = nextindent = rightindent = wordcount = 0;
    nextarg = NULL;
    anotherpass = false;
    flags = f_fill | f_justify;
    setbasicfonts();
    ftext = x;
    formatseq();
    if (currpage < 0)
      { fprintf(messout, "File: ");
	fprintf(messout, "%d pages, %d lines, ", (linenum+linesperbody-1)/linesperbody, linenum);
      }
    else fprintf(messout, "Page %d: ", firstpage+currpage);
    fprintf(messout, "%d words\n", wordcount);
  }

static setbasicfonts()
  { int k;
    for (k=0; k < NUMFP; k++) fonts[k] = -1;
    setfonts(Roman,  "C",  12);
    setfonts(Italic, "CO", 12);
    setfonts(Bold,   "CB", 12);
    for (k=0; k < NUMFP; k++)
      unless (k == Symbol || k == Boldix || k == Symbix)
	if (fonts[k] < 0) a_giveup("can't find basic font (position %d)!", k);
  }

static formatseq()
  { if (context < MAXCONTEXT)
      { _setjmp(abort[context++]); /* the "_" variety avoids the sigblock nonsense */
	fmtseq();
	context--;
      }
    else
      { fprintf(messout, "Spans nested too deeply!\n");
	fmtseq();
      }
  }

static fmtseq()
  { until (ftext == NULL || ftext -> typ == s_end) formatitem();
    dobreak(false);
  }

static formatitem()
  { switch (ftext -> typ)
      { default:
	    fprintf(messout, "bug: in formatitem %d\n", ftext -> typ);
	    ftext = ftext -> fwd;
	    break;

	case s_stop:	case s_bol:
	    continflag = false;
	    ftext = ftext -> fwd;
	    break;

	case s_eol:
	    unless ((flags & f_fill) || continflag)
	      { if (oline[0] > 0) dobreak(false);
		else skiplines(linespacing);
	      }
	    ftext = ftext -> fwd;
	    break;

	case s_begin:
	    dobreak(false);
	    formatbegin();
	    break;

	case s_ubegin:	case s_uend:
	  { char v[MAXSTRLEN+1];
	    sprintf(v, "*** %s ***", ftext -> wrd);
	    fmtword(v, ftext -> spb);
	    ftext = ftext -> fwd;
	    break;
	  }

	case s_word:
	    formatword();
	    break;
      }
  }

static formatword()
  { if (ftext -> edt)
      { if (pvpage < 0) pvpage = linenum/linesperbody;
	ftext -> edt = false;
      }
    if (nextarg != NULL && ftext -> wrd == dotsword)
      { struct node *ft = ftext;
	ftext = nextarg; nextarg = NULL;
	formatseq();
	nextarg = ftext; ftext = ft;
	ftext = ftext -> fwd;
      }
    else if (flags & f_eqn) formatequation();
    else formatwd();
  }

static formatequation()
  { char *w = ftext -> wrd;
    if (w == leqnword)
      { eqnposition = linewid;
	eqnvert(-1);					/* go up 1 */
	eqntabs[0] = eqnright(0);			/* filled in later */
	ftext = ftext -> fwd;
      }
    else if (w == overword)
      { deltrailglue();
	topeqnwidth = linewid - eqnposition;
	eqnvert(2);					/* go down 2 */
	eqntabs[1] = eqnback();				/* back to eqnposition */
	ftext = ftext -> fwd;
      }
    else if (w == reqnword)
      { int w;
	deltrailglue();
	boteqnwidth = linewid - eqnposition;
	eqnvert(-1);					/* go up 1 */
	eqntabs[2] = eqnback();				/* back to eqnposition */
	w = max(topeqnwidth, boteqnwidth);		/* max width of components */
	drawbar(w);					/* draw a bar */
	if (flags & f_justify) { stuffol(GLUECH); gluecount++; }
	if (flags & f_eqnce)
	  { /* go back and adjust tabs */
	    if (topeqnwidth > boteqnwidth)
	      { /* centre bot component in eqn */
		int dx = (topeqnwidth-boteqnwidth)/2;
		adjusttab(eqntabs[1], +dx);
		adjusttab(eqntabs[2], -dx);
	      }
	    if (boteqnwidth > topeqnwidth)
	      { /* centre top component in eqn */
		int dx = (boteqnwidth-topeqnwidth)/2;
		adjusttab(eqntabs[0], +dx);
		adjusttab(eqntabs[1], -dx);
	      }
	  }
	ftext = ftext -> fwd;
      }
    else if (w == lparword)
      { eqncompound(0346);
	ftext = ftext -> fwd;
      }
    else if (w == lbracword)
      { eqncompound(0351);
	ftext = ftext -> fwd;
      }
    else if (w == rparword)
      { deltrailglue();
	eqncompound(0366);
	ftext = ftext -> fwd;
      }
    else if (w == rbracword)
      { deltrailglue();
	eqncompound(0371);
	ftext = ftext -> fwd;
      }
    else if (w == integword)
      { eqncompound(0363);
	ftext = ftext -> fwd;
      }
    else formatwd();
  }

static adjusttab(olp, dx) int olp, dx;
  { if (olp > 0 && olp <= oline[0] && dx != 0)
      { int c = oline[olp];
	int ft = c >> 8, ch = c & 0377;
	oline[olp] = tabtranslit('h', (fontinfo[ft] -> advance[ch].x) + dx);
	linewid += dx;
      }
  }

static eqncompound(c) int c;	/* {c, c+1, c+2} are {top, mid, bot} chars */
  { if (fonts[Symbol] < 0)
      { int ps = fontinfo[fonts[Roman]] -> ptsize;
	setfonts(Symbol, "S", ps);
      }
    if (fonts[Symbol] >= 0)
      { eqnposition = linewid;
	eqnvert(-1);
	stuffol(Mkchar(fonts[Symbol], c));	    /* top char */
	eqnback(); eqnvert(1);
	if (pmode)
	  { stuffol(Mkchar(fonts[Symbol], c+1));    /* mid char - omitted in non-pmode; screen looks better w/o it */
	    eqnback();
	  }
	eqnvert(1);
	stuffol(Mkchar(fonts[Symbol], c+2));	    /* bot char */
	eqnvert(-1);
      }
  }

static eqnvert(ndown) int ndown;
  { int c = tabtranslit('v', ndown*vertspacing);    /* move down "ndown" lines */
    stuffol(c);
  }

static int eqnback()
  { return eqnright(eqnposition-linewid);	    /* move back to eqnposition */
  }

static int eqnright(d) int d;
  { int c = tabtranslit('h', d);
    stuffol(c);
    return oline[0];
  }

static drawbar(w) int w;
  { int romf = fonts[Roman];
    int enwid = fontinfo[romf] -> advance[ENCHAR].x;
    if (w < enwid) w = enwid;
    while (w > enwid)
      { stuffol(Mkchar(romf, ENCHAR));
	w -= enwid;
      }
    if (w < enwid)
      { int c = tabtranslit('h', w-enwid);  /* move left for partial dash */
	stuffol(c);
      }
    stuffol(Mkchar(romf, ENCHAR));
  }

static formatwd()
  { char *w = ftext -> wrd;
    if (w[0] == '\\')
      { if (testbit(escapable, w[1] & 0377))
	  { fmtword(&w[1], ftext -> spb); /* remove leading backslash */
	    ftext = ftext -> fwd;
	  }
	else if (w[1] >= '0' && w[1] <= '9')
	  { int n;
	    setnumber(&w[1], &n, 0);
	    dobreak(false); skiplines(n);
	    ftext = ftext -> fwd;
	  }
	else
	  { int i = 0;
	    while (i < dirbase && symtab[i].str != w) i++;
	    if (i < dirbase)
	      { dirname = w;
		ftext = ftext -> fwd;
		bsdir(&symtab[i]);
	      }
	    else
	      { fprintf(messout, "Unrecognized command \"%s\"\n", w);
		ftext = ftext -> fwd;
	      }
	  }
      }
    else
      { fmtword(w, ftext -> spb);
	ftext = ftext -> fwd;
      }
  }

static formatbegin()
  { uint f = flags;
    if (ftext -> edt) flags |= f_reformat;	/* "edt" set means format all below this level */
    ftext -> edt = false;			/* reset the flag			       */
    if ((flags & f_reformat) || (ftext -> spn < 0)) formatspan();
    else if ((linenum + ftext -> spn - 1 >= minline) && (linenum <= maxline))
      { /* span overlaps current page; re-format if it's moved up or down, else skip */
	int k = (linenum%linesperbody) + vertoffset;
	if (ftext -> gln != generation + k) formatspan();
	else
	  { struct node *x = ftext;
	    skipspan();
	    vertoffset = x -> vof;
	  }
      }
    else
      { /* spn is known, but span is off page; invalidate gln and skip the span */
	struct node *x = ftext;
	skipspan();
	x -> gln = -1;
      }
    flags = f;
  }

static formatspan()
  { /* re-format span */
    struct node *x = ftext; int ln = linenum;
    int k = (linenum%linesperbody) + vertoffset;
    fmtspan();
    /* don't record spn for macro defns; this forces them to be evaluated */
    if (nextarg == NULL)
      { x -> spn = linenum-ln;
	x -> skp = linesskipped;
	x -> vof = vertoffset;
	x -> gln = generation + k;
      }
  }

static skipspan()
  { /* skip span */
    linenum += ftext -> spn;
    linesskipped = ftext -> skp;
    ftext = ftext -> slk;
    unless (ftext == NULL) ftext = ftext -> fwd;
  }

static fmtspan()
  { char *w = ftext -> wrd;
    int i = symtop-1; /* search symbol table from most recent */
    int in = indent, ni = nextindent;
    while (i >= dirbase && symtab[i].str != w) i--;
    if (i >= macrobase)
      { struct node *na = nextarg, *ft = ftext;
	nextarg = ftext -> fwd;
	ftext = symtab[i].xval;
	formatitem();
	nextarg = na;
	ftext = ft -> slk;
      }
    else if (i >= dirbase)
      { dirname = w;
	ftext = ftext -> fwd;
	blkdir(&symtab[i]);
      }
    else
      { fprintf(messout, "Unrecognized command \"%s\"\n", w);
	ftext = ftext -> slk;
      }
    unless (ftext == NULL)
      { unless (ftext -> typ == s_end) fprintf(messout, "bug: end expected, got %d\n", ftext -> typ);
	ftext = ftext -> fwd;
      }
    indent = in; nextindent = ni;
  }

static blkdir(x) struct symtabent *x;
  { switch (x -> dval)
      { default:
	    fprintf(messout, "bug: in directive %s\n", x -> str);
	    break;

	case d_bit:
	    /* set/clear a flag bit */
	    if (x -> str[1] == '-')
	      { /* clear the bit */
		flags &= ~x -> bval;
	      }
	    else
	      { /* set the bit */
		flags |= x -> bval;
	      }
	    formatseq();
	    break;

	case d_def:
	  { int st = symtop;
	    struct node *arg1, *arg2;
	    char v[MAXSTRLEN+1];
	    arg1 = collectarg(s_word);
	    arg2 = collectarg(s_begin);
	    v[0] = '<'; strcpy(&v[1], arg1 -> wrd);
	    declare(v, 0, 0, arg2);
	    formatseq();
	    symtop = st;
	    break;
	  }

	case d_text:	case d_null:
	    formatseq();
	    break;

	case d_ft:
	  { int oldf[NUMFP]; int k;
	    for (k=0; k < NUMFP; k++) oldf[k] = fonts[k];
	    for (k = Roman; k <= Bold; k++) /* Roman, Italic, Bold */
	      { struct node *arg; char *w;
		arg = collectarg(s_word);
		w = arg -> wrd;
		unless (w == dashword)
		  { char fn[MAXSTRLEN+1]; int ps;
		    splitfontname(w, fn, &ps);
		    setfonts(k, fn, ps);
		    if (k == Roman) fonts[Symbol] = -1; /* force re-eval of Symbol font */
		  }
	      }
	    formatseq();
	    for (k=0; k < NUMFP; k++) fonts[k] = oldf[k];
	    break;
	  }

	case d_ls:
	  { int ls = linespacing;
	    struct node *arg = collectarg(s_word);
	    setnumber(arg -> wrd, &linespacing, n_signok | n_rel);
	    formatseq();
	    linespacing = ls;
	    break;
	  }

	case d_lth:
	  { int lth = linethickness;
	    struct node *arg = collectarg(s_word);
	    setnumber(arg -> wrd, &linethickness, n_signok | n_rel | n_unitsok);
	    formatseq();
	    linethickness = lth;
	    break;
	  }

	case d_in:
	  { /* indentation is saved in fmtspan; no need to save here */
	    struct node *arg = collectarg(s_word);
	    setnumber(arg -> wrd, &nextindent, n_signok | n_rel | n_unitsok);
	    indent = nextindent;
	    formatseq();
	    break;
	  }

	case d_rin:
	  { int ri = rightindent;
	    struct node *arg = collectarg(s_word);
	    setnumber(arg -> wrd, &rightindent, n_signok | n_rel | n_unitsok);
	    formatseq();
	    rightindent = ri;
	    break;
	  }

	case d_tabs:
	  { int otabs[MAXTABS];
	    int n = 0; int i;
	    while ((n < MAXTABS) && (ftext != NULL) && (ftext -> typ == s_word) && (ftext -> wrd[0] != '\\'))
	      { otabs[n] = tabstops[n];
		tabstops[n] = (n > 0) ? tabstops[n-1] : 0; /* for relative positioning */
		setnumber(ftext -> wrd, &tabstops[n], n_signok | n_rel | n_unitsok);
		if (n > 0 && tabstops[n] <= tabstops[n-1])
		  fprintf(messout, "Tabstop at \"%s\" out of order; ignored\n", ftext -> wrd);
		else n++;
		ftext = ftext -> fwd;
	      }
	    if ((ftext != NULL) && (ftext -> typ == s_word) && (ftext -> wrd[0] != '\\'))
	      fprintf(messout, "Too many tab stops!\n");
	    formatseq();
	    for (i=0; i<n; i++) tabstops[i] = otabs[i];
	    break;
	  }

	case d_tr:
	  { int nt = numtlits, k = -1; uint tb;
	    struct node *arg1, *arg2; char *w1, *w2;
	    arg1 = collectarg(s_word);
	    arg2 = collectarg(s_word);
	    w1 = arg1 -> wrd; w2 = arg2 -> wrd;
	    if (w1[0] == '\\') w1 = lookupword(&w1[1]); /* remove leading backslash if any */
	    if (numtlits < MAXTLITS)
	      { tlittab[numtlits].len = strlen(w1);
		tlittab[numtlits].tv = w1;
		tlittab[numtlits].u = symtranslit(w2);
		if (w1[0] != '\0')
		  { /* set bit in bitmap corresponding to first char */
		    int fc = w1[0] & 0377;
		    k = fc/BPW; tb = tlitbits[k];
		    setbit(tlitbits, fc);
		    numtlits++;
		  }
		else fprintf(messout, "Zero-length transliteration ignored\n");
	      }
	    else fprintf(messout, "Too many transliterations!\n");
	    formatseq();
	    if (k >= 0) tlitbits[k] = tb;
	    numtlits = nt;
	    break;
	  }

	case d_head:	case d_foot:
	  { struct node *arg1, *arg2, *arg3; int n1, n2;
	    int pn = (minline/linesperbody)+firstpage;
	    arg1 = collectarg(s_word);
	    arg2 = collectarg(s_word);
	    arg3 = collectarg(s_begin);
	    setnumber(arg1 -> wrd, &n1, 0);
	    setnumber(arg2 -> wrd, &n2, 0);
	    if (currpage >= 0 && n2 != 0 && (pn-1)%n2 == n1-1)
	      { struct node *ft = ftext;
		int ln = linenum, ls = linesskipped, vo = vertoffset;
		ftext = arg3;
		linenum = minline; linesskipped = linesperbody;
		vertoffset = (x -> dval == d_head) ? headposn : footposn; /* title position */
		formatitem();
		ftext = ft; linenum = ln; linesskipped = ls; vertoffset = vo;
	      }
	    formatseq();
	    break;
	  }

	case d_fn:
	  { struct node *arg; int fnh, ln;
	    arg = collectarg(s_word);
	    setnumber(arg -> wrd, &fnh, 0);
	    if (fnh > linesperbody)
	      { fprintf(messout, "Footnote is longer than page body!\n");
		fnh = linesperpage;
	      }
	    ln = linenum%linesperbody;
	    if (fnh > 0 && ln+fnh < linesperbody)
	      { int ls = linesskipped, vo = vertoffset;
		linesskipped = linesperbody;
		vertoffset = topmargin + linesperbody - (ln+fnh);
		formatseq();
		linesskipped = ls;
		vertoffset = vo - fnh;
	      }
	    else formatseq();
	    break;
	  }

	case d_use:
	  { struct node *arg; char *s;
	    arg = collectarg(s_word);
	    s = arg -> wrd;
	    if (stylefn != s) fprintf(messout, "Using format file \"%s\"; ignoring \"<use %s\"\n", stylefn, s);
	    formatseq();
	    break;
	  }
      }
  }

static setfonts(kf, fn, ps) int kf; char *fn; int ps;
  { /* set fonts in normal & index positions */
    int ft;
    ft = lookupfont(fn, ps);
    if (ft >= 0) fonts[kf] = ft;
    if (kf < Bold)
      { ft = lookupfont(fn, (ps*indexmag+500)/1000);
	if (ft >= 0) fonts[INDEX+kf] = ft;
      }
  }

static declare(w, d, b, x) char *w; enum direct d; uint b; struct node *x;
  { if (symtop < MAXSYMS)
      { symtab[symtop].str = lookupword(w);
	symtab[symtop].dval = d;
	symtab[symtop].bval = b;
	symtab[symtop].xval = x;
	symtop++;
      }
    else fprintf(messout, "Too many names!\n");
  }

static bsdir(x) struct symtabent *x;
  { switch (x -> dval)
      { default:
	    fprintf(messout, "bug: in bsdir %s\n", x -> str);
	    break;

	case b_comment:
	    /* comment, skip till bol */
	    until (ftext == NULL || ftext -> typ == s_bol)
	      { if (ftext -> typ == s_begin) ftext = ftext -> slk;
		unless (ftext == NULL) ftext = ftext -> fwd;
	      }
	    break;

	case b_glue:
	    /* stuff a "superglue" char */
	    stuffol(SUPERGLUECH);
	    supergluecount++;
	    break;

	case b_horiz:	case b_vert:
	  { /* horizontal space */
	    struct node *arg; int d, c;
	    arg = collectarg(s_word);
	    setnumber(arg -> wrd, &d, n_signok | n_unitsok);
	    c = tabtranslit(((x -> dval == b_horiz) ? 'h' : 'v'), d);
	    stuffol(c);
	    break;
	  }

	case b_tab:
	    tabulate();
	    break;

	case b_fix:
	    fixglue(); /* "fix" the glue by changing gluech to space */
	    fixindent();
	    break;

	case b_tabfix:
	    tabulate(); fixindent(); /* shorthand */
	    break;

	case b_line:
	  { /* go to a specific line on page */
	    struct node *arg; int n, lip;
	    dobreak(false);
	    lip = linenum%linesperbody;
	    arg = collectarg(s_word);
	    setnumber(arg -> wrd, &n, 0);
	    if (lip > n)
	      { /* new page */
		skiplines(linesperbody-1);
		lip = 0;
	      }
	    linesskipped = 0; /* o'wise skiplines doesn't do anything */
	    skiplines(n-lip);
	    break;
	  }

	case b_need:
	  { /* need so many lines */
	    struct node *arg; int n, lip;
	    lip = linenum%linesperbody;
	    arg = collectarg(s_word);
	    setnumber(arg -> wrd, &n, 0);
	    if (linesperbody-lip < n)
	      { /* new page */
		dobreak(false); skiplines(linesperbody-1);
	      }
	    break;
	  }

	case b_page:
	    dobreak(false); skiplines(linesperbody-1);
	    break;

	case b_arg:
	    if (nextarg != NULL && nextarg -> typ == s_begin)
	      { struct node *ft = ftext;
		ftext = nextarg; nextarg = NULL;
		formatitem();
		nextarg = ftext; ftext = ft;
	      }
	    else fprintf(messout, "Formal/actual mismatch in macro call\n");
	    break;

	case b_tr:
	  { struct node *arg = collectarg(s_word);
	    int c = symtranslit(arg -> wrd);
	    if (c & LIGBIT)
	      { fprintf(messout, "Ligature illegal after \\tr; define font\n");
		c = ERROR_TCHAR;
	      }
	    stuffol(c);
	    break;
	  }

	case b_box:
	  { struct node *arg; int wid, hgt, c;
	    arg = collectarg(s_word);	/* width  */
	    setnumber(arg -> wrd, &wid, n_unitsok);
	    arg = collectarg(s_word);	/* height */
	    setnumber(arg -> wrd, &hgt, n_unitsok);
	    c = boxtranslit(wid, hgt, linethickness);
	    stuffol(c);
	    break;
	  }

	case b_indent:	/* set indent for next line to be output */
	  { struct node *arg = collectarg(s_word);
	    setnumber(arg -> wrd, &nextindent, n_signok | n_rel | n_unitsok);
	    break;
	  }
      }
  }

static tabulate()
  { int k;
    fixglue(); /* "fix" the glue by changing gluech to space */
    /* work out distance to next tab stop (rel. to current indent) */
    k = 0;
    while (k < MAXTABS && tabstops[k] <= linewid) k++;
    if (k < MAXTABS)
      { int dx = tabstops[k] - linewid;
	int c = tabtranslit('h', dx); /* c is char. in translit. font which represents the tab */
	stuffol(c);
      }
  }

static fixindent()
  { /* fix indentation at current position */
    nextindent = indent+linewid;
  }

static struct node *collectarg(sy) enum nodetype sy;
  { struct node *arg = ftext;
    if (ftext == NULL || ftext -> wrd == NULL) badarg();
    if (ftext -> typ != sy) badarg();
    if (sy == s_begin)
      { ftext = ftext -> slk;
	unless (ftext == NULL) ftext = ftext -> fwd;
      }
    else ftext = ftext -> fwd;
    return arg;
  }

static badarg()
  { fprintf(messout, "Illegal/insufficient arguments to \"%s\" directive\n", dirname);
    if (context > 0) _longjmp(abort[context-1]);
    fprintf(messout, "bug: no context to longjump to!\n");
  }

static fmtword(w, nsp) char *w; int nsp;
  { char s[MAXSTRLEN+1];
    int v1[MAXSTRLEN+1], v2[MAXSTRLEN+1];
    addspaces(w, s, nsp);
    decodeescapes(s, v1);
    decodetlits(v1, v2);
    if (flags & f_justify) stuffc(GLUECH, v2);
    fword(v2);
    wordcount++;
  }

static addspaces(w, s, nsp) char *w, *s; int nsp;
  { int k = 0;
    if (flags & f_whitespace)
      { /* add spaces before */
	int i;
	for (i=0; i<nsp; i++) s[k++] = ' ';
      }
    strcpy(&s[k], w);
  }

static decodeescapes(s, v) char *s; int *v;
  { int i = 0, index = 0; int ch;
    v[0] = 0;
    ch = s[i++];
    until (ch == '\0')
      { int kf = Roman, ix = 0;
	bool cf = false;
	while (ch == '\033')
	  { ch = s[i++];
	    if (ch == '^')			kf = Italic;  /* italic font		       */
	    else if (ch == '#')			kf = Bold;    /* bold font		       */
	    else if (ch == '/' || ch == '\\')	kf = Romix;   /* roman index font	       */
	    else if (ch == '\'' || ch == '`')	kf = Italix;  /* italic index font	       */
	    if (ch == '/' || ch == '\'')	ix = -1;      /* superscript		       */
	    else if (ch == '\\' || ch == '`')	ix = 1;	      /* subscript		       */
	    if (ch == '>')			cf = true;    /* continued on next source line */
	    unless (ch == '\0') ch = s[i++];
	  }
	if (ix != index)
	  { stuffindex(ix-index, v);
	    index = ix;
	  }
	unless (ch == '\0')
	  { unless (cf) stuffc(Mkchar(fonts[kf], ch), v);
	    ch = s[i++];
	  }
	continflag |= cf;
      }
    if (index != 0) stuffindex(-index, v);
  }

static stuffindex(dix, v) int dix; int *v;
  { int c = tabtranslit('v', dix*indexspacing);
    stuffc(c, v);
  }

static decodetlits(v1, v2) int *v1, *v2;
  { int i = 1;
    v2[0] = 0;
    while (i <= v1[0])
      { int ch = v1[i] & 0377;
	int lft = v1[i] >> 8;
	int matched = 0;
	if (lft > 0)
	  { if ((vertoffset == headposn || vertoffset == footposn) && (ch == '#' || ch == '%' || ch == '$'))
	      { int pn = (linenum/linesperbody)+firstpage;
		int k = v2[0];
		int j;
		if (ch == '#') stuffpagenum(pn, v2);
		if (ch == '%') stuffpageroman(pn, v2);
		if (ch == '$') stuffpageletter(pn, v2);
		for (j = k+1; j <= v2[0]; j++) v2[j] |= (lft << 8); /* copy font */
		matched = 1;
	      }
	    else if (testbit(tlitbits, ch)) matched = matchtlit(v1, i, v1[0], v2);
	  }
	if (matched == 0)
	  { stuffc(v1[i], v2); /* no match, copy input char */
	    matched = 1;
	  }
	i += matched; /* skip over chars matched */
      }
  }

static stuffpagenum(n, v) int n; int *v;
  { if (n > 0)
      { stuffpagenum(n/10, v);
	stuffc(n%10 + '0', v);
      }
  }

static stuffpageroman(n, v) int n; int *v;
  { static char ctab[] = {  'm',  'd',	'c',  'l',  'x',  'v',	'i' };
    static int	dtab[] = { 1000,  500,	100,   50,   10,    5,	  1 };
    static int	rtab[] = {  900,  450,	 90,   45,    9, 9999, 9999 };
    static int	stab[] = { 9999,  400, 9999,   40, 9999,    4, 9999 };
    int i = 0;
    while (n > 0)
      { while (n >= dtab[i])
	  { stuffc(ctab[i], v);
	    n -= dtab[i];
	  }
	if (n >= rtab[i])
	  { stuffc(ctab[i+2], v);
	    stuffc(ctab[i], v);
	    n -= rtab[i];
	  }
	if (n >= stab[i])
	  { stuffc(ctab[i+1], v);
	    stuffc(ctab[i], v);
	    n -= stab[i];
	  }
	i++;
      }
  }

static stuffpageletter(n, v) int n; int *v;
  { if (n >= 1 && n <= 26) stuffc('a'+(n-1), v); }

#define ismatch(tc,vc) \
    /* There is a match between a template char and an actual (vector) char if:
       -- vc's font is not a translit font, and
       -- tc == vc regardless of font */ \
    (((vc >> 8) > NUMTFONTS) && ((tc & 0377) == (vc & 0377)))

static int matchtlit(v1, p1, p2, v2) int *v1; int p1, p2; int *v2;
  { /* search list of translits, find longest which matches v1[p1..p2], stuff it into v2 */
    int j, maxj;
    int len = 0;
    for (j=0; j<numtlits; j++)
      { int tl = tlittab[j].len;
	if (tl >= len && p1+tl <= p2+1) /* N.B. ``>='' so we find most local one */
	  { char *tv = tlittab[j].tv;
	    int *iv = &v1[p1];
	    until ((*tv == '\0') || !ismatch(*tv, *iv)) { tv++; iv++; }
	    if (*tv == '\0')
	      { len = tl;
		maxj = j;
	      }
	  }
      }
    if (len > 0)
      { int u = tlittab[maxj].u; /* "word2" of longest matching transliteration */
	if (u & LIGBIT)
	  { /* it's a ligature (dynamically bound font); retain the font */
	    int lft = v1[p1] >> 8;
	    bool ok = stuffligature(lft, u, v2);
	    unless (ok)
	      { /* not present in this font; copy input chars unchanged (e.g. "fi" in Courier) */
		int i;
		for (i=0; i<len; i++) stuffc(v1[p1+i], v2);
	      }
	  }
	else stuffc(u, v2);	/* it's a transliteration (statically bound font); use translit font */
      }
    return len;
  }

static bool stuffligature(lft, u, v) int lft, u; int *v;
  { int dx = 0;
    bool any = false; int k, fw;
    for (k=0; k < 3; k++)
      { int c = (u >> 16) & 0xff;
	u <<= 8;
	if (c != '\0' && testbit(fontinfo[lft] -> defd, c))
	  { int cw = fontinfo[lft] -> advance[c].x;
	    if (k == 0) fw = cw; /* width of first char (for centring) */
	    dx += (fw-cw)/2;
	    if (dx != 0)
	      { int hc = tabtranslit('h', dx);
		stuffc(hc, v);
		dx = 0;
	      }
	    stuffc(Mkchar(lft, c), v); /* implicit move right cw millipoints */
	    dx -= (fw+cw)/2;
	    any = true;
	  }
      }
    if (any)
      { dx += fw;
	if (dx != 0)
	  { int hc = tabtranslit('h', dx);
	    stuffc(hc, v);
	  }
      }
    return any;
  }

static fword(v) int *v;
  { /* split at hyphens */
    int p1 = 1;
    until (p1 > v[0])
      { int p2 = p1;
	until (p2 >= v[0] || (v[p2] & 0377) == '-') p2++;
	fmtsyllable(v, p1, p2);
	p1 = p2+1;
      }
  }

static fmtsyllable(v, p1, p2) int *v; int p1, p2;
  { int i; int wid = 0;
    int nc = p2-p1+1;
    if (nc > MAXSTRLEN-oline[0]) nc = MAXSTRLEN-oline[0];
    for (i=0; i<nc; i++)
      { int ft = v[p1+i] >> 8; /* font number */
	int ch = v[p1+i] & 0377;
	wid += fontinfo[ft] -> advance[ch].x;
      }
    if (linewid+wid >= bodywidth-indent-rightindent) dobreak(true);
    for (i=0; i<nc; i++)
      { int x = v[p1+i];
	stuffc(x, oline);
	if (x == GLUECH) gluecount++;
	if (x == SUPERGLUECH) supergluecount++;
      }
    linewid += wid;
  }

static dobreak(just) bool just;
  { if (oline[0] > 0)
      { outputline(just);
	skiplines(linespacing-1);
      }
  }

static outputline(just) bool just;
  { int i;
    if (linenum >= minline && linenum <= maxline)
      { int ind; /* indentation */
	int iws; /* inter-word space */
	deltrailglue();
	if (supergluecount > 0)
	  { fixglue(); /* cvt glue to space */
	    changesuperglue(); /* cvt superglue to "ordinary" glue */
	    ind = leftmargin + indent;
	    iws = mingluewidth + (bodywidth-linewid-indent-rightindent)/gluecount;
	  }
	else if (flags & f_centre)
	  { ind = leftmargin + indent + (bodywidth-linewid-indent-rightindent)/2;
	    iws = mingluewidth;
	  }
	else if (just && (flags & f_justify) && (gluecount >= 1))
	  { ind = leftmargin + indent;
	    iws = mingluewidth + (bodywidth-linewid-indent-rightindent)/gluecount;
	  }
	else
	  { ind = leftmargin + indent;
	    iws = mingluewidth;
	  }
	startline(ind, iws);
	for (i=1; i <= oline[0]; i++)
	  { int ft = oline[i] >> 8;
	    int ch = oline[i] & 0377;
	    unless (ft == currfont || ch == GLUECH)
	      { outproc(o_selfont, ft); /* select font */
		currfont = ft;
	      }
	    outproc(o_char, ch); /* output the character */
	  }
	endline();
      }
    indent = nextindent;
    oline[0] = 0;
    eqnposition = topeqnwidth = boteqnwidth = 0;
    for (i=0; i<3; i++) eqntabs[i] = 0;
    linewid = gluecount = supergluecount = 0;
    linenum++;
    linesskipped = (linenum%linesperbody == 0) ? linesperbody : 0;
  }

static deltrailglue()
  { while (oline[0] > 0 && oline[oline[0]] == GLUECH)
      { oline[0]--;
	gluecount--; linewid -= mingluewidth;
      }
  }

static changesuperglue()
  { /* change superglue to "ordinary" glue */
    int i;
    for (i = 1; i <= oline[0]; i++)
      { if (oline[i] == SUPERGLUECH)
	  { oline[i] = GLUECH;
	    gluecount++;
	  }
      }
    supergluecount = 0;
  }

static fixglue()
  { /* "fix" glue by changing gluech to space */
    int i;
    for (i = 1; i <= oline[0]; i++)
      { if (oline[i] == GLUECH)
	  { int ft = fonts[Roman];
	    oline[i] = Mkchar(ft, ' ');
	    linewid += (fontinfo[ft] -> advance[' '].x - mingluewidth);
	  }
      }
    gluecount = 0;
  }

static skiplines(n) int n;
  { int dn = n-linesskipped;
    if (dn > 0)
      { bool top = false; int i;
	int lip = linenum%linesperbody;
	if (dn > linesperbody-lip) dn = linesperbody-lip;
	for (i=0; i<dn; i++)
	  { if (linenum >= minline && linenum <= maxline)
	      { startline(leftmargin, mingluewidth);
		endline();
	      }
	    linenum++;
	    if (linenum%linesperbody == 0) top = true;
	    linesskipped++;
	  }
	if (top) linesskipped = linesperbody;
      }
  }

static startline(ind, iws) int ind, iws;
  { int k = (linenum%linesperbody) + vertoffset;
    if (k >= 0 && k < linesperpage)
      { if (ind < 0 || iws < 0) fprintf(messout, "Line too long to set! (%d, %d)\n", ind, iws);
	if (ind < 0) ind = 0;
	if (iws < 0) iws = 0;
	outproc(o_boline, k, ind, iws); /* del to eol; specify indent, glue width */
      }
  }

static endline()
  {
#ifdef DEBUG_ENDL
    debug_endl();
#endif
    outproc(o_eoline); /* carriage return */
  }

#ifdef DEBUG_ENDL
static debug_endl()
  { char v[MAXSTRLEN+1]; int k;
    sprintf(v, " [%d+%d]", vertoffset, linenum%linesperbody);
    outproc(o_selfont, 1); currfont = 1;
    k = 0;
    until (v[k] == '\0') outproc(o_char, v[k++]);
  }
#endif

static clearscreen()
  { outproc(o_bopage, firstpage+currpage, currpage+1);
    generation += linesperpage; /* invalidate all gln entries */
  }

static int Mkchar(f, c) int f, c;
  { return (f << 8) + (c & 0377);
  }

static stuffol(c) int c;
  { int ft, ch;
    stuffc(c, oline);
    ft = c >> 8; ch = c & 0377;
    linewid += fontinfo[ft] -> advance[ch].x;
  }

static stuffc(c, v) int c; int v[];
  { if (v[0] < MAXSTRLEN) v[++v[0]] = c;
  }

