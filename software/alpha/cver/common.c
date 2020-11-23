/* Alpha document preparation system
   common routines, used by alpha
   AJF	 June 1988 */

#include "alpha.h"
#include "gfxlib.h"

#include <stdio.h>

extern struct fishfont *FindFishFont(); /* from gfxlib.a */
extern char *malloc();

/* style variables, from readstyle: */
extern int vertspacing, mingluewidth;

extern FILE *messout;			/* from alpha  */
extern int fonts[];			/* from format */
extern proc outproc;			/* from edit   */

struct dictnode
  { struct dictnode *lft, *rgt;
    char *str;
  };

static struct dictnode *dictionary;
static int numfonts;

global struct fishfont *fontinfo[MAXFONTS];

forward char *lookupword();


global initfonts()
  { numfonts = 0;
  }

global splitfontname(w, fn, ps) char *w; char *fn; int *ps;
  { /* split a font name "FONT.nn" into face name & pt size */
    int k = 0;
    until (w[k] == '.' || w[k] == '\0') { fn[k] = w[k]; k++; }
    fn[k] = '\0';
    if (w[k] == '.') k++;
    *ps = 0;
    while (w[k] >= '0' && w[k] <= '9')
      *ps = ((*ps) * 10) + (w[k++] - '0');
  }

global int lookupfont(fn, ps) char *fn; int ps;
  { int fk = 0;
    char *wfn = lookupword(fn);
    while (fk < numfonts && !(fontinfo[fk] -> short_name == wfn && fontinfo[fk] -> ptsize == ps)) fk++;
    if (fk >= numfonts)
      { fk = -1; /* assume error */
	if (numfonts < MAXFONTS)
	  { struct fishfont *ft = FindFishFont(wfn, ps);
	    if (ft != NULL && ft -> advance != NULL)
	      { ft -> short_name = wfn; /* search key */
		ft -> advance[GLUECH].x = mingluewidth; /* set min. glue width */
		ft -> advance[SUPERGLUECH].x = mingluewidth;
		fk = numfonts++;
		fontinfo[fk] = ft;
		outproc(o_deffont, fk); /* define font */
	      }
	    else fprintf(messout, "Can't find font \"%s\"\n", wfn);
	  }
	else fprintf(messout, "Too many fonts!\n");
      }
    return fk;
  }

global initdict()
  { dictionary = NULL;
  }

global char *lookupword(v) char *v;
  { /* returns a canonical version of a string */
    struct dictnode **m = &dictionary;
    bool found = false;
    until (found || (*m == NULL))
      { int k = strcmp(v, (*m) -> str);
	if (k < 0) m = &(*m) -> lft;
	if (k > 0) m = &(*m) -> rgt;
	if (k == 0) found = true;
      }
    unless (found)
      { struct dictnode *t = heap(1, struct dictnode);
	t -> lft = t -> rgt = NULL;
	t -> str = heap(strlen(v)+1, char);
	strcpy(t -> str, v);
	*m = t;
      }
    return (*m) -> str;
  }

global bool setnumber(w, xx, nflags) char *w; int *xx; uint nflags;
  { /* decodes number in w; puts result in *xx; returns true iff ok */
    int p = 0, sgn = 1, fac = 1, n = 0, dp = 0;
    bool haddp = false, hadsign = false, ok = true;
    while (w[p] == '+' || w[p] == '-')
      { int c = w[p++];
	unless (nflags & n_signok) ok = false;
	if (c == '-') sgn = -sgn;
	hadsign = true;
      }
    while ((w[p] >= '0' && w[p] <= '9') || w[p] == '.')
      { int c = w[p++];
	if (c == '.')
	  { if (haddp) ok = false;
	    haddp = true;
	  }
	else
	  { n = (n * 10) + (c - '0');
	    if (haddp) dp++;
	  }
      }
    if (w[p] != '\0' && (nflags & n_unitsok))
      { int uc = w[p++];
	fac = upci_fac(uc);
	if (fac == 0 && (nflags & n_nmvok)) fac = nmv_fac(uc);
	if (fac == 0) ok = false;
      }
    if (w[p] != '\0') ok = false;
    n *= fac;
    while (dp > 0) { n = (n+5)/10; dp--; }
    n *= sgn;
    if (hadsign && (nflags & n_rel)) *xx += n;
    else *xx = n;
    unless (ok) fprintf(messout, "Illegal number \"%s\"\n", w);
    return ok;
  }

static int upci_fac(uc) int uc;
  { switch (uc)
      { default:	return 0;		   /* error		  */
	case 'u':	return 1;		   /* units (millipixels) */
	case 'p':	return 1000;		   /* points		  */
	case 'c':	return 28346;		   /* centimetres	  */
	case 'i':	return 72000;		   /* inches		  */
      }
  }

static int nmv_fac(uc) int uc;
  { switch (uc)
      { default:	return 0;					/* error */
	case 'n':	return fontinfo[fonts[Roman]] -> ptsize * 500;	/* ens	 */
	case 'm':	return fontinfo[fonts[Roman]] -> ptsize * 1000; /* ems	 */
	case 'v':	return vertspacing;				/* lines */
      }
  }

global char *cheap(n) int n;
  { /* called by "heap" macro to allocate n bytes */
    char *s = malloc(n);
    if (s == NULL) a_giveup("no room");
    return s;
  }

global a_giveup(msg, p1) char *msg; word p1;
  { fprintf(stderr, "alpha: ");
    fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(2);
  }

global bool testbit(m, k) uint *m; int k;
  { int i = k/BPW, j = k%BPW;
    return (m[i] & (1 << j));
  }

global setbit(m, k) uint *m; int k;
  { int i = k/BPW, j = k%BPW;
    m[i] |= (1 << j);
  }

