#include "vwg.h"
#include <stdio.h>

#define NULLBIT (1 << 31)
#define STOPBIT (1 << 29)

extern struct ruleset *hypersyntax;
extern struct dictnode *metalist, *ghead;
extern uint debugbits;
extern int linenum;

static bool again;	/* for computing closure, used in various places */

extern error();		/* from common	*/
extern bool ispn();	/* from common	*/
extern bool hnmatch();	/* from hnmatch */

forward uint starters();


global checkgrammar()
  { findstarters();
    checkll1();
    checklhsides();
    deducelrtypes();
    makexref();
    makexrefclosure();
    deduceptype();
    checknotlrec();
  }

static findstarters()
  { struct dictnode *z;
    for (z = metalist; z != NULL; z = z -> nxt) z -> sta = z -> fol = 0;
    do
      { again = false;
	for (z = metalist; z != NULL; z = z -> nxt)
	  { int ln = linenum;
	    struct metarhs *y;
	    for (y = z -> rhs; y != NULL; y = y -> lnk)
	      { struct hypernotion *hn;
		struct dictnode *metavec[MAXSUBSTS];
		int nmetas = 0;
		int j; uint m;
		hn = y -> hn;
		linenum = hn -> lnum;
		m = starters(hn, 0);
		addbits(m, &z -> sta);
		for (j=0; j < hn -> hlen; j++)
		  { struct hitem *x = &hn -> hdef[j];
		    if (x -> sy == s_meta)
		      { struct dictnode *zj = x -> it_z;
			int i = 0;
			while (i < nmetas && metavec[i] != zj) i++;
			if (i >= nmetas)
			  { /* not yet seen in this hypernotion; record followers */
			    uint mj = starters(hn, j+1);
			    addbits(mj & ~NULLBIT, &zj -> fol);
			    if (mj & NULLBIT) addbits(z -> fol, &zj -> fol);
			    if (nmetas < MAXSUBSTS) metavec[nmetas++] = zj;
			    else error("too many different metanotions in hypernotion ``%h''", hn);
			  }
		      }
		  }
	      }
	    linenum = ln;
	  }
      }
    while (again);
  }

static addbits(b1, bb2) uint b1; uint *bb2;
  { /* add bits b1 into b2 */
    if (b1 & ~*bb2)
      { *bb2 |= b1;
	again = true;
       }
  }

static checkll1()
  { struct dictnode *z;
    for (z = metalist; z != NULL; z = z -> nxt)
      { unless (z == ghead)
	  { int ln = linenum;
	    uint m1 = 0, clash = 0;
	    struct metarhs *y;
	    for (y = z -> rhs; y != NULL; y = y -> lnk)
	      { struct hypernotion *hn; uint m2;
		hn = y -> hn;
		linenum = hn -> lnum;
		m2 = starters(hn, 0);
		if (m2 & NULLBIT) m2 |= z -> fol;
		if (m1 & NULLBIT)
		  error("metanotion %s: nullable alternative does not come last; re-order alternatives!", z -> str);
		clash |= (m1 & m2);
		m1 |= m2;
	      }
	    if (clash) error("metanotion %s is not LL(1); clashes \"%t\" [R1]", z -> str, clash);
	    linenum = ln;
	  }
      }
  }

static checklhsides()
  { int ln = linenum;
    struct ruleset *rs;
    for (rs = hypersyntax; rs != NULL; rs = rs -> lnk)
      { struct hypernotion *hn; uint m;
	hn = rs -> hr -> lhs;
	linenum = hn -> lnum;
	m = starters(hn, 0);
	if (m & STOPBIT) error("lhs of hyperrule ``%h'' matches empty protonotion", hn);
      }
    linenum = ln;
  }

static deducelrtypes()	/* classify each hyperrule as type X, L, R, LR */
  { int ln = linenum;
    struct ruleset *rs;
    for (rs = hypersyntax; rs != NULL; rs = rs -> lnk)
      { struct hyperrule *hr = rs -> hr;
	linenum = hr -> lhs -> lnum;
	if (debugbits & 1) printf("\nRule ``%h: %a.''\n", hr -> lhs, hr -> rhs);
	hr -> type = type_LR; /* assume the best case */
	check_lb(hr);
	check_rb(hr);
	if (debugbits & 1) printtype(hr -> type);
	if ((hr -> type & type_LR) == 0) error("hyperrule ``%h'' is type X [R2]", hr -> lhs);
      }
    linenum = ln;
  }

