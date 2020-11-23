/* Audio library for MIPS   AJF	  December 1996 */

#include <stdio.h>	//
#include <sys/hdsp.h>

#include "fishaudio.h"
#include "private.h"

void audio::setduplex(int d)
  { fprintf(stderr, "[%d:", d); //
    control(AU_SETIFILL, 1);			   /* set input and output non-blocking */
    control(AU_SETOFILL, MAXSAMPLES);
    int ns = gdelay();
    until (ns == d)
      { while (ns > d) { read(); ns--; }	   /* delay too big, discard input */
	while (ns < d) { write(0); ns++; }	   /* delay too small, prime output buffer */
	ns = gdelay();
      }
    control(AU_SETIFILL, d/2);			   /* set input wake-up point to three-quarters of delay */
    fprintf(stderr, "]\r\n"); //
  }

// Consider the following loop:
//
//	for (;;)
//	  { int x = Audio -> read();
//	    Audio -> write(x);
//	  }
//
// Because of buffering and hardware delays, samples arriving at the
// electrical input will be delayed before appearing at the electrical
// output.  Member function "gdelay" returns the delay in samples.

#define SRATE 24000	/* assumed, unfortunately */

typedef unsigned long long u64;	    /* 64 bits */
typedef signed long long s64;	    /* 64 bits */

static u64 gettime(int, int);

int audio::gdelay()	/* private */
  { u64 itime = gettime(ifd, -1);	    /* get time when next input to be processed by "read" came in */
    u64 otime = gettime(ofd, +1);	    /* get time when next output to be processed by "write" will go out */
    double dt = (double) (otime - itime) * (double) SRATE * 1e-9;	/* difference, in samples */
    int ns = (int) (dt + 0.5);		    /* return as an integer num. of samples */
    fprintf(stderr, "%d/", ns); //
    return ns;
  }

static u64 gettime(int fd, int io)
  { hdsp_rbmsc_t inf;
    doioctl(4, fd, HDSP_GET_RB_MSC, &inf);			/* get frame num, hd & tl (indivisibly) */
    uint n = ((inf.tail - inf.head) & (RBLENGTH-1)) / 2;	/* num. of samples in queue */
    u64 fnum = (io < 0) ? (inf.msc - n) : (inf.msc + n);	/* frame num. of next frame to be processed by read/write */
    doioctl(5, fd, HDSP_GET_RB_USTMSC, &inf);			/* get frame num, ust (indivisibly) */
    s64 dt = (s64) (fnum - inf.msc) * (int) 1e9 / SRATE;	/* time adj in ns (+ve for output, -ve for input) */
    return inf.ust + dt;					/* return time for fnum */
  }

