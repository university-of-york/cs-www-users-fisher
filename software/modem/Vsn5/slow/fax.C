/* Modem for MIPS   AJF	  January 1995
   Fax routines (T.30) */

#include <stdio.h>
#include <fishaudio.h>
#include "modem.h"
#include "fcf.h"

#define MAXFRAME	252
#define MAXMESSAGE	(MAXFRAME+4)	/* frame plus address, control, checksum */

#define RX_CTL_STATE	0		/* receive control info	 */
#define TX_CTL_STATE	1		/* transmit control info */
#define TX_DOC_STATE	2		/* transmit document	 */

#define MYNAME	"+44 1904 432767"       /* sent in TSI frame     */

static uchar shuffle[256] =
  { 0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240,
    8, 136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248,
    4, 132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244,
    12, 140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252,
    2, 130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242,
    10, 138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250,
    6, 134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246,
    14, 142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254,
    1, 129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241,
    9, 137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249,
    5, 133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245,
    13, 141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253,
    3, 131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243,
    11, 139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251,
    7, 135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247,
    15, 143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255,
  };

static uchar speedtab[16] =
  { /* Decodes DIS speed bits into capabilities:
       err  V.33  V.29	V.27fb	V.27  V.17  */
    0x04, 0x20, 0x20, 0x20, 0x02, 0x20, 0x20, 0x20,
    0x08, 0x20, 0x20, 0x20, 0x0a, 0x1b, 0x1a, 0x20,
  };

static uchar dcstranstab[16] =
  { /* decodes DIS bits 21-23 into DCS equivalent */
    0, 1, 2, 2, 4, 1, 0, 7,	/* low density	*/
    0, 1, 2, 4, 4, 0, 2, 7,	/* high density */
  };

static short scanbitstab[8] =
  { /* decodes DCS scan time bits into num. of bits at 7200 bit/s; factor (7/6) for safety  */
    168, 336, 84, -1, 42, -1, -1, 0,
  };

static uchar mpsframe[] = { MPS };
static uchar eopframe[] = { EOP };
static uchar dcnframe[] = { DCN };

static int rxval, framelen, msgendtime;
static bool isfinal, frameok, timeout;
static uchar state;
static uchar message[MAXMESSAGE];
static uchar frame[MAXFRAME];
static uchar tsiframe[21], dcsframe[4];

static void maketsi(), makedcs(uchar*), retrain(), setstate(uchar), sendpreamble();
static void getmessage(), printcsi(), getmsg(), resettimeout(), nextval();
static void msgerror(char*, word = 0, word = 0, word = 0);
static void sendframe(uchar*, int, bool), printframe(char*, uchar*, int);
static char *frametype(uchar);
static bool csumok(uchar*, int);
static ushort computecsum(uchar*, int);


global void becomefax()
  {
#ifdef ALARM
    alarm(600);		/* safety alarm goes off after 10 mins */
#endif
    maketsi();
    initrx(V21o); state = RX_CTL_STATE;
    int ntries = 0;
    do
      { getmessage();	/* try for DIS */
	ntries++;
      }
    until ((frameok && frame[0] == DIS && framelen >= 4) || ntries >= 6);
    unless (frameok && frame[0] == DIS && framelen >= 4) giveup("Failed to get a valid DIS frame");
    makedcs(frame);				    /* construct DCS from DIS */
    frame[0] = RTP;	/* set correct starting conditions for loop */
    while ((morepages || frame[0] == RTN) && frameok)	     /* !frameok means timeout */
      { uchar code = frame[0];
	if (code == MCF || code == RTP) readdocpage();
	if (code == RTP || code == RTN) retrain();
	setstate(TX_DOC_STATE);
	senddocpage();
	ntries = 0;
	do
	  { setstate(TX_CTL_STATE);
	    sendframe(morepages ? mpsframe : eopframe, 1, true);   /* send MPS or EOP */
	    setstate(RX_CTL_STATE);
	    getmessage();	/* shd be MCF, RTP, RTN */
	    ntries++;
	  }
	until (frameok || ntries >= 3);
	if (frameok && (frame[0] == MCF || frame[0] == RTP)) pagessent++;
      }
    setstate(TX_CTL_STATE);
    sendframe(dcnframe, 1, true);   /* send DCN */
    unless (frameok) giveup("Timeout; no response from remote fax");
    infomsg("Success - delivered %d pages", pagessent);
  }

