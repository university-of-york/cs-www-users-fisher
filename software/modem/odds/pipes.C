/* Modem for Mips   AJF	  January 1996
   Pipes
*/

#include "modem.h"


mpipe::mpipe()
  { int code = pipe(fds);
    if (code < 0) giveup("pipe failed");
  }

mpipe::~mpipe()
  { close(fds[0]); close(fds[1]);
  }

int mpipe::rd()
  { int tmo[] = { 0, 0 }; uchar ch;
    uint ib = (1 << fds[0]);
    int nb = select(32, &ib, NULL, NULL, tmo);	/* don't block */
    if (nb < 0) giveup("select failed (pipe)");
    if (nb == 0) return -2;	/* timeout */
    int nc = read(fds[0], &ch, 1);
    unless (nc == 1) giveup("read from pipe failed");
    return ch;
  }

void mpipe::wr(uchar ch)
  { int nb = write(fds[1], &ch, 1);
    unless (nb == 1) giveup("write to pipe failed");
  }

