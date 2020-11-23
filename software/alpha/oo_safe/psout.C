/* Alpha document preparation system
   PostScript output module
   AJF	 January 1988
   Modified to generate PostScript (used to generate ditcode): AJF May 1992 */

#include <errno.h>

#include "alpha.h"

#ifdef mips
#define PRELUDEFN	"/usr/fisher/mipslib/alpha/prelude.ps"
#endif

#ifdef sun
#define PRELUDEFN	"/usr/fisher/sunlib/alpha/prelude.ps"
#endif

#define BMFUDGE		15.591	/* move bitmaps up this much (points) */
#define LINELENGTH	300
#define MAXFACES	25

static trlit *translits[NUMTFONTS][FONTSIZE];
static char *faces[MAXFACES];

static int currfont, selected, numfaces;
static uint abbrevfonts[MAXFONTS/BPW];
static float newxpos, newypos, gluewidth;
static bool textmode, insetup;
static enum { none, rel, abs } moveneeded;

static void printtranslit(trlit*), printspecial(trlit*), printchar(Char), printbitmap(trlit*), printpostscript(trlit*);
static char *cfname(char*);
static void move(float, float);
static void ensuremove(), intext(), outtext(), include(char*, bool);
static bool rdline(char*, FILE*, char[]);
static void wrline(char[]);

inline int rdown(int n) { return n/1000; }		 /* cvt millipts to pts, round down */
inline int rup(int n)	{ return (n+999)/1000; }	 /* cvt millipts to pts, round up   */


global void ps_outproc(outpop code, word p1, word p2, word p3)
  { switch (code)
      { case o_bobook:
	    if (epsflag)
	      { printf("%%!PS-Adobe-3.0 EPSF-3.0\n");
		printf("%%%%BoundingBox: %d %d %d %d\n",
		       rdown(ps_tlx), rdown(ps_tly - pageheight),
		       rup(ps_tlx + pagewidth), rup(ps_tly));
	      }
	    else
	      { printf("%%!PS-Adobe-3.0\n");
	      }
	    include(PRELUDEFN, false);
	    insetup = true;
	    numfaces = 0;
	    break;

	case o_bofile:
	    for (int j=0; j < NUMTFONTS; j++)
	      for (int i=0; i < FONTSIZE; i++) translits[j][i] = NULL;
	    textmode = false; moveneeded = none;
	    break;

	case o_eofile:
	    if (insetup)
	      { printf("%%%%EndSetup\n");
		insetup = false;
	      }
	    break;

	case o_bopage:
	    if (insetup)
	      { printf("%%%%EndSetup\n");
		insetup = false;
	      }
	    printf("\n%%%%Page: alpha %d\n", p1);
	    printf("%% Beginning of page %d/%d\n", p1, p2);
	    printf("Bp ");
	    selected = -1;	/* Bp/Ep does a save & restore which unsets font selection */
	    for (int i=0; i < MAXFONTS/BPW; i++) abbrevfonts[i] = 0;	/* abbrevs. for fonts are local to pages */
	    break;

	case o_eopage:
	    outtext();
	    printf("Ep\n");
	    printf("%% End of page %d\n", p1);
	    break;

	case o_boline:
	    newxpos = (ps_tlx + p2.i) / 1000.0;
	    newypos = (ps_tly - p1.i * vertspacing) / 1000.0;
	    gluewidth = p3.i / 1000.0;
	    moveneeded = abs;
	    break;

	case o_eoline:
	    moveneeded = none;
	    break;

	case o_deffont:
	  { /* define font - in pmode, this will occur before the first bopage */
	    fishfont *ft = fontinfo[p1.i];
	    if (ft -> ps_name == NULL) giveup("fishfont %s does not contain a PostScript font", ft -> path_name);
	    char *face = lookupword(ft -> ps_name);
	    int k = 0;
	    while (k < numfaces && faces[k] != face) k++;
	    if (k >= numfaces)
	      { if (numfaces >= MAXFACES) giveup("sorry, too many faces!");
		faces[numfaces++] = face;
		unless (insetup) giveup("sorry, can't introduce new face in 2nd or subsequent source file! (%s)", face);
		if (ft -> ps_fn == NULL) printf("%%%%IncludeFont: %s\n", face);
		else
		  { printf("%%%%BeginFont %s\n", face);
		    include(ft -> ps_fn, false); /* PostScript defn for down-loaded font */
		    printf("%%%%EndFont\n");
		  }
	      }
	    break;
	  }

	case o_selfont:
	    currfont = p1.i;
	    break;

	case o_deftrlit:
	  { trlit *tr = new trlit;
	    memcpy(tr, p3.p, sizeof(trlit)); /* copy the struct */
	    translits[p1.i][p2.i] = tr;
	    break;
	  }

	case o_char:
	    if (currfont < NUMTFONTS)
	      { /* transliteration font */
		trlit *tr = translits[currfont][p1.i];
		if (tr != NULL) printtranslit(tr);
		else printchar(Char(currfont, p1.i));
	      }
	    else printchar(Char(currfont, p1.i));
	    break;
      }
  }

