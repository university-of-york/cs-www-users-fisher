/* Audio library for MIPS   AJF	  January 1995 */

#include "private.h"
#include "audio.h"

#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/hdsp.h>
#include <sys/mman.h>

static struct hdsp_acquireaudiorb_s acq =
  { sizeof(struct ringbuf) / sizeof(int),
    0,
  };

static char *progname;

extern int errno;


global InitAudio(pn) char *pn;
  { progname = pn; /* for error msgs */
  }

global struct audio *OpenAudio(dir) int dir;
  { char fn[HRB_STRLEN];
    int n = 0; int fd; uint rb; struct audio *au;
    do
      { sprintf(fn, "/dev/hdsp/hdsp0r%d", n);
	fd = open(fn, O_RDWR);
	n++;
      }
    until (fd >= 0 || n >= 20);
    if (fd < 0) giveup("can't find audio device /dev/hdsp...");
    rb = mmap(NULL, sizeof(struct ringbuf), (PROT_READ | PROT_WRITE), MAP_PRIVATE, fd, 0);
    if (rb == ~0) giveup("hdsp mmap failed (%d)", errno);
    acq.direction = (dir == AU_OUT) ? HRB_FROMMIPS :
		    (dir == AU_IN) ? HRB_TOMIPS : -1;
    doioctl(1, fd, HDSP_ACQUIRE_AUDIO_RB, &acq);
    au = heap(1, struct audio);
    au -> fd = fd;
    au -> rb = (struct ringbuf *) rb;
    au -> dir = dir;
    return au;
  }

global int ControlAudio(au, code, nval) struct audio *au; int code, nval;
  { int pvec[3]; int oval;
    pvec[0] = 4; /* length in shorts! */
    pvec[1] = code;
    doioctl(2, au -> fd, HDSP_GET_AUDIO_PARMS, pvec);
    oval = pvec[2]; pvec[2] = nval;
    doioctl(3, au -> fd, HDSP_SET_AUDIO_PARMS, pvec);
    return oval; /* return old value */
  }

global WriteMono(au, val) struct audio *au; int val;
  { struct ringbuf *rb = au -> rb;
    int p = rb -> tail;
    rb -> buf[p++] = val;	/* left	 */
    rb -> buf[p++] = val;	/* right */
    rb -> tail = p & (RBLEN-1);
  }

global ReplayAudio(au, ns) struct audio *au; int ns;
  { /* replay last ns samples of output */
    if (ns < 0 || ns >= RBLEN/2) giveup("bug: bad param to ReplayAudio (%d)", ns);
    au -> rb -> head = ((au -> rb -> head) - (ns*2)) & (RBLEN-1);
  }

global int ReadMono(au) struct audio *au;
  { int p, vl, vr;
    struct ringbuf *rb = au -> rb;
    p = rb -> head;
    vl = (rb -> buf[p++]);	/* left	 */
    vr = (rb -> buf[p++]);	/* right */
    rb -> head = p & (RBLEN-1);
    return vl+vr;
  }

global CloseAudio(au) struct audio *au;
  { FlushAudio(au);
    close(au -> fd);
    free(au);
  }

global FlushAudio(au) struct audio *au;
  { switch (au -> dir)
      { default:
	    giveup("bug: bad param to FlushAudio");

	case AU_OUT:
	    WaitAudio(au, 1);			    /* wait for Tx ring buffer to empty */
	    break;

	case AU_IN:
	    au -> rb -> head = au -> rb -> tail;    /* empty Rx ring buffer */
	    break;
      }
  }

global WaitAudio(au, fpt) struct audio *au; int fpt;
  { /* wait for input queue to become half full, or output queue to become half empty */
    int nf = NumFilled(au);
    int dir = au -> dir;
    if ((dir == AU_IN && nf < fpt) || (dir == AU_OUT && nf >= fpt))	/* quick test first to avoid select */
      { uint iob; int nb;
	au -> rb -> fillpt = fpt;   /* tells select when to unblock */
	iob = 1 << (au -> fd);
	nb = (dir == AU_IN) ? select(32, &iob, NULL, NULL, NULL) :
	     (dir == AU_OUT) ? select(32, NULL, &iob, NULL, NULL) : -1; /* block */
	if (nb < 0) giveup("select failed (WaitAudio)");
      }
  }

global int NumFilled(au) struct audio *au;
  { /* returns num. of slots filled in buffer */
    struct ringbuf *rb = au -> rb;
    int n = (rb -> tail) - (rb -> head); /* num. occupied */
    return (n & (RBLEN-1));
  }

static doioctl(n, fd, key, p1) int n, fd, key; word p1;
  { int code = ioctl(fd, key, p1);
    if (code < 0) giveup("hdsp ioctl failed (n=%d errno=%d)", n, errno);
  }

static char *au_cheap(nb) int nb; /* called by "heap" macro */
  { char *x = malloc(nb);
    if (x == NULL) giveup("No room! (audio)");
    return x;
  }

static giveup(msg, p1, p2) char *msg; word p1, p2;
  { fprintf(stderr, "%s: ", progname);
    fprintf(stderr, msg, p1, p2); putc('\n', stderr);
    exit(2);
  }

