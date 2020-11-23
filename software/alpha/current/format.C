/* Alpha document preparation system
   formatting module
   AJF	 December 1987 */

#include <setjmp.h>

#include "alpha.h"

#define MAXCONTEXT   160    /* max. depth of nesting of begin-end brackets		   */
#define MAXSYMS	     100    /* max. symbols (begin brackets, backslash directives, macros) */
#define MAXTABS	     30	    /* max. num. of tab stops					   */

#define LIGYSCALE    250    /* "y" accent displacement in ligatures, millipts per pt of current font */

#define f_centre     0x01   /* <ce in force				*/
#define f_fill	     0x02   /* <fi in force				*/
#define f_justify    0x04   /* <ju in force				*/
#define f_whitespace 0x08   /* <ws in force				*/
#define f_eqn	     0x10   /* <eqn in force				*/
#define f_eqnce	     0x20   /* <eqnce in force				*/
#define f_reformat   0x40   /* begin bracket edited; reformat all below */

#define XREFFILE     "Xrefs"

enum direct
  { /* < > directives */
    d_bit, d_def, d_text, d_null, d_ft, d_ls, d_lth, d_in, d_rin, d_set, d_tabs, d_tr,
    d_head, d_foot, d_fn, d_use, d_style, d_macro,
    /* backslash directives */
    b_comment, b_glue, b_horiz, b_vert, b_tab, b_fix, b_tabfix,
    b_line, b_need, b_skip, b_page, b_arg, b_tr, b_box, b_indent, b_xr,
    /* special wds for eqns */
    w_ebeg, w_eend, w_over, w_above, w_lpar, w_rpar, w_lbar, w_rbar,
    w_lbrac, w_rbrac, w_biglbrac, w_bigrbrac,
    w_lcurl, w_rcurl, w_biglcurl, w_bigrcurl, w_integ, w_sigma,
  };

struct symtabent
  { char *str;
    direct dval;	/* d_ / b_ code		    */
    uint bval;		/* for <ce / <-ce etc. only */
    node *xval;		/* for macros		    */
    int nargs;		/* for macros		    */
  };

struct line
  { void clear() { len = gc = sgc = wid = 0; }
    void stuffc(Char);		/* insert a Char in line */
    void fixglue(int);		/* change glue to space, or superglue to ordinary glue */
    void deltrailglue();	/* delete trailing glue */
    int len;			/* num. of chars in vec */
    int gc, sgc;		/* count of glue chars, superglue chars */
    int wid;			/* width of line, in millipixels */
    Char vec[MAXSTRLEN];
  };

global oproc outproc;		/* used by common */
global int fonts[NUMFP];	/* table of indices into fontinfo for R,I,B fonts (also used by common) */
global int pvpage;		/* used by edit	  */
global bool anotherpass;	/* used by scrout */

static substitution *xrefs, *sets, *tlits;
static jmp_buf abort[MAXCONTEXT];
static symtabent symtab[MAXSYMS];
static int context, symtop, bsbase, dirbase, macrobase;
static int minline, maxline, linenum, linesskipped;
static int vertoffset, linespacing, linethickness;
static int indent, nextindent, rightindent, currpage;
static int wordcount, generation;
static int eqnposition, topeqnwidth, boteqnwidth, eqntabs[3];
static direct eqnstack;
static char *dashword, *dotsword, *ebeginword, *dirname;
static bool xrefchanges, continflag, firstfile;
static uint escapable[FONTSIZE/BPW], flags;
static node *ftext, *firstarg, *nextarg;
static line oline;
static int tabstops[MAXTABS];

static void declsyswords();
static void dopass(node*), setbasicfonts(), formatseq(), fmtseq();
static void formatargument(), formatitem(), formatword();
static void fmteqn(symtabent*), adjusttab(int, int), eqncompound(int, int, int, int), eqnvert(int);
static int eqnback(), eqnright(int);
static void completestack(int), formatwd(), formatbegin(), formatspan(), skipspan(), fmtspan(), blkdir(symtabent*);
static void setfonts(int, char*, int);
static void declare(char*, direct, uint = 0, node* = NULL, int = 0);
static void bsdir(symtabent*), addxref(char*, char*), tabulate(), fixindent();
static node *collectspan();
static char *collectword(bool);
static node *findarg(int);
static char *collectsectname(char*, int&);
static int findsectnum(char*, char*);
static void badarg(int), fmtword(char*, int);
static char *substparams(char*), *substdollars(char*, bool&);
static char *substfor(substitution*, char*, bool&);
static void addspaces(char*, char*, int), decodeescapes(char*, line*), stuffindex(int, line*);
static void decodepagenums(line*, line*, int);
static void stuffpagenum(int, line*), stuffpageroman(int, line*), stuffpageletter(int, line*);
static void decodetlits(line*, line*);
static bool trmatch(Char*, char*, int);
static bool stufftranslit(int, Char*, line*);
static void fword(line*), fmtsyllable(line*, int, int), dobreak(bool), outputline(bool);
static void skiplines(int), startline(int, int), endline(), clearscreen();


global void initfmtbook()
  { /* note codeout, style vars are not set up yet, and messout == stderr */
    outproc = pmode ? ps_outproc : scr_outproc;
    declsyswords();
    dashword = lookupword("-");
    dotsword = lookupword("...");
    ebeginword = lookupword("<");
    for (int i=0; i < FONTSIZE/BPW; i++) escapable[i] = 0;
    char *esc = "<>{}].\\";   /* escapable chars */
    for (int i=0; esc[i] != '\0'; i++)
      { char *esc = "<>{}].\\";   /* escapable chars */
	setbit(escapable, esc[i]);
      }
    initfonts();
    xrefs = new substitution;
    xrefs -> read(XREFFILE);
    xrefchanges = false;
    firstfile = true;
  }

