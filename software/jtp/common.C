/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include <string.h>
#include "jtp.h"
#include "tcodes.h"

#define MAXTPROG    16384
#define MAXFPROG    65536

static void appends(char*, int&, char*);


global float bitsfp(uint u)
  { /* return the float corresponding to a bit pattern */
    union { uint u; float f; } w;
    w.u = u;
    return w.f;
  }

global double bitsdp(uint hi, uint lo)
  { /* return the double corresponding to a bit pattern */
    union { uint u[2]; double d; } w;
    w.u[0] = hi; w.u[1] = lo;
    return w.d;
  }

global Class *lookupclass(char *name)
  { int n = 0;
    until (n >= numitems || (items[n].ty == ty_class && seq(items[n].cl -> name, name))) n++;
    return (n < numitems) ? items[n].cl : NULL;
  }

global char *mkname(fullname *fn)
  { /* given a full name, create a C++ style name e.g. "class::name(IFI)" */
    char buf[MAXSTRING+1]; int p = 0;
    appends(buf, p, fn -> cl); appends(buf, p, "::"); appends(buf, p, fn -> id);
    char *sig = fn -> sig;
    if (sig[0] == '(')
      { int k = 0;
	until (sig[k] == ')' || sig[k] == '\0') buf[p++] = sig[k++];
	if (sig[k] == '\0') warn("bad method signature! ``%s''", sig);
	buf[p++] = ')';
      }
    buf[p++] = '\0';
    return lookupstring(buf);
  }

static void appends(char *buf, int &p, char *s)
  { int k = 0;
    until (s[k] == '\0') buf[p++] = s[k++];
  }

global char *trsig(char *sig)
  { /* mangle the signature */
    char buf[MAXSTRING+1]; int k = 0;
    until (sig[k] == '\0')
      { buf[k] = (sig[k] == '[') ? 'R' : sig[k];	/* R for Row (looks nicer in external names) */
	k++;
      }
    buf[k] = '\0';
    return lookupstring(buf);
  }

global int objectsize(char sch)
  { switch (sch)
      { default:
	    warn("bug: bad signature char ``%c''", sch);
	    return 0;

	case 'V':
	    return 0;

	case 'B':   case 'C':	case 'F':   case 'I':	case 'S':   case 'Z':
	case 'L':   case 'R':
	    return 1;

	case 'D':   case 'J':
	    return 2;
      }
  }

global tprog::tprog()
  { code = new tinstr*[MAXTPROG];
    clen = 0;
  }

global tprog::~tprog()
  { for (int i = 0; i < clen; i++) delete code[i];
    delete code;
  }

global void tprog::gen(int op, int m1, word p1, int m2, word p2)
  { if (clen >= MAXTPROG) giveup("program is too big! (1)");
    code[clen++] = new tinstr(op, m1, p1, m2, p2);
  }

global void tprog::append(tprog *x)
  { int nw = x -> clen;
    if (clen + nw > MAXTPROG) giveup("program is too big! (2)");
    memmove(&code[clen], x -> code, nw * sizeof(tinstr*));
    x -> clen = 0;	/* to stop x's elements from being deleted! */
    clen += nw;
  }

global tinstr *tprog::last()
  { return (clen > 0) ? code[clen-1] : NULL;	/* most recently generated tinstr */
  }

global fprog::fprog()
  { code = new uchar[MAXFPROG];
    clen = 0;
  }

global fprog::~fprog()
  { delete code;
  }

global void fprog::gen(uchar x)
  { if (clen >= MAXFPROG) giveup("program is too big! (3)");
    code[clen++] = x;
  }

struct dnode
  { dnode *lft, *rgt;
    char *str;
  };

static dnode *dictionary = NULL;	/* statically init'ed */

global char *lookupstring(char *s)
  { dnode **x = &dictionary; bool found = false;
    until (*x == NULL || found)
      { int k = strcmp((*x) -> str, s);
	if (k < 0) x = &(*x) -> lft;
	if (k > 0) x = &(*x) -> rgt;
	if (k == 0) found = true;
      }
    unless (found)
      { dnode *y = new dnode;
	y -> str = copystr(s);
	y -> lft = y -> rgt = NULL;
	*x = y;
      }
    return (*x) -> str;
  }

global char *copystr(char *s)
  { char *sx = new char[strlen(s) + 1];
    return strcpy(sx, s);
  }

global formvec::formvec(int mx)
  { max = mx; num = 0;
    vec = new form*[mx];
  }

global formvec::~formvec()
  { /* don't delete constituent forms! */
    delete vec;
  }

global formvec *formvec::clone()
  { formvec *fv = new formvec(max);
    for (int i = 0; i < num; i++) fv -> add(vec[i]);
    return fv;
  }

global void formvec::add(form *x)
  { while (num >= max)
      { max = (3*max)/2;
	vec = (form**) realloc(vec, max * sizeof(form*));
	if (vec == NULL) giveup("No room! (realloc)");
      }
    vec[num++] = x;
  }

global void giveup(char *msg, word p1, word p2, word p3)
  { warn(msg, p1, p2, p3);
    exit(1);
  }

global void warn(char *msg, word p1, word p2, word p3)
  { fprintf(stderr, "jtp: "); fprintf(stderr, msg, p1, p2, p3); putc('\n', stderr);
    anyerrors = true;
  }

