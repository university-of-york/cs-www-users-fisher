/* Alpha document preparation system
   screen output module
   AJF	 January 1988 */

#define FONTOFFSET 1	/* to avoid touching wshell's menu font */

#include "/usr/fisher/lib/fishfont.h"
#include "alpha.h"
#include <stdio.h>

extern FILE *codeout, *messout;		/* from alpha  */
extern bool anotherpass;		/* from format */
extern struct fishfont *fontinfo[];	/* from common */

extern int vertspacing; /* style variable, defined in style file, from readstyle */

static int currline;


global scr_outproc(code, p1, p2, p3) enum outpop code; word p1, p2, p3;
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
	    if (p1 >= currline-10 && p1 <= currline+10)
	      { while (currline < p1) { putc('\012', codeout); currline++; }
		while (currline > p1) { putc('\013', codeout); currline--; }
	      }
	    else
	      { fprintf(codeout, "\033[%d;1H", p1+1); /* absolute move */
		currline = p1;
	      }
	    /* del to eol; specify indent, inter-word space */
	    fprintf(codeout, "\033[%d;%dI", p2, p3);
	    break;

	case o_eoline:
	    /* carriage return */
	    putc('\r', codeout);
	    break;

	case o_deffont:
	  { /* define font */
	    struct fishfont *ft = fontinfo[p1];
	    if (ft -> pf_fn == NULL)
	      fprintf(messout, "warning: fishfont \"%s\" does not contain any bitmap fonts\n", ft -> path_name);
	    if (ft -> ps_name == NULL)
	      fprintf(messout, "warning: fishfont \"%s\" does not contain a PostScript font\n", ft -> path_name);
	    fprintf(codeout, "\033]F%d;%s.%d\033\\", FONTOFFSET+p1, ft -> short_name, ft -> ptsize);
	    break;
	  }

	case o_selfont:
	    /* select font */
	    fprintf(codeout, "\033[%dF", FONTOFFSET+p1);
	    break;

	case o_deftrlit:
	    /* specify transliteration */
	    definetrlit(p1, p2);
	    anotherpass = true; /* this esc. seq. clears the screen, so we need another pass */
	    break;

	case o_char:
	    /* write a char */
	    putc(p1, codeout);
	    break;
      }
  }

static definetrlit(tc, tr) int tc; struct trlit *tr;
  { switch (tr -> type)
      { default:
	    fprintf(messout, "Bug! (definetrlit in scrout)\n");
	    break;

	case t_unkn:
	    /* msg has already been printed */
	    break;

	case t_char:
	  { /* special character */
	    int i;
	    fprintf(codeout, "\033]T");
	    fprintf(codeout, "%d;%d;", tr -> wid, tr -> hgt);         /* advance (millipoints) */
	    fprintf(codeout, "%d;%d;", FONTOFFSET, tc);               /* char which stands for sequence */
	    for (i=0; i < tr -> nsu; i++)
	      { int x = tr -> subst[i];
		fprintf(codeout, "%d;%d;", FONTOFFSET + (x >> 8), x & 0377); /* components of sequence */
	      }
	    fprintf(codeout, "\033\\");
	    break;
	  }

	case t_bitmap:
	    /* bitmap */
	    fprintf(codeout, "\033]B");
	    fprintf(codeout, "%d;%d;", tr -> wid, tr -> hgt);         /* advance (millipoints) */
	    fprintf(codeout, "%d;%d;", FONTOFFSET, tc);               /* char which stands for bitmap */
	    fprintf(codeout, "0;0;0;0;");                             /* 4 "x B" params ignored */
	    fprintf(codeout, "%d;%s", tr -> scale, tr -> fn);         /* scale factor, bitmap filename */
	    fprintf(codeout, "\033\\");
	    break;
      }
  }

