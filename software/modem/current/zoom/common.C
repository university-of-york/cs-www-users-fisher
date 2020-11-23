/* Modem for MIPS   AJF	  January 1995
   Common routines */

#include <stdio.h>

#include "modem.h"

static void writemsg(char*, char*, word, word, word);


global void giveup(char *msg, word p1, word p2, word p3)
  { writemsg("Error", msg, p1, p2, p3);
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

