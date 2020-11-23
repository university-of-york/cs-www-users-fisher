/* pcalib - interactive calibration tool for AJF's LTU
   A.J. Fisher	 April 1996 */

#include <X11/Xlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <fishaudio.h>

#include "header.h"
#include "gfxlib.h"
#include "filters.h"

#define FONTNAME   "DEF"
#define PTSIZE	   18

#define BORDER	   2	/* even, so drawborder behaves predictably */
#define PANELSIZE  450
#define CANVASSIZE (2*PANELSIZE + 3*BORDER)
#define AXESSCALE  215
#define TIMESCALE  430

#define GORIGINX   465
#define TDORIGINY  225
#define FDORIGINY  675

#define BUTX	   16
#define BUTSIZE	   16
#define INFOX	   200
#define TEXTSEP	   16
#define TDINFOY	   (2*TEXTSEP)
#define FDINFOY	   (3*TEXTSEP)
#define FLINFOY	   (4*TEXTSEP)
#define DLINFOY	   (5*TEXTSEP)
#define TPINFOY	   440

#define MOUSEFN	   "/usr/fisher/bitmaps/mouse"
#define TOFN	   "/dev/ttyd2"
#define SAMPLERATE 9600		// ??? was 24000
#define AMPLITUDE  0x200000

/* plotting modes */
#define REALMODE   0
#define IMAGMODE   1
#define CARTMODE   2	/* real and imaginary */
#define MAGMODE	   3
#define PHMODE	   4
#define POLARMODE  5	/* magnitude and phase */
#define NUMMODES   6

static audio_pval iopvals[] =
  {
#if (SAMPLERATE == 24000)
    { AU_INPUT_RATE, AU_RATE_24000   },
    { AU_OUTPUT_RATE, AU_RATE_24000  },
#endif
#if (SAMPLERATE == 12000)
    { AU_INPUT_RATE, AU_RATE_12000   },
    { AU_OUTPUT_RATE, AU_RATE_12000  },
#endif
#if (SAMPLERATE == 9600)
    { AU_INPUT_RATE, AU_RATE_9600    },
    { AU_OUTPUT_RATE, AU_RATE_9600   },
#endif
    { AU_INPUT_SOURCE, AU_INPUT_LINE },
    { AU_LEFT_INPUT_ATTEN, 0	     },
    { AU_RIGHT_INPUT_ATTEN, 0	     },
    { AU_MONITOR_CTL, 0		     },
    { AU_SETIFILL, 1		     },
    { -1, -1			     },
  };

typedef double (*dfunc)(complex);

union word	/* for error msgs */
  { word(int ix)   { i = ix; }
    word(char *sx) { s = sx; }
    int i; char *s;
  };

static audio *Audio;
static rect canvasrect;
static fishfont *font;
static window *canvas;
static gfx_ctxt *gc_clr1, *gc_copy, *gc_thick, *gc_and, *gc_white;
static cursor *mouse_cur;
static uchar tdmode, fdmode;
static double tdxscale, tdyscale, fdxscale, fdyscale;
static int seqlen, seizefd, timedelay;
static complex tdvec[MAXSEQLEN], fdvec[MAXSEQLEN];
static filter *noise_filter;

static char *buttons[] =
  { "Quit", "Toggle TD", "Toggle FD", "Increase seqlen", "Decrease seqlen",
    "Seize/Release", "Impulse", "Noise",
    NULL,	/* terminator */
  };

#if (SAMPLERATE == 24000)
   static fspec *noise_fs = mkfilter("-Bu -Bp -o 6 -a 0.0125 0.1375");      /* 300 .. 3300 Hz */
#endif

#if (SAMPLERATE == 12000)
   static fspec *noise_fs = mkfilter("-Bu -Bp -o 6 -a 0.025 0.275");        /* 300 .. 3300 Hz */
#endif

#if (SAMPLERATE == 9600)
   static fspec *noise_fs = mkfilter("-Bu -Bp -o 6 -a 0.03125 0.34375");    /* 300 .. 3300 Hz */
