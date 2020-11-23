/* Alpha document preparation system
   readstyle - deduce format from "<use" line, and read & check format file
   AJF	 December 1987 */

#include "alpha.h"
#include <stdio.h>
#include <errno.h>

#define PREFIX	   "/usr/fisher/print"  /* subst. for ~ */
#define TEMPLATE   "~/century.hdr"      /* if file doesn't exists, it is copied from here */
#define DFLTFMT	   "~/a4-11on14.fmt"    /* suggested <use format */

/* style variables, defined in style file */
global int pagewidth, pageheight, ps_tlx, ps_tly, bodywidth, leftmargin, mingluewidth,
	   linesperpage, linesperbody, topmargin, headposn, footposn,
	   vertspacing, indexspacing, indexmag;

global char *stylefn;		/* used by format */

extern char *lookupword();	/* from common	  */

forward char *expandfn();


global readstyle(fn) char *fn;
  { deduceformat(fn);	    /* sets stylefn */
    readformat();
    checkformat();
  }

static deduceformat(fn) char *fn;
  { /* look for "<use" line */
    if (fn != NULL)
      { int nc;
	FILE *fi = fopen(fn, "r");
	if (fi == NULL && errno == ENOENT)
	  { /* file doesn't exist; try to create it */
	    fprintf(stderr, "alpha: new file \"%s\"\n", fn);
	    copytemplate(fn);
	    fi = fopen(fn, "r");
	  }
	if (fi == NULL) a_giveup("can't open \"%s\"", fn);
	deducefmt(fi, &nc);
	fclose(fi);
      }
    else
      { int nc;
	deducefmt(stdin, &nc);
	/* attempt to rewind stdin -- won't work if we've read more than a bufferful */
	unless (stdin -> _base + nc == stdin -> _ptr)
	  a_giveup("sorry, lookahead failed; insert \"<use\" directive at top of file");
	stdin -> _ptr -= nc;
	stdin -> _cnt += nc;
      }
    if (stylefn == NULL) a_giveup("format is undefined; suggest \"<use %s\" at top of file", DFLTFMT);
  }

static deducefmt(f, nc) FILE *f; int *nc;
  { char *tab = "<use ";
    int ch, i;
    *nc = 0;
    do { ch = getc(f); (*nc)++; } until (ch == tab[0] || ch == EOF);
    i = 0;
    while (i<5 && ch == tab[i]) { ch = getc(f); (*nc)++; i++; }
    if (i >= 5)
      { char v[MAXSTRLEN+1]; int n = 0;
	while (ch == ' ' || ch == '\t') { ch = getc(f); (*nc)++; }
	until (ch == '\n' || ch == EOF || n >= MAXSTRLEN)
	  { v[n++] = ch;
	    ch = getc(f); (*nc)++;
	  }
	v[n++] = '\0';
	stylefn = lookupword(v);
      }
    else stylefn = NULL;
  }

static copytemplate(ofn) char *ofn;
  { FILE *ifi, *ofi; int ch;
    char *ifn = expandfn(TEMPLATE);
    ifi = fopen(ifn, "r");
    if (ifi == NULL) a_giveup("can't open \"%s\"", ifn);
    ofi = fopen(ofn, "w");
    if (ofi == NULL) a_giveup("can't create \"%s\"", ofn);
    ch = getc(ifi);
    until (ch < 0)
      { putc(ch, ofi);
	ch = getc(ifi);
      }
    fclose(ifi); fclose(ofi);
  }

static readformat()
  { char *fn; FILE *fi; int vn;
    fn = expandfn(stylefn);
    fi = fopen(fn, "r");
    if (fi == NULL) a_giveup("can't open \"%s\"", fn);
    vn = getformat(fi); /* version number */
    unless (vn == 2 || vn == 3)
      { fprintf(stderr, "alpha: format file \"%s\" has illegal version number:\n", fn);
	fprintf(stderr, "File is vsn %d; alpha expects vsn 2 or 3.\n", vn);
	exit(1);
      }
    pagewidth = getformat(fi);	    /* ( 1) width of page, = width of window on screen	      */
    pageheight = getformat(fi);	    /* ( 2) height of ditto				      */
    if (vn == 3)
      { ps_tlx = getformat(fi);	    /* ( 3) x-coord of top left of paper (PostScript origin)  */
	ps_tly = getformat(fi);	    /* ( 4) y-coord of top left of paper (PostScript origin)  */
      }
    else
      { ps_tlx = 0;		    /* dflt values for Vsn 2 formats			      */
	ps_tly = 819840;
      }
    bodywidth = getformat(fi);	    /* ( 5) body width (measure, width of text)		      */
    leftmargin = getformat(fi);	    /* ( 6) left margin (paper edge to text)		      */
    mingluewidth = getformat(fi);   /* ( 7) minimum width of glue, stretched by justification */
    linesperpage = getformat(fi);   /* ( 8) num. of lines per page			      */
    linesperbody = getformat(fi);   /* ( 9) num. of lines per body (text proper)	      */
    topmargin = getformat(fi);	    /* (10) top margin (lines from page top to text top)      */
    headposn = getformat(fi);	    /* (11) lines from page top to header		      */
    footposn = getformat(fi);	    /* (12) lines from page top to footer		      */
    vertspacing = getformat(fi);    /* (13) spacing between lines			      */
    indexspacing = getformat(fi);   /* (14) index spacing (superscripts & subscripts)	      */
    indexmag = getformat(fi);	    /* (15) index magnification				      */
    checkeof(fi); fclose(fi);
  }

static int getformat(f) FILE *f;
  { char w[MAXSTRLEN+1];
    int n = 0, nc = 0;
    bool ok;
    int ch = getc(f);
    until (ch == EOF || (ch >= '0' && ch <= '9')) ch = getc(f);
    if (ch == EOF) a_giveup("premature eof in format file");
    until (nc >= MAXSTRLEN || ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF)
      { w[nc++] = ch;
	ch = getc(f);
      }
    w[nc++] = '\0';
    ok = setnumber(w, &n, n_upciok); /* only units allowed in format file are "upci"! */
    unless (ok) a_giveup("error in format file; giving up!");
    until (ch == '\n' || ch == EOF) ch = getc(f);
    return n;
  }

static char *expandfn(w) char *w;
  { char v[MAXSTRLEN+1];
    int i = 0, n = 0;
    until (w[i] == '\0' || n >= MAXSTRLEN)
      { if (w[i] == '~' && n+strlen(PREFIX) <= MAXSTRLEN)
	  { strcpy(&v[n], PREFIX);
	    i++; n += strlen(PREFIX);
	  }
	else v[n++] = w[i++];
      }
    v[n++] = '\0';
    return lookupword(v);
  }

static checkeof(f) FILE *f;
  { int ch = getc(f);
    while (ch == ' ' || ch == '\t' || ch == '\n') ch = getc(f);
    unless (ch == EOF) a_giveup("too many items in format file");
  }

static checkformat()
  { ckfmt(linesperpage, 1, MAXINT, "lines per page");
    ckfmt(linesperbody, 1, linesperpage-topmargin, "lines per body");
    ckfmt(headposn, 0, linesperpage-1, "header position");
    ckfmt(footposn, 0, linesperpage-1, "footer position");
  }

static ckfmt(x, xmin, xmax, s) int x, xmin, xmax; char *s;
  { if (x < xmin || x > xmax)
      a_giveup("error in format file: %s has illegal value", s);
  }

