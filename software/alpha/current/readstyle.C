/* Alpha document preparation system
   readstyle - deduce format from "<use" line, and read & check format file
   AJF	 December 1987 */

#include "alpha.h"

#define PREFIX	   "/usr/fisher/print"          /* subst. for ~ */
#define SUGGFMT	   "~/a4-11on14.fmt"            /* suggested <use format */

/* style variables, defined in style file */
global int version, pagewidth, pageheight, ps_tlx, ps_tly, bodywidth, leftmargin, mingluewidth,
	   linesperpage, linesperbody, topmargin, headposn, footposn,
	   vertspacing, indexspacing, indexmag;

static void readformat(char*, int);
static bool starts(char*, char*), readline(FILE*, char*);
static void readfile(FILE*);
static int getformat(FILE*);
static char *expandfn(char*);
static void checkformat();
static void ckfmt(int, int, int, char*);


global void readstyle(char *fn)
  { readformat(fn, 0);
    checkformat();
  }

static void readformat(char *fn, int dep)
  { if (dep > 4) giveup("<include nested too deeply!");
    FILE *fi = fopen(fn, "r");
    if (fi == NULL) giveup("can't open \"%s\"", fn);
    char line[MAXSTRLEN+1]; bool got = false;
    bool ok = readline(fi, line);
    until (!ok || got)
      { if (starts(line, "<include "))
	  { readformat(&line[9], dep+1);    /* recurse */
	    got = true;
	  }
	else if (starts(line, "<use "))
	  { char *sfn = &line[5];
	    FILE *sfi = fopen(expandfn(sfn), "r");
	    if (sfi == NULL) giveup("can't open \"%s\"", sfn);
	    readfile(sfi);
	    fclose(sfi);
	    got = true;
	  }
	else if (seq(line, "<style"))
	  { readfile(fi);
	    got = true;
	  }
	ok = readline(fi, line);
      }
    fclose(fi);
    unless (got) giveup("format is undefined; suggest \"<use %s\" at top of file", SUGGFMT);
  }

static bool starts(char *s1, char *s2)
  { int n1 = strlen(s1), n2 = strlen(s2);
    return (n1 >= n2 && memcmp(s1, s2, n2) == 0);
  }

static bool readline(FILE *fi, char *line)
  { int ch = getc(fi);
    int n = 0;
    until (ch < 0 || ch == '\n' || n >= MAXSTRLEN)
      { line[n++] = ch;
	ch = getc(fi);
      }
    line[n] = '\0';
    return !(n == 0 && ch < 0);
  }

static void readfile(FILE *fi)
  { version = getformat(fi); /* version number */
    unless (version >= 2 && version <= 4) giveup("Ilgl vsn num %d: expected 2..4", version);
    pagewidth = getformat(fi);	    /* ( 1) width of page, = width of window on screen	      */
    pageheight = getformat(fi);	    /* ( 2) height of ditto				      */
    if (version >= 3)
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
  }

static int getformat(FILE *f)
  { int ch = getc(f);
    until (ch == EOF || (ch >= '0' && ch <= '9')) ch = getc(f);
    if (ch == EOF) giveup("premature eof in format file");
    char w[MAXSTRLEN+1]; int nc = 0;
    until (nc >= MAXSTRLEN || ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF)
      { w[nc++] = ch;
	ch = getc(f);
      }
    w[nc++] = '\0';
    int n; bool ok = setnumber(w, n, n_upciok); /* only units allowed in format file are "upci"! */
    unless (ok) giveup("error in format file; giving up!");
    until (ch == '\n' || ch == EOF) ch = getc(f);
    return n;
  }

static char *expandfn(char *w)
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

static void checkformat()
  { ckfmt(linesperpage, 1, MAXINT, "lines per page");
    ckfmt(linesperbody, 1, linesperpage-topmargin, "lines per body");
    ckfmt(headposn, 0, linesperpage-1, "header position");
    ckfmt(footposn, 0, linesperpage-1, "footer position");
  }

static void ckfmt(int x, int xmin, int xmax, char *s)
  { if (x < xmin || x > xmax) giveup("error in format file: %s has illegal value", s);
  }