#endif

static void initialize(), newhandler(), catchsignal(int), sighandler(int);
static void makecursor();
static void makegcs();
static void handleevent();
static void buttonpress(int, point);
static bool inbutton(point, int);
static void handlebutton(int, int), takesnapshot(int);
static float getsample(int, int);
static void tellpoint(double, int, char*);
static void scalegraph(double&, int);
static void togglemode(uchar&);
static void repaircanvas();
static void drawborder(rect);
static void drawbuttons();
static void drawgraphs();
static void drawgraph(complex[], uchar, double, double, int, int);
static char *modestring(uchar);
static void dgraph(complex[], dfunc, int, int, double, double);
static void wprintf(point, gfx_ctxt*, char*, word = 0);
static void giveup(char*, word = 0);


global int main(int argc, char *argv[])
  { initialize();
    for (;;) handleevent();
    /* not reached */
  }

#define EVENTMASK (ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask)

static void initialize()
  { set_new_handler(newhandler);
    catchsignal(SIGINT); catchsignal(SIGTERM);
    Audio = new audio(AU_IN | AU_OUT | AU_LOCK | AU_SAVE, iopvals);
    font = new fishfont(FONTNAME, PTSIZE);
    if (font -> ok) font -> getpixfont(BM_PIXM);
    unless (font -> ok && font -> pf != NULL) giveup("can't find font %s", FONTNAME);
    canvasrect = rect(100, 100, CANVASSIZE, CANVASSIZE);  /* left, top, width, height */
    canvas = new window(NULL, canvasrect);
    makecursor(); makegcs();
    canvas -> defcursor(mouse_cur);
    canvas -> setprops();
    canvas -> select(EVENTMASK);
    canvas -> map();
    canvasrect.left = canvasrect.top = 0; /* zero-origin from now on */
    tdmode = REALMODE; fdmode = MAGMODE;
    tdxscale = fdxscale = 1.0;
    tdyscale = fdyscale = 5.0;
    seqlen = 1024; seizefd = -1;
    initcompute(seqlen);
  }

static void newhandler()
  { giveup("No room");
  }

static void catchsignal(int sig)
  { signal(sig, (SIG_PF) sighandler);
  }

static void sighandler(int sig)
  { giveup("Signal %d!", sig);
  }

static void makecursor()
  { int col = colour(0, 0, 0xffff);			/* blue */
    bitmap *bm = new bitmap(MOUSEFN, BM_PIXM);
    unless (bm -> ok) giveup("can't read %s", MOUSEFN);
    mouse_cur = new cursor(bm, point(0,6), col);	/* "hot spot" is end of mouse's nose */
    delete bm;
  }

static void makegcs()
  { gc_clr1  = new gfx_ctxt(GXclear, 1, 1, 0);
    gc_copy  = new gfx_ctxt(GXcopy, 8, Black, White);
    gc_thick = new gfx_ctxt(GXcopy, 8, Black, White);
    gc_thick -> setlineattrs(BORDER);
    gc_and   = new gfx_ctxt(GXand, 8, Black, White);
    gc_white = new gfx_ctxt(GXcopy, 8, White, Black);
  }

static void handleevent()
  { XEvent event;
    NextEvent(&event);
    switch (event.type)
      { case ButtonPress:
	    buttonpress(event.xbutton.button, point(event.xbutton.x, event.xbutton.y));
	    break;

	case Expose:
	    repaircanvas();
	    break;
      }
  }

