#include "vwg.h"
#include <stdio.h>

extern struct dictnode *metalist;

extern struct dictnode *newmetanotion();	/* from common */

forward struct metarhs *definemeta();


global checkmetasyntax()
  { struct dictnode *z;
    for (z = metalist; z != NULL; z = z -> nxt)
      { if (z -> rhs == NULL)
	  { char *s = z -> str;
	    int nc = strlen(s); int n = nc;
	    while (s[n-1] >= '0' && s[n-1] <= '9') n--;
	    if (n < nc)
	      { char s1[MAXNAMELEN+1]; int i; struct dictnode *z1;
		for (i=0; i<n; i++) s1[i] = s[i];
		s1[n] = '\0';
		z1 = newmetanotion(s1);
		if (z1 -> rhs == NULL) error("metanotion %s is used, but %s is not defined", s, s1);
		z -> rhs = definemeta(z1);
	      }
	    else error("metanotion %s is not defined", s);
	  }
      }
  }

static struct metarhs *definemeta(y) struct dictnode *y;
  { struct hitem *vec; struct hypernotion *hn; struct metarhs *rhs;
    vec = heap(1, struct hitem);
    vec[0].sy = s_meta;
    vec[0].it_z = y;
    hn = heap(1, struct hypernotion);
    hn -> hlen = 1;
    hn -> hdef = vec;
    hn -> flags = 0;
    hn -> lnum = -1;
    rhs = heap(1, struct metarhs);
    rhs -> hn = hn;
    rhs -> lnk = NULL;
    return rhs;
  }