static check_lb(hr) struct hyperrule *hr;
  { /* check left-bound-ness, i.e. whether each MN on LHS appears also on RHS */
    struct hypernotion *lhs = hr -> lhs;
    struct hyperalt *rhs = hr -> rhs;
    int i, j, k;
    for (i=0; i < lhs -> hlen; i++)
      { struct hitem *lx = &lhs -> hdef[i];
	if (lx -> sy == s_meta)
	  { bool found = false;
	    for (k=0; k < rhs -> rlen && !found; k++)
	      { struct hypernotion *rhn = rhs -> rdef[k];
		if (rhn -> hlen >= 0) /* unless it's a representation rule */
		  { for (j=0; j < rhn -> hlen && !found; j++)
		      { struct hitem *rx = &rhn -> hdef[j];
			if (rx -> sy == s_meta)
			  { if (lx -> it_z == rx -> it_z) found = true;
			  }
		      }
		  }
	      }
	    unless (found)
	      { if (debugbits & 1) printf("   metanotion %s on lhs but not on rhs\n", lx -> it_z -> str);
		hr -> type &= ~type_L;
	      }
	  }
      }
  }

static check_rb(hr) struct hyperrule *hr;
  { /* check right-bound-ness, i.e. whether each MN on RHS appears also on LHS */
    int i, j, k;
    struct hypernotion *lhs = hr -> lhs;
    struct hyperalt *rhs = hr -> rhs;
    for (k=0; k < rhs -> rlen; k++)
      { struct hypernotion *rhn = rhs -> rdef[k];
	if (rhn -> hlen >= 0) /* unless it's a representation rule */
	  { for (j=0; j < rhn -> hlen; j++)
	      { struct hitem *rx = &rhn -> hdef[j];
		if (rx -> sy == s_meta)
		  { bool found = false;
		    for (i=0; i < lhs -> hlen && !found; i++)
		      { struct hitem *lx = &lhs -> hdef[i];
			if (lx -> sy == s_meta)
			  { if (lx -> it_z == rx -> it_z) found = true;
			  }
		      }
		    unless (found)
		      { if (debugbits & 1) printf("   metanotion %s on rhs but not on lhs\n", rx -> it_z -> str);
			hr -> type &= ~type_R;
		      }
		  }
	      }
	  }
      }
  }

static makexref()
  { int ln = linenum;
    struct ruleset *rs;
    /* compute linkage throught 1st rhs component (Y), and check R3 */
    for (rs = hypersyntax; rs != NULL; rs = rs -> lnk)
      { struct hyperrule *hr = rs -> hr;
	hr -> lhs -> bxref = NULL;
      }
    for (rs = hypersyntax; rs != NULL; rs = rs -> lnk)
      { struct hyperrule *hr = rs -> hr; int k;
	linenum = hr -> lhs -> lnum;
	if (debugbits & 2) printf("\nRule ``%h: %a.''\n", hr -> lhs, hr -> rhs);
	if (hr -> type == type_L) testbound(hr);
	/* compute cross-reference */
	for (k=0; k < hr -> rhs -> rlen; k++)
	  { struct hypernotion *rhn = hr -> rhs -> rdef[k];
	    rhn -> xrefclo = rhn -> bxref = NULL;
	    if (rhn -> hlen == 1) rhn -> flags |= hn_dnull; /* if it's ``*'', set dnull bit */
	    if ((debugbits & 2) && (rhn -> flags & hn_dnull)) printf("   setting dnull on hn ``%h''\n", rhn);
	    if (rhn -> hlen > 1)  /* if it's non-terminal, non-EMPTY */
	      { struct ruleset *z;
		bool any = false;
		for (z = hypersyntax; z != NULL; z = z -> lnk)
		  { struct hyperrule *xhr = z -> hr;
		    struct hypernotion *lhn = xhr -> lhs;
		    if (supermatch(lhn, rhn))
		      { if ((hr -> type & type_LR) == type_L &&
			    (xhr -> type & type_LR) == type_R) checkr3(rhn, lhn, hr -> lhs);
			addfxref(xhr, rhn); /* add it to xrefclo */
			if (k == 0) addbxref(hr, lhn);
			any = true;
		      }
		  }
		if (!any && ispn(rhn)) error("protonotion ``%h'' used but not defined", rhn);
	      }
	    rhn -> fxref = rhn -> xrefclo;
	  }
      }
    linenum = ln;
  }

