#include "alpha.h"

global void giveup(char *msg, word p1, word p2)
  { fprintf(stderr, "alpha: ");
    fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    exit(2);
  }

