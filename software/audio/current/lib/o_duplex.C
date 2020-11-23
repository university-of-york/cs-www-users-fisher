/* Audio library for MIPS   AJF	  December 1996 */

#include <stdio.h>	//

#include "fishaudio.h"
#include "private.h"


void audio::setduplex(int d)
  { fprintf(stderr, "[%d: ", d); //
    control(AU_SETIFILL, 1);			   /* set input and output non-blocking */
    control(AU_SETOFILL, MAXSAMPLES);
    int ns = gdelay();
    until (ns == d)
      { while (ns > d) { read(); ns--; }	   /* delay too big, discard input */
	while (ns < d) { write(0); ns++; }	   /* delay too small, prime output buffer */
	ns = gdelay();
      }
    while (icount() > 0) { read(); write(0); }	   /* aim for Tx full, Rx empty */
    control(AU_SETIFILL, d/2);			   /* set blocking points to half-way */
    control(AU_SETOFILL, d/2);
    fprintf(stderr, "]\r\n"); //
  }

#define XX 0

static uchar hwdtab[8] =
  { /* table of h/w delays at various sample rates */
    0, 90, 0, 66, 60, 0, 0, 0	/* 24000, 12000, 9600 */
  };

int audio::gdelay()	/* private */
  { int ni1, ni2, no1, no2;
    ni2 = icount();
    no2 = ocount();
    do
      { ni1 = ni2; no1 = no2;
	ni2 = icount();
	no2 = ocount();
      }
    until (ni1 == ni2 && no1 == no2);
    int hwd = hwdtab[(srate >> 4) & 7];
    int tot = ni1 + no1 + hwd;
    fprintf(stderr, "%d+%d+%d=%d ", ni1, no1, hwd, tot); //
    return tot;
  }

