/* jtp - Compiler - Java bytecode to Transputer assembly code
   A.J. Fisher	 June 1997 */

#include <stdio.h>
#include "jtp.h"
#include "tcodes.h"

static void listopnd(int, word);


global void tlist(tinstr *ti)
  { switch (ti -> op)
      { default:
	    printf("\t%s", topstring(ti -> op));
	    unless (ti -> m1 == m_none)
	      { putchar('\t');
		listopnd(ti -> m1, ti -> p1);
		unless (ti -> m2 == m_none)
		  { printf(", ");
		    listopnd(ti -> m2, ti -> p2);
		  }
	      }
	    putchar('\n');
	    break;

	case t_entry:
	    printf("***\tEntry\t``%s'' ", ti -> p1.s);
	    printf("locs=%d fps=%d aps=%d\n", ti -> lvi -> locs, ti -> lvi -> fps, ti -> lvi -> aps);
	    break;

	case t_label:
	    listopnd(ti -> m1, ti -> p1);
	    break;
      }
  }

static void listopnd(int m, word p)
  { char *fmt = addrfmt(m);
    switch (m)
      { default:
	    printf(fmt, p);
	    break;

	case m_float:	case m_afloat:
	    printf(fmt, bitsfp(p.w[0].n));
	    break;

	case m_double:	case m_adouble:
	    printf(fmt, bitsdp(p.w[0].n, p.w[1].n));
	    break;
      }
  }

global char *topstring(int op)
  { switch (op)
      { default:	    return "???";
	case t_adc:	    return "adc";
	case t_and:	    return "and";
	case t_ajw:	    return "ajw";
	case t_ascii:	    return "ascii";
	case t_bitcnt:	    return "bitcnt";
	case t_blkb:	    return "blkb";
	case t_call:	    return "call";
	case t_calld:	    return "calld";
	case t_callf:	    return "callf";
	case t_calli:	    return "calli";
	case t_callv:	    return "callv";
	case t_case:	    return "case";
	case t_cexpr:	    return "cexpr";
	case t_cj:	    return "cj";
	case t_clrhalterr:  return "clrhalterr";
	case t_cmpnd:	    return "cmpnd";
	case t_comma:	    return "comma";
	case t_diff:	    return "diff";
	case t_div:	    return "div";
	case t_dup:	    return "dup";
	case t_dup2:	    return "dup2";
	case t_dup2x2:	    return "dup2x2";
	case t_dupx1:	    return "dupx1";
	case t_dupx2:	    return "dupx2";
	case t_end:	    return "end";
	case t_entry:	    return "entry";
	case t_eqc:	    return "eqc";
	case t_fpadd:	    return "fpadd";
	case t_fpcvtdi:	    return "fpcvtdi";
	case t_fpcvtfi:	    return "fpcvtfi";
	case t_fpcvtid:	    return "fpcvtid";
	case t_fpcvtif:	    return "fpcvtif";
	case t_fpdiv:	    return "fpdiv";
	case t_fpeq:	    return "fpeq";
	case t_fpgt:	    return "fpgt";
	case t_fpi32tor32:  return "fpi32tor32";
	case t_fpi32tor64:  return "fpi32tor64";
	case t_fpint:	    return "fpint";
	case t_fpldnldb:    return "fpldnldb";
	case t_fpldnlsn:    return "fpldnlsn";
	case t_fpldzerodb:  return "fpldzerodb";
	case t_fpldzerosn:  return "fpldzerosn";
	case t_fpmul:	    return "fpmul";
	case t_fprem:	    return "fprem";
	case t_fpremfirst:  return "fpremfirst";
	case t_fpremstep:   return "fpremstep";
	case t_fprev:	    return "fprev";
	case t_fpstnldb:    return "fpstnldb";
	case t_fpstnli32:   return "fpstnli32";
	case t_fpstnlsn:    return "fpstnlsn";
	case t_fpsub:	    return "fpsub";
	case t_fpuabs:	    return "fpuabs";
	case t_fpur32tor64: return "fpur32tor64";
	case t_fpur64tor32: return "fpur64tor32";
	case t_fpurz:	    return "fpurz";
	case t_fpusqrtfirst:return "fpusqrtfirst";
	case t_fpusqrtstep: return "fpusqrtstep";
	case t_fpusqrtlast: return "fpusqrtlast";
	case t_gajw:	    return "gajw";
	case t_gcall:	    return "gcall";
	case t_gt:	    return "gt";
	case t_in:	    return "in";
	case t_j:	    return "j";
	case t_label:	    return "label";
	case t_lb:	    return "lb";
	case t_ldc:	    return "ldc";
	case t_ldl:	    return "ldl";
	case t_ldlp:	    return "ldlp";
	case t_ldnl:	    return "ldnl";
	case t_ldnlp:	    return "ldnlp";
	case t_ldpi:	    return "ldpi";
	case t_lend:	    return "lend";
	case t_mint:	    return "mint";
	case t_not:	    return "not";
	case t_or:	    return "or";
	case t_out:	    return "out";
	case t_outbyte:	    return "outbyte";
	case t_pop:	    return "pop";
	case t_prod:	    return "prod";
	case t_psect:	    return "psect";
	case t_rem:	    return "rem";
	case t_ret:	    return "ret";
	case t_retnv:	    return "retnv";
	case t_retv:	    return "retv";
	case t_rev:	    return "rev";
	case t_rot:	    return "rot";
	case t_runp:	    return "runp";
	case t_sb:	    return "sb";
	case t_seq:	    return "seq";
	case t_seterr:	    return "seterr";
	case t_sethalterr:  return "sethalterr";
	case t_shl:	    return "shl";
	case t_shr:	    return "shr";
	case t_startp:	    return "startp";
	case t_sthf:	    return "sthf";
	case t_stl:	    return "stl";
	case t_stlf:	    return "stlf";
	case t_stnl:	    return "stnl";
	case t_stopp:	    return "stopp";
	case t_sttimer:	    return "sttimer";
	case t_sum:	    return "sum";
	case t_testerr:	    return "testerr";
	case t_word:	    return "word";
	case t_wcnt:	    return "wcnt";
	case t_wsub:	    return "wsub";
	case t_wsubdb:	    return "wsubdb";
	case t_xor:	    return "xor";
      }
  }

global char *addrfmt(int m)
  { switch (m)
      { default:	return "???";
	case m_lab:	return "L%d";
	case m_loc:	return "%d(loc)";
	case m_name:	return "%s";
	case m_num:	return "%d";
	case m_str:	return "\"%s\"";
	case m_astr:	return "&\"%s\"";       /* address of string, resolved later */
	case m_float:	return "%14.6e";
	case m_afloat:	return "&%14.6e";       /* address of float, resolved later */
	case m_double:	return "%18.10e";
	case m_adouble: return "&%18.10e";      /* address of double, resolved later */
	case m_wksp:	return "%d(wp)";
      }
  }

