/* Modem for MIPS   AJF	  January 1995
   V.22 transmit routines */

#include "modem.h"
#include "/usr/fisher/mipslib/audio.h"

#include <stdio.h>

/* tables for DPSK encoding */
static uchar phinctab[8] = { 4, 8, 0, 12 };

static struct complex constellation[16] =
  { { +1.0, +1.0 }, { +1.0, +3.0 }, { +3.0, +1.0 }, { +3.0, +3.0 },	/* Phase Quadrant 1 */
    { -1.0, +1.0 }, { -3.0, +1.0 }, { -1.0, +3.0 }, { -3.0, +3.0 },	/* Phase Quadrant 2 */
    { -1.0, -1.0 }, { -1.0, -3.0 }, { -3.0, -1.0 }, { -3.0, -3.0 },	/* Phase Quadrant 3 */
    { +1.0, -1.0 }, { +3.0, -1.0 }, { +1.0, -3.0 }, { +3.0, -3.0 },	/* Phase Quadrant 4 */
  };

static struct complex czero = { 0.0, 0.0 };

static uint scrambler;
static uchar sigelement, bitcount, onecount;


global inittx_v22()
  { int i;
    /* Segment 1 */
    sendqam(0, czero, 84000);			/* silence for 3.5 sec */
    /* Segment 2 */
    for (i=0; i < 60; i++)
      { sigelement = (i & 1) ? 9 : 8;		/* alternating dibit 00, 11 at 1200 bit/s */
	sendbaud();
      }
    /* Segment 3 */
    scrambler = bitcount = onecount = 0;
    for (i=0; i < 1560; i++) pbit(1, false);	/* scrambled 1s at 1200 bit/s */
  }

global pbit_v22(bit) uint bit;		    /* V.22 (bis) bit output (1200 bit/s) */
  { pbit(1, false);
  }

static pbit(bit, fast) uint bit; bool fast;
  { uint b14 = (scrambler >> 3) & 1;
    uint b17 = scrambler & 1;
    bit = bit ^ b14 ^ b17;
    if (onecount == 64) bit ^= 1;
    if (bit) onecount++;
    else onecount = 0;
    scrambler >>= 1;
    if (bit) scrambler |= 0x10000;
    if (fast)
      { if (++bitcount == 4)
	  { uchar x = scrambler >> 13;				    /* most recent 4 bits */
	    sigelement = (sigelement + phinctab[x & 3]) & 0xc;	    /* first 2 bits in time determine phase change */
	    sigelement |= (x >> 2);				    /* next 2 bits determine element within quadrant */
	    sendbaud();
	    bitcount = 0;
	  }
      }
    else
      { if (++bitcount == 2)
	  { uchar x = scrambler >> 15;				    /* most recent 2 bits */
	    sigelement = (sigelement + phinctab[x]) & 0x0c;	    /* determine phase change */
	    sigelement |= 2;					    /* elemenet within quadrant is always 2 */
	    sendbaud();
	    bitcount = 0;
	  }
      }
  }

static sendbaud()
  { sendqam(1200, constellation[sigelement], 40);
  }

