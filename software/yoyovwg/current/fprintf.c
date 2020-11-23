/* fprintf which knows about "%h" to print hypernotions */

#include "vwg.h"
#include "states.h"
#include <stdio.h>

typedef (*proc)();

forward proc format();


global printf(s, p1, p2, p3, p4) char *s; word p1, p2, p3, p4;
  { fprintf(stdout, s, p1, p2, p3, p4);
  }

global fprintf(f, s, p1, p2, p3, p4) FILE *f; char *s; word p1, p2, p3, p4;
  { int j = 0, k = 0;
    word *pp = &p1;
    char ch = s[j++];
    until (ch == '\0')
      { if (ch == '%')
	  { int d = 0;
	    ch = s[j++];
	    while (ch >= '0' && ch <= '9') { d = d*10 + (ch-'0'); ch = s[j++]; }
	    if (ch == '%') putc('%', f);
	    else
	      { proc fmt = format(ch);
		if (fmt == NULL) putc('?', f);
		else fmt(pp[k++], d, f);
	      }
	  }
	else putc(ch, f);
	ch = s[j++];
      }
  }

forward fwritea(), fwriteb(), fwritec(), fwrited(), fwritee(), fwriteh(), fwritem(), fwrites(), fwritet(), fwritex();

static proc format(ch) char ch;
  { switch (ch)
      { default:    return NULL;
	case 'a':   return fwritea;
	case 'b':   return fwriteb;
	case 'c':   return fwritec;
	case 'd':   return fwrited;
	case 'e':   return fwritee;
	case 'h':   return fwriteh;
	case 'm':   return fwritem;
	case 's':   return fwrites;
	case 't':   return fwritet;
	case 'x':   return fwritex;
      }
  }

static fwritea(alt, d, f) struct hyperalt *alt; int d; FILE *f;
  { int i;
    for (i=0; i < alt -> rlen; i++)
      { if (i > 0) fprintf(f, ", ");
	fwriteh(alt -> rdef[i], d, f);
      }
  }

static fwriteb(b, d, f) bool b; int d; FILE *f;
  { putc(b ? 'Y' : 'N', f);
  }

static fwritec(c, d, f) int c, d; FILE *f;
  { putc(c, f);
  }

static fwrited(n, d, f) int n, d; FILE *f;
  { if (n < 0) writeneg(true, n, d-1, f);
    if (n > 0) writeneg(false, -n, d, f);
    if (n == 0) { spaces(d-1, f); putc('0', f); }
  }

static writeneg(s, n, d, f) bool s; int n, d; FILE *f;
  { if (n == 0)
      { spaces(d, f);
	if (s) putc('-', f);
      }
    else
      { writeneg(s, n/10, d-1, f);
	putc(-(n%10) + '0', f);
      }
  }

static spaces(d, f) int d; FILE *f;
  { while (d > 0) putc(' ', f), d--; }

static fwritee(st, d, f) struct state *st; int d; FILE *f;
  { struct hyperrule *hr = st -> pval;
    struct hyperalt *rhs = hr -> rhs;
    int i;
    fprintf(f, "%h:", hr -> lhs);
    for (i=0; i < rhs -> rlen + 1; i++)
      { if (i == st -> jval) fprintf(f, " <DOT>");
	if (i < rhs -> rlen) fprintf(f, " %h", rhs -> rdef[i]);
	if (i < rhs -> rlen - 1) putc(',', f);
      }
  }

static fwriteh(hn, d, f) struct hypernotion *hn; int d; FILE *f;
  { if (hn -> hlen < 0) fprintf(f, "\"%s\"", hn -> term -> str);
    else wrhn(hn, f);
  }

static wrhn(hn, f) struct hypernotion *hn; FILE *f;
  { int k; bool sep;
    int n1 = 0;
    if (hn -> flags & hn_negpred)
      { /* change "where" into "unless" */
	fprintf(f, "unless");
	n1 = 5;
      }
    for (k = n1; k < hn -> hlen; k++)
      { struct hitem *z = &hn -> hdef[k];
	if (z -> sy == s_ssm)
	  { if (k > n1 && sep) putc(' ', f);
	    putc(z -> it_s, f);
	    sep = false; /* i.e. only if followed by a meta */
	  }
	if (z -> sy == s_meta)
	  { if (k > n1) putc(' ', f);
	    fprintf(f, "%s", z -> it_z -> str);
	    sep = true;
	  }
      }
  }

static fwritem(y, d, f) struct metarhs *y; int d; FILE *f;
  { bool had = false;
    until (y == NULL)
      { if (had) fprintf(stderr, "; ");
	fprintf(f, "%h", y -> hn);
	y = y -> lnk; had = true;
      }
  }

static fwrites(s, d, f) char *s; int d; FILE *f;
  { int i = 0;
    until (s[i] == '\0')
      { int c = s[i++];
	putc(c, f);
      }
  }

static fwritet(t, d, f) uint t; int d; FILE *f;
  { /* write LL(1) starters */
    static char *tab = "abcdefghijklmnopqrstuvwxyz<>_*.@";   /* * is stopper; @ is null bit */
    int k = 0;
    until (t == 0)
      { if (t & 1) putc(tab[k], f);
	t >>= 1; k++;
      }
  }

static fwritex(n, d, f) uint n; int d; FILE *f;
  { static char *hextab = "0123456789abcdef";
    if (d > 0)
      { fwritex(n >> 4, d-1, f);
	putc(hextab[n & 0xf], f);
      }
  }

