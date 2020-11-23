/* Modem for MIPS   AJF	  January 1995
   Receive side */

#include "modem.h"

static rxhandler *rxhandler;


global void initrx(vmode mode)
  { switch (mode)
      { default:
	    giveup("Bug: Unimplemented Rx mode (%d)", mode);

	case V21o:  case V21a:	case V23o:  case V23a:
	    rxhandler = &fsk_rxhandler;
	    break;
      }
    rxhandler -> init(mode);	/* call mode-specific init proc */
  }

global int getaval()		    /* asynchronous input */
  { int n = rxhandler -> gasync();  /* dispatch */
    if (n >= 0 && (options & opt_7)) n &= 0x7f;
    return n;
  }

global int getsval()		    /* synchronous input */
  { return rxhandler -> gsync();    /* dispatch */
  }

global int getbit()		    /* bit input */
  { return rxhandler -> gbit();	    /* dispatch */
  }

