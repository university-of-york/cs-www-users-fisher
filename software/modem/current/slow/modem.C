/* Modem for MIPS   AJF	  January 1995
   Modem routines */

#include <stdio.h>
#include <coro.h>

#include <myaudio.h>
#include <mystdio.h>

#include "modem.h"

static void rxloop(), txloop();


global void becomemodem()
  { if (veemode == V32o || veemode == V34o)
      { /* exec [fz]modem program to do all the work */
	char *modem = (veemode == V34o) ? "zmodem" : "fmodem";
	char cmd[256]; sprintf(cmd, "/usr/fisher/mipslib/%s -bps %d", modem, bitrates);
	int code = system(cmd);
	if (code != 0) exit(1); /* error msg already written by [fz]modem */
      }
    else
      { /* fsk mode */
	openstdio();
	atexit(closestdio);
	setduplex(SAMPLERATE/5);    /* 0.2 secs */
	coroutine *rx = new coroutine(rxloop);
	coroutine *tx = new coroutine(txloop);
	inparallel(rx, tx);
      }
  }

static void rxloop()
  { initrx_fsk(veemode);
    for (;;)
      { int ch = getasync();	    /* get char from 'phone line */
	my_putchar(ch);		    /* to stdout */
      }
  }

static void txloop()
  { inittx_fsk(veemode);
    int ch = my_getchar();	    /* from stdin */
    until (ch == EOF)
      { putasync(ch);		    /* to 'phone line */
	ch = my_getchar();
      }
    callco(mainco);		    /* terminate */
  }

