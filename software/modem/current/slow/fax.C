/* Modem for MIPS   AJF	  January 1995
   Fax routines (T.30) */

#include <stdio.h>

#include <myaudio.h>

#include "modem.h"
#include "fcf.h"

#define MAXFRAME	252
#define MAXMESSAGE	(MAXFRAME+4)	/* frame plus address, control, checksum */
#define HDLC_TIMEOUT	(-3)

#define NO_STATE	0		/* state not set yet	 */
#define RX_CTL_STATE	1		/* receive control info	 */
#define TX_CTL_STATE	2		/* transmit control info */
#define RX_DOC_STATE	3		/* receive document	 */
#define TX_DOC_STATE	4		/* transmit document	 */

#define MYNAME	"+44 1904 432738"       /* sent in TSI/CSI frame */

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

static uchar dcstranstab[16] =
  { /* decodes DIS bits 21-23 into DCS equivalent */
    0, 1, 2, 2, 4, 1, 0, 7,	/* low density	*/
    0, 1, 2, 4, 4, 0, 2, 7,	/* high density */
  };

static short scanbitstab[8] =
  { /* decodes DCS scan time bits into num. of bits at 7200 bit/s */
    144, 288, 72, -1, 36, -1, -1, 0,
  };

static uchar mpsframe[] = { MPS };
static uchar eopframe[] = { EOP };
static uchar dcnframe[] = { DCN };
static uchar fttframe[] = { FTT };
static uchar cfrframe[] = { CFR };
static uchar mcfframe[] = { MCF };

static uchar disframe[] = { DIS, 0x00, 0xe2, 0x0e };	/* V.29 only; no 2d coding; set "fine" bit; no scan-line padding */

static int pagecount, state, rxval, framelen, msgendtime, scanbits;
static bool isfinal;
static uchar frame[MAXFRAME];
static uchar identframe[21], dcsframe[4];

static void senddoc(), makedcs(uchar*), sendtraining();
static void receivedoc(), getdcs(bool), checkdcs(uchar*);
static bool gettraining();
static void makeident(uchar);
static void getmessage(), printident(), getmsg(), nextval();
static void msgerror(char*, word = 0, word = 0, word = 0);
static void sendframe(uchar*, int, bool), printframe(char*, uchar*, int);
static char *frametype(uchar);
static bool csumok(uchar*, int);
static ushort computecsum(uchar*, int);
static void setstate(int);


global void becomefax()
  { pagecount = 0; state = NO_STATE;
    if (options & opt_org) senddoc(); else receivedoc();
  }

static void senddoc()
  { makeident(TSI);
    int ntries = 0;
    do
      { getmessage();	/* try for DIS */
	ntries++;
      }
    until ((frame[0] == DIS && framelen >= 4) || ntries >= 6);
    unless (frame[0] == DIS && framelen >= 4) giveup("Failed to get a valid DIS frame");
    makedcs(frame);				    /* construct DCS from DIS */
    sendtraining();
    while (pagecount < numpages)
      { setstate(TX_DOC_STATE);
	sendpage(pagecount+1, scanbits);
	ntries = 0;
	do
	  { sendframe((pagecount+1 < numpages) ? mpsframe : eopframe, 1, true);	    /* send MPS or EOP */
	    getmessage();	/* shd be MCF, RTP, RTN */
	    ntries++;
	  }
	until (frame[0] == MCF || frame[0] == RTP || frame[0] == RTN || ntries >= 3);
	unless (frame[0] == MCF || frame[0] == RTP || frame[0] == RTN) break;
	if (frame[0] == MCF || frame[0] == RTP) pagecount++;
	if (frame[0] == RTP || frame[0] == RTN) sendtraining();
      }
    sendframe(dcnframe, 1, true);   /* send DCN */
    if (pagecount < numpages) giveup("Document transmission failed");
    infomsg("Success - delivered %d pages", pagecount);
  }