static void buttonpress(int bn, point bpt)
  { int swin = (bpt.x < PANELSIZE && bpt.y < PANELSIZE) ? 0 :
	       (bpt.x < PANELSIZE && bpt.y >= PANELSIZE) ? 1 :
	       (bpt.x >= PANELSIZE && bpt.y < PANELSIZE) ? 2 :
	       (bpt.x >= PANELSIZE && bpt.y >= PANELSIZE) ? 3 : -1;
    switch (swin)
      { default:
	    /* shdn't happen */
	    XBell(gfx_display, 0);
	    break;

	case 0:
	  { /* button panel */
	    int i = 0;
	    until (buttons[i] == NULL || inbutton(bpt,i)) i++;
	    if (buttons[i] != NULL) handlebutton(bn, i);
	    else XBell(gfx_display, 0);
	    break;
	  }

	case 1:
	    /* bottom left panel, unused */
	    XBell(gfx_display, 0);
	    break;

	case 2:
	    /* time-domain graph panel */
	    if (bn == 1) tellpoint(tdxscale * seqlen / (double) SAMPLERATE, bpt.x, "%11.4e s");
	    else
	      { double &sc = fabs(bpt.x - GORIGINX) < fabs(bpt.y - TDORIGINY) ? tdyscale : tdxscale;
		scalegraph(sc, bn);
		drawgraphs();
	      }
	    break;

	case 3:
	    /* freq-domain graph panel */
	    if (bn == 1) tellpoint(fdxscale * SAMPLERATE, bpt.x, "%11.4e Hz");
	    else
	      { double &sc = fabs(bpt.x - GORIGINX) < fabs(bpt.y - FDORIGINY) ? fdyscale : fdxscale;
		scalegraph(sc, bn);
		drawgraphs();
	      }
	    break;
      }
  }

static bool inbutton(point bpt, int bn)
  { return (bpt.x >= BUTX && bpt.x <= BUTX + BUTSIZE) &&
	   (bpt.y >= (2*bn+1)*BUTSIZE && bpt.y <= (2*bn+2)*BUTSIZE);
  }

static void handlebutton(int bn, int code)
  { switch (code)
      { default:
	    XBell(gfx_display, 0);	/* shdn't happen */
	    break;

	case 0:
	    /* quit */
	    exit(0);

	case 1:
	    /* toggle TD mode */
	    togglemode(tdmode);
	    drawgraphs();
	    break;

	case 2:
	    /* toggle FD mode */
	    togglemode(fdmode);
	    drawgraphs();
	    break;

	case 3:
	    /* increase seqlen */
	    if (seqlen < MAXSEQLEN) { seqlen *= 2; tdxscale /= 2.0; }
	    initcompute(seqlen); compute(tdvec, fdvec); timedelay = computedelay(tdvec);
	    drawgraphs();
	    break;

	case 4:
	    /* decrease seqlen */
	    if (seqlen > 1) { seqlen /= 2; tdxscale *= 2.0; }
	    initcompute(seqlen); compute(tdvec, fdvec); timedelay = computedelay(tdvec);
	    drawgraphs();
	    break;

	case 5:
	    /* toggle seize/release */
	    if (seizefd < 0)
	      { /* seize line */
		seizefd = open(TOFN, O_RDWR); /* this asserts DTR */
		if (seizefd < 0) giveup("can't open %s", TOFN);
	      }
	    else
	      { close(seizefd);
		seizefd = -1;
	      }
	    break;

	case 6:	    case 7:
	    /* take snapshot */
	    takesnapshot(code);
	    compute(tdvec, fdvec); timedelay = computedelay(tdvec);
	    drawgraphs();
	    break;
      }
  }

static void debugcounts(); // ???
static void my_setduplex(int);
static int gdelay();

static void takesnapshot(int code)
  { if (code == 7) noise_filter = new filter(noise_fs);
    my_setduplex(500);
//  fprintf(stderr, "??? Before: "); debugcounts(); Audio -> debuggit();
    for (int i=0; i < seqlen; i++)
      { float tval = getsample(code, i);
	Audio -> write((int) (AMPLITUDE * tval));
      }
//  fprintf(stderr, "??? Middle: "); debugcounts(); Audio -> debuggit();
    for (int i=0; i < seqlen; i++) tdvec[i] = complex(Audio -> read());
//  fprintf(stderr, "??? After:  "); debugcounts(); Audio -> debuggit();
//  putc('\n', stderr);
    if (code == 7) delete noise_filter;
  }

