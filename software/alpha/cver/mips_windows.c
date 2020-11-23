/* Alpha document preparation system
   Mips window manager
   AJF	 May 1989 */

#include <stdio.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "gfxlib.h"
#include "alpha.h"

#define ICONFN	 "/usr/fisher/mipslib/alpha/icon"
#define MENUFN	 "/usr/fisher/mipslib/alpha/menu"

#define EMWID	544	/* width of editor & msg windows	   */
#define EHGT	568	/* height of editor window		   */
#define MHGT	296	/* height of message window		   */
#define BORDER	2	/* even, so drawborder behaves predictably */

#define EVENT	 (1 << winchfd)
#define CH_DEATH (1 << childfd)

extern struct bitmap *ReadBitmap();
extern char *malloc();

static Display *display;
static Window rootwin, frame;
static GC gc_copy, gc_stipple;
static struct bitmap *icon;
static int winchfd, childfd;

static int fwid, fhgt;	/* frame dimensions */
static int pwid, phgt;	/* preview window dimensions */

#define STIPSZ 16	/* 16x16 */

static uchar stipple[(STIPSZ*STIPSZ)/8] = /* from "/usr/sun/pix/texture/lightgrey" */
  { 0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
    0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88, 0x22, 0x22, 0x88, 0x88,
  };

forward uint waitforinput();


global main(argc, argv) int argc; char *argv[];
  { initialize(argc, argv);
    mainloop();
    exit(0);
  }

static initialize(argc, argv) int argc; char *argv[];
  { int dhgt, fypos, snum;
    unless (argc == 9) w_giveup("parameter error");
    initspawn("windows"); initsigpipes("windows");
    childfd = open_sigpipe(SIGCHLD);
    display = XOpenDisplay(NULL);
    if (display == NULL) w_giveup("can't open display");
    winchfd = ConnectionNumber(display);
    snum = DefaultScreen(display);
    rootwin = RootWindow(display, snum);
    makegcs(); makeicon();
    pwid = atoi(argv[1]); phgt = atoi(argv[2]);
    fwid = EMWID+pwid + 3*BORDER;
    fhgt = max(EHGT+MHGT + 3*BORDER, phgt + 2*BORDER) ;
    dhgt = DisplayHeight(display, snum);
    fypos = max(dhgt-fhgt, 0);
    makeframe(0, fypos, fwid, fhgt);	/* make frame (parent) */
    setstdprops(0, fypos, fwid, fhgt, argc, argv);
    XMapWindow(display, frame);
    subwin(BORDER, BORDER, EMWID, EHGT, "%s -d %p %s", argv[3], atoi(argv[5]), argv[4]);         /* editor window  */
    subwin(EMWID + 2*BORDER, BORDER, pwid, phgt, "-cat %p %p -mm %s -cb 1",
	   atoi(argv[7]), atoi(argv[6]), MENUFN);						 /* preview window */
    subwin(BORDER, EHGT + 2*BORDER, EMWID, MHGT, "-cat %p -pm 4", atoi(argv[8]));                /* message window */
  }

#define GCMASK (GCFunction | GCForeground | GCBackground | GCLineWidth | GCGraphicsExposures)

static makegcs()
  { int snum = DefaultScreen(display);
    XGCValues values;
    values.function = GXcopy;
    values.foreground = BlackPixel(display, snum);
    values.background = WhitePixel(display, snum);
    values.line_width = BORDER;
    values.graphics_exposures = false;	/* inhibit GraphicsExpose, NoExpose events */
    gc_copy = XCreateGC(display, rootwin, GCMASK, &values);
    values.fill_style = FillStippled;
    values.stipple = XCreateBitmapFromData(display, rootwin, stipple, STIPSZ, STIPSZ);
    gc_stipple = XCreateGC(display, rootwin, GCMASK | GCFillStyle | GCStipple, &values);
  }

static makeicon()
  { icon = ReadBitmap(display, ICONFN, BM_PIXM);
    if (icon == NULL) w_giveup("can't read icon \"%s\"", ICONFN);
  }

static makeframe(fr) struct rect fr;
  { int snum = DefaultScreen(display);
    int black = BlackPixel(display, snum), white = WhitePixel(display, snum);
    frame = XCreateSimpleWindow(display, rootwin, fr, 0, black, white);
    setenv("WINDOW_PARENT=%08x", frame);
    setenv("DEFAULT_FONT=C.12");
    XSelectInput(display, frame, ExposureMask | StructureNotifyMask);
  }

static setenv(s, p) char *s; word p;
  { char cmd[MAXSTRLEN+1]; char *es; int code;
    sprintf(cmd, s, p);
    es = heap(strlen(cmd) + 1, char);
    strcpy(es, cmd);
    code = putenv(es);
    if (code < 0) w_giveup("no room! (putenv)");
  }

static setstdprops(fr, argc, argv) struct rect fr; int argc; char *argv[];
  { /* fill in hints structure & pass to WM */
    XSizeHints hints;
    hints.flags = USPosition | USSize; /* program knows best! */
    memcpy(&hints.x, &fr, sizeof(struct rect));
    XSetStandardProperties(display, frame, "Alpha", "Alpha", icon -> pixm, argv, argc, &hints);
  }

static subwin(wr, astr, p1, p2, p3) struct rect wr; char *astr; word p1, p2, p3;
  { char buf[MAXSTRLEN+1];
    strcpy(buf, "wshell -Wl alpha -pm 0 -nw -r %d %d %d %d "); strcat(buf, astr);
    spawn("/usr/fisher/mipsbin/wshell", buf, wr.left, wr.top, wr.left + wr.width, wr.top + wr.height, p1, p2, p3);
  }

static mainloop()
  { bool running = true;
    while (running)
      { uint m = waitforinput(EVENT | CH_DEATH);
	if (m & EVENT)
	  { XEvent event;
	    XNextEvent(display, &event);
	    handleevent(&event);
	  }
	if (m & CH_DEATH)
	  { dismiss_signal(childfd);
	    running = false;
	  }
      }
  }

static uint waitforinput(ib) uint ib;
  { /* Wait for input from either event queue or application (child death) */
    uint ob = 0;
    if ((ib & EVENT) && (QLength(display) > 0)) ob |= EVENT;
    if (ob == 0)
      { int nb;
	XFlush(display);
	nb = select(32, &ib, NULL, NULL, NULL); /* block */
	if (nb >= 0) ob = ib;
      }
    return ob;
  }

static handleevent(evptr) XEvent *evptr;
  { switch (evptr -> type)
      { case Expose:
	    XFillRectangle(display, frame, gc_stipple, 0, 0, fwid, fhgt);
	    drawborder(0, 0, EMWID, EHGT);
	    drawborder(0, EHGT + BORDER, EMWID, MHGT);
	    drawborder(EMWID + BORDER, 0, pwid, phgt);
	    break;

	case ConfigureNotify:
	    fwid = evptr -> xconfigure.width;
	    fhgt = evptr -> xconfigure.height;
	    break;
      }
  }

static drawborder(r) struct rect r;
  { XDrawRectangle(display, frame, gc_copy, r.left + BORDER/2, r.top + BORDER/2, r.width + BORDER, r.height + BORDER);
  }

static char *cheap(n) int n;
  { /* called by "heap" macro to allocate n bytes */
    char *s = malloc(n);
    if (s == NULL) w_giveup("no room");
    return s;
  }

static w_giveup(msg, p1) char *msg; word p1;
  { fprintf(stderr, "windows: ");
    fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(2);
  }