static testbound(hr) struct hyperrule *hr;
  { /* see which MNs in rhn have occurred previously in rhs of hr */
    struct dictnode *metavec[MAXSUBSTS];
    int nmetas = 0; int k;
    for (k=0; k < hr -> rhs -> rlen; k++)
      { struct hypernotion *rhn = hr -> rhs -> rdef[k];
	int i;
	for (i=0; i < rhn -> hlen; i++)
	  { struct hitem *hx = &rhn -> hdef[i];
	    if (hx -> sy == s_meta)
	      { struct dictnode *zj = hx -> it_z;
		int j = 0;
		while (j < nmetas && metavec[j] != zj) j++;
		if (j >= nmetas)
		  { hx -> isbound = false; /* it's a new metanotion */
		    if (nmetas < MAXSUBSTS) metavec[nmetas++] = zj;
		    else error("too many different metanotions in rhs of hyperrule ``%h''", hr -> lhs);
		  }
		else hx -> isbound = true;
	      }
	  }
      }
  }

static checkr3(rhn, lhn, hlhs) struct hypernotion *rhn, *lhn, *hlhs;
  { int i; bool ok;
    if (debugbits & 1) printf("   Possible R3 violation ``%h'' :: ``%h''\n", rhn, lhn);
    /* see whether all MNs in rhn have occurred previously in rhs of hr */
    ok = true;
    for (i=0; i < rhn -> hlen; i++)
      { struct hitem *hx = &rhn -> hdef[i]; bool bound;
	if (hx -> sy == s_meta)
	  { if (debugbits & 1) printf("      is %s bound?  %b\n", hx -> it_z -> str, hx -> isbound);
	    unless (hx -> isbound) ok = false;
	  }
      }
    unless (ok) error("hyperrule ``%h'' is type L :: ``%h'' is type R [R3]", hlhs, lhn);
  }

static makexrefclosure()
  { /* compute closure of linkage (Y+) */
    do
      { int ln = linenum;
	struct ruleset *rs;
	again = false;
	if (debugbits & 2) printf("\nRound again\n===========\n");
	for (rs = hypersyntax; rs != NULL; rs = rs -> lnk)
	  { struct hyperrule *hr = rs -> hr; int k;
	    linenum = hr -> lhs -> lnum;
	    if (debugbits & 2) printf("\nRule ``%h: %a.''\n", hr -> lhs, hr -> rhs);
	    for (k=0; k < hr -> rhs -> rlen; k++)
	      { struct hypernotion *rhn = hr -> rhs -> rdef[k];
		struct ruleset *z;
		for (z = rhn -> fxref; z != NULL; z = z -> lnk)
		  { struct hyperrule *xhr = z -> hr;
		    addfxrefset(xhr, rhn);
		  }
	      }
	  }
	linenum = ln;
      }
    while (again);
  }

static addfxrefset(hr, hn) struct hyperrule *hr; struct hypernotion *hn;
  { /* add xrefclo set of leading component(s) of hr to xrefclo set of hn */
    int k; bool allnull = true;
    for (k=0; k < hr -> rhs -> rlen && allnull; k++)
      { struct hypernotion *rhn = hr -> rhs -> rdef[k];
	struct ruleset *z;
	for (z = rhn -> xrefclo; z != NULL; z = z -> lnk)
	  { struct hyperrule *xhr = z -> hr;
	    addfxref(xhr, hn);
	  }
	unless (rhn -> flags & hn_dnull) allnull = false;
      }
    if (allnull && !(hn -> flags & hn_dnull))
      { if (debugbits & 2) printf("   propagating dnull to hn ``%h''\n", hn);
	hn -> flags |= hn_dnull;
	again = true;
      }
  }

