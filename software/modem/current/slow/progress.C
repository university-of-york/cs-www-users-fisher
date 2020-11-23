#line 1 "progress.F"
/* Modem for MIPS   AJF	  January 1995
   Wait for tone (dial, connect, etc.) */

#include <stdio.h>
#include <math.h>

#include <complex.h>
#include <filters.h>
#line 1 "<<generated_fspecs>>"

static float _fstepf_1(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 2 -a 0.04062500000 0.04270833333 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] =    (  4.2443368477e-05 * x)
            + ( -0.9816582826 * v[0]) + (  3.8104707151 * v[1])
            + ( -5.6793132062 * v[2]) + (  3.8459049510 * v[3]);
    return    (v[0] + v[4]) - 2 * v[2];
  }

static fspec _fspecs_1 = { 4, 4, _fstepf_1 };

static float _fstepf_2(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 2 -a 0.04583333333 0.04791666667 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] =    (  4.2443368700e-05 * x)
            + ( -0.9816582826 * v[0]) + (  3.7750239468 * v[1])
            + ( -5.6108354356 * v[2]) + (  3.8101285572 * v[3]);
    return    (v[0] + v[4]) - 2 * v[2];
  }

static fspec _fspecs_2 = { 4, 4, _fstepf_2 };

static float _fstepf_3(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Bp -o 2 -a 0.21770833333 0.21979166667 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; v[2] = v[3]; v[3] = v[4]; 
    v[4] =    (  4.2443368410e-05 * x)
            + ( -0.9816582826 * v[0]) + (  0.7696097760 * v[1])
            + ( -2.1323332640 * v[2]) + (  0.7767665124 * v[3]);
    return    (v[0] + v[4]) - 2 * v[2];
  }

static fspec _fspecs_3 = { 4, 4, _fstepf_3 };

static float _fstepf_4(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Hp -Bu -o 2 -a 0.03125 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; 
    v[2] =    (  8.7033077934e-01 * x)
            + ( -0.7575469445 * v[0]) + (  1.7237761728 * v[1]);
    return    (v[0] + v[2]) - 2 * v[1];
  }

static fspec _fspecs_4 = { 2, 2, _fstepf_4 };

static float _fstepf_5(filter *fi, float x)
  { /* /usr/fisher/mipsbin/mkfilter -Bu -Lp -o 2 -a 0.004166666667 -l */
    float *v = fi -> v;
    v[0] = v[1]; v[1] = v[2]; 
    v[2] =    (  1.6822370862e-04 * x)
            + ( -0.9636529842 * v[0]) + (  1.9629800894 * v[1]);
    return    (v[0] + v[2]) + 2 * v[1];
  }

static fspec _fspecs_5 = { 2, 2, _fstepf_5 };

#line 9 "progress.F"
#include <myaudio.h>
#include <tonedec.h>

#include "modem.h"

#define debug_cadence false

struct cadence_detector
  { cadence_detector()
      { pcnt = acnt = 0;	    /* sample counts */
	oncount = offcount = 0;	    /* cadence on/off counts */
      }
    void insert(bool);
    void debug();
    int oncount, offcount;
private:
    int pcnt, acnt;
  };

/* Filter coeffs constructed by:
   mkfilter -Bu -Bp -o 2 -a (A1) (A2)
   where A1 = F1 / SAMPLERATE, A2 = F2 / SAMPLERATE
   Indexed by tone; see modem.h */

static fspec *bpfspecs[] =
  { (&_fspecs_1),        /*  390 ..  410 Hz, centre  400 Hz    [0]        */
    (&_fspecs_2),        /*  440 ..  460 Hz, centre  450 Hz    [1] (dial) */
    (&_fspecs_3),        /* 2090 .. 2110 Hz, centre 2100 Hz    [2] (conn) */
  };

static fspec *fefs = (&_fspecs_4);           /* 300 Hz hpf */
static fspec *lpfs = (&_fspecs_5);    /*  40 Hz lpf */

inline int secs(float f) { return (int) (f * SAMPLERATE); }


global void waitfortone(int tone)
  { tone_detector
	*td1 = new tone_detector(fefs, bpfspecs[0], lpfs, true),
	*td2 = new tone_detector(fefs, bpfspecs[tone], lpfs, true);
    cadence_detector
	*cd1 = new cadence_detector(),
	*cd2 = new cadence_detector();
    int totcount = 0;
    bool found = false;
    until (found)
      { float x = insample();
	td1 -> insert(x); cd1 -> insert(td1 -> present);
	td2 -> insert(x); cd2 -> insert(td2 -> present);
	if (debug_cadence && totcount%500 == 0)
	  { td1 -> debug(); cd1 -> debug();
	    td2 -> debug(); cd2 -> debug();
	    putc('\n', stderr);
	  }
	switch (tone)
	  { case DIAL_TONE:
		if (totcount >= secs(5.0f)) giveup("No dial tone");
		if (td2 -> prescount > secs(1.5f)) found = true;
		break;

	    case CONN_TONE:
		if (totcount % secs(3.5f) == 0)
		  { /* send CNG or V.25 CT every 3.5 secs */
		    float f = (options & opt_fax) ? 1100.0 : 1300.0;
		    sendfreq(f, 0.5f);
		  }
		if (totcount >= secs(45.0f)) giveup("No reply"); /* long delay in case there's an answering m/c */
		if ((td2 -> prescount > secs(2.7f)) || (cd2 -> oncount > 6)) found = true;	/* V.25, V.25 bis */
		break;
	  }
	if (td1 -> prescount >= secs(3.0f)) giveup("Number unobtainable");
	if (cd1 -> oncount >= 4 && cd1 -> offcount >= 4) giveup("Number busy");
	totcount++;
      }
    delete td1; delete td2; delete cd1; delete cd2;
  }

void cadence_detector::insert(bool pres)
  { if (pres)
      { pcnt++;
	if (acnt >= secs(0.25f) && acnt <= secs(0.55f)) offcount++;	/* 0.25 was 0.3 */
	acnt = 0;
      }
    else
      { acnt++;
	if (pcnt >= secs(0.25f) && pcnt <= secs(0.55f)) oncount++;	/* 0.25 was 0.3 */
	pcnt = 0;
      }
  }

void cadence_detector::debug()
  { fprintf(stderr, " [%d:%d]", offcount, oncount);
  }