static void printtranslit(trlit *tr)
  { move(0.0, -(tr -> hgt / 1000.0));  /* move this much before placing char */
    switch (tr -> type)
      { default:
	    fprintf(messout, "Bug! (printtranslit in psout)\n");
	    break;

	case t_unkn:
	    /* msg has already been printed */
	    break;

	case t_char:
	    /* special char */
	    printspecial(tr);
	    break;

	case t_bitmap:
	    /* bitmap */
	    printbitmap(tr);
	    break;

	case t_pscript:
	    /* PostScript */
	    printpostscript(tr);
	    break;
      }
    move(tr -> wid / 1000.0, 0.0);
  }

static void printspecial(trlit *tr)
  { float fw;
    for (int i=0; i < tr -> nsu; i++)
      { Char x = tr -> subst[i];
	float cw = x.width() / 1000.0;
	if (i == 0) fw = cw; /* width of first char (for centring) */
	move((fw-cw)/2.0, 0.0);
	printchar(x);	     /* implicit move(cw, 0) */
	move(-(fw+cw)/2.0, 0.0);
      }
  }

static void printchar(Char c)
  { switch (c.ch)
      { default:
	    ensuremove();
	    unless (selected == c.ft)
	      { fishfont *ft = fontinfo[c.ft];
		outtext();
		unless (testbit(abbrevfonts, c.ft))
		  { printf("/Ft%d %d /%s FFt ", c.ft, ft -> ptsize, ft -> ps_name);
		    setbit(abbrevfonts, c.ft);
		  }
		printf("Ft%d SFt ", c.ft);
		selected = c.ft;
	      }
	    intext();
	    if (c.ch >= '!' && c.ch <= '~')
	      { if (c.ch == '(' || c.ch == ')' || c.ch == '\\') putchar('\\');
		putchar(c.ch);
	      }
	    else printf("\\%03o", c.ch);
	    break;

	case GLUECH:
	    move(gluewidth, 0.0);
	    break;

	case ' ':
	  { float spw = c.width() / 1000.0;
	    move(spw, 0.0);
	  }
      }
  }

