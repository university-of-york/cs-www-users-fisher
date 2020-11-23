/* Alpha document preparation system
   PostScript output module
   AJF	 January 1988
   Modified to generate PostScript (used to generate ditcode): AJF May 1992 */

#include "alpha.h"
#include "gfxlib.h"

#include <stdio.h>

#define rdown(n)      ((n)/1000)		/* cvt millipts to pts, round down */
#define rup(n)	      (((n)+999)/1000)		/* cvt millipts to pts, round up   */

#ifdef mips
#define PRELUDEFN	"/usr/fisher/mipslib/alpha/prelude.ps"
#define readbm(fn)	ReadBitmap(NULL, fn, BM_BITS)
#define freebm(bm)	Bfree(NULL, bm)
#endif

#ifdef sun
#define PRELUDEFN	"/usr/fisher/sunlib/alpha/prelude.ps"
#define readbm(fn)	ReadBitmap(fn)
#define freebm(bm)	Bfree(bm)
#endif

#define BMFUDGE 15.591		    /* move bitmaps up this much (points) */
#define LINELENGTH 300
#define MAXFACES   25

extern char *lookupword();	    /* from common   */
extern bool testbit();		    /* from common   */
extern setbit();		    /* form common   */
extern struct bitmap *ReadBitmap(); /* from gfxlib.a */

extern FILE *messout;		    /* from alpha    */
extern bool epsflag;		    /* from alpha    */
extern struct fishfont *fontinfo[]; /* from common   */

/* style variables, defined in style file, from readstyle */
extern int ps_tlx, ps_tly, pagewidth, pageheight, vertspacing;

static struct trlit *translits[NUMTFONTS][FONTSIZE];
static char *faces[MAXFACES];

static int currfont, selected, numfaces;
static uint abbrevfonts[MAXFONTS/BPW];
static double newxpos, newypos, gluewidth;
static bool textmode, insetup;
static enum { none, rel, abs } moveneeded;

forward char *cfname();


global ps_outproc(code, p1, p2, p3) enum outpop code; word p1, p2, p3;
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
	  { int i, j;
	    for (j=0; j < NUMTFONTS; j++)
	      for (i=0; i < FONTSIZE; i++) translits[j][i] = NULL;
	    textmode = false; moveneeded = none;
	    break;
	  }

	case o_eofile:
	    if (insetup)
	      { printf("%%%%EndSetup\n");
		insetup = false;
	      }
	    break;

	case o_bopage:
	  { int i;
	    if (insetup)
	      { printf("%%%%EndSetup\n");
		insetup = false;
	      }
	    printf("\n%%%%Page: alpha %d\n", p1);
	    printf("%% Beginning of page %d/%d\n", p1, p2);
	    printf("Bp ");
	    selected = -1;	/* Bp/Ep does a save & restore which unsets font selection */
	    for (i=0; i < MAXFONTS/BPW; i++) abbrevfonts[i] = 0;    /* abbrevs. for fonts are local to pages */
	    break;
	  }

	case o_eopage:
	    outtext();
	    printf("Ep\n");
	    printf("%% End of page %d\n", p1);
	    break;

	case o_boline:
	    newxpos = (ps_tlx + p2) / 1000.0;
	    newypos = (ps_tly - p1*vertspacing) / 1000.0;
	    gluewidth = p3 / 1000.0;
	    moveneeded = abs;
	    break;

	case o_eoline:
	    moveneeded = none;
	    break;

	case o_deffont:
	  { /* define font - in pmode, this will occur before the first bopage */
	    struct fishfont *ft = fontinfo[p1];
	    int k = 0; char *face;
	    if (ft -> ps_name == NULL)
	      a_giveup("fishfont %s does not contain a PostScript font", ft -> path_name);
	    face = lookupword(ft -> ps_name);
	    while (k < numfaces && faces[k] != face) k++;
	    if (k >= numfaces)
	      { if (numfaces >= MAXFACES) a_giveup("sorry, too many faces!");
		faces[numfaces++] = face;
		unless (insetup) a_giveup("sorry, can't introduce new face in 2nd or subsequent source file! (%s)", face);
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
	    currfont = p1;
	    break;

	case o_deftrlit:
	  { struct trlit *tr = heap(1, struct trlit);
	    int ft = p1 >> 8, ch = p1 & 0377;
	    memcpy(tr, p2, sizeof(struct trlit)); /* copy the struct */
	    translits[ft][ch] = tr;
	    break;
	  }

	case o_char:
	    if (currfont < NUMTFONTS)
	      { /* transliteration font */
		struct trlit *tr = translits[currfont][p1];
		if (tr != NULL) printtranslit(tr);
		else printchar(currfont, p1);
	      }
	    else printchar(currfont, p1);
	    break;
      }
  }

static printtranslit(tr) struct trlit *tr;
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

static printspecial(tr) struct trlit *tr;
  { int i; double fw;
    for (i=0; i < tr -> nsu; i++)
      { int x = tr -> subst[i];
	int fn = x >> 8, ch = x & 0377;
	double cw = fontinfo[fn] -> advance[ch].x / 1000.0;
	if (i == 0) fw = cw; /* width of first char (for centring) */
	move((fw-cw)/2.0, 0.0);
	printchar(fn, ch);	   /* implicit move(cw, 0) */
	move(-(fw+cw)/2.0, 0.0);
      }
  }

