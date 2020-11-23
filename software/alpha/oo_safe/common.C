/* Alpha document preparation system
   common routines, used by alpha
   AJF	 June 1988 */

#include "alpha.h"


struct dictnode
  { struct dictnode *lft, *rgt;
    char *str;
  };

static dictnode *dictionary = NULL;
static int numfonts;

global fishfont *fontinfo[MAXFONTS];


global void initfonts()
  { numfonts = 0;
  }

global void splitfontname(char *w, char *fn, int &ps)
  { /* split a font name "FONT.nn" into face name & pt size */
    int k = 0;
    while ((w[k] >= 'A' && w[k] <= 'Z') || (w[k] >= '0' && w[k] <= '9')) { fn[k] = w[k]; k++; }
    fn[k] = '\0';
    if (w[k++] == '.')
      { ps = 0;
	while (w[k] >= '0' && w[k] <= '9') ps = (ps * 10) + (w[k++] - '0');
      }
    else ps = -1;
  }

global int lookupfont(char *fn, int ps)
  { int fk = 0;
    char *wfn = lookupword(fn);
    while (fk < numfonts && !(fontinfo[fk] -> short_name == wfn && fontinfo[fk] -> ptsize == ps)) fk++;
    if (fk >= numfonts)
      { fk = -1; /* assume error */
	if (numfonts < MAXFONTS)
	  { fishfont *ft = new fishfont(wfn, ps);
	    if (ft -> ok && ft -> advance != NULL)
	      { ft -> short_name = wfn; /* search key */
		ft -> advance[GLUECH].x = mingluewidth; /* set min. glue width */
		ft -> advance[SUPERGLUECH].x = mingluewidth;
		fk = numfonts++;
		fontinfo[fk] = ft;
		if (ps >= 0) outproc(o_deffont, fk); /* define font */
	      }
	    else fprintf(messout, "Can't find font \"%s\"\n", wfn);
	  }
	else fprintf(messout, "Too many fonts!\n");
      }
    return fk;
  }

global char *lookupword(char *v)
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
      { dictnode *t = new dictnode;
	t -> lft = t -> rgt = NULL;
	t -> str = new char[strlen(v) + 1];
	strcpy(t -> str, v);
	*m = t;
      }
    return (*m) -> str;
  }

static int upci_fac(int), nmv_fac(int);

global bool setnumber(char *w, int &x, uint nflags)
  { /* decodes number in w; puts result in x; returns true iff ok */
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
    if (hadsign && (nflags & n_rel)) x += n;
    else x = n;
    unless (ok) fprintf(messout, "Illegal number \"%s\"\n", w);
    return ok;
  }

static int upci_fac(int uc)
  { switch (uc)
      { default:	return 0;		   /* error		  */
	case 'u':	return 1;		   /* units (millipixels) */
	case 'p':	return 1000;		   /* points		  */
	case 'c':	return 28346;		   /* centimetres	  */
	case 'i':	return 72000;		   /* inches		  */
      }
  }

static int nmv_fac(int uc)
  { switch (uc)
      { default:	return 0;					/* error */
	case 'n':	return fontinfo[fonts[Roman]] -> ptsize * 500;	/* ens	 */
	case 'm':	return fontinfo[fonts[Roman]] -> ptsize * 1000; /* ems	 */
	case 'v':	return vertspacing;				/* lines */
      }
  }

