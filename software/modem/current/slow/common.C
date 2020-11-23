/* Modem for MIPS   AJF	  January 1995
   Common routines */

#include <stdio.h>

#include <complex.h>
#include <sinegen.h>
#include <myaudio.h>

#include "modem.h"

static void writemsg(char*, char*, word, word, word);


global void sendfreqs(float f1, float f2, float t)   /* send 2 tones */
  { sinegen *sgen1 = new sinegen(f1);
    sinegen *sgen2 = new sinegen(f2);
    int ns = (int) (t * SAMPLERATE);
    for (int i = 0; i < ns; i++)
      { float val = (sgen1 -> fnext()) + (sgen2 -> fnext());
	outsample(val);
      }
    delete sgen1; delete sgen2;
  }

global void sendfreq(float f, float t)		/* send a single tone */
  { sinegen *sgen = new sinegen(f);
    int ns = (int) (t * SAMPLERATE);
    for (int i = 0; i < ns; i++)
      { float val = sgen -> fnext();
	outsample(val);
      }
    delete sgen;
  }

global void sendpause(float t)			/* silence */
  { int ns = (int) (t * SAMPLERATE);
    for (int i = 0; i < ns; i++) outsample(0.0);
  }

global void giveup(char *msg, word p1, word p2, word p3)
  { writemsg("Error", msg, p1, p2, p2);
    exit(1);
  }

global void infomsg(char *msg, word p1, word p2, word p3)
  { writemsg("Info", msg, p1, p2, p3);
  }

static void writemsg(char *typ, char *msg, word p1, word p2, word p3)
  { fprintf(stderr, "*** %s: ", typ);
    fprintf(stderr, msg, p1, p2, p3);
    putc('\r', stderr); /* in case we're in raw mode */
    putc('\n', stderr);
  }

