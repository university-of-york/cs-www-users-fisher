/* G2PSG parser generator   A.J. Fisher	  September 1990 */
/* error message routine */

#include "g2psg.h"
#include <stdio.h>

extern struct errorpos errorpos;
extern bool errors;
extern char *freprs[];

extern bool testbit();

#define ERRMSGFILE "/usr/fisher/lib/g2psg.emf"


global prerror(n,p1,p2,p3,p4) int n,p1,p2,p3,p4;
  { char v[81];
    getmessage(n,v);
    fprintf(stderr, "*** ");
    writevec(v,0,11);
    if (errorpos.ln >= 0) fprintf(stderr, "line %d ", errorpos.ln);
    if (errorpos.fn != NULL) fprintf(stderr, "file %s ", errorpos.fn);
    writevec(v,12,strlen(v)-1,p1,p2,p3,p4); putc('\n',stderr);
    if (v[6] == 'B' && v[7] == 'G')
      { fprintf(stderr, "Aborting... "); fflush(stderr);
	kill(getpid(), 6);
      }
    errors = true;
  }

static getmessage(n,v) int n; char *v;
  { int nc = 0; int c;
    FILE *f = fopen(ERRMSGFILE,"r");
    if (f == NULL)
      { fprintf(stderr, "g2psg: can't open error message file %s\n", ERRMSGFILE);
	exit(1);
      }
    fseek(f,(n-100)*80,0);
    c = getc(f);
    until (nc >= 80 || c == '\n' || c == EOF)
      { v[nc++] = c;
	c = getc(f);
      }
    v[nc++] = '\0';
    fclose(f);
  }

static writevec(v,k1,k2,p1,p2,p3,p4) char *v; int k1,k2,p1,p2,p3,p4;
  { char s[81]; int i;
    for (i=k1; i <= k2; i++) s[i-k1] = v[i];
    s[k2-k1+1] = '\0';
    fprintf(stderr,s,p1,p2,p3,p4);
  }

forward proc format();

global fprintf(f,s,p1,p2,p3,p4) FILE *f; char *s; int p1,p2,p3,p4;
  { int j = 0, k = 0;
    int *pp = &p1;
    char ch = s[j++];
    until (ch == '\0')
      { if (ch == '%')
	  { int d = 0;
	    ch = s[j++];
	    while (ch >= '0' && ch <= '9') { d = d*10 + (ch-'0'); ch = s[j++]; }
	    if (ch == '%') putc('%',f);
	    else
	      { proc fmt = format(ch);
		if (fmt == NULL) putc('?',f);
		else fmt(pp[k++],d,f);
	      }
	  }
	else putc(ch,f);
	ch = s[j++];
      }
  }

forward fwritec(), fwrited(), fwritem(), fwriteo(), fwrites(), fwritex(), fwritey();

static proc format(ch) char ch;
  { switch (ch)
      { default:    return NULL;
	case 'c':   return fwritec;
	case 'd':   return fwrited;
	case 'm':   return fwritem;
	case 'o':   return fwriteo;
	case 's':   return fwrites;
	case 'x':   return fwritex;
	case 'y':   return fwritey;
      }
  }

static fwritec(c,d,f) int c,d; FILE *f; { putc(c,f); }

static fwrited(n,d,f) int n,d; FILE *f;
  { if (n < 0) writeneg(true,n,d-1,f);
    if (n > 0) writeneg(false,-n,d,f);
    if (n == 0) { spaces(d-1,f); putc('0',f); }
  }

static writeneg(s,n,d,f) bool s; int n,d; FILE *f;
  { if (n == 0)
      { spaces(d,f);
	if (s) putc('-',f);
      }
    else
      { writeneg(s,n/10,d-1,f);
	putc(-(n%10) + '0', f);
      }
  }

static spaces(d,f) int d; FILE *f;
  { while (d > 0) putc(' ',f), d--; }

/* static fwritem(m,d,f) struct fmask *m; int d; FILE *f;
     { int k;
       for (k=FMASKSIZE-1; k >= 0; k--) fwritex(m -> m[k], 8, f);
     } */

static fwritem(m,d,f) struct fmask *m; int d; FILE *f;
  { int k;
    bool had = false;
    putc('[', f);
    for (k=0; k<MAXFSPECS; k++)
      { if (testbit(k, m -> m))
	  { char *s = freprs[k];
	    if (s == NULL) s = "???";
	    if (had) putc(',', f);
	    fputs(s, f);
	    had = true;
	  }
      }
    putc(']', f);
  }

#define hextab "0123456789ABCDEF"

static fwriteo(n,d,f) uint n; int d; FILE *f;
  { if (d > 0)
      { fwriteo(n >> 3, d-1, f);
	putc(hextab[n & 7], f);
      }
  }

static fwritex(n,d,f) uint n; int d; FILE *f;
  { if (d > 0)
      { fwritex(n >> 4, d-1, f);
	putc(hextab[n & 0xf], f);
      }
  }

static fwrites(s,d,f) char *s; int d; FILE *f; { fputs(s,f); }

static fwritey(c,d,f) int c,d; FILE *f; /* "sane" verion of %c */
  { c &= 0377;
    if (c >= ' ' && c <= '~') putc(c,f);
    else fprintf(f,"#%3o",c);
  }

