/* Alpha document preparation system
   Sun window manager
   AJF	 May 1989 */

#include <stdio.h>
#include <signal.h>

#include "gfxlib.h"
#include "alpha.h"

#define ICONFN	 "/usr/fisher/sunlib/alpha/icon"
#define MENUFN	 "/usr/fisher/sunlib/alpha/menu"

#define REFRESH	 (1 << winchfd)
#define CH_DEATH (1 << childfd)

extern struct fpwin *fpw_create();
extern struct bitmap *ReadBitmap(), *Balloc();
extern char *malloc(), *realloc();

static struct fpwin *fpwin;
static struct bitmap *icon, *background;
static int winchfd, childfd;

static ushort bgnd_bits[] = /* from "/usr/sun/pix/texture/lightgrey" */
  { 0x2222, 0x8888, 0x2222, 0x8888, 0x2222, 0x8888, 0x2222, 0x8888,
    0x2222, 0x8888, 0x2222, 0x8888, 0x2222, 0x8888, 0x2222, 0x8888,
  };

static ushort curs_bits[] = /* from "/usr/sun/pix/cursor/bullseye" */
  { 0x07c0, 0x0fe0, 0x1830, 0x3018, 0x600c, 0xc006, 0xc386, 0xc386,
    0xc386, 0xc006, 0x600c, 0x3018, 0x1830, 0x0fe0, 0x07c0, 0x0000,
  };

fpr_static(bgnd_bm, 16, 16, 1, bgnd_bits);
fpr_static(curs_bm, 16, 16, 1, curs_bits);

static struct cursor cursor = { 0, 0, F_XOR, &curs_bm };


global main(argc, argv) int argc; char *argv[];
  { initialize(argc, argv);
    mainloop();
    exit(0);
  }

static initialize(argc, argv) int argc; char *argv[];
  { int wid, hgt, fwid, fhgt, mypid; struct rect prect;
    unless (argc == 9) w_giveup("parameter error");
    icon = ReadBitmap(ICONFN);
    if (icon == NULL) w_giveup("can't read icon \"%s\"", ICONFN);
    /* blow up bm to make replrop'ing the background faster */
    background = Balloc(128, 64, 1);
    fpr_replrop(background, 0, 0, 128, 64, F_STORE, &bgnd_bm, 0, 0);
    mypid = getpid();
    setpgrp(mypid, mypid);
    initspawn("windows"); initsigpipes("windows");
    winchfd = open_sigpipe(SIGWINCH);
    childfd = open_sigpipe(SIGCHLD);
    getrect(NULL, &prect); /* get parent rect, which normally describes physical screen */
    wid = atoi(argv[1]); hgt = atoi(argv[2]);
    fwid = 544+wid; fhgt = max(568+298, hgt);
    makeframe(0, max(prect.height-fhgt, 0), fwid, fhgt);				  /* frame (parent) */
    subwin(  0,	  0, 544, 568, "%s -d %p %s", argv[3], atoi(argv[5]), argv[4]);            /* editor window  */
    subwin(544,	  0, wid, hgt, "-cat %p %p -mm %s -cb 1", atoi(argv[7]), atoi(argv[6]), MENUFN);  /* preview window */
    subwin(  0, 568, 544, 298, "-cat %p -pm 4", atoi(argv[8]));                            /* message window */
  }

static makeframe(px, py, sx, sy) int px, py, sx, sy;
  { struct rect rect;
    rect.left = px; rect.top = py; rect.width = sx; rect.height = sy;
    fpwin = fpw_create(NULL);
    setrect(fpwin, &rect);
    fpw_setcursor(fpwin, &cursor);
    setenv("WINDOW_PARENT", fpwin -> wfname);
    setenv("DEFAULT_FONT", "/usr/lib/fonts/fixedwidthfonts/serif.r.10");
    sendmux(fpwin, 'T'); /* ask to be brought to the top of the pile */
  }

static setenv(s1, s2) char *s1, *s2;
  { char *s; int code;
    s = heap(MAXSTRLEN+1, char);
    sprintf(s, "%s=%s", s1, s2);
    s = realloc(s, strlen(s) + 1);
    code = putenv(s);
    if (code < 0) w_giveup("no room! (putenv)");
  }

static subwin(px, py, sx, sy, astr, p1, p2, p3) int px, py, sx, sy; char *astr; word p1, p2, p3;
  { char buf[MAXSTRLEN+1];
    strcpy(buf, "wshell -Wl alpha -pm 0 -nw -r %d %d %d %d "); strcat(buf, astr);
    spawn("/usr/fisher/sunbin/wshell", buf, px, py, px+sx, py+sy, p1, p2, p3);
  }

static mainloop()
  { bool running = true;
    while (running)
      { uint m = win_wait(REFRESH | CH_DEATH, -1);
	if (m & REFRESH)
	  { dismiss_signal(winchfd);
	    repairframe();
	  }
	if (m & CH_DEATH)
	  { dismiss_signal(childfd);
	    running = false;
	  }
      }
  }

static repairframe()
  { struct rect rect;
    getrect(fpwin, &rect);
    fpw_startdelta(fpwin);
    fpw_replrop(fpwin, 0, 0, rect.width, rect.height, F_STORE, background, 0, 0);
    fpw_rop(fpwin, 0, 0, icon -> wid, icon -> hgt, F_STORE, icon, 0, 0);
    fpw_border(fpwin, 0, 0, rect.width, rect.height, F_SET, NULL, 0, 0);
    fpw_donedelta(fpwin);
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