static void maketsi()
  { int len = strlen(MYNAME);
    int p = 0;
    tsiframe[p++] = TSI;
    while (len > 0) tsiframe[p++] = shuffle[MYNAME[--len]];
    while (p < 21) tsiframe[p++] = 0x04; /* pad with spaces */
  }

static void makedcs(uchar *disfr)
  { /* make DCS frame from received DIS; establish capabilities */
    dcsframe[0] = DCS;
    dcsframe[1] = 0x00;
    unless (disfr[2] & 0x40) giveup("Remote fax can't do Group 3 (T.4)");
    uchar spd = speedtab[(disfr[2] >> 2) & 0xf];
    dcsframe[2] = 0x70;			/* set to V.29 7200 bit/s; no 2d coding; clear "fine" bit */
    uchar n = (disfr[3] >> 1) & 7;	/* DIS bits 21-23 */
    if (options & opt_H)
      { unless (disfr[2] & 0x02) giveup("Remote fax can't do 200 dpi; try -l");
	dcsframe[2] |= 0x02;		/* ask for high density */
	n += 8;
      }
    n = dcstranstab[n];			/* translate to DCS equivalent */
    dcsframe[3] = n << 1;		/* set A4 1728 dots per scan line; set scantime bits; clear extend bit */
    scanbits = scanbitstab[n];		/* min. scan time, in bits */
    infomsg("resolution=%c; speeds=%02x; scanbits=%d", (options & opt_H) ? 'H' : 'L', spd, scanbits);
    unless (spd & 8) giveup("Remote fax can't do V.29");
  }

static void retrain()
  { int ntries = 0;
    do
      { setstate(TX_CTL_STATE);
	sendframe(tsiframe, 21, false);		    /* send TSI */
	sendframe(dcsframe, 4, true);		    /* send DCS */
	setstate(TX_DOC_STATE);
	for (int j=0; j < 10800; j++) putbit(0);    /* TCF, 1.5 sec */
	setstate(RX_CTL_STATE);
	getmessage();
	ntries++;
      }
    until ((frameok && frame[0] == CFR) || ntries >= 3);
    unless (frameok && frame[0] == CFR) giveup("Remote fax training failed");
  }

static void setstate(uchar newst)
  { unless (newst == state)
      { switch (newst)
	  { case RX_CTL_STATE:
		Audio -> oflush();	    /* flush Tx before switching to Rx */
		initrx(V21o);
		break;

	    case TX_CTL_STATE:
		if (state == TX_DOC_STATE) sendpause(1800); /* pause before switching from TX_DOC to TX_CTL */
		inittx(V21a);
		sendpreamble();		    /* send HDLC preamble */
		break;

	    case TX_DOC_STATE:
		sendpause(1800);	    /* pause before switching from anything to TX_DOC */
		inittx(V29);
		break;
	  }
	state = newst;
      }
  }

static void sendpreamble()
  { for (int i=0; i < 40; i++) putsval(HDLC_FLAG);
  }

static void getmessage()
  { isfinal = true;
    nextval();	/* read 1st byte of preamble */
    do
      { getmsg();
	if (frameok && frame[0] == CSI) printcsi();
      }
    while (frameok && !isfinal);
  }

static void printcsi()
  { char buf[21];
    int p = framelen, k = 0;
    while (p > 1 && frame[p-1] == 0x04) p--; /* delete spaces */
    while (p > 1 && k < 20) buf[k++] = shuffle[frame[--p]];
    buf[k++] = '\0';
    infomsg("Remote fax identifies as: %s", buf);
  }

