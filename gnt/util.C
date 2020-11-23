#include <stdio.h>
#include <string.h>

#include "gnt.h"


template<class T> global void vector<T>::add(T x)
  { while (num >= max)
      { max = (3*max)/2;
	vec = (T*) realloc(vec, max * sizeof(T));
	if (vec == NULL) hfatal("No room! (vector)");
      }
    vec[num++] = x;
  }

template class vector<Context>;
template class vector<int>;

global char *copystring(char *s)
  { int len = strlen(s);
    return strcpy(new char[len+1], s);
  }

global void hfatal(char *msg, word p1, word p2, word p3, word p4)
  { char temp [MAXSTR+1]; sprintf(temp, msg, p1, p2, p3, p4);
    unless (hadtitle) prtitle("Error!");
    prheading("Error!", 3);
    prheading(temp, 4);
    exit(0);
  }

global void prtitle(char *tit)
  { printf("<title> %s </title>\n", tit);
    hadtitle = true;
  }

global void prheading(char *hdg, int n)
  { printf("<h%d> %s </h%d>\n", n, hdg, n);
  }

