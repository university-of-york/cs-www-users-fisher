/* Modem for MIPS   AJF	  January 1995
   Wait for tone (dial, connect, etc.) */

#include <stdio.h>
#include <fishaudio.h>
#include "filters.h"
#include "modem.h"

#define MILLISEC    24
#define THRESH	    3.0		/* tone threshold */

/* Filter coeffs constructed by:
   mkfilter -Bu -Bp -o 2 -a (A1) (A2)
   where A1 = F1 / 24000, A2 = F2 / 24000
   Indexed by tone; see modem.h */

static fspec *bpfspecs[] =
  { mkfilter("-Bu -Bp -o 2 -a 0.01625000000 0.01708333333"),    /*  390 ..  410 Hz, centre  400 Hz    [0]        */
    mkfilter("-Bu -Bp -o 2 -a 0.01833333333 0.01916666667"),    /*  440 ..  460 Hz, centre  450 Hz    [1] (dial) */
    mkfilter("-Bu -Bp -o 2 -a 0.04541666667 0.04625000000"),    /* 1090 .. 1110 Hz, centre 1100 Hz    [2] (merc) */
    mkfilter("-Bu -Bp -o 2 -a 0.17416666667 0.17583333333"),    /* 4180 .. 4220 Hz, centre 4200 Hz    [3] (conn) */
  };

static fspec *lpfspec = mkfilter("-Bu -Lp -o 2 -a 0.001666666667");     /* 40 Hz */

struct tone_detector
  { tone_detector(fspec*);	/* constructor */
    ~tone_detector();		/* destructor  */
    void insert(float);
    void debug();
    filter *bpf, *lpf;
    float pow;
    int count_pres, count_abs, prescount, abscount;
  };


global void waitfortone(int tone)
  { if (tone == CONN_TONE) sendfreq(1100, 12000);	/* send CNG (0.5 sec, 1100 Hz) */
    tone_detector *td0 = new tone_detector(bpfspecs[0]),
		  *td1 = new tone_detector(bpfspecs[tone]);
    int totcount = 0;
    bool found = false;
    until (found)
      { float x = (float) Audio -> read() * 1.5e-5;	/* scale to avoid overflow */
	td0 -> insert(x);
	/* fabsf to deal with phase reversals in answer tone */
	td1 -> insert((tone == CONN_TONE) ? fabsf(x) : x);
	totcount++;
	switch (tone)
	  { case DIAL_TONE: case MERC_TONE:
		if (totcount >= 5000*MILLISEC) giveup("No dial tone");
		break;

	    case CONN_TONE:
		if (totcount >= 45000*MILLISEC) giveup("No reply"); /* long delay in case there's an answering m/c */
		if (totcount % (3500*MILLISEC) == 0)
		  { /* re-play 0.5s CNG pulse */
		    Audio -> replay(SAMPLERATE/2);
		  }
		break;
	  }
	if (td0 -> count_pres >= 2000*MILLISEC) giveup("Number unobtainable");
	if (td0 -> prescount >= 4 && td0 -> abscount >= 4) giveup("Number busy");
	if (td1 -> count_pres > 500*MILLISEC) found = true;    /* we've found what we were looking for */
//	if ((totcount & 1023) == 0) { td0 -> debug(); td1 -> debug(); putc('\n', stderr); }
      }
    delete td0; delete td1;
  }

tone_detector::tone_detector(fspec *bpfs)
  { bpf = new filter(bpfs);
    lpf = new filter(lpfspec);
    pow = 0.0;
    count_pres = count_abs = prescount = abscount = 0;
  }

tone_detector::~tone_detector()
  { delete bpf; delete lpf;
  }

void tone_detector::insert(float x)
  { float y = bpf -> fstep(x);
    pow = lpf -> fstep(sqr(y));
    if (pow >= THRESH)
      { count_pres++;
	if (count_abs >= 300*MILLISEC && count_abs <= 550*MILLISEC) abscount++;
	count_abs = 0;
      }
    else
      { count_abs++;
	if (count_pres >= 300*MILLISEC && count_pres <= 550*MILLISEC) prescount++;
	count_pres = 0;
      }
  }

void tone_detector::debug()
  { fprintf(stderr, "%14.5e (%5d)", pow, count_pres);
  }