static void my_setduplex(int d)
  { fprintf(stderr, "[%d:", d);
    Audio -> idiscard(); Audio -> odiscard();
    int ns = gdelay();
    until (ns == d)
      { while (ns > d) { Audio -> read(); ns--; }	    /* delay too big, discard input */
	while (ns < d) { Audio -> write(0); ns++; }	    /* delay too small, prime output buffer */
	ns = gdelay();
      }
    fprintf(stderr, "]\r\n");
  }

static int gdelay()
  { int ni1, ni2, no1, no2;
    ni2 = Audio -> icount();
    no2 = Audio -> ocount();
    do
      { ni1 = ni2; no1 = no2;
	ni2 = Audio -> icount();
	no2 = Audio -> ocount();
      }
    until (ni1 == ni2 && no1 == no2);
    fprintf(stderr, "%d+%d/ ", ni1, no1);
    return ni1 + no1;
  }

static void debugcounts()
  { int ic = Audio -> icount(), oc = Audio -> ocount();
    fprintf(stderr, "ic=%5d oc=%5d ic+oc=%5d\n", ic, oc, ic+oc);
  }

static float getsample(int code, int n)
  { switch (code)
      { default:
	    giveup("Bug! code=%d", code);

	case 6:
	    return (n == 0) ? 1.0 : 0.0;

	case 7:
	  { static uint wd;
	    if (n == 0) wd = 0; /* initialize */
	    wd >>= 1;
	    uint bit = (1 ^ (wd >> 18) ^ wd) & 1;
	    if (bit) wd |= 0x800000;
	    float x = noise_filter -> fstep(bit ? +0.2 : -0.2);
	    if (x < -1.0 || x > +1.0) giveup("Amplitude out of range!");
	    return x;
	  }
      }
  }

static void tellpoint(double scale, int x, char *fmt)
  { XEvent event;
    do
      { char buf[MAXSTR+1]; sprintf(buf, fmt, (double) (x - GORIGINX) * scale / (double) TIMESCALE);
	wprintf(point(INFOX, TPINFOY), gc_copy, buf);
	NextEvent(&event);
	x = event.xbutton.x;
      }
    until (event.type == ButtonRelease); /* until button comes up */
    wprintf(point(INFOX, TPINFOY), gc_copy, "%14c", ' ');
  }

static void scalegraph(double &scale, int bn)
  { if (bn == 2 && scale < 1000.0) scale *= 1.5;
    if (bn == 3 && scale > 0.0001) scale /= 1.5;
  }

static void togglemode(uchar &mode)
  { if (++mode >= NUMMODES) mode = 0;
  }

static void repaircanvas()
  { FillRectangle(canvas, gc_white, canvasrect);
    drawborder(rect(0, 0, PANELSIZE, PANELSIZE));
    drawborder(rect(0, PANELSIZE + BORDER, PANELSIZE, PANELSIZE));
    drawborder(rect(PANELSIZE + BORDER, 0, PANELSIZE, PANELSIZE));
    drawborder(rect(PANELSIZE + BORDER, PANELSIZE + BORDER, PANELSIZE, PANELSIZE));
    drawbuttons(); drawgraphs();
  }

static void drawborder(rect r)
  { DrawRectangle(canvas, gc_thick, rect(r.left + BORDER/2, r.top + BORDER/2, r.width + BORDER, r.height + BORDER));
  }

static void drawbuttons()
  { int buty = BUTSIZE;
    for (int i = 0; buttons[i] != NULL; i++)
      { DrawRectangle(canvas, gc_thick, rect(BUTX + BORDER/2, buty + BORDER/2, BUTSIZE + BORDER, BUTSIZE + BORDER));
	wprintf(point(BUTX + 2*BUTSIZE, buty + BUTSIZE), gc_copy, buttons[i]);
	buty += 2*BUTSIZE;
      }
  }

