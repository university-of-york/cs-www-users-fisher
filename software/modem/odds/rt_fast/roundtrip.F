/* Modem for MIPS   AJF	  August 1996
   V.32 round-trip routines */

#include <stdio.h>
#include <string.h>	/* memset */

#include <complex.h>
#include <filters.h>
#include <debug.h>
#include <sinegen.h>
#include <myaudio.h>
#include <mystdio.h>
#include <phrdec.h>

#include "modem.h"

static fspec *bpfs = mkfilter("-Bu -Bp -o 2 -a 0.0616666667 0.0633333333");     /* bandpass, 592 - 608 Hz  */
static fspec *lpfs = mkfilter("-Av 8");                                         /* 8-sample moving average */

static sinegen *sgen;
static phr_detector *pd;
static debugger *debug;

static void tidydebug(), waitfortone();
static int lookforphaseshift(float);
static void nextsample(float);


global int roundtrip()
  { /* estimate round-trip delay NT */
    my_alarm(15);  /* 15 sec timeout */
    sgen = new sinegen(1800.0f);
    pd = new phr_detector(bpfs, lpfs);
    debug = new debugger(2, 48000);
    atexit(tidydebug);
    discardoutput(); discardinput();
    setduplex(64*SYMBLEN);			/* required by protocol */
    waitfortone();				/* wait for filters to settle and for tone to appear */
    debug -> reset();
    debug -> tick('A');
    int pc1 = lookforphaseshift(+1.0f);		/* send AA, rcv AC, wait for CA */
    debug -> tick('B');
    int nt;
    do
      { int pc2 = lookforphaseshift(-1.0f);	/* send CC, rcv CA, wait for AC */
	debug -> tick('C');
	nt = (pc2-pc1) - (64*SYMBLEN);		/* NT in samples */
      }
    while (nt < 64*SYMBLEN);
    setduplex(TRDELAY);				/* allow a longer turn-around time */
    float ms = (float) (nt - 64*SYMBLEN) / (float) SAMPLERATE * 1000.0f;
    char nts[32]; sprintf(nts, "%.1f", ms); infomsg("NT=%d; actual RTD = %sms", nt, nts);
    lookforphaseshift(0.0f);			/* wait for silence following AC */
    delete sgen; delete pd;
    my_alarm(0);   /* cancel alarm */
    return nt;
  }

static void tidydebug()
  { debug -> print("debug_nt.grap");
  }

static void waitfortone()
  { int num = 0;
    do
      { nextsample(0.0);
	if (pd -> reversal()) num = 0; else num++;
      }
    while (num < 100);
  }

static int lookforphaseshift(float a)
  { /* wait for phase change */
    while (pd -> reversal()) nextsample(a);	/* wait for phase to stabilize */
    until (pd -> reversal()) nextsample(a);	/* wait for phase change */
    return samplecount;				/* return time when it happened */
  }

static void nextsample(float a)
  { /* transmit next sample of AA or CC */
    float cx = sgen -> fnext();
    outsample(a*cx);
    /* deal with next received sample */
    float x = insample();
    pd -> insert(x);
    debug -> insert(x, pd -> phase);
  }