static void declsyswords()
  { symtop = 0;
    /* declare "special words" for equations */
    declare("{{", w_ebeg);               declare("}}", w_eend);
    declare("over", w_over);             declare("above", w_above);
    declare("lpar", w_lpar);             declare("rpar", w_rpar);
    declare("lbar", w_lbar);             declare("rbar", w_rbar);
    declare("lbrac", w_lbrac);           declare("rbrac", w_rbrac);
    declare("Lbrac", w_biglbrac);        declare("Rbrac", w_bigrbrac);
    declare("lcurl", w_lcurl);           declare("rcurl", w_rcurl);
    declare("Lcurl", w_biglcurl);        declare("Rcurl", w_bigrcurl);
    declare("integ", w_integ);           declare("sigma", w_sigma);
    /* declare backslash directives */
    bsbase = symtop;
    declare("\\;", b_comment);           declare("\\g", b_glue);
    declare("\\h", b_horiz);             declare("\\v", b_vert);
    declare("\\t", b_tab);               declare("\\f", b_fix);
    declare("\\tf", b_tabfix);           declare("\\line", b_line);
    declare("\\need", b_need);           declare("\\skip", b_skip);
    declare("\\page", b_page);           declare("\\arg", b_arg);
    declare("\\tr", b_tr);               declare("\\xr", b_xr);
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
    declare("<set", d_set);              declare("<tabs", d_tabs);
    declare("<tr", d_tr);                declare("<head", d_head);
    declare("<foot", d_foot);            declare("<fn", d_fn);
    declare("<use", d_use);              declare("<style", d_style);
    macrobase = symtop;
  }

global void initfmtfile()
  { if (firstfile)
      { /* do book-level initialization now messout, codeout, style vars are set up */
	outproc(o_bobook);
	for (int i=0; i < NUMTFONTS; i++)
	  { int tf = lookupfont("C", 12); /* transliteration font */
	    if (tf < 0) giveup("can't find transliteration font!"); /* give up */
	    fontinfo[tf] -> short_name = NULL; /* zap font name so it won't be found */
	  }
	firstfile = false;
      }
    outproc(o_bofile);
    inittranslit();
    currpage = -1;
    generation = 0;
    symtop = macrobase; /* delete down to "permanent" symbols */
  }

global void tidyfmtfile()
  { tidytranslit();	/* remove temp. files */
    outproc(o_eofile);
  }

global void tidyfmtbook()
  { if (rpt_unusedflag) xrefs -> rpt_unused();
    if (xrefchanges) xrefs -> write(XREFFILE);
    delete xrefs;
    outproc(o_eobook);
  }

global int formatpage(node *x, int pn)
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

