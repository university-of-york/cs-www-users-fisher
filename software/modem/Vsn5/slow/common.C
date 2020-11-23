/* Modem for MIPS   AJF	  January 1995
   Common routines */

#include <stdio.h>
#include <fishaudio.h>
#include "modem.h"

static uint sineptr = 0;

static void writemsg(char*, char*, word, word, word);
static void append(char*, int&, char*, word = 0, word = 0, word = 0);


global void sendanswer()		/* send V.25 answer sequence   */
  { sendpause(51600);			/* silence for 2.15 sec	       */
    for (int i=0; i < 7; i++)
      { sendfreq(2100, 10800);		/* 2100 Hz for 450 ms	       */
	sineptr ^= (SINELEN/2);		/* flip phase for next segment */
      }
    sendpause(1800);			/* silence for 75 ms	       */
  }

global void sendfreq(int f, int ns)	/* send a single tone */
  { int dph = (f * SINELEN) / SAMPLERATE;
    int n = 0;
    while (n < ns)
      { float val = (0.5 * MAXAMPL) * sinetab[sineptr & (SINELEN-1)];
	Audio -> write((int) val);
	sineptr += dph; n++;
      }
  }

global void sendpause(int ns)		/* silence for ns samples */
  { int n = 0;
    while (n < ns)
      { Audio -> write(0);
	n++;
      }
  }

global void giveup(char *msg, word p1, word p2, word p3)
  { writemsg("Error", msg, p1, p2, p2);
    writelog("ER", msg, p1, p2, p3);
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

global void writelog(char *typ, char *msg, word p1, word p2, word p3)
  { char str[256]; int len = 0;
    append(str, len, "%-16s ", (options & opt_org) ? telno : "incoming");
    str[len++] = (options & opt_mod) ? 'M' :	    /* modem mode, not fax	     */
		 (!(options & opt_fax)) ? 'V' :	    /* dial number only, presumed voice call */
		 (options & opt_H) ? 'H' : 'L';	    /* fax, high/low resolution	     */
    str[len++] = (options & opt_v) ? 'v' : ' ';
    str[len++] = (options & opt_m) ? 'm' : ' ';
    str[len++] = (options & opt_p) ? 'p' : ' ';
    do str[len++] = ' '; until (len >= 24);
    append(str, len, (contime >= 0) ? "%4ds " : "   NC ", time(NULL) - contime); /* connect time */
    append(str, len, "%2dp %s ", pagessent, typ);   /* OK or ER */
    append(str, len, msg, p1, p2, p3);
    int pid = fork(); /* ignore fork/exec errors */
    if (pid == 0)
      { execl("/usr/fisher/mipslib/logfax", "logfax", str, NULL);
	_exit(255);
      }
  }

static void append(char *str, int &len, char *fmt, word p1, word p2, word p3)
  { sprintf(&str[len], fmt, p1, p2, p3);
    until (str[len] == '\0') len++;
  }

