#include <stdio.h>
#include <string.h>

#include "solvecct.h"

global char *copystring(char *s)
  { int n = strlen(s);
    return strcpy(new char[n+1], s);
  }

global void giveup(char *msg, word p1, word p2)
  { fprintf(stderr, "solvecct: "); fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    exit(1);
  }