static void drawgraphs()
  { char buf[MAXSTR+1];
    FillRectangle(canvas, gc_white, rect(PANELSIZE + 2*BORDER, BORDER, PANELSIZE, PANELSIZE));
    sprintf(buf, "TD: %s x %10.4e (X) %10.4e (Y)", modestring(tdmode), tdxscale, tdyscale);
    wprintf(point(INFOX, TDINFOY), gc_copy, buf);
    drawgraph(tdvec, tdmode, tdxscale, tdyscale, GORIGINX, TDORIGINY);
    FillRectangle(canvas, gc_white, rect(PANELSIZE + 2*BORDER, PANELSIZE + 2*BORDER, PANELSIZE, PANELSIZE));
    sprintf(buf, "FD: %s x %10.4e (X) %10.4e (Y)", modestring(fdmode), fdxscale, fdyscale);
    wprintf(point(INFOX, FDINFOY), gc_copy, buf);
    drawgraph(fdvec, fdmode, fdxscale, fdyscale, GORIGINX, FDORIGINY);
    wprintf(point(INFOX, FLINFOY), gc_copy, "Seqlen: %4d", seqlen);
    wprintf(point(INFOX, DLINFOY), gc_copy, "Delay:  %4d samples", timedelay);
  }

static void drawgraph(complex vec[], uchar mode, double xscale, double yscale, int xorg, int yorg)
  { DrawLine(canvas, gc_copy, point(xorg, yorg), point(xorg + TIMESCALE, yorg));		/* horiz */
    DrawLine(canvas, gc_copy, point(xorg, yorg - AXESSCALE), point(xorg, yorg + AXESSCALE));	/* vert	 */
    if (mode == REALMODE || mode == CARTMODE)
      { /* plot real component */
	dgraph(vec, creal, xorg, yorg, xscale, yscale * (2.0*AMPLITUDE));
      }
    if (mode == IMAGMODE || mode == CARTMODE)
      { /* plot imaginary component */
	dgraph(vec, cimag, xorg, yorg, xscale, yscale * (2.0*AMPLITUDE));
      }
    if (mode == MAGMODE || mode == POLARMODE)
      { /* plot magnitude */
	dgraph(vec, cmag, xorg, yorg, xscale, yscale * (2.0*AMPLITUDE));
      }
    if (mode == PHMODE || mode == POLARMODE)
      { /* plot phase */
	dgraph(vec, carg, xorg, yorg, xscale, PI);
      }
  }

static char *modestring(uchar mode)
  { switch (mode)
      { default:	return "???      ";
	case REALMODE:	return "Real     ";
	case IMAGMODE:	return "Imag     ";
	case CARTMODE:	return "Real/Imag";
	case MAGMODE:	return "Mag      ";
	case PHMODE:	return "Phase    ";
	case POLARMODE: return "Mag/Phase";
      }
  }

static void dgraph(complex vec[], dfunc func, int xorg, int yorg, double xscale, double yscale)
  { double fvec[MAXSEQLEN];
    for (int i = 0; i < seqlen; i++) fvec[i] = func(vec[i]);
    for (int i = 1; i < seqlen; i++)
      { double t0 = (double) (i-1) / (xscale*seqlen), t1 = (double) i / (xscale*seqlen);
	double y0 = fvec[i-1] / yscale, y1 = fvec[i] / yscale;
	if ((t0 >= 0.0 && t1 <= 1.0) && (y0 >= -1.0 && y1 <= +1.0))
	  { DrawLine(canvas, gc_copy,
		     point(xorg + ifix(t0 * TIMESCALE), yorg - ifix(y0 * AXESSCALE)),
		     point(xorg + ifix(t1 * TIMESCALE), yorg - ifix(y1 * AXESSCALE)));
	  }
      }
  }

static void wprintf(point pt, gfx_ctxt *gc, char *fmt, word p1)
  { char buf[MAXSTR+1];
    sprintf(buf, fmt, p1);
    WriteString(buf, font, canvas, gc, pt);
  }

static void giveup(char *msg, word p1)
  { fprintf(stderr, "pcalib: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

