#include "vwg.h"
#include <stdio.h>

static int anon;

extern struct dictnode *metalist, *ghead;
extern uint debugbits;

extern struct dictnode *newmetanotion();	/* from common */

forward struct hypernotion *copyhn();


global factorize()
  { bool again;
    anon = 0;
    do
      { struct dictnode *z;
	again = false;
	if (debugbits & 1) fprintf(stderr, "\nROUND AGAIN\n");
	for (z = metalist; z != NULL; z = z -> nxt)
	  { unless (z == ghead)
	      { struct dictnode *ml = metalist;
		if (debugbits & 1) fprintf(stderr, "\nFactorizing %s\n", z -> str);
		factoralts(z);
		if (metalist != ml)
		  { if (debugbits & 1) fprintf(stderr, "   After:  %s:: %m.\n", z -> str, z -> rhs);
		    again = true;	/* if we've factorized, must go round again */
		  }
	      }
	  }
      }
    while (again);
  }

static factoralts(z) struct dictnode *z;
  { struct metarhs *yi, *yj;
    for (yi = z -> rhs; yi != NULL; yi = yi -> lnk)
      { for (yj = yi -> lnk; yj != NULL; yj = yj -> lnk)
	  { struct hypernotion *hni = yi -> hn, *hnj = yj -> hn;
	    int n = substringlen(hni, hnj);
	    if (n > 0)
	      { char s[MAXNAMELEN+1]; struct dictnode *mn; struct hypernotion *hn;
		sprintf(s, "%s_%d", z -> str, ++anon);  /* make unique name */
		mn = newmetanotion(s);
		addalt(mn, copyhn(hni, n, hni -> hlen));
		addalt(mn, copyhn(hnj, n, hnj -> hlen));
		if (debugbits & 1) fprintf(stderr, "   New:    %s:: %m.\n", mn -> str, mn -> rhs);
		hn = copyhn(hni, 0, n+1);
		hn -> hdef[n].sy = s_meta;
		hn -> hdef[n].it_z = mn;
		yi -> hn = hn;		/* overwrite "i" hypernotion */
		deletealt(z, hnj);	/* delete "j" hypernotion    */
	      }
	  }
      }
  }

static int substringlen(hn1, hn2) struct hypernotion *hn1, *hn2;
  { /* return length of leading substring common to both hypernotions */
    int n = 0;
    while (n < hn1 -> hlen && n < hn2 -> hlen && samessm(&hn1 -> hdef[n], &hn2 -> hdef[n])) n++;
    return n;
  }

static bool samessm(x1, x2) struct hitem *x1, *x2;
  { return (x1 -> sy == s_ssm && x2 -> sy == s_ssm && x1 -> it_s == x2 -> it_s) ||
	   (x1 -> sy == s_meta && x2 -> sy == s_meta && x1 -> it_z == x2 -> it_z);
  }

static struct hypernotion *copyhn(old, k1, k2) struct hypernotion *old; int k1, k2;
  { struct hypernotion *new; int i;
    new = heap(1, struct hypernotion);
    new -> hlen = k2-k1;
    new -> hdef = heap(new -> hlen, struct hitem);
    for (i=0; i < new -> hlen; i++)
      new -> hdef[i] = old -> hdef[k1+i];
    new -> flags = old -> flags;
    new -> lnum = old -> lnum;
    return new;
  }

static addalt(z, hn) struct dictnode *z; struct hypernotion *hn;
  { /* add new alternative on to end of list */
    struct metarhs **ptr = &z -> rhs;
    until ((*ptr) == NULL) ptr = &(*ptr) -> lnk;
    *ptr = heap(1, struct metarhs);
    (*ptr) -> hn = hn;
    (*ptr) -> lnk = NULL;
  }

static deletealt(z, hn) struct dictnode *z; struct hypernotion *hn;
  { /* delete alternative from list */
    struct metarhs **ptr = &z -> rhs;
    until ((*ptr) -> hn == hn) ptr = &(*ptr) -> lnk;
    *ptr = (*ptr) -> lnk;
  }

