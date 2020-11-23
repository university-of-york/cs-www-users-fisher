/* phoneman -- telephone manager
   Main program	   A.J. Fisher	 January 1996 */

#include <stdio.h>
#include <signal.h>
#include <fishaudio.h>

#include "phoneman.h"

#define DFLTRC	"/usr/fisher/mipslib/phoneman/dfltrc"

static audio_pval iopvals[] =
  { { AU_INPUT_RATE, AU_RATE_8000    },
    { AU_OUTPUT_RATE, AU_RATE_8000   },
    { AU_INPUT_SOURCE, AU_INPUT_LINE },
    { AU_LEFT_INPUT_ATTEN, 0	     },
    { AU_RIGHT_INPUT_ATTEN, 0	     },
    { AU_MONITOR_CTL, 0		     },
    { -1, -1			     },
  };

global audio *Audio;

static void newhandler(), catchsignal(int), sighandler(int), tidyaudio(), usage();
static char *findrcfile();


global int main(int argc, char *argv[])
  { set_new_handler(newhandler);
    catchsignal(SIGINT); catchsignal(SIGTERM);
    unless (argc == 1 || argc == 2) usage();
    char *pfn = (argc == 2) ? argv[1] : findrcfile();
    pnode *program = parsescript(pfn);
    Audio = new audio(AU_IN | AU_OUT | AU_LOCK | AU_SAVE, iopvals);
    atexit(tidyaudio);
    obeyscript(program);
  }

static void newhandler()
  { giveup("No room");
  }

static void catchsignal(int sig)
  { signal(sig, (SIG_PF) sighandler);
  }

static void sighandler(int sig)
  { giveup("Signal %d!", sig);
  }

static void tidyaudio()
  { delete Audio;
  }

static void usage()
  { fprintf(stderr, "Usage: phoneman [dur]\n");
    exit(2);
  }

static char *findrcfile()
  { char *pfn = DFLTRC;
    char *hdir = getenv("HOME");
    if (hdir != NULL)
      { char fn[MAXSTR+1]; sprintf(fn, "%s/phoneman.rc", hdir);
	if (exists(fn)) pfn = copystr(fn);  /* file exists? */
      }
    return pfn;
  }

