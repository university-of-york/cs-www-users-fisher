/* phoneman -- telephone manager
   A.J. Fisher	 January 1996 */

#include <stdio.h>
#include <fishaudio.h>

#include "phoneman.h"

static void appends(char*, char*, int&);


global char *substparams(char *v1, eproc error, char *nval, char *uval, char *tval)
  { char v2[MAXSTR+1]; int n1 = 0, n2 = 0; bool any = false;
    until (v1[n1] == '\0')
      { int c = v1[n1++];
	if (c == '$')
	  { char var[MAXSTR+1]; int p = n1, k = 0;
	    until (v1[n1] == '/' || v1[n1] == '.' || v1[n1] == ':' || v1[n1] == ' ' || v1[n1] == '\t' || v1[n1] == '\0')
	      var[k++] = v1[n1++];
	    var[k] = '\0';
	    if (seq(var,"num"))
	      { if (nval != NULL) { appends(nval, v2, n2); any = true; }
		else appends("$num", v2, n2);   /* postpone until later */
	      }
	    else if (seq(var,"uid"))
	      { if (uval != NULL) { appends(uval, v2, n2); any = true; }
		else appends("$uid", v2, n2);   /* postpone until later */
	      }
	    else if (seq(var,"time"))
	      { if (tval != NULL) { appends(tval, v2, n2); any = true; }
		else appends("$time", v2, n2);  /* postpone until later */
	      }
	    else
	      { char *x = getenv(var);
		if (x != NULL && n2 + strlen(x) <= MAXSTR) { appends(x, v2, n2); any = true; }
		else error("variable $%s undefined", var);
	      }
	  }
	else v2[n2++] = c;
      }
    v2[n2] = '\0';
    if (any) v1 = copystr(v2);
    return v1;
  }

static void appends(char *s, char *v, int &n)
  { int k = 0;
    until (s[k] == '\0') v[n++] = s[k++];
    v[n] = '\0';
  }

global char *copystr(char *s1)
  { int len = strlen(s1);
    char *s2 = new char[len+1];
    return strcpy(s2, s1);
  }

global void giveup(char *msg, word p1, word p2)
  { fprintf(stderr, "phoneman: "); fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    exit(1);
  }

tone::tone(float f)
  { vec = new uchar[TONELEN];
    for (int i=0; i < TONELEN; i++)
      { float theta = TWOPI * f * (double) i / (double) SAMPLERATE;
	vec[i] = mu_compress((int) ((1 << 14) * sin(theta)));
      }
  }

tone::~tone()
  { delete vec;
  }

pnode::pnode(symbol x1, pnode *x2, pnode *x3)
  { h1 = x1; h2 = x2; h3 = x3;
  }

pnode::pnode(symbol x1, char *s1, char *s2)
  { h1 = x1; str1 = s1; str2 = s2;
  }

pnode::pnode(symbol x1, int n)
  { h1 = x1; num = n;
  }

