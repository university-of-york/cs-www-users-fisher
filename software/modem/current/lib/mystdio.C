#include <stdio.h>
#include <sgtty.h>
#include <sys/time.h>
#include <signal.h>

#include "private.h"
#include "mystdio.h"

#define INTERVAL    100000		    /* timer interrupts every 0.1 secs */
#define PERSEC	    (1000000 / INTERVAL)

#define BUFFERSIZE  1024		    /* power of 2 */
#define BUFFERMASK  (BUFFERSIZE-1)

#define IN_BIT	    1	/* fd 0 */
#define OUT_BIT	    2	/* fd 1 */

struct Buffer
  { Buffer() { head = tail = 0; eof = false; }
    int getch(); void putch(int);
    void fill(), empty();
    int head, tail; bool eof;
    char buf[BUFFERSIZE];
  };

static Buffer inbuffer, outbuffer;
static int timeout;
static termios cmode, rmode;

static void getmode(termios*), setmode(termios*), clockhandler(int);
static uint doselect();
static void fatal(char*);

global void openstdio()
  { getmode(&cmode);					/* get current mode    */
    rmode = cmode;					/* copy the struct     */
    rmode.c_iflag = rmode.c_oflag = rmode.c_lflag = 0;	/* modify for raw mode */
    rmode.c_cc[VMIN] = 1; rmode.c_cc[VTIME] = 0;	/* set timeout params  */
    setmode(&rmode);					/* set raw mode	       */
    timeout = 0;
    signal(SIGALRM, (SIG_PF) clockhandler);
    static itimerval itval = { { 0, INTERVAL }, { 0, INTERVAL } };
    setitimer(ITIMER_REAL, &itval, NULL);		/* set timer interrupting */
  }

global void closestdio()
  { setmode(&cmode);		    /* set cooked mode */
  }

static void getmode(termios *tm)
  { int code = tcgetattr(0, tm);
    unless (code == 0) fatal("tcgetattr failed");
  }

static void setmode(termios *tm)
  { tcsetattr(0, TCSADRAIN, tm);    /* ignore errors here */
  }

global void my_alarm(int n)
  { timeout = n * PERSEC;
  }

global int my_getchar()
  { /* get char from stdin */
    return inbuffer.getch();
  }

int Buffer::getch()
  { return ((head-tail) > 0) ? buf[tail++ & BUFFERMASK] & 0xff :
	   eof ? EOF : NOCHAR;
  }

global void my_putchar(int ch)
  { /* put char to stdout */
    outbuffer.putch(ch);
  }

void Buffer::putch(int ch)
  { if (ch >= 0)
      { if ((head-tail) >= BUFFERSIZE) fatal("putchar buffer overflow");
	buf[head++ & BUFFERMASK] = ch;
      }
  }

static void clockhandler(int sig)
  { signal(SIGALRM, (SIG_PF) clockhandler);	/* reset handler */
    if (timeout > 0 && --timeout == 0) kill(getpid(), SIGUSR1);	    /* "Remote modem is not responding" */
    inbuffer.fill();
    outbuffer.empty();
  }

void Buffer::fill()
  { while ((head-tail) < BUFFERSIZE)
      { uint rdy = doselect();
	unless (rdy & IN_BIT) break;
	int hd = head & BUFFERMASK;
	int tl = tail & BUFFERMASK;
	int nb = read(0, &buf[hd], (tl > hd) ? tl-hd : BUFFERSIZE-hd);
	if (nb < 0) fatal("read failed");
	if (nb == 0) { eof = true; break; }
	head += nb;
      }
  }

void Buffer::empty()
  { while ((head-tail) > 0)
      { uint rdy = doselect();
	unless (rdy & OUT_BIT) break;
	int hd = head & BUFFERMASK;
	int tl = tail & BUFFERMASK;
	int nb = write(1, &buf[tl], (hd > tl) ? hd-tl : BUFFERSIZE-tl);
	if (nb < 0) fatal("write failed");
	if (nb == 0) { eof = true; break; }
	tail += nb;
      }
  }

static uint doselect()
  { uint ib = IN_BIT, ob = OUT_BIT;	/* test stdin, stdout */
    static int tmo[] = { 0, 0 };
    int nb = select(2, &ib, &ob, NULL, tmo);
    if (nb < 0) fatal("select failed");
    return (ib | ob);
  }

extern int errno;

static void fatal(char *msg)
  { fprintf(stderr, "%s [errno=%d]\r\n", msg, errno);
    abort();
  }