static void dopass(node *x)
  { for (int i = 0; i < MAXTABS; i++) tabstops[i] = (i+1)*36000; /* half-inch intervals */
    oline.clear();
    linenum = 0;
    tlits = new substitution;
    sets = new substitution;
    eqnposition = topeqnwidth = boteqnwidth = 0;
    eqnstack = (direct) 0;
    for (int i=0; i<3; i++) eqntabs[i] = -1;
    linesskipped = linesperbody;
    vertoffset = topmargin;
    linespacing = 1;
    linethickness = 0; /* means "use thinnest possible" */
    context = indent = nextindent = rightindent = wordcount = 0;
    firstarg = nextarg = NULL;
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

static void setbasicfonts()
  { for (int k=0; k < NUMFP; k++) fonts[k] = -1;
    setfonts(Roman,  "C",  12);
    setfonts(Italic, "CO", 12);
    setfonts(Bold,   "CB", 12);
    for (int k=0; k < NUMFP; k++)
      unless (k == Symbol || k == Boldix || k == Symbix)
	if (fonts[k] < 0) giveup("can't find basic font (position %d)!", k);
  }

static void formatseq()
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

static void fmtseq()
  { until (ftext == NULL || ftext -> typ == s_end) formatitem();
    dobreak(false);
  }


static void formatargument()
  { if (version >= 4)
      { /* Vsn 4 goes below < > ; Vsn 3 and earlier doesn't */
	if (ftext -> typ == s_begin && ftext -> wrd == ebeginword) ftext = ftext -> fwd;
	until (ftext == NULL || ftext -> typ == s_end) formatitem();
	unless (ftext == NULL) ftext = ftext -> fwd;
      }
    else formatitem();
  }

static void formatitem()
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
	      { if (oline.len > 0) dobreak(false);
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

static void formatword()
  { if (ftext -> edt)
      { if (pvpage < 0) pvpage = linenum/linesperbody;
	ftext -> edt = false;
      }
    if (nextarg != NULL && ftext -> wrd == dotsword)
      { node *ft = ftext, *fa = firstarg;
	ftext = nextarg; firstarg = nextarg = NULL;
	formatseq();
	nextarg = ftext; ftext = ft;
	ftext = ftext -> fwd;
	firstarg = fa;
      }
    else if (flags & f_eqn)
      { char *w = ftext -> wrd;
	int i = 0;
	while (i < bsbase && symtab[i].str != w) i++;
	if (i < bsbase)
	  { ftext = ftext -> fwd;
	    fmteqn(&symtab[i]);
	  }
	else formatwd();
      }
    else formatwd();
  }

static void fmteqn(symtabent *x)
  { switch (x -> dval)
      { default:
	    fprintf(messout, "bug: in fmteqn %s\n", x -> str);
	    break;

	case w_ebeg:	/* {{ */
	    oline.fixglue(GLUECH);
	    eqnposition = oline.wid;
	    eqnstack = (direct) 0;
	    eqnvert(-1);				    /* go up 1 */
	    eqntabs[0] = eqnright(0);			    /* filled in later */
	    break;

	case w_eend:	/* }} */
	  { oline.deltrailglue();
	    oline.fixglue(GLUECH);
	    boteqnwidth = oline.wid - eqnposition;
	    eqnvert(-1);				    /* go up 1 */
	    eqntabs[2] = eqnback();			    /* back to eqnposition */
	    int w = max(topeqnwidth, boteqnwidth);	    /* max width of components */
	    completestack(w);				    /* draw a bar, Sigma, etc. */
	    if (flags & f_justify) oline.stuffc(Char(0, GLUECH));
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
	    break;
	  }

	case w_over:	case w_above:	case w_sigma:
	    /* forms of stacking */
	    oline.deltrailglue();
	    oline.fixglue(GLUECH);
	    eqnstack = x -> dval;
	    topeqnwidth = oline.wid - eqnposition;
	    eqnvert(2);			    /* go down 2 */
	    eqntabs[1] = eqnback();	    /* back to eqnposition */
	    break;

	case w_lpar:	    /* lpar */
	    eqncompound(0346, 0347, 0350, 0);
	    break;

	case w_rpar:	    /* rpar */
	    oline.deltrailglue();
	    eqncompound(0366, 0367, 0370, 0);
	    break;

	case w_lbar:	    /* lbar */
	    eqncompound(0275, 0275, 0275, 0);
	    break;

	case w_rbar:	    /* rbar */
	    oline.deltrailglue();
	    eqncompound(0275, 0275, 0275, 0);
	    break;

	case w_lbrac:	    /* lbrac */
	    eqncompound(0351, 0352, 0353, 0);
	    break;

	case w_rbrac:	    /* rbrac */
	    oline.deltrailglue();
	    eqncompound(0371, 0372, 0373, 0);
	    break;

	case w_biglbrac:    /* Lbrac */
	    eqncompound(0351, 0352, 0353, 0352);
	    break;

	case w_bigrbrac:    /* Rbrac */
	    oline.deltrailglue();
	    eqncompound(0371, 0372, 0373, 0372);
	    break;

	case w_lcurl:	    /* lcurl */
	    eqncompound(0354, 0355, 0356, 0);
	    break;

	case w_rcurl:	    /* rcurl */
	    oline.deltrailglue();
	    eqncompound(0374, 0375, 0376, 0);
	    break;

	case w_biglcurl:    /* Lcurl */
	    eqncompound(0354, 0355, 0356, 0357);
	    break;

	case w_bigrcurl:    /* Rcurl */
	    oline.deltrailglue();
	    eqncompound(0374, 0375, 0376, 0357);
	    break;

	case w_integ:	    /* integ */
	    eqncompound(0363, 0364, 0365, 0);
	    break;
      }
  }

static void adjusttab(int olp, int dx)
  { if (olp >= 0 && olp < oline.len && dx != 0)
      { Char c = oline.vec[olp];
	oline.vec[olp] = tabtranslit('h', c.width() + dx);
	oline.wid += dx;
      }
  }

static void eqncompound(int ctop, int cmid, int cbot, int cext)
  { if (fonts[Symbol] < 0)
      { int ps = fontinfo[fonts[Roman]] -> ptsize;
	setfonts(Symbol, "S", ps);
      }
    if (fonts[Symbol] >= 0)
      { eqnposition = oline.wid;
	if (cext != 0)	/* "tall" */
	  { eqnvert(-2);
	    oline.stuffc(Char(fonts[Symbol], ctop));	    /* top char */
	    eqnback(); eqnvert(1);
	    if (pmode)
	      { oline.stuffc(Char(fonts[Symbol], cext));    /* extn char */
		eqnback();
	      }
	    eqnvert(1);
	    oline.stuffc(Char(fonts[Symbol], cmid));	    /* mid char */
	    eqnback(); eqnvert(1);
	    if (pmode)
	      { oline.stuffc(Char(fonts[Symbol], cext));    /* extn char */
		eqnback();
	      }
	    eqnvert(1);
	    oline.stuffc(Char(fonts[Symbol], cbot));	    /* bot char */
	    eqnvert(-2);
	  }
	else
	  { eqnvert(-1);
	    oline.stuffc(Char(fonts[Symbol], ctop));	    /* top char */
	    eqnback(); eqnvert(1);
	    if (pmode)
	      { oline.stuffc(Char(fonts[Symbol], cmid));    /* mid char */
		eqnback();
	      }
	    eqnvert(1);
	    oline.stuffc(Char(fonts[Symbol], cbot));	    /* bot char */
	    eqnvert(-1);
	  }
      }
  }

static void eqnvert(int ndown)
  { Char c = tabtranslit('v', ndown*vertspacing);   /* move down "ndown" lines */
    oline.stuffc(c);
  }

static int eqnback()
  { return eqnright(eqnposition - oline.wid);	    /* move back to eqnposition */
  }

static int eqnright(int d)
  { Char c = tabtranslit('h', d);
    oline.stuffc(c);
    return oline.len - 1;
  }

static void completestack(int w)
  { switch (eqnstack)
      { case w_sigma:
	  { if (fonts[Symbol] < 0)
	      { int ps = fontinfo[fonts[Roman]] -> ptsize;
		setfonts(Symbol, "S", ps);
	      }
	    if (fonts[Symbol] >= 0)
	      { Char sig = Char(fonts[Symbol], 0345);		    /* Sigma char */
		int sigwid = sig.width();
		if (flags & f_eqnce)
		  { Char c = tabtranslit('h', (w-sigwid)/2);	    /* centre the Sigma */
		    oline.stuffc(c);
		    oline.stuffc(sig);
		    oline.stuffc(c);
		  }
		else
		  { Char c = tabtranslit('h', w-sigwid);
		    oline.stuffc(sig);
		    oline.stuffc(c);
		  }
	      }
	    break;
	  }

	case w_over:
	  { Char en = Char(fonts[Roman], 0261);			    /* en dash */
	    int enwid = en.width();
	    if (w < enwid) w = enwid;
	    while (w > enwid) { oline.stuffc(en); w -= enwid; }
	    if (w < enwid)
	      { Char c = tabtranslit('h', w-enwid);		    /* move left for partial dash */
		oline.stuffc(c);
	      }
	    oline.stuffc(en);
	    break;
	  }

	case w_above:
	  { Char c = tabtranslit('h', w);
	    oline.stuffc(c);
	    break;
	  }
      }
  }

static void formatwd()
  { char *w = ftext -> wrd;
    if (w[0] == '\\')
      { if (testbit(escapable, w[1] & 0377))
	  { fmtword(&w[1], ftext -> spb); /* remove leading backslash */
	    ftext = ftext -> fwd;
	  }
	else if (w[1] >= '0' && w[1] <= '9')	/* \n is shorthand for \skip n */
	  { int n; setnumber(&w[1], n, 0);
	    dobreak(false); skiplines(n);
	    ftext = ftext -> fwd;
	  }
	else
	  { int i = bsbase;
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
    else if (version >= 4 && firstarg != NULL && w[0] == '$' && (w[1] >= '1' && w[1] <= '9') && w[2] == '\0')
      { int an = w[1] - '0';
	node *nx = findarg(an);
	if (nx != NULL && nx -> typ == s_begin)
	  { node *ft = ftext, *fa = firstarg, *na = nextarg;
	    ftext = nx; firstarg = nextarg = NULL;
	    formatargument();
	    ftext = ft -> fwd; firstarg = fa; nextarg = na;
	  }
	else
	  { fmtword(w, ftext -> spb);
	    ftext = ftext -> fwd;
	  }
      }
    else
      { fmtword(w, ftext -> spb);
	ftext = ftext -> fwd;
      }
  }

static void formatbegin()
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
	node *x = ftext;
	skipspan();
	x -> gln = -1;
      }
    flags = f;
  }

static void formatspan()
  { /* re-format span */
    node *x = ftext; int ln = linenum;
    int k = (linenum%linesperbody) + vertoffset;
    fmtspan();
    /* don't record spn for macro defns; this forces them to be evaluated */
    if (firstarg == NULL)
      { x -> spn = linenum-ln;
	x -> skp = linesskipped;
	x -> vof = vertoffset;
	x -> gln = generation + k;
      }
  }

static void skipspan()
  { /* skip span */
    linenum += ftext -> spn;
    linesskipped = ftext -> skp;
    ftext = ftext -> slk;
    unless (ftext == NULL) ftext = ftext -> fwd;
  }

static void fmtspan()
  { char *w = ftext -> wrd;
    int i = symtop-1; /* search symbol table from most recent */
    int in = indent, ni = nextindent;
    while (i >= dirbase && symtab[i].str != w) i--;
    if (i >= macrobase)
      { symtabent *sy = &symtab[i];
	node *fa = firstarg, *na = nextarg, *ft = ftext;
	firstarg = ftext -> fwd;
	nextarg = findarg(sy -> nargs + 1);	/* skip positional actual params */
	ftext = sy -> xval;
	formatargument();
	firstarg = fa; nextarg = na;
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

static void blkdir(symtabent *x)
  { switch (x -> dval)
      { default:
	    fprintf(messout, "bug: in directive %s\n", x -> str);
	    break;

	case d_use:
	    collectword(false); /* skip */
	    formatseq();
	    break;

	case d_style:
	    ftext = ftext -> bwd -> slk;	/* skip */
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
	    char *arg1 = collectword(true);
	    int nargs;
	    if (ftext -> typ == s_word)
	      { char *arg = collectword(true);
		setnumber(arg, nargs, 0);
	      }
	    else nargs = 0;
	    node *arg2 = collectspan();
	    char v[MAXSTRLEN+1]; v[0] = '<'; strcpy(&v[1], arg1);
	    declare(v, d_macro, 0, arg2, nargs);
	    formatseq();
	    symtop = st;
	    break;
	  }

	case d_text:	case d_null:
	    formatseq();
	    break;

	case d_ft:
	  { int oldf[NUMFP];
	    for (int k=0; k < NUMFP; k++) oldf[k] = fonts[k];
	    for (int k = Roman; k <= Bold; k++) /* Roman, Italic, Bold */
	      { char *w = collectword(true);
		unless (w == dashword)
		  { char fn[MAXSTRLEN+1]; int ps;
		    splitfontname(w, fn, ps);
		    if (ps > 0)
		      { setfonts(k, fn, ps);
			if (k == Roman) fonts[Symbol] = -1; /* force re-eval of Symbol font */
		      }
		    else fprintf(messout, "Missing or zero point size\n");
		  }
	      }
	    formatseq();
	    for (int k=0; k < NUMFP; k++) fonts[k] = oldf[k];
	    break;
	  }

	case d_ls:
	  { int ls = linespacing;
	    char *arg = collectword(true);
	    setnumber(arg, linespacing, n_signok | n_rel);
	    formatseq();
	    linespacing = ls;
	    break;
	  }

	case d_lth:
	  { int lth = linethickness;
	    char *arg = collectword(true);
	    setnumber(arg, linethickness, n_signok | n_rel | n_unitsok);
	    formatseq();
	    linethickness = lth;
	    break;
	  }

	case d_in:
	  { /* indentation is saved in fmtspan; no need to save here */
	    char *arg = collectword(true);
	    setnumber(arg, nextindent, n_signok | n_rel | n_unitsok);
	    indent = nextindent;
	    formatseq();
	    break;
	  }

	case d_rin:
	  { int ri = rightindent;
	    char *arg = collectword(true);
	    setnumber(arg, rightindent, n_signok | n_rel | n_unitsok);
	    formatseq();
	    rightindent = ri;
	    break;
	  }

	case d_tabs:
	  { int otabs[MAXTABS];
	    int n = 0;
	    while ((n < MAXTABS) && (ftext != NULL) && (ftext -> typ == s_word) && (ftext -> wrd[0] != '\\'))
	      { otabs[n] = tabstops[n];
		tabstops[n] = (n > 0) ? tabstops[n-1] : 0; /* for relative positioning */
		setnumber(ftext -> wrd, tabstops[n], n_signok | n_rel | n_unitsok);
		if (n > 0 && tabstops[n] <= tabstops[n-1])
		  fprintf(messout, "Tabstop at \"%s\" out of order; ignored\n", ftext -> wrd);
		else n++;
		ftext = ftext -> fwd;
	      }
	    if ((ftext != NULL) && (ftext -> typ == s_word) && (ftext -> wrd[0] != '\\'))
	      fprintf(messout, "Too many tab stops!\n");
	    formatseq();
	    for (int i=0; i<n; i++) tabstops[i] = otabs[i];
	    break;
	  }

	case d_tr:	case d_set:
	  { substitution *subst = (x -> dval == d_tr) ? tlits : sets;
	    int num = subst -> num;
	    int tk; uint tb;
	    char *w1 = collectword(false);	/* no substs here! */
	    char *w2 = (ftext != NULL && ftext -> typ == s_word) ? collectword(true) : NULL;
	    if (w1[0] == '\\') w1 = lookupword(&w1[1]); /* remove leading backslash if any */
	    if (w1[0] != '\0')
	      { int fc = w1[0] & 0377;
		tk = fc/BPW; tb = subst -> bits[tk];
	      }
	    if (x -> dval == d_tr)
	      { Char *sub = (w2 == NULL) ? NULL : symtranslit(w2);
		subst -> add(w1, sub);	    /* 2nd arg is Char* */
	      }
	    else subst -> add(w1, w2);	    /* 2nd arg is char* */
	    formatseq();
	    if (w1[0] != '\0') subst -> bits[tk] = tb;
	    subst -> num = num;
	    break;
	  }

	case d_head:	case d_foot:
	  { int pn = (minline/linesperbody)+firstpage;
	    char *arg1 = collectword(true);
	    char *arg2 = collectword(true);
	    node *arg3 = collectspan();
	    int n1; setnumber(arg1, n1, 0);
	    int n2; setnumber(arg2, n2, 0);
	    if (currpage >= 0 && n2 != 0 && (pn-1)%n2 == n1-1)
	      { node *ft = ftext;
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
	  { char *arg = collectword(true);
	    int fnh; setnumber(arg, fnh, 0);
	    if (fnh > linesperbody)
	      { fprintf(messout, "Footnote is longer than page body!\n");
		fnh = linesperpage;
	      }
	    int ln = linenum%linesperbody;
	    if (fnh > 0 && ln+fnh < linesperbody)
	      { int ls = linesskipped, vo = vertoffset;
		linesskipped = linesperbody;
		vertoffset = topmargin + linesperbody - (ln+fnh);
		formatseq();
		linesskipped = ls;
		vertoffset = vo-fnh;
	      }
	    else formatseq();
	    break;
	  }
      }
  }

static void setfonts(int kf, char *fn, int ps)
  { /* set fonts in normal & index positions */
    int ft = lookupfont(fn, ps);
    if (ft >= 0) fonts[kf] = ft;
    if (kf < Bold)
      { ft = lookupfont(fn, (ps*indexmag+500)/1000);
	if (ft >= 0) fonts[INDEX+kf] = ft;
      }
  }

static void declare(char *w, direct d, uint b, node *x, int na)
  { if (symtop < MAXSYMS)
      { symtabent *sy = &symtab[symtop++];
	sy -> str = lookupword(w);
	sy -> dval = d;
	sy -> bval = b;
	sy -> xval = x;
	sy -> nargs = na;
      }
    else fprintf(messout, "Too many names!\n");
  }

static void bsdir(symtabent *x)
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
	    oline.stuffc(Char(0, SUPERGLUECH));
	    break;

	case b_horiz:	case b_vert:
	  { /* horizontal space */
	    char *arg = collectword(true);
	    int d; setnumber(arg, d, n_signok | n_unitsok);
	    Char c = tabtranslit(((x -> dval == b_horiz) ? 'h' : 'v'), d);
	    oline.stuffc(c);
	    break;
	  }

	case b_tab:
	    tabulate();
	    break;

	case b_fix:
	    oline.fixglue(GLUECH); /* "fix" the glue by changing gluech to space */
	    fixindent();
	    break;

	case b_tabfix:
	    tabulate(); fixindent(); /* shorthand */
	    break;

	case b_line:
	  { /* go to a specific line on page */
	    dobreak(false);
	    int lip = linenum%linesperbody;
	    char *arg = collectword(true);
	    int n; setnumber(arg, n, 0);
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
	    int lip = linenum%linesperbody;
	    char *arg = collectword(true);
	    int n; setnumber(arg, n, 0);
	    if (linesperbody-lip < n)
	      { /* new page */
		dobreak(false); skiplines(linesperbody-1);
	      }
	    break;
	  }

	case b_skip:
	  { /* skip so many lines */
	    char *arg = collectword(true);
	    int n; setnumber(arg, n, 0);
	    dobreak(false); skiplines(n);
	    break;
	  }

	case b_page:
	    dobreak(false); skiplines(linesperbody-1);
	    break;

	case b_arg:
	    if (nextarg != NULL && nextarg -> typ == s_begin)
	      { node *ft = ftext, *fa = firstarg;
		ftext = nextarg; firstarg = nextarg = NULL;
		formatargument();
		firstarg = fa; nextarg = ftext; ftext = ft;
	      }
	    else fprintf(messout, "Formal/actual mismatch in macro call\n");
	    break;

	case b_tr:
	  { char *arg = collectword(true);
	    Char *sub = symtranslit(arg);
	    unless (sub == NULL) stufftranslit(fonts[Roman], sub, &oline);
	    break;
	  }

	case b_box:
	  { char *arg; int wid, hgt;
	    arg = collectword(true);	    /* width  */
	    setnumber(arg, wid, n_unitsok);
	    arg = collectword(true);	    /* height */
	    setnumber(arg, hgt, n_unitsok);
	    Char c = boxtranslit(wid, hgt, linethickness);
	    oline.stuffc(c);
	    break;
	  }

	case b_indent:	/* set indent for next line to be output */
	  { char *arg = collectword(true);
	    setnumber(arg, nextindent, n_signok | n_rel | n_unitsok);
	    break;
	  }

	case b_xr:
	  { char *w1 = collectword(false);	/* no substs here! */
	    char *w2 = collectword(true);
	    addxref(w1, w2);
	    break;
	  }
      }
  }

static void addxref(char *w1, char *w2)
  { tlit *t = xrefs -> lookup(w1);
    if (t != NULL)
      { if (t -> sub != w2)
	  { fprintf(messout, "Redefining `%s', was `%s', now `%s'\n", w1, t -> sub, w2);
	    t -> sub = w2;
	    xrefchanges = anotherpass = true;
	  }
      }
    else
      { fprintf(messout, "Defining new `%s' as `%s'\n", w1, w2);
	xrefs -> add(w1, w2);
	xrefchanges = anotherpass = true;
      }
  }

static void tabulate()
  { oline.fixglue(GLUECH); /* "fix" the glue by changing gluech to space */
    /* work out distance to next tab stop (rel. to current indent) */
    int k = 0;
    while (k < MAXTABS && tabstops[k] <= oline.wid) k++;
    if (k < MAXTABS)
      { int dx = tabstops[k] - oline.wid;
	Char c = tabtranslit('h', dx); /* c is char. in translit. font which represents the tab */
	oline.stuffc(c);
      }
  }

static void fixindent()
  { /* fix indentation at current position */
    nextindent = indent + oline.wid;
  }

static node *collectspan()
  { node *arg = ftext;
    if (ftext == NULL || ftext -> wrd == NULL || ftext -> typ != s_begin) badarg(1);
    ftext = ftext -> slk;
    unless (ftext == NULL) ftext = ftext -> fwd;
    return arg;
  }

static char *collectword(bool sub)
  { node *arg = ftext;
    if (ftext == NULL || ftext -> wrd == NULL || ftext -> typ != s_word) badarg(2);
    ftext = ftext -> fwd;
    char *s = arg -> wrd;
    if (sub && version >= 4) s = substparams(s);
    return s;
  }

static node *findarg(int n)
  { node *arg = firstarg; int k = 1;
    while (k < n && arg != NULL)
      { if (arg -> typ == s_begin) arg = arg -> slk;
	unless (arg == NULL) arg = arg -> fwd;
	k++;
      }
    return (k >= n) ? arg : NULL;
  }

static char *collectsectname(char *s, int &j)
  { char v[MAXSTRLEN+1]; int k = 0;
    v[k++] = '<';
    until (s[j] == '/' || s[j] == '}' || s[j] == '\0') v[k++] = s[j++];
    if (s[j] == '/') j++;
    v[k] = '\0';
    return lookupword(v);
  }

static int findsectnum(char *s1, char *s2)
  { /* follow path to root */
    node *x = (firstarg != NULL) ? firstarg : ftext;	/* use actual in preference to formal */
    int n = 0;
    until (x == NULL || x -> wrd == s2)
      { if (x -> wrd == s1) n++;
	/* skip identical "s1" spans; need to look inside others */
	x = (x -> typ == s_end && x -> slk -> wrd == s1) ? (x -> slk) :
	    isbrac(x) ? (x -> xbwd) : (x -> bwd);
      }
    if (n == 0 || x == NULL) fprintf(messout, "Can't find %s in %s\n", s1, s2);
    return n;
  }

static void badarg(int code)
  { fprintf(messout, "Illegal/insufficient arguments to \"%s\" directive (%d)\n", dirname, code);
    if (context > 0) _longjmp(abort[context-1], 0);
    fprintf(messout, "bug: no context to longjump to!\n");
  }

static void fmtword(char *w, int nsp)
  { char s[MAXSTRLEN+1]; line v1, v2;
    if (version >= 4) w = substparams(w);
    addspaces(w, s, nsp);
    decodeescapes(s, &v1);
    if (vertoffset == headposn || vertoffset == footposn)
      { line vx;
	decodepagenums(&v1, &vx, (linenum/linesperbody) + firstpage);
	decodetlits(&vx, &v2);
      }
    else decodetlits(&v1, &v2);
    if (flags & f_justify) v2.stuffc(Char(0, GLUECH));
    fword(&v2);
    wordcount++;
  }

static char *substparams(char *s)
  { int np = 0; bool any = true;
    while (np++ < 10 && any)
      { any = false;
	if (firstarg != NULL) s = substdollars(s, any); /* $n, ${S2/S2}	    */
	s = substfor(xrefs, s, any);			/* cross-references */
	s = substfor(sets, s, any);			/* local <set	    */
      }
    if (any) fprintf(messout, "Too many substs in `%s'!\n", s);
    return s;
  }

static char *substdollars(char *s, bool &any)
  { char v[MAXSTRLEN+1]; int j = 0, k = 0;
    until (s[j] == '\0')
      { if (s[j] == '$' && s[j+1] >= '1' && s[j+1] <= '9')
	  { int an = s[j+1] - '0';
	    node *nx = findarg(an);
	    if (nx == NULL || nx -> typ != s_word || nx -> wrd == NULL)
	      { fprintf(messout, "Ilgl reference to $%d in `%s'\n", an, s);
		return s;
	      }
	    int sxlen = strlen(nx -> wrd);
	    if (k+sxlen > MAXSTRLEN)
	      { fprintf(messout, "Subst for $%d in `%s' is too long!\n", an, s);
		return s;
	      }
	    strcpy(&v[k], nx -> wrd);
	    j += 2; k += sxlen; any = true;
	  }
	else if (s[j] == '$' && s[j+1] == '{')
	  { j += 2;
	    char *s1 = collectsectname(s, j);
	    char *s2 = collectsectname(s, j);
	    until (s[j] == '}' || s[j] == '\0') j++;
	    if (s[j] == '}') j++;
	    int sn = findsectnum(s1, s2);
	    if (sn > 9999 || k+4 > MAXSTRLEN)
	      { fprintf(messout, "Subst for ${%s/%s} in `%s' is too long!\n", &s1[1], &s2[1], s);
		return s;
	      }
	    sprintf(&v[k], "%d", sn);
	    until (v[k] == '\0') k++;
	    any = true;
	  }
	else v[k++] = s[j++];
      }
    if (any)
      { v[k] = '\0';
	s = lookupword(v);
      }
    return s;
  }

static char *substfor(substitution *subst, char *s, bool &any)
  { char v[MAXSTRLEN+1];
    subst -> match(s, v, any);
    if (any) s = lookupword(v);
    return s;
  }

static void addspaces(char *w, char *s, int nsp)
  { int k = 0;
    if (flags & f_whitespace)
      { /* add spaces before */
	for (int i = 0; i < nsp; i++) s[k++] = ' ';
      }
    strcpy(&s[k], w);
  }

static void decodeescapes(char *s, line *v)
  { int i = 0, index = 0;
    v -> clear();
    int ch = s[i++];
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
	    else if (ch == '\\' || ch == '`')	ix = +1;      /* subscript		       */
	    if (ch == '>')			cf = true;    /* continued on next source line */
	    unless (ch == '\0') ch = s[i++];
	  }
	if (ix != index)
	  { stuffindex(ix-index, v);
	    index = ix;
	  }
	unless (ch == '\0')
	  { unless (cf) v -> stuffc(Char(fonts[kf], ch));
	    ch = s[i++];
	  }
	continflag |= cf;
      }
    if (index != 0) stuffindex(-index, v);
  }

static void stuffindex(int dix, line *v)
  { Char c = tabtranslit('v', dix*indexspacing);
    v -> stuffc(c);
  }

static void decodepagenums(line *v1, line *v2, int pn)
  { v2 -> clear();
    for (int i = 0; i < v1 -> len; i++)
      { Char c = v1 -> vec[i];
	if (c.ft > NUMTFONTS && (c.ch == '#' || c.ch == '%' || c.ch == '$'))
	  { int k = v2 -> len;
	    if (c.ch == '#') stuffpagenum(pn, v2);
	    if (c.ch == '%') stuffpageroman(pn, v2);
	    if (c.ch == '$') stuffpageletter(pn, v2);
	    for (int j = k; j < v2 -> len; j++) v2 -> vec[j].ft = c.ft; /* copy font */
	  }
	else v2 -> stuffc(c);
      }
  }

static void stuffpagenum(int n, line *v)
  { if (n > 0)
      { stuffpagenum(n/10, v);
	int c = n%10 + '0';
	v -> stuffc(Char(0, c));
      }
  }

static void stuffpageroman(int n, line *v)
  { static char ctab[] = {  'm',  'd',	'c',  'l',  'x',  'v',	'i' };
    static int	dtab[] = { 1000,  500,	100,   50,   10,    5,	  1 };
    static int	rtab[] = {  900,  450,	 90,   45,    9, 9999, 9999 };
    static int	stab[] = { 9999,  400, 9999,   40, 9999,    4, 9999 };
    int i = 0;
    while (n > 0)
      { while (n >= dtab[i])
	  { v -> stuffc(Char(0, ctab[i]));
	    n -= dtab[i];
	  }
	if (n >= rtab[i])
	  { v -> stuffc(Char(0, ctab[i+2]));
	    v -> stuffc(Char(0, ctab[i]));
	    n -= rtab[i];
	  }
	if (n >= stab[i])
	  { v -> stuffc(Char(0, ctab[i+1]));
	    v -> stuffc(Char(0, ctab[i]));
	    n -= stab[i];
	  }
	i++;
      }
  }

static void stuffpageletter(int n, line *v)
  { if (n >= 1 && n <= 26)
      { int c = 'a' + (n-1);
	v -> stuffc(Char(0, c));
      }
  }

static void decodetlits(line *v1, line *v2)
  { int p1 = 0;
    v1 -> vec[v1 -> len] = Char(0, 0);
    v2 -> clear();
    while (p1 < v1 -> len)
      { if (v1 -> vec[p1].ft > NUMTFONTS && testbit(tlits -> bits, v1 -> vec[p1].ch))
	  { int maxj; int len = 0;
	    for (int j = 0; j < tlits -> num; j++)
	      { tlit *t = &tlits -> tab[j];
		if (t -> len >= len && trmatch(&v1 -> vec[p1], t -> tv, t -> len))
		  { /* N.B. ``>='' so we find most local one */
		    len = t -> len; maxj = j;
		  }
	      }
	    if (len > 0)
	      { tlit *t = &tlits -> tab[maxj];
		if (t -> sub == NULL)
		  { /* no subst defined; copy input chars unchanged */
		    for (int i = 0; i < len; i++) v2 -> stuffc(v1 -> vec[p1++]);
		  }
		else
		  { bool ok = stufftranslit(v1 -> vec[p1].ft, (Char*) t -> sub, v2);
		    if (ok) p1 += len;	/* skip past input chars */
		    else
		      { /* not present in this font; copy input chars unchanged (e.g. "fi" in Courier) */
			for (int i = 0; i < len; i++) v2 -> stuffc(v1 -> vec[p1++]);
		      }
		  }
	      }
	    else v2 -> stuffc(v1 -> vec[p1++]);
	  }
	else v2 -> stuffc(v1 -> vec[p1++]);
      }
  }

static bool trmatch(Char *iv, char *tv, int len)
  { for (int k = 0; k < len; k++)
      { unless (iv[k].ft > NUMTFONTS && iv[k].ch == tv[k]) return false;
      }
    return true;
  }

static bool stufftranslit(int cft, Char *u, line *v)
  { int dx = 0;
    bool any = false; int fw;
    for (int k = 0; u[k].ch != '\0'; k++)
      { int c = u[k].ch & 0xff;
	if (c == LIGUPCH || c == LIGDNCH)
	  { int ps = fontinfo[fonts[Roman]] -> ptsize;
	    int dy = (c == LIGDNCH) ? + (ps*LIGYSCALE) : - (ps*LIGYSCALE);
	    Char cv = tabtranslit('v', dy);
	    v -> stuffc(cv);
	  }
	else
	  { int ft = u[k].ft;
	    if (ft < 0) ft = cft;	/* copy current font */
	    else if (fontinfo[ft] -> ptsize < 0)
	      { /* copy current font size */
		ft = lookupfont(fontinfo[ft] -> short_name, fontinfo[cft] -> ptsize);
		if (ft < 0) ft = cft;
	      }
	    if (ft < NUMTFONTS || testbit(fontinfo[ft] -> defd, c))
	      { Char lc = Char(ft, c);
		int cw = lc.width();
		if (!any) fw = cw;  /* width of first char (for centring) */
		dx += (fw-cw)/2;
		if (dx != 0)
		  { Char hc = tabtranslit('h', dx);
		    v -> stuffc(hc);
		    dx = 0;
		  }
		v -> stuffc(lc); /* implicit move right cw millipoints */
		dx -= (fw+cw)/2;
		any = true;
	      }
	  }
      }
    if (any)
      { dx += fw;
	if (dx != 0)
	  { Char hc = tabtranslit('h', dx);
	    v -> stuffc(hc);
	  }
      }
    return any;
  }

inline bool ishyphen(Char ch)
  { /* don't hyphenate at any old hyphen! */
    return (ch.ch == '-') && (ch.ft == fonts[Roman] || ch.ft == fonts[Italic] || ch.ft == fonts[Bold]);
  }

static void fword(line *v)
  { /* split at hyphens */
    int p1 = 0;
    until (p1 >= v -> len)
      { int p2 = p1;
	until ((p2 >= v -> len - 1) || ishyphen(v -> vec[p2])) p2++;
	fmtsyllable(v, p1, p2);
	p1 = p2+1;
      }
  }

static void fmtsyllable(line *v, int p1, int p2)
  { int wid = 0;
    int nc = p2-p1+1;
    if (nc > MAXSTRLEN - oline.len) nc = MAXSTRLEN - oline.len;
    for (int i=0; i < nc; i++)
      { Char c = v -> vec[p1+i];
	wid += c.width();
      }
    if (oline.wid + wid >= bodywidth-indent-rightindent) dobreak(true);
    for (int i=0; i < nc; i++)
      { Char c = v -> vec[p1+i];
	oline.stuffc(c);
      }
  }

static void dobreak(bool just)
  { if (oline.len > 0)
      { outputline(just);
	skiplines(linespacing-1);
      }
  }

static void outputline(bool just)
  { if (linenum >= minline && linenum <= maxline)
      { int ind; /* indentation */
	int iws; /* inter-word space */
	oline.deltrailglue();
	if (oline.sgc > 0)
	  { /* oline contains superglue chars */
	    oline.fixglue(GLUECH);	/* cvt glue to space */
	    oline.fixglue(SUPERGLUECH); /* cvt superglue to "ordinary" glue */
	    ind = leftmargin + indent;
	    iws = mingluewidth + (bodywidth - oline.wid - indent - rightindent) / oline.gc;
	  }
	else if (flags & f_centre)
	  { ind = leftmargin + indent + (bodywidth - oline.wid - indent - rightindent)/2;
	    iws = mingluewidth;
	  }
	else if (just && (flags & f_justify) && (oline.gc > 0))
	  { ind = leftmargin + indent;
	    iws = mingluewidth + (bodywidth - oline.wid - indent - rightindent) / oline.gc;
	  }
	else
	  { ind = leftmargin + indent;
	    iws = mingluewidth;
	  }
	int currfont = -1;
	startline(ind, iws);
	for (int i = 0; i < oline.len; i++)
	  { Char c = oline.vec[i];
	    unless (c.ft == currfont)
	      { outproc(o_selfont, c.ft); /* select font */
		currfont = c.ft;
	      }
	    outproc(o_char, c.ch); /* output the character */
	  }
	endline();
      }
    indent = nextindent;
    oline.clear();
    eqnposition = topeqnwidth = boteqnwidth = 0;
    eqnstack = (direct) 0;
    for (int i=0; i < 3; i++) eqntabs[i] = -1;
    linenum++;
    linesskipped = (linenum%linesperbody == 0) ? linesperbody : 0;
  }

static void skiplines(int n)
  { int dn = n-linesskipped;
    if (dn > 0)
      { bool top = false;
	int lip = linenum%linesperbody;
	if (dn > linesperbody-lip) dn = linesperbody-lip;
	for (int i=0; i < dn; i++)
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

static void startline(int ind, int iws)
  { int k = (linenum%linesperbody) + vertoffset;
    if (k >= 0 && k < linesperpage)
      { if (ind < 0 || iws < 0) fprintf(messout, "Line too long to set! (%d, %d)\n", ind, iws);
	if (ind < 0) ind = 0;
	if (iws < 0) iws = 0;
	outproc(o_boline, k, ind, iws); /* del to eol; specify indent, glue width */
      }
  }

static void endline()
  { outproc(o_eoline); /* carriage return */
  }

static void clearscreen()
  { outproc(o_bopage, firstpage+currpage, currpage+1);
    generation += linesperpage; /* invalidate all gln entries */
  }

/*** operations on lines ***/

void line::stuffc(Char c)
  { if (len < MAXSTRLEN)
      { vec[len++] = c;
	if (c.ch == GLUECH) gc++;
	if (c.ch == SUPERGLUECH) sgc++;
	wid += c.width();
      }
  }

void line::fixglue(int code)
  { switch (code)
      { case GLUECH:
	  { /* "fix" glue by changing gluech to space */
	    for (int i = 0; i < len && gc > 0; i++)
	      { if (vec[i].ch == GLUECH)
		  { Char sp = Char(fonts[Roman], ' ');
		    vec[i] = sp;
		    wid += (sp.width() - mingluewidth);
		    gc--;
		  }
	      }
	    break;
	  }

	case SUPERGLUECH:
	  { /* change superglue to "ordinary" glue */
	    for (int i = 0; i < len && sgc > 0; i++)
	      { if (vec[i].ch == SUPERGLUECH)
		  { vec[i].ch = GLUECH;
		    gc++; sgc--;
		  }
	      }
	    break;
	  }
      }
  }

void line::deltrailglue()
  { while (len > 0 && vec[len-1].ch == GLUECH)
      { len--; gc--;
	wid -= mingluewidth;
      }
  }

