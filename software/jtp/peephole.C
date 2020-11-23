/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include "jtp.h"
#include "tcodes.h"

static tinstr **code;
static int ilen, iptr, optr;

static void apply(tprog*, proc);
static void pass_x1(), mulconst(int), divconst(int);
static void pass_x2();
static void pass_y1();
static bool isldop(int);
static void pass_y2(), pass_y3();
static void gen(int, int = m_none, word = 0, int = m_none, word = 0);


global void peephole(tprog *tp)
  { /* peephole optimization of Transputer code in-place */
    proc ps[] = { pass_x1, pass_x2, pass_y1, pass_y2, pass_y3 };
    for (int i = 0; i < 5; i++) apply(tp, ps[i]);
  }

static void apply(tprog *tp, proc p)
  { code = tp -> code;
    ilen = tp -> clen;
    iptr = optr = 0;
    while (iptr < ilen) p();
    tp -> clen = optr;
  }

static void pass_x1()
  { if (iptr+1 < ilen)
      { tinstr *ti1 = code[iptr], *ti2 = code[iptr+1];
	if (ti1 -> op == t_ldc && ti1 -> m1 == m_num)
	  { int n = ti1 -> p1.n;
	    if (n == MINT)
	      { gen(t_mint);
		iptr++;
	      }
	    else
	      { switch (ti2 -> op)
		  { default:
			gen(t_ldc, m_num, n);
			iptr++;
			break;

		    case t_sum:
			unless (n == 0) gen(t_adc, m_num, n);
			iptr += 2;
			break;

		    case t_diff:
			unless (n == 0) gen(t_adc, m_num, -n);
			iptr += 2;
			break;

		    case t_prod:
			mulconst(n);
			iptr += 2;
			break;

		    case t_div:
			divconst(n);
			iptr += 2;
			break;
		  }
	      }
	  }
	else code[optr++] = code[iptr++];
      }
    else code[optr++] = code[iptr++];
  }

static void mulconst(int n)
  { switch (n)
      { case 0:
	    warn("multiplication by zero!");    /* not an error, but we probably want to know */
	    break;

	case 1:
	    break;

	default:
	    if ((n & (n-1)) == 0)
	      { uint m = n; int k = 0;
		until (m == 1) { m >>= 1; k++; }
		gen(t_ldc, m_num, k);
		gen(t_shl);
	      }
	    else
	      { gen(t_ldc, m_num, n);
		gen(t_prod);
	      }
	    break;
      }
  }

static void divconst(int n)
  { switch (n)
      { case 0:
	    warn("division by zero!");
	    break;

	case 1:
	    break;

	default:
	    if ((n & (n-1)) == 0)
	      { uint m = n; int k = 0;
		until (m == 1) { m >>= 1; k++; }
		gen(t_ldc, m_num, k);
		gen(t_shr);
	      }
	    else
	      { gen(t_ldc, m_num, n);
		gen(t_div);
	      }
	    break;
      }
  }

static void pass_x2()
  { if (iptr+1 < ilen)
      { tinstr *ti1 = code[iptr], *ti2 = code[iptr+1];
	if (ti1 -> op == t_adc && ti1 -> m1 == m_num && ti2 -> op == t_eqc && ti2 -> m1 == m_num && ti2 -> p1.n == 0)
	  { int n = ti1 -> p1.n;
	    gen(t_eqc, m_num, -n);
	    iptr += 2;
	  }
	else code[optr++] = code[iptr++];
      }
    else code[optr++] = code[iptr++];
  }

static void pass_y1()
  { if (iptr+2 < ilen)
      { tinstr *ti1 = code[iptr], *ti2 = code[iptr+1], *ti3 = code[iptr+2];
	if ((ti3 -> op == t_rev || ti3 -> op == t_fprev) && isldop(ti1 -> op) && isldop(ti2 -> op))
	  { gen(ti2 -> op, ti2 -> m1, ti2 -> p1, ti2 -> m2, ti2 -> p2);
	    gen(ti1 -> op, ti1 -> m1, ti1 -> p1, ti1 -> m2, ti1 -> p2);
	    iptr += 3;
	  }
	else code[optr++] = code[iptr++];
      }
    else code[optr++] = code[iptr++];
  }

static bool isldop(int op)
  { switch (op)
      { default:
	    return false;

	case t_ldc:	case t_ldl:	case t_ldpi:	case t_ldlp:
	    return true;
      }
  }

static void pass_y2()
  { if (iptr+1 < ilen)
      { tinstr *ti1 = code[iptr], *ti2 = code[iptr+1];
	if (ti1 -> op == t_ldnlp && ti1 -> m1 == m_num)
	  { if (ti1 -> p1.n == 0) iptr++;
	    else if ((ti2 -> op == t_ldnl || ti2 -> op == t_stnl) && ti2 -> m1 == m_num)
	      { gen(ti2 -> op, m_num, (ti2 -> p1.n) + (ti1 -> p1.n));
		iptr += 2;
	      }
	    else code[optr++] = code[iptr++];
	  }
	else code[optr++] = code[iptr++];
      }
    else code[optr++] = code[iptr++];
  }

static void pass_y3()
  { static lvinfo *lviptr;	/* set when an "entry" is found */
    tinstr *ti = code[iptr];
    switch (ti -> op)
      { default:
	    if (ti -> m1 == m_loc)
	      { ti -> m1 = m_wksp;
		if ((ti -> p1.n) >= (lviptr -> fps)) ti -> p1.n++;  /* skip "iptr" slot */
		ti -> p1.n = (lviptr -> locs + lviptr -> aps) - (ti -> p1.n);
	      }
	    code[optr++] = code[iptr++];
	    break;

	case t_entry:
	    lviptr = ti -> lvi;
	    code[optr++] = code[iptr++];
	    break;

	case t_ajw:
	  { int n = (lviptr -> locs + lviptr -> aps) - (lviptr -> fps);
	    if (n > 0)
	      { ti -> p1.n = (ti -> p1.n > 0) ? +n : -n;
		code[optr++] = code[iptr++];
	      }
	    else iptr++;
	    break;
	  }
      }
  }

static void gen(int op, int m1, word p1, int m2, word p2)
  { code[optr++] = new tinstr(op, m1, p1, m2, p2);
  }