static addfxref(hr, hn) struct hyperrule *hr; struct hypernotion *hn;
  { /* add hr to xrefclo set of hn */
    newxref(hr, &hn -> xrefclo);
    if (isrepr(hr))
      { hn -> flags |= hn_dterm;    /* hn derives terminal word */
	if (hn -> flags & (hn_pospred | hn_negpred))
	  error("rhs hypernotion ``%h'' looks like a predicate, but derives a non-empty terminal word", hn);
      }
  }

static addbxref(hr, hn) struct hyperrule *hr; struct hypernotion *hn;
  { /* add hr to bxref set of hn */
    newxref(hr, &hn -> bxref);
  }

static newxref(hr, rsp) struct hyperrule *hr; struct ruleset **rsp;
  { unless (inxrefset(hr, *rsp))
      { struct ruleset *rs = heap(1, struct ruleset);
	rs -> hr = hr;
	rs -> lnk = *rsp;
	*rsp = rs;
	again = true;
      }
  }

static bool supermatch(hn1, hn2) struct hypernotion *hn1, *hn2;
  { return ispn(hn1) ? hnmatch(hn1, hn2) :
	   ispn(hn2) ? hnmatch(hn2, hn1) :
	   possiblematch(hn1, hn2);
  }

static bool possiblematch(hn1, hn2) struct hypernotion *hn1, *hn2;
  { /* try to match; we know that both hn1 and hn2 contain metanotions */
    bool match;
    int p1 = 0, q1 = hn1 -> hlen;
    int p2 = 0, q2 = hn2 -> hlen;
    struct hitem *hd1 = hn1 -> hdef, *hd2 = hn2 -> hdef;
    /* chop off identical leading & trailing ssms */
    while (hd1[p1].sy == s_ssm && hd2[p2].sy == s_ssm && hd1[p1].it_s == hd2[p2].it_s) { p1++; p2++; }
    while (hd1[q1-1].sy == s_ssm && hd2[q2-1].sy == s_ssm && hd1[q1-1].it_s == hd2[q2-1].it_s) { q1--; q2--; }
    if ((hd1[p1].sy == s_meta || hd2[p2].sy == s_meta) && (hd1[q1-1].sy == s_meta || hd2[q2-1].sy == s_meta))
      { uint mp = starters(hn1, p1) & starters(hn2, p2);
	match = (mp != 0); /* probably! */
      }
    else match = false;
    return match;
  }

static uint starters(hn, k) struct hypernotion *hn; int k;
  { uint m = 0;
    if (hn -> hlen < 0) error("bug 1");
    else
      { bool allnull = true; int i;
	for (i=k; i < hn -> hlen && allnull; i++)
	  { struct hitem *x = &hn -> hdef[i];
	    if (x -> sy == s_ssm)
	      { int c = x -> it_s;
		int b = (c >= 'a' && c <= 'z') ? (c-'a') :
			(c == '<') ? 26 :
			(c == '>') ? 27 :
			(c == '_') ? 28 :
			(c == '*') ? 29 : -1;
		if (b >= 0) m |= (1 << b);
		else error("bug 2 (%c)", c);
		allnull = false;
	      }
	    if (x -> sy == s_meta)
	      { struct dictnode *z1 = x -> it_z;
		m |= (z1 -> sta & ~NULLBIT);
		unless (z1 -> sta & NULLBIT) allnull = false;
	      }
	  }
	if (allnull) m |= NULLBIT;
      }
    return m;
  }

static deduceptype()
  { int ln = linenum;
    struct ruleset *rs;
    for (rs = hypersyntax; rs != NULL; rs = rs -> lnk)
      { struct hyperrule *hr = rs -> hr;
	linenum = hr -> lhs -> lnum;
	if (debugbits & 1) printf("\nRule ``%h: %a.''\n", hr -> lhs, hr -> rhs);
	if (isrepr(hr)) hr -> type |= type_T;	/* representation rule */
	else
	  { bool dterm = false; int k;
	    for (k=0; k < hr -> rhs -> rlen && !dterm; k++)
	      { struct hypernotion *rhn = hr -> rhs -> rdef[k];
		if (rhn -> flags & hn_dterm) dterm = true;
	      }
	    if (dterm) hr -> type |= type_T;
	  }
	if (debugbits & 1) printtype(hr -> type);
	if ((hr -> type & type_T) && (hr -> lhs -> flags & (hn_pospred | hn_negpred)))
	  error("hyperrule ``%h: %a.'' looks like a predicate, but derives a non-empty terminal word",
		hr -> lhs, hr -> rhs);
      }
    linenum = ln;
  }

