/* Alpha document preparation system
   Mips window manager
   AJF	 May 1989 */

#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include <spawn.h>

#include "alpha.h"

#define ICONFN	 "/usr/fisher/mipslib/alpha/icon"
#define MENUFN	 "/usr/fisher/mipslib/alpha/menu"
#define STIPFN	 "/usr/fisher/bitmaps/lightgrey"

#define EMWID	544	/* width of editor & msg windows	   */
#define EHGT	568	/* height of editor window		   */
#define MHGT	296	/* height of message window		   */
#define BORDER	2	/* even, so drawborder behaves predictably */

#define EVENT	 (1 << winchfd)
#define CH_DEATH (1 << childfd)

static window *frame;
static gfx_ctxt *gc_copy, *gc_stipple;
static bitmap *icon;
static int winchfd, childfd;

static int fwid, fhgt;	/* frame dimensions */
static int pwid, phgt;	/* preview window dimensions */

static void newhandler(), initialize(int, char*[]);
static void makegcs(), makeicon(), makeframe(rect), setenv(char*, word = 0);
static void subwin(rect, char*, word = 0, word = 0, word = 0);
static void mainloop();
static uint waitforinput(uint);
static void handleevent(XEvent*);
static void drawborder(rect);


global int main(int argc, char *argv[])
  { set_new_handler(newhandler);
    initialize(argc, argv);
    mainloop();
    return 0;
  }

static void newhandler()
  { giveup("No room");
  }

static void initialize(int argc, char *argv[])
  { unless (argc == 9) giveup("parameter error");
    childfd = open_sigpipe(SIGCHLD);
    winchfd = ConnectionNumber(gfx_display);
    makegcs(); makeicon();
    pwid = atoi(argv[1]); phgt = atoi(argv[2]);
    fwid = EMWID+pwid + 3*BORDER;
    fhgt = max(EHGT+MHGT + 3*BORDER, phgt + 2*BORDER) ;
    int snum = DefaultScreen(gfx_display);
    int dhgt = DisplayHeight(gfx_display, snum);
    int fypos = max(dhgt-fhgt, 0);
    makeframe(rect(0, fypos, fwid, fhgt));    /* make frame (parent) */
    frame -> setprops("Alpha", "Alpha", icon);
    frame -> map();
    subwin(rect(BORDER, BORDER, EMWID, EHGT), "%s -d %p %s", argv[3], atoi(argv[5]), argv[4]);  /* editor window  */
    subwin(rect(EMWID + 2*BORDER, BORDER, pwid, phgt), "-cat %p %p -mm %s -cb 1",
	   atoi(argv[7]), atoi(argv[6]), MENUFN);						/* preview window */
    subwin(rect(BORDER, EHGT + 2*BORDER, EMWID, MHGT), "-cat %p -pm 4", atoi(argv[8]));         /* message window */
  }

static void makegcs()
  { gc_copy = new gfx_ctxt(GXcopy, 8, Black, White);
    gc_copy -> setlineattrs(BORDER);
    gc_stipple = new gfx_ctxt(GXcopy, 8, Black, White);
    bitmap *spx = new bitmap(STIPFN, BM_PIXM);
    unless (spx -> ok) giveup("can't read bitmap \"%s\"", STIPFN);
    gc_stipple -> setstipple(spx);
  }

static void makeicon()
  { icon = new bitmap(ICONFN, BM_PIXM);
    unless (icon -> ok) giveup("can't read icon \"%s\"", ICONFN);
  }

static void makeframe(rect fr)
  { frame = new window(NULL, fr);
    setenv("WINDOW_PARENT=%08x", frame -> wind);
    setenv("DEFAULT_FONT=C.12");
    frame -> select(ExposureMask | StructureNotifyMask);
  }

static void setenv(char *s, word p)
  { char cmd[MAXSTRLEN+1];
    sprintf(cmd, s, p);
    char *es = new char[strlen(cmd) + 1];
    strcpy(es, cmd);
    int code = putenv(es);
    if (code < 0) giveup("no room! (putenv)");
  }

static void subwin(rect fr, char *astr, word p1, word p2, word p3)
  { char buf[MAXSTRLEN+1];
    strcpy(buf, "wshell -Wl alpha -pm 0 -nw -r %d %d %d %d "); strcat(buf, astr);
    spawn("/usr/fisher/mipsbin/wshell", buf, fr.left, fr.top, fr.left + fr.width, fr.top + fr.height, p1, p2, p3);
  }

static void mainloop()
  { bool running = true;
    while (running)
      { uint m = waitforinput(EVENT | CH_DEATH);
	if (m & EVENT)
	  { XEvent event;
	    NextEvent(&event);
	    handleevent(&event);
	  }
	if (m & CH_DEATH)
	  { dismiss_signal(childfd);
	    running = false;
	  }
      }
  }

static uint waitforinput(uint ib)
  { /* Wait for input from either event queue or application (child death) */
    uint ob = 0;
    if ((ib & EVENT) && (QLength(gfx_display) > 0)) ob |= EVENT;
    if (ob == 0)
      { XFlush(gfx_display);
	int nb = select(32, &ib, NULL, NULL, NULL); /* block */
	if (nb >= 0) ob = ib;
      }
    return ob;
  }

static void handleevent(XEvent *evptr)
  { switch (evptr -> type)
      { case Expose:
	    FillRectangle(frame, gc_stipple, rect(0, 0, fwid, fhgt));
	    drawborder(rect(0, 0, EMWID, EHGT));
	    drawborder(rect(0, EHGT + BORDER, EMWID, MHGT));
	    drawborder(rect(EMWID + BORDER, 0, pwid, phgt));
	    break;

	case ConfigureNotify:
	    fwid = evptr -> xconfigure.width;
	    fhgt = evptr -> xconfigure.height;
	    break;
      }
  }

static void drawborder(rect fr)
  { DrawRectangle(frame, gc_copy,
		  rect(fr.left + BORDER/2, fr.top + BORDER/2, fr.width + BORDER, fr.height + BORDER));
  }