static void printbitmap(trlit *tr)
  { char *cfn = cfname(tr -> fn);
    bitmap *bm = new bitmap(tr -> fn, BM_BITS);
    unless (bm -> ok) giveup("can't read bitmap file \"%s\"", tr -> fn);
    ensuremove(); outtext();
    printf("%% BEGIN BITMAP %s\n", cfn);
    printf("gsave\n");
    printf("%.3f Up currentpoint translate\n", BMFUDGE);
    int sy = 0;
    ushort *ptr = (ushort*) (bm -> image -> data);
    while (sy < bm -> hgt)
      { int wpl = (bm -> wid + 15) / 16;  /* words per line */
	/* estimate no. of lines we can print safely, based on string length limit */
	int nl = (bm -> wid > 0) ? 130000 / bm -> wid : MAXINT;
	if (nl > bm -> hgt - sy) nl = bm -> hgt - sy;
	/* print nl lines starting at sy */
	printf("%d %d true [%d 0 0 %d 0 0] { <\n", wpl*16, nl, tr -> scale, -(tr -> scale));
	int nw = nl*wpl; /* num. of words in nl lines */
	for (int i=0; i < nw; i++)
	  { printf("%04x", *(ptr++));
	    if ((i+1)%30 == 0) putchar('\n');
	  }
	unless (nw%30 == 0) putchar('\n');
	printf("> } imagemask\n");
	printf("0 %.3f translate\n", -((float) nl / (float) tr -> scale)); /* translate to base of segment */
	sy += nl;
      }
    printf("grestore\n");
    printf("%% END BITMAP %s\n", cfn);
    delete bm;
  }

static void printpostscript(trlit *tr)
  { char *cfn = cfname(tr -> fn);
    ensuremove(); outtext();
    printf("%% BEGIN ENCAPSULATED %s\n", cfn);
    printf("/encaps_saved save def\n");
    printf("/showpage {} def /copypage {} def /erasepage {} def\n");
    printf("%.3f Up currentpoint translate ", BMFUDGE);
    if (tr -> scale != 1)
      { float s = 1.0 / (float) tr -> scale;
	printf("%.3f %.3f scale ", s, s);
      }
    printf("%.3f %.3f translate\n", - (tr -> bb.llx) / 1000.0, - (tr -> bb.ury) / 1000.0);
    include(tr -> fn, true); /* the imported PostScript (suppress '%%' comments) */
    printf("encaps_saved restore\n");
    printf("%% END ENCAPSULATED %s\n", cfn);
  }

static char *cfname(char *fn)
  { /* compute filename for comments; makes "diffing" ps files easier if temp files are used */
    return (strncmp(fn, "/tmp/", 5) == 0) ? "/tmp/..." : fn;
  }

static void move(float dx, float dy)
  { if (moveneeded == none)
      { newxpos = newypos = 0.0;
	moveneeded = rel;
      }
    newxpos += dx; newypos += dy;
  }

static void ensuremove()
  { if (moveneeded == abs)
      { outtext();
	printf("%.3f %.3f Mv ", newxpos, newypos);
      }
    if (moveneeded == rel)
      { if (newxpos != 0.0) { outtext(); printf("%.3f Rt ", newxpos); }
	if (newypos != 0.0) { outtext(); printf("%.3f Up ", newypos); }
      }
    moveneeded = none;
  }

static void intext()
  { unless (textmode)
      { putchar('(');
	textmode = true;
      }
  }

static void outtext()
  { if (textmode)
      { printf(") T\n");
	textmode = false;
      }
  }

static void include(char *fn, bool embed)
  { FILE *fi = fopen(fn, "r");
    if (fi == NULL) giveup("can't open %s", fn);
    char line[LINELENGTH+1];
    bool ok = rdline(fn, fi, line);
    while (ok)
      { if (embed && line[0] == '%' && (line[1] == '!' || line[1] == '%'))
	  { /* change "%!" and "%% to "%" in encapsulated files so they don't look like DSC comments */
	    putchar('%'); wrline(&line[2]);
	  }
	else wrline(line);
	ok = rdline(fn, fi, line);
      }
    fclose(fi);
  }

static bool rdline(char *fn, FILE *fi, char line[])
  { int nc = 0;
    int ch = getc(fi);
    until (ch == '\n' || ch == EOF || nc >= LINELENGTH)
      { line[nc++] = ch;
	ch = getc(fi);
      }
    line[nc++] = '\0';
    unless (ch == EOF || ch == '\n') giveup("line too long in PostScript file %s!", fn);
    return (ch != EOF);
  }

static void wrline(char line[])
  { fputs(line, stdout); putchar('\n');
  }

