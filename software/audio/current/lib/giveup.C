/* Audio library for MIPS   AJF	  December 1996 */

#include "fishaudio.h"
#include "private.h"
#include <stdio.h>

extern int __Argc;
extern char **__Argv;

global void giveup(char *msg, word p1, word p2)
  { char *progname = (__Argc >= 1) ? __Argv[0] : "???";
    fprintf(stderr, "%s: ", progname);
    fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    exit(2);
  }

