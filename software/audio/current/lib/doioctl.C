/* Audio library for MIPS   AJF	  December 1996 */

#include "fishaudio.h"
#include "private.h"
#include <errno.h>

global void doioctl(int n, int fd, int key, void *arg)
  { int code = ioctl(fd, key, arg);
    if (code < 0) giveup("hdsp ioctl failed (n=%d errno=%d)", n, errno);
  }