static void getmsg()
  { int msglen;
    resettimeout();
    frameok = false;
    until (frameok || timeout)
      { until (rxval == HDLC_FLAG || timeout) nextval();
	int nflags = 0;
	until (rxval != HDLC_FLAG || timeout) { nflags++; nextval(); }
	unless (isfinal && nflags < 10) /* preamble too short? */
	  { msglen = 0;
	    until (rxval == HDLC_FLAG || msglen >= MAXMESSAGE || timeout)
	      { unless (rxval == HDLC_ABORT) message[msglen++] = rxval;
		nextval();
	      }
	    unless (timeout)
	      { if (rxval != HDLC_FLAG)
		  msgerror("Message too long: %02x %02x %02x...", message[0], message[1], message[2]);
		else if (msglen < 5) /* ensure frame length .ge. 1 */
		  msgerror("Message too short");
		else unless (csumok(message, msglen))
		  msgerror("Checksum error");
		else unless (message[0] == 0xff)
		  msgerror("Got %02x after frame, exp ff", message[0]);
		else unless (message[1] == 0xc0 || message[1] == 0xc8)
		  msgerror("Got %02x after addr, exp c0 or c8", message[1]);
		else frameok = true;
	      }
	  }
      }
    if (frameok)
      { framelen = msglen-4; /* length excludes addr, control, checksum */
	memcpy(frame, &message[2], framelen);
	isfinal = message[1] & 8;   /* final frame? */
	printframe("<<<", frame, framelen);
	if (frame[0] == XCN) giveup("Called fax has disconnected you!");
      }
    else msgerror("Timeout");
  }

static void resettimeout()
  { msgendtime = time(NULL) + 4;    /* 4 secs in future */
    timeout = false;
  }

static void nextval()
  { rxval = getsval();
    if (time(NULL) >= msgendtime) timeout = true;
  }

static void msgerror(char *msg, word p1, word p2, word p3)
  { if (options & opt_v) infomsg(msg, p1, p2, p3);
  }

static void sendframe(uchar *fr, int frlen, bool final)
  { printframe(">>>", fr, frlen);
    message[0] = 0xff; message[1] = final ? 0xc8 : 0xc0;
    memcpy(&message[2], fr, frlen);
    ushort csum = computecsum(message, frlen+2);
    message[frlen+2] = (csum >> 8) ^ 0xff; message[frlen+3] = (csum & 0xff) ^ 0xff;
    unless (csumok(message, frlen+4)) giveup("Bug! Tx checksum");
    putsval(HDLC_FLAG); putsval(HDLC_FLAG);
    for (int i=0; i < frlen+4; i++) putsval(message[i]);
    putsval(HDLC_FLAG);
  }

static void printframe(char *io, uchar *fr, int frlen)
  { if (options & opt_v)
      { fprintf(stderr, "%s %s:", io, frametype(fr[0]));
	for (int i=0; i < frlen; i++) fprintf(stderr, " %02x", fr[i]);
	putc('\n', stderr);
      }
  }

static char *frametype(uchar x)
  { switch (x)
      { default:    return "???";
	case DIS:   return "DIS";
	case CSI:   return "CSI";
	case NSF:   return "NSF";
	case CFR:   return "CFR";
	case FTT:   return "FTT";
	case MCF:   return "MCF";
	case RTN:   return "RTN";
	case RTP:   return "RTP";
	case DCS:   return "DCS";
	case TSI:   return "TSI";
	case DCN:   return "DCN";
	case XCN:   return "XCN";
	case MPS:   return "MPS";
	case EOP:   return "EOP";
      }
  }

static bool csumok(uchar *msg, int len)
  { ushort csum = computecsum(msg, len);
    return (csum == 0x1d0f);
  }

static ushort computecsum(uchar *msg, int len)
  { /* CCITT V.41 crc (sort of) */
    ushort reg = 0xffff;
    while (len-- > 0)
      { uchar x = *(msg++);
	for (int j=0; j < 8; j++)
	  { uchar bit = (reg >> 15) ^ (x >> 7);
	    reg <<= 1; x <<= 1;
	    if (bit) reg ^= 0x1021;
	  }
      }
    return reg;
  }

