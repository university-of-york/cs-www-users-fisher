/* Alpha document preparation system
   screen output module
   AJF	 January 1988 */

#include <errno.h>

#include "alpha.h"

#define FONTOFFSET 1	/* to avoid touching wshell's menu font */

static int currline;

static void definetrlit(Char, trlit*), outchar(Char);


global void scr_outproc(outpop code, word p1, word p2, word p3)
  { switch (code)
      { case o_bofile:
	  { /* initialize */
	    int n = (vertspacing+500)/1000;
	    fprintf(codeout, "\033[%dV", n); /* clear screen; set vertical spacing */
	    break;
	  }

	case o_bopage:
	    /* new page (clear screen) */
	    putc('\014', codeout);
	    currline = 0;
	    break;

	case o_clcbar:
	    /* clear changebars */
	    fprintf(codeout, "\033[c");
	    break;

	case o_boline:
	    /* move current position */
	    if (p1.i >= currline-10 && p1.i <= currline+10)
	      { while (currline < p1.i) { putc('\012', codeout); currline++; }
		while (currline > p1.i) { putc('\013', codeout); currline--; }
	      }
	    else
	      { fprintf(codeout, "\033[%d;1H", p1.i + 1); /* absolute move */
		currline = p1.i;
	      }
	    /* del to eol; specify indent, inter-word space */
	    fprintf(codeout, "\033[%d;%dI", p2.i, p3.i);
	    break;

	case o_eoline:
	    /* carriage return */
	    putc('\r', codeout);
	    break;

	case o_deffont:
	  { /* define font */
	    fishfont *ft = fontinfo[p1.i];
	    if (ft -> pf_fn == NULL)
	      fprintf(messout, "warning: fishfont \"%s\" does not contain any bitmap fonts\n", ft -> path_name);
	    if (ft -> ps_name == NULL)
	      fprintf(messout, "warning: fishfont \"%s\" does not contain a PostScript font\n", ft -> path_name);
	    fprintf(codeout, "\033]F%d;%s.%d\033\\", FONTOFFSET + p1.i, ft -> short_name, ft -> ptsize);
	    break;
	  }

	case o_selfont:
	    /* select font */
	    fprintf(codeout, "\033[%dF", FONTOFFSET + p1.i);
	    break;

	case o_deftrlit:
	    /* specify transliteration */
	    definetrlit(Char(p1.i, p2.i), (trlit*) p3.p);
	    anotherpass = true; /* this esc. seq. clears the screen, so we need another pass */
	    break;

	case o_char:
	    /* write a char */
	    putc(p1.i, codeout);
	    break;
      }
  }

static void definetrlit(Char tc, trlit *tr)
  { switch (tr -> type)
      { default:
	    fprintf(messout, "Bug! (definetrlit in scrout)\n");
	    break;

	case t_unkn:
	    /* msg has already been printed */
	    break;

	case t_char:
	    /* special character */
	    fprintf(codeout, "\033]T");
	    fprintf(codeout, "%d;%d;", tr -> wid, tr -> hgt);           /* advance (millipoints) */
	    outchar(tc);						/* char which stands for sequence */
	    for (int i=0; i < tr -> nsu; i++) outchar(tr -> subst[i]);	/* components of sequence */
	    fprintf(codeout, "\033\\");
	    break;

	case t_bitmap:
	    /* bitmap */
	    fprintf(codeout, "\033]B");
	    fprintf(codeout, "%d;%d;", tr -> wid, tr -> hgt);           /* advance (millipoints) */
	    outchar(tc);						/* char which stands for bitmap */
	    fprintf(codeout, "0;0;0;0;");                               /* 4 "x B" params ignored */
	    fprintf(codeout, "%d;%s", tr -> scale, tr -> fn);           /* scale factor, bitmap filename */
	    fprintf(codeout, "\033\\");
	    break;
      }
  }

static void outchar(Char c)
  { fprintf(codeout, "%d;%d;", FONTOFFSET + c.ft, c.ch);
  }

