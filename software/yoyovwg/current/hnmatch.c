#include "vwg.h"
#include <stdio.h>

extern uint debugbits;

struct subst			/* describes a consistent substitution */
  { struct dictnode *meta;	/* substitute for this metanotion      */
    int k1, k2;			/* ptr to first & just past last ssm   */
  };

static struct subst substs[MAXSUBSTS];
static struct hypernotion *pnot;
static int pptr, pend, nsubsts;

forward struct hyperrule *substrule();
forward struct hyperalt *substalt();
forward struct hypernotion *substhn();


global bool hnmatch(pn, hn) struct hypernotion *pn, *hn;
  { bool ok;
    if (debugbits & 0x10) printf("\nhnmatch pn=%h   hn=%h\n", pn, hn);
    pnot = pn; pptr = 0; pend = pn -> hlen;
    if (pend < 0) printf(" BUG3!");
    nsubsts = 0;
    ok = parsehn(hn, 0, hn -> hlen, 0);
    if (debugbits & 0x10) printf("ret %b\n", ok);
    return ok;
  }

static bool parsehn(hnot, hptr, hend, level) struct hypernotion *hnot; int hptr, hend, level;
  { /* Recursive-descent parser for parsing a protonotion (pnot) wrt a hypernotion (hnot) */
    bool ok = true;
    if (debugbits & 0x10)
      { printf("   "); dots(level, stdout); printf("parsehn  ");
	prcontext(pnot, pptr, pend); prcontext(hnot, hptr, hend); putchar('\n');
      }
    if (hptr > hend) printf(" BUG2!");
    until (hptr >= hend || !ok)	    /* can't run off the end of pnot because of the stop marker */
      { unless (pnot -> hdef[pptr].sy == s_ssm) printf(" BUG1!");
	if (hnot -> hdef[hptr].sy == s_ssm)
	  { if (pnot -> hdef[pptr].it_s == hnot -> hdef[hptr].it_s) { pptr++; hptr++; }
	    else ok = false;
	  }
	else
	  { if (level > 0)
	      { ok = parsemeta(hnot, hptr, level+1);
		if (ok) hptr++;
	      }
	    else
	      { struct dictnode *y = hnot -> hdef[hptr].it_z;
		int i = 0;
		until (i >= nsubsts || substs[i].meta == y) i++;
		if (i < nsubsts)
		  { /* enforce consistent substitution */
		    ok = parsehn(pnot, substs[i].k1, substs[i].k2, level+1);
		    if (ok) hptr++;
		  }
		else
		  { /* it's a new metanotion */
		    if (nsubsts >= MAXSUBSTS) printf(" BUG5!");
		    else
		      { substs[nsubsts].meta = y;
			substs[nsubsts].k1 = pptr;
			ok = parsemeta(hnot, hptr, level+1); /* was level */
			if (ok) hptr++;
			substs[nsubsts].k2 = pptr;
			nsubsts++;
		      }
		  }
	      }
	  }
      }
    if (debugbits & 0x10) { printf("   "); dots(level, stdout); printf("returns %b\n", ok); }
    return ok;
  }

static bool parsemeta(hnot, hptr, level) struct hypernotion *hnot; int hptr, level;
  { struct metarhs *x = hnot -> hdef[hptr].it_z -> rhs;
    bool ok = false;
    int p = pptr;
    if (debugbits & 0x10)
      { printf("   "); dots(level, stdout); printf("parsemeta");
	prcontext(pnot, pptr, pend); prcontext(hnot, hptr, hptr+1); putchar('\n');
      }
    until (x == NULL || pptr > p || ok)
      { if (parsehn(x -> hn, 0, x -> hn -> hlen, level+1)) ok = true;	/* recurse */
	x = x -> lnk;
      }
    if (debugbits & 0x10) { printf("   "); dots(level, stdout); printf("returns %b\n", ok); }
    return ok;
  }

static prcontext(hn, ptr, end) struct hypernotion *hn; int ptr, end;
  { struct hypernotion pr;
    pr.hdef = &hn -> hdef[ptr];
    pr.hlen = end - ptr;
    printf("   /%h/", &pr);
  }

global struct hyperrule *substrule(old) struct hyperrule *old;
  { struct hyperrule *new;
    if (nsubsts > 0)
      { new = heap(1, struct hyperrule);
	new -> lhs = substhn(old -> lhs);
	new -> rhs = substalt(old -> rhs);
	new -> type = old -> type; /* inherit type */
      }
    else new = old;
    return new;
  }

static struct hyperalt *substalt(old) struct hyperalt *old;
  { struct hypernotion **vec; struct hyperalt *new; int nr, i;
    nr = old -> rlen;
    vec = heap(nr, struct hypernotion *);
    for (i=0; i < nr; i++) vec[i] = substhn(old -> rdef[i]);
    new = heap(1, struct hyperalt);
    new -> rdef = vec;
    new -> rlen = nr;
    return new;
  }

static struct hypernotion *substhn(old) struct hypernotion *old;
  { struct hypernotion *hn;
    if (old -> hlen < 0) hn = old; /* it's a terminal */
    else
      { int i;
	struct hitem vec[MAXHITEMS];
	int nr = 0; bool ok = true;
	for (i=0; i < old -> hlen && ok; i++)
	  { struct hitem *x = &old -> hdef[i];
	    if (x -> sy == s_ssm)
	      { if (nr < MAXHITEMS) vec[nr++] = *x; /* struct assignment */
		else ok = false;
	      }
	    else
	      { int i = 0;
		until (i >= nsubsts || substs[i].meta == x -> it_z) i++;
		if (i < nsubsts)
		  { int k;
		    for (k = substs[i].k1; k < substs[i].k2 && ok; k++)
		      { if (nr < MAXHITEMS) vec[nr++] = pnot -> hdef[k]; /* struct assignment */
			else ok = false;
		      }
		  }
		else
		  { if (nr < MAXHITEMS) vec[nr++] = *x; /* struct assignment */
		    else ok = false;
		  }
	      }
	  }
	unless (ok) printf(" BUG4!"); /* too long */
	hn = heap(1, struct hypernotion);
	hn -> hdef = heap(nr, struct hitem);
	memcpy(hn -> hdef, vec, nr * sizeof(struct hitem));
	hn -> hlen = nr;
	hn -> fxref = old -> fxref;	/* inherit xref sets */
	hn -> xrefclo = old -> xrefclo;
	hn -> bxref = old -> bxref;
	hn -> flags = old -> flags;	/* inherit flags */
	hn -> lnum = old -> lnum;	/* inherit line number */
      }
    return hn;
  }

global bool eqhn(hn1, hn2) struct hypernotion *hn1, *hn2;
  { /* tests whether two hns are textually equal - used by earley, ftree */
    bool eq;
    if (hn1 -> hlen == hn2 -> hlen)
      { if (hn1 -> hlen >= 0)
	  { int k;
	    eq = true;
	    for (k=0; k < hn1 -> hlen && eq; k++)
	      { struct hitem *x1 = &hn1 -> hdef[k], *x2 = &hn2 -> hdef[k];
		if (x1 -> sy == x2 -> sy)
		  { switch (x1 -> sy)
		      { case s_ssm:	unless (x1 -> it_s == x2 -> it_s) eq = false; break;
			case s_meta:	unless (x1 -> it_z == x2 -> it_z) eq = false; break;
		      }
		  }
		else eq = false;
	      }
	  }
	else
	  { eq = (hn1 -> term == hn2 -> term);
	  }
      }
    else eq = false;
    return eq;
  }