static void makedcs(uchar *disfr)
  { /* make DCS frame from received DIS; establish capabilities */
    dcsframe[0] = DCS;
    dcsframe[1] = 0x00;
    unless (disfr[2] & 0x40) giveup("Remote fax can't do Group 3 (T.4)");
    uchar spd = (disfr[2] >> 2) & 0xf;
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
    unless (spd & 0x8) giveup("Remote fax can't do V.29");
  }

static void sendtraining()
  { int ntries = 0;
    do
      { sendframe(identframe, 21, false);	    /* send TSI */
	sendframe(dcsframe, 4, true);		    /* send DCS */
	if (options & opt_v) fprintf(stderr, ">>> TCF\n");
	setstate(TX_DOC_STATE);
	for (int j = 0; j < 10800; j++) putbit(0);  /* TCF, 1.5 sec @ 7200 bps */
	getmessage();
	ntries++;
      }
    until (frame[0] == CFR || ntries >= 3);
    unless (frame[0] == CFR) giveup("Remote fax training failed");
  }

static void receivedoc()
  { makeident(CSI);
    getdcs(true);
    bool ok = gettraining();
    int ntries = 0;
    until (ok || ntries >= 3)
      { getdcs(false);
	ok = gettraining();
	ntries++;
      }
    unless (ok) giveup("Local fax training failed");
    sendframe(cfrframe, 1, true);
    bool more;
    do
      { setstate(RX_DOC_STATE);
	receivepage(pagecount+1);
	ntries = 0;
	do
	  { getmessage();
	    ntries++;
	  }
	until (frame[0] == EOP || frame[0] == MPS || ntries >= 3);
	unless (frame[0] == EOP || frame[0] == MPS) giveup("Remote fax has given up");
	more = (frame[0] == MPS);
	sendframe(mcfframe, 1, true);
	pagecount++;
      }
    while (more);
    getmessage();	/* should be DCN */	// ???
    infomsg("Success - received %d pages", pagecount);
  }

static void getdcs(bool first)
  { int ntries = 0;
    do
      { if (first)
	  { sendframe(identframe, 21, false);	/* send CSI */
	    sendframe(disframe, 4, true);	/* send DIS */
	  }
	else sendframe(fttframe, 1, true);	/* send FTT */
	getmessage();	/* try for DCS */
	ntries++;
      }
    until ((frame[0] == DCS && framelen >= 4) || frame[0] == DCN || ntries >= 6);
    unless (frame[0] == DCS && framelen >= 4) giveup("Failed to get a valid DCS frame");
    checkdcs(frame);
  }

static void checkdcs(uchar *dcsfr)
  { /* check DCS frame from remote sender */
    uchar spd = (dcsfr[2] >> 2) & 0xf;
    if (dcsfr[2] & 0x02) options |= opt_H;
    infomsg("resolution=%c; speeds=%02x; scanbits=%d", (options & opt_H) ? 'H' : 'L', spd, scanbits);
    unless (dcsfr[2] & 0x40) giveup("Remote fax can't do Group 3 (T.4)");
    unless (spd & 0x8) giveup("Remote fax can't do V.29");
  }

static bool gettraining()
  { setstate(RX_DOC_STATE);
    int totnz = 0, tot = 0;
    for (int j = 0; j < 15; j++)
      { int nz = 0;
	for (int k = 0; k < 720; k++) if (!getbit()) nz++;
	if (options & opt_v) fprintf(stderr, "%d ", nz);
	/* V.29 training is short, so ignore first 720 symbols.
	   Spec says TCF is +-10%, so ignore last 1440 symbols. */
	unless (j == 0 || j == 14 || j == 15) { totnz += nz; tot += 720; }
      }
    bool ok = (tot-totnz < 10);		/* tolerate 0.1% error */
    if (options & opt_v)
      { fprintf(stderr, "= %d out of %d\n", totnz, tot);
	fprintf(stderr, "<<< TCF %s\n", ok ? "ok" : "fail");
      }
    return ok;
  }

static void makeident(uchar cmd)
  { int len = strlen(MYNAME);
    int p = 0;
    identframe[p++] = cmd;  /* TSI or CSI */
    while (len > 0) identframe[p++] = shuffle[MYNAME[--len]];
    while (p < 21) identframe[p++] = 0x04; /* pad with spaces */
  }

