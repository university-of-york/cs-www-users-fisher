/* Modem for MIPS   AJF	  January 1995
   Procedures for starting sessions (V.8) */

#include <stdio.h>
#include <coro.h>

#include <myaudio.h>
#include <mystdio.h>

#include "modem.h"

#define MAXBYTES 8

typedef unsigned long long u64bit;

static u64bit menu;

static void txloop(), putmenu(u64bit);
static uint computemodes(vmode);
static bool samemsg(u64bit, u64bit);
static void norm(u64bit&);
static void rxloop();
static u64bit getmsg();
static int getbyte();


global void startsession()
  { my_alarm(10);   /* 10 sec timeout */
    menu = 0;
    setduplex(SAMPLERATE/5);	/* 0.2 secs */
    coroutine *txco = new coroutine(txloop);
    coroutine *rxco = new coroutine(rxloop);
    inparallel(txco, rxco);
    delete txco; delete rxco;
    my_alarm(0);    /* cancel alarm */
    infomsg("V.8 negotiation O.K.");
  }

static void txloop()
  { inittx_fsk(V21o);
    uint m = computemodes(veemode);
    u64bit msg = ((u64bit) 0xe0c1 << 48) | ((u64bit) m << 24);
    while (menu == 0) putmenu(msg);
    unless (samemsg(menu, msg)) giveup("Negotiation failed (V.8)");
    putasync(0); putasync(0); putasync(0);	/* send CJ */
    sendpause(0.075); flushoutput();
    callco(currentco -> creator);
  }

static void putmenu(u64bit msg)
  { // fprintf(stderr, ">>> %08x %08x\r\n", (uint) (msg >> 32), (uint) msg);
    putasync(-1);		/* 10 `1' bits */
    while (msg)
      { putasync(msg >> 56);
	msg <<= 8;
      }
  }

static uint computemodes(vmode m)	/* work out modulation octets, given vmode */
  { switch (m)
      { default:	return 0x051010;
	case V21o:	return 0x051090;
	case V23o:	return 0x051014;
	case V32o:	return 0x051110;
	case V34o:	return 0x451010;
      }
  }

static bool samemsg(u64bit m1, u64bit m2)
  { norm(m1); norm(m2);
    return (m1 == m2);
  }

static void norm(u64bit &m)
  { until ((m == 0) || (m & 0xff)) m >>= 8;	/* position at bottom om word */
    while ((m & 0xff) == 0x10) m >>= 8;		/* discard extension octets with no bits set */
  }

static void rxloop()
  { initrx_fsk(V21o);
    u64bit m1 = getmsg();
    int n = 0;
    while (n < 5)
      { u64bit m2 = getmsg();
	if (m1 == m2) n++; else n = 0;
	m1 = m2;
      }
    menu = m1;
    for (;;) getasync();
  }

static u64bit getmsg()
  { int b; u64bit msg = 0;
    do b = getbyte(); until (b == 0xe0);	/* look for sync word */
    do
      { msg = (msg << 8) | b;
	b = getbyte();
      }
    until (b == 0xe0);
    until (msg >> 56) msg <<= 8;
    // fprintf(stderr, "<<< %08x %08x\r\n", (uint) (msg >> 32), (uint) msg);
    return msg;
  }

static int getbyte()
  { int b;
    do b = getasync(); while (b < 0);	/* ignore timeouts */
    return b;
  }

