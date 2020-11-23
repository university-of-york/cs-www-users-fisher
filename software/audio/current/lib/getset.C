/* Audio library for MIPS   AJF	  January 1995
   C++ vsn   AJF   December 1995 */

#include "fishaudio.h"
#include "private.h"

#include <sys/fcntl.h>
#include <sys/hdsp.h>


global int audio_inuse()
  { if (getenv("AUDIO_INUSE") != NULL) return AU_INUSEBYME;     /* parent, or this process previously, has claimed lock */
    int mfd = open(MASTERFN, O_RDWR);
    if (mfd < 0) giveup("can't open %s (audio_isinuse)", MASTERFN);
    int nin = au_getset(mfd, AU_INPUT_RBCOUNT, -1);		/* get num. of input users  */
    int nout = au_getset(mfd, AU_OUTPUT_RBCOUNT, -1);		/* get num. of output users */
    close(mfd);
    return (nin+nout > 0) ? AU_INUSEBYOTHER : AU_NOTINUSE;
  }

global int au_getset(int fd, int code, int nval)
  { /* set new value; return old value */
    int oval;
    int pvec[3] = { 4, code };	/* length in shorts! */
    doioctl(2, fd, HDSP_GET_AUDIO_PARMS, pvec);
    oval = pvec[2]; pvec[2] = nval;
    doioctl(3, fd, HDSP_SET_AUDIO_PARMS, pvec);
    return oval;
  }