static void getmessage()
  { setstate(RX_CTL_STATE);
    isfinal = true;
    msgendtime = samplecount + 4*SAMPLERATE;	/* reset timeout counter (4 secs in future) */
    nextval();					/* read 1st byte of preamble */
    do
      { getmsg();
	if (frame[0] == CSI || frame[0] == TSI) printident();
      }
    until (isfinal || frame[0] == TMO);
  }

static void printident()
  { char buf[21];
    int p = framelen, k = 0;
    while (p > 1 && frame[p-1] == 0x04) p--; /* delete spaces */
    while (p > 1 && k < 20) buf[k++] = shuffle[frame[--p]];
    buf[k++] = '\0';
    infomsg("Remote fax identifies as: %s", buf);
  }

static void getmsg()
  { uchar message[MAXMESSAGE];
    int msglen;
    bool ok = false;
    until (ok || rxval == HDLC_TIMEOUT)
      { until (rxval == HDLC_FLAG || rxval == HDLC_TIMEOUT) nextval();
	int nflags = 0;
	while (rxval == HDLC_FLAG) { nflags++; nextval(); }
	unless (isfinal && nflags < 10) /* preamble too short? */
	  { msglen = 0;
	    until (rxval == HDLC_FLAG || rxval == HDLC_TIMEOUT || msglen >= MAXMESSAGE)
	      { unless (rxval == HDLC_ABORT) message[msglen++] = rxval;
		nextval();
	      }
	    unless (rxval == HDLC_TIMEOUT)
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
		else ok = true;
	      }
	  }
      }
    if (ok)
      { framelen = msglen-4;	/* length excludes addr, control, checksum */
	memcpy(frame, &message[2], framelen);
	isfinal = message[1] & 8;   /* final frame? */
      }
    else
      { frame[0] = TMO;		/* fake "timeout" frame */
	framelen = 1;
	isfinal = true;
      }
    printframe("<<<", frame, framelen);
  }

static void nextval()
  { rxval = (after(samplecount, msgendtime)) ? HDLC_TIMEOUT : getsync();
  }

static void msgerror(char *msg, word p1, word p2, word p3)
  { if (options & opt_v)
      { fprintf(stderr, "*** "); fprintf(stderr, msg, p1, p2, p3); putc('\n', stderr);
      }
  }

static void sendframe(uchar *fr, int frlen, bool final)
  { setstate(TX_CTL_STATE);
    printframe(">>>", fr, frlen);
    uchar message[MAXMESSAGE];
    message[0] = 0xff; message[1] = final ? 0xc8 : 0xc0;
    memcpy(&message[2], fr, frlen);
    ushort csum = computecsum(message, frlen+2);
    message[frlen+2] = (csum >> 8) ^ 0xff; message[frlen+3] = (csum & 0xff) ^ 0xff;
    unless (csumok(message, frlen+4)) giveup("Bug! Tx checksum");
    putsync(HDLC_FLAG); putsync(HDLC_FLAG);
    for (int i=0; i < frlen+4; i++) putsync(message[i]);
    putsync(HDLC_FLAG);
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
	case TMO:   return "TMO";
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

static void setstate(int st)
  { unless (st == state)
      { switch (st)
	  { case RX_CTL_STATE:
		flushoutput(); discardinput();			/* flush Tx before switching to Rx */
		initrx_fsk(V21o);
		break;

	    case TX_CTL_STATE:
		if (state == TX_DOC_STATE) sendpause(0.075);	/* pause before switching from TX_DOC to TX_CTL */
		inittx_fsk(V21a);
		for (int i=0; i < 40; i++) putsync(HDLC_FLAG);	/* send HDLC preamble */
		break;

	    case RX_DOC_STATE:
		/* old state is tx_ctl or rx_ctl */
		if (state == TX_CTL_STATE) { flushoutput(); discardinput(); }
		initrx_v29();
		break;

	    case TX_DOC_STATE:
		sendpause(0.075);	/* pause before switching from anything to TX_DOC */
		inittx_v29();
		break;
	  }
	state = st;
      }
  }

