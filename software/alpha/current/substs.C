/* Alpha document preparation system
   substitutions (tr/set/xr)
   AJF	 December 1987 */

#include "alpha.h"

substitution::substitution()
  { max = num = 0;
    tab = NULL;
    for (int i = 0; i < FONTSIZE/BPW; i++) bits[i] = 0;
  }

substitution::~substitution()
  { delete tab;
  }

void substitution::read(char *fn)
  { FILE *fi = fopen(fn, "r");
    if (fi != NULL)
      { char v1[MAXSTRLEN+1], v2[MAXSTRLEN+1];
	int ni = fscanf(fi, "%s%s\n", v1, v2);
	while (ni == 2 && v1[0] != '\0' && v2[0] != '\0')
	  { char *w1 = lookupword(v1), *w2 = lookupword(v2);
	    tlit *t = lookup(w1);
	    if (t != NULL) t -> sub = w2;
	    else add(w1, w2);
	    ni = fscanf(fi, "%s%s\n", v1, v2);
	  }
	if (ni >= 0) fprintf(stderr, "Warning: format error in xref file\n");
	fclose(fi);
      }
  }

tlit *substitution::lookup(char *w)
  { int k = 0;
    while (k < num && tab[k].tv != w) k++;
    return (k < num) ? &tab[k] : NULL;
  }

void substitution::rpt_unused()
  { for (int k = 0; k < num; k++)
      { tlit *t = &tab[k];
	unless (t -> used) fprintf(stderr, "Unused: %s\n", t -> tv);
      }
  }

void substitution::write(char *fn)
  { FILE *fi = fopen(fn, "w");
    if (fi != NULL)
      { for (int k = 0; k < num; k++)
	  { tlit *t = &tab[k];
	    fputs(t -> tv, fi);
	    int len = strlen(t -> tv);
	    do { putc('\t', fi); len = (len+8) & ~7; }
	    while (len < 24);	    /* try to be tidy! */
	    fputs((char*) t -> sub, fi); putc('\n', fi);
	  }
	fclose(fi);
      }
    else fprintf(stderr, "Can't create xref file\n");
  }

void substitution::add(char *w1, void *w2)	/* w2 is char* or Char* */
  { while (num >= max)
      { max = (max == 0) ? 8 : max*2;
	tab = (tlit*) realloc(tab, max * sizeof(tlit));
	if (tab == NULL) giveup("No room! (realloc)");
      }
    tlit *t = &tab[num++];
    t -> tv = w1;
    t -> len = strlen(w1);
    t -> sub = w2;
    if (w2 != NULL)
      { /* set bit in bitmap corresponding to first char */
	setbit(bits, w1[0] & 0377);
      }
    t -> used = false;
  }

void substitution::match(char *s1, char *s2, bool &any)
  { int p1 = 0, p2 = 0;
    until (s1[p1] == '\0')
      { if (testbit(bits, s1[p1]))
	  { int maxj; int len = 0;
	    for (int j = 0; j < num; j++)
	      { tlit *t = &tab[j];
		if (t -> len >= len && memcmp(&s1[p1], t -> tv, t -> len) == 0)
		  { /* N.B. ``>='' so we find most local one */
		    len = t -> len; maxj = j;
		  }
	      }
	    if (len > 0)
	      { tlit *t = &tab[maxj];
		if (t -> sub == NULL)
		  { /* no subst defined; copy input chars unchanged */
		    for (int i = 0; i < len; i++) s2[p2++] = s1[p1++];
		  }
		else
		  { char *s = (char*) t -> sub;
		    for (int i = 0; s[i] != '\0'; i++) s2[p2++] = s[i];
		    p1 += len; any = true;
		    t -> used = true;
		  }
	      }
	    else s2[p2++] = s1[p1++];
	  }
	else s2[p2++] = s1[p1++];
      }
    s2[p2] = '\0';
  }