static bool isrepr(hr) struct hyperrule *hr;
  { /* is hr a representation rule? */
    return (hr -> rhs -> rlen == 1) && (hr -> rhs -> rdef[0] -> hlen < 0);
  }

static printtype(t) uint t;
  { printf("   is type ");
    if ((t & type_LR) == 0) putchar('X');
    else
      { if (t & type_L) putchar('L');
	if (t & type_R) putchar('R');
      }
    if (t & type_T) putchar('T');
    putchar('\n');
  }

static checknotlrec()	/* check R4 */
  { int ln = linenum;
    struct ruleset *rs;
    for (rs = hypersyntax; rs != NULL; rs = rs -> lnk)
      { struct hyperrule *hr = rs -> hr;
	linenum = hr -> lhs -> lnum;
	if (debugbits & 2) printxref(hr);
	unless (hr -> type & type_T)
	  { /* it's a predicate */
	    int nlhs, k;
	    if (debugbits & 2) printf("   *** PREDICATE***\n");
	    unless (hr -> type & type_R) error("predicate ``%h'' is not type R [PR1]", hr -> lhs);
#ifdef CHECK_PR2
	    nlhs = numssms(hr -> lhs);
	    for (k=0; k < hr -> rhs -> rlen; k++)
	      { struct hypernotion *rhn = hr -> rhs -> rdef[k];
		if (numssms(rhn) >= nlhs) error("predicate ``%h'' is not strictly type R [PR2]", hr -> lhs);
	      }
#endif
	  }
	else
	  { bool allnull = true; int k;
	    for (k=0; k < hr -> rhs -> rlen && allnull; k++)
	      { struct hypernotion *rhn = hr -> rhs -> rdef[k];
		if (inxrefset(hr, rhn -> xrefclo)) error("hyperrule ``%h'' is left-recursive [R4]", hr -> lhs);
		unless (rhn -> flags & hn_dnull) allnull = false;
	      }
	  }
      }
    linenum = ln;
  }

static int numssms(hn) struct hypernotion *hn;
  { int num = 0;
    if (hn -> hlen >= 0)
      { int k;
	for (k=0; k < hn -> hlen; k++)
	  if (hn -> hdef[k].sy == s_ssm) num++;
      }
    else error("bug 3");
    return num;
  }

static printxref(hr) struct hyperrule *hr;
  { int k;
    printf("\nRule %h:\n", hr -> lhs);
    for (k=0; k < hr -> rhs -> rlen; k++)
      { struct hypernotion *rhn = hr -> rhs -> rdef[k];
	printf("   rhs hn ``%h'':\n", rhn);
	printf("      FXref is\n"); printruleset(rhn -> fxref, 9);
	printf("      Closed FXref is\n"); printruleset(rhn -> xrefclo, 9);
	if (rhn -> flags & hn_dnull) printf("      (derives empty)\n");
	if (rhn -> flags & hn_dterm) printf("      (derives terminal word)\n");
      }
    printf("   BXref is\n"); printruleset(hr -> lhs -> bxref, 6);
  }

static printruleset(rs, nsp) struct ruleset *rs; int nsp;
  { struct ruleset *z;
    for (z = rs; z != NULL; z = z -> lnk)
      { struct hyperrule *hr = z -> hr;
	int k;
	for (k=0; k<nsp; k++) putchar(' ');
	printf("%h: %a.\n", hr -> lhs, hr -> rhs);
      }
  }

static bool inxrefset(hr, rs) struct hyperrule *hr; struct ruleset *rs;
  { until (rs == NULL || rs -> hr == hr) rs = rs -> lnk;
    return (rs != NULL);
  }

