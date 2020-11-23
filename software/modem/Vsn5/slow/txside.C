/* Modem for MIPS   AJF	  January 1995
   Transmit side */

#include "modem.h"

static txhandler *txhandler;


global void inittx(vmode mode)
  { switch (mode)
      { default:
	    giveup("Bug: Unimplemented Tx mode (%d)", mode);

	case V21o:  case V21a:	case V23o:  case V23a:
	    txhandler = &fsk_txhandler;
	    break;

	case V29:
	    txhandler = &v29_txhandler;
	    break;
      }
    txhandler -> init(mode);	/* call mode-specific init proc */
  }

global void putaval(int n)	/* asynchronous output */
  { txhandler -> pasync(n);	/* dispatch */
  }

global void putsval(int x)	/* synchronous output */
  { txhandler -> psync(x);	/* dispatch */
  }

global void putbit(int bit)	/* bit output */
  { txhandler -> pbit(bit);	/* dispatch */
  }