static printchar(fn, ch) int fn, ch;
  { if (ch == GLUECH) move(gluewidth, 0.0);
    else if (ch == ' ')
      { double spw = fontinfo[fn] -> advance[' '].x / 1000.0;
	move(spw, 0.0);
      }
    else
      { ensuremove();
	unless (selected == fn)
	  { struct fishfont *ft = fontinfo[fn];
	    outtext();
	    unless (testbit(abbrevfonts, fn))
	      { printf("/Ft%d %d /%s FFt ", fn, ft -> ptsize, ft -> ps_name);
		setbit(abbrevfonts, fn);
	      }
	    printf("Ft%d SFt ", fn);
	    selected = fn;
	  }
	intext();
	if (ch >= '!' && ch <= '~')
	  { if (ch == '(' || ch == ')' || ch == '\\') putchar('\\');
	    putchar(ch);
	  }
	else printf("\\%03o", ch);
      }
  }

static printbitmap(tr) struct trlit *tr;
  { int sy; struct bitmap *bm; ushort *ptr;
    char *cfn = cfname(tr -> fn);
    bm = readbm(tr -> fn);
    if (bm == NULL) a_giveup("can't read bitmap file \"%s\"", tr -> fn);
    ensuremove(); outtext();
    printf("%% BEGIN BITMAP %s\n", cfn);
    printf("gsave\n");
    printf("%.3f Up currentpoint translate\n", BMFUDGE);
    sy = 0; ptr = bm -> image;
    while (sy < bm -> hgt)
      { int wpl, nw, nl, i;
	wpl = (bm -> wid + 15) / 16;  /* words per line */
	/* estimate no. of lines we can print safely, based on string length limit */
	nl = (bm -> wid > 0) ? 130000 / bm -> wid : MAXINT;
	if (nl > bm -> hgt - sy) nl = bm -> hgt - sy;
	/* print nl lines starting at sy */
	printf("%d %d true [%d 0 0 %d 0 0] { <\n", wpl*16, nl, tr -> scale, -(tr -> scale));
	nw = nl*wpl; /* num. of words in nl lines */
	for (i=0; i < nw; i++)
	  { printf("%04x", *(ptr++));
	    if ((i+1)%30 == 0) putchar('\n');
	  }
	unless (nw%30 == 0) putchar('\n');
	printf("> } imagemask\n");
	printf("0 %.3f translate\n", -((double) nl / (double) tr -> scale)); /* translate to base of segment */
	sy += nl;
      }
    printf("grestore\n");
    printf("%% END BITMAP %s\n", cfn);
    freebm(bm);
  }

static printpostscript(tr) struct trlit *tr;
  { char *cfn = cfname(tr -> fn);
    ensuremove(); outtext();
    printf("%% BEGIN ENCAPSULATED %s\n", cfn);
    printf("/encaps_saved save def\n");
    printf("/showpage {} def /copypage {} def /erasepage {} def\n");
    printf("%.3f Up currentpoint translate ", BMFUDGE);
    if (tr -> scale != 1)
      { double s = 1.0 / (double) tr -> scale;
	printf("%.3f %.3f scale ", s, s);
      }
    printf("%.3f %.3f translate\n", - (tr -> bbox.llx) / 1000.0, - (tr -> bbox.ury) / 1000.0);
    include(tr -> fn, true); /* the imported PostScript (suppress '%%' comments) */
    printf("encaps_saved restore\n");
    printf("%% END ENCAPSULATED %s\n", cfn);
  }

static char *cfname(fn) char *fn;
  { /* compute filename for comments; makes "diffing" ps files easier if temp files are used */
    return (strncmp(fn, "/tmp/", 5) == 0) ? "/tmp/..." : fn;
  }

static move(dx, dy) double dx, dy;
  { if (moveneeded == none)
      { newxpos = newypos = 0.0;
	moveneeded = rel;
      }
    newxpos += dx; newypos += dy;
  }

static ensuremove()
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

static intext()
  { unless (textmode)
      { putchar('(');
	textmode = true;
      }
  }

static outtext()
  { if (textmode)
      { printf(") T\n");
	textmode = false;
      }
  }

static include(fn, embed) char *fn; bool embed;
  { FILE *fi; char line[LINELENGTH+1]; bool ok;
    fi = fopen(fn, "r");
    if (fi == NULL) a_giveup("can't open %s", fn);
    ok = rdline(fn, fi, line);
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

static bool rdline(fn, fi, line) char *fn; FILE *fi; char line[];
  { int nc = 0;
    int ch = getc(fi);
    until (ch == '\n' || ch == EOF || nc >= LINELENGTH)
      { line[nc++] = ch;
	ch = getc(fi);
      }
    line[nc++] = '\0';
    unless (ch == EOF || ch == '\n') a_giveup("line too long in PostScript file %s!", fn);
    return (ch != EOF);
  }

static wrline(line) char line[];
  { fputs(line, stdout); putchar('\n');
  }

