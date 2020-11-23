/* Audio library for MIPS   AJF	  January 1995
   C++ vsn   AJF   December 1995 */

#include "fishaudio.h"
#include "private.h"

#include <stdio.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/hdsp.h>
#include <sys/mman.h>

static void xputenv(char*);
static int findaudio();
static ringbuf *allocringbuf(int, int);
static void unmapringbuf(ringbuf*);

struct audio_plist
  { audio_plist *link;
    int code, val;
  };

audio::audio(uint b, audio_pval pv[])
  { irb = orb = NULL; pl = NULL; ifd = ofd = mfd = -1;
    ifill = ofill = MAXSAMPLES/2;
    srate = 0;	/* unknown */
    bits = b;
    if (bits & AU_LOCK)
      { if (audio_inuse() == AU_INUSEBYOTHER) giveup("audio is in use");
	xputenv("AUDIO_INUSE=");
      }
    mfd = open(MASTERFN, O_RDWR);
    if (mfd < 0) giveup("can't open %s", MASTERFN);
    if (bits & AU_IN)
      { ifd = findaudio();
	irb = allocringbuf(ifd, HRB_TOMIPS);
      }
    if (bits & AU_OUT)
      { ofd = findaudio();
	orb = allocringbuf(ofd, HRB_FROMMIPS);
      }
    unless (pv == NULL)
      for (int i = 0; pv[i].code >= 0; i++) control(pv[i].code, pv[i].nval);
  }

static void xputenv(char *s)
  { int code = putenv(s);
    if (code < 0) giveup("no room! (putenv)");
  }

static int findaudio()
  { int n = 0; int fd;
    do
      { char fn[HRB_STRLEN];
	sprintf(fn, "/dev/hdsp/hdsp0r%d", n++);
	fd = open(fn, O_RDWR);
      }
    until (fd >= 0 || n >= 20);
    if (fd < 0) giveup("can't find audio device /dev/hdsp/...");
    return fd;
  }

static ringbuf *allocringbuf(int fd, int code)
  { ringbuf *rb = (ringbuf*) mmap(NULL, sizeof(ringbuf), (PROT_READ | PROT_WRITE), MAP_PRIVATE, fd, 0);
    if (rb == (void*) ~0) giveup("hdsp mmap failed (%d)", errno);
    rb -> head = rb -> tail = rb -> intreq = rb -> fillpt = 0;
    hdsp_acquireaudiorb_s acq = { sizeof(ringbuf) / sizeof(int), code };
    doioctl(1, fd, HDSP_ACQUIRE_AUDIO_RB, &acq);
    return rb;
  }

audio::~audio()
  { oflush();
    until (pl == NULL)
      { audio_plist *p = pl;
	control(p -> code, p -> val);
	pl = p -> link;
	delete p;
      }
    unmapringbuf(irb); close(ifd);
    unmapringbuf(orb); close(ofd);
    close(mfd);
  }

static void unmapringbuf(ringbuf *rb)
  { unless (rb == NULL)
      { int code = munmap(rb, sizeof(ringbuf));
	unless (code == 0) giveup("munmap failed (%d)", errno);
      }
  }

int audio::control(int code, int nval)
  { int oval;
    switch (code)
      { case AU_SETIFILL:
	    oval = ifill; ifill = nval;
	    break;

	case AU_SETOFILL:
	    oval = ofill; ofill = nval;
	    break;

	case AU_INPUT_RATE:
	case AU_OUTPUT_RATE:
	    oval = au_getset(mfd, code, nval);
	    srate = nval;
	    break;

	default:
	    oval = au_getset(mfd, code, nval);
	    break;
      }
    if (bits & AU_SAVE)
      { audio_plist *p = new audio_plist;
	p -> link = pl; pl = p;
	p -> code = code;
	p -> val = oval;
      }
    return oval; /* return old value */
  }

void audio::oflush()
  { /* wait for Tx ring buffer to empty */
    unless (orb == NULL) owait(1);
  }

void audio::idiscard()
  { /* empty Rx ring buffer */
    unless (irb == NULL) irb -> head = irb -> tail;
  }

void audio::odiscard()
  { /* empty Tx ring buffer */
    unless (orb == NULL) orb -> tail = orb -> head;
  }

int audio::read()
  { if (icount() == 0) iwait(ifill);		/* if empty, wait until half-full or more */
    int p = irb -> head;
    int vl = (irb -> buf[p++]);	    /* left  */
    int vr = (irb -> buf[p++]);	    /* right */
    irb -> head = p & (RBLENGTH-1);
    return vl+vr;
  }

void audio::write(int val)
  { if (ocount() == MAXSAMPLES) owait(ofill);	/* if full, wait until half-full or less */
    int p = orb -> tail;
    orb -> buf[p++] = val;	/* left	 */
    orb -> buf[p++] = val;	/* right */
    orb -> tail = p & (RBLENGTH-1);
  }

void audio::iwait(int nfi)
  { /* wait for .ge. nfi filled samples in input queue */
    if (icount() < nfi)		/* quick test first to avoid select */
      { irb -> fillpt = (MAXSAMPLES - nfi)*2;  /* tells select when to unblock */
	// fprintf(stderr, "IB %d", nfi);
	int nb;
	do
	  { uint iob = 1 << ifd;
	    nb = select(32, &iob, NULL, NULL, NULL);	/* block */
	  }
	while (nb < 0 && errno == EINTR);
	// putc('\n', stderr);
	if (nb < 0) giveup("select failed (audio::iwait)");
      }
  }

void audio::owait(int nfi)
  { /* wait for .lt. nfi filled samples in output queue */
    if (ocount() >= nfi)	/* quick test first to avoid select */
      { orb -> fillpt = nfi*2;	/* tells select when to unblock */
	// fprintf(stderr, "OB %d", nfi);
	int nb;
	do
	  { uint iob = 1 << ofd;
	    nb = select(32, NULL, &iob, NULL, NULL);	/* block */
	  }
	while (nb < 0 && errno == EINTR);
	// putc('\n', stderr);
	if (nb < 0) giveup("select failed (audio::owait)");
      }
  }

int audio::icount()
  { /* returns num. of samples in input buffer */
    int n = (irb -> tail) - (irb -> head); /* num. slots occupied */
    return (n & (RBLENGTH-1)) / 2;	   /* return num. of samples */
  }

int audio::ocount()
  { /* returns num. of samples in output buffer */
    int n = (orb -> tail) - (orb -> head); /* num. slots occupied */
    return (n & (RBLENGTH-1)) / 2;	   /* return num. of samples */
  }

void audio::reread(int ns)
  { /* reread last ns samples of input */
    if (ns < 0 || ns >= MAXSAMPLES) giveup("bug: bad param to audio::replay (%d)", ns);
    irb -> head = ((irb -> head) - (ns*2)) & (RBLENGTH-1);
  }

void audio::replay(int ns)
  { /* replay last ns samples of output */
    if (ns < 0 || ns >= MAXSAMPLES) giveup("bug: bad param to audio::replay (%d)", ns);
    orb -> head = ((orb -> head) - (ns*2)) & (RBLENGTH-1);
  }

