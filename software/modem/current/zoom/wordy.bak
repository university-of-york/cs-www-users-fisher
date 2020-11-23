#include <complex.h>
#include <myaudio.h>
#include <sinegen.h>

#include "modem.h"

static void sendinfo(uchar*, int), pbit(int), outsymbol(float);

static uchar info0c[] = { 0xf7, 0x2f, 0xf8, 0x00, 0x76, 0x8f, 0x80 };

static float shapetab[2*SYMBLEN+1] =
  { +1.0000000000, +0.9926801157, +0.9709416672, +0.9354394838,
    +0.8872360730, +0.8277597175, +0.7587485480, +0.6821831650,
    +0.6002108802, +0.5150649954, +0.4289827154, +0.3441252800,
    +0.2625037258, +0.1859133401, +0.1158793838, +0.0536160576,
    -0.0000000000, -0.0444411170, -0.0795250663, -0.1053974855,
    -0.1225017354, -0.1315382297, -0.1334156708, -0.1291968976,
    -0.1200421673, -0.1071526632, -0.0917168476, -0.0748619803,
    -0.0576127231, -0.0408582688, -0.0253289076, -0.0115824006,
    +0.0000000000,
  };

static float probing[4*SYMBLEN] =
  {  0.2380952,	 0.2393710,  0.1284067, -0.1539139, -0.0523930,	 0.0680675, -0.2297064,	 0.0045923,
     0.0812908, -0.1809436,  0.1310969, -0.0137466, -0.2081474,	 0.1719744,  0.1724568,	 0.1854495,
     0.0952381, -0.1779333,  0.0502188,	 0.2156533,  0.1802528, -0.0099911, -0.2293379,	 0.0361867,
     0.0139473, -0.2433915,  0.0585733,	 0.1934423, -0.1101886, -0.2021118, -0.0817082, -0.1327051,
    -0.2380952, -0.1327051, -0.0817082, -0.2021118, -0.1101886,	 0.1934423,  0.0585733, -0.2433915,
     0.0139473,	 0.0361867, -0.2293379, -0.0099911,  0.1802528,	 0.2156533,  0.0502188, -0.1779333,
     0.0952381,	 0.1854495,  0.1724568,	 0.1719744, -0.2081474, -0.0137466,  0.1310969, -0.1809436,
     0.0812908,	 0.0045923, -0.2297064,	 0.0680675, -0.0523930, -0.1539139,  0.1284067,	 0.2393710,
  };

static sinegen *carrier;


global void txside()
  { carrier = new sinegen(1200.0);
    pbit(0);	/* the "point of arbitrary phase" */
    while (mstate == 0) sendinfo(info0c, 45);
    while (mstate == 1) pbit(0);			/* B */
    pbit(1);	/* phase reversal */
    for (int i = 0; i < 38; i++) pbit(0);		/* Bbar */
    for (;;) outsymbol(0.0f);				/* silence */
  }

static void sendinfo(uchar *info, int nb)
  { int p = 0; uchar w;
    for (int i = 0; i < nb; i++)
      { if (i%8 == 0) w = info[p++];
	pbit(w >> 7);
	w <<= 1;
      }
  }

static void pbit(int b)
  { static float x = 1.0;
    if (b) x = -x;  /* diff. encode */
    outsymbol(x);
  }

static void outsymbol(float x)
  { static float a0 = 0.0, a1 = 0.0, a2 = 0.0, a3 = 0.0;
    a0 = a1; a1 = a2; a2 = a3; a3 = x;
    for (int k = 0; k < SYMBLEN; k++)
      { /* baseband pulse shaping */
	float s = shapetab[SYMBLEN + k]	  * a0
		+ shapetab[k]		  * a1
		+ shapetab[SYMBLEN - k]	  * a2
		+ shapetab[2*SYMBLEN - k] * a3;
	/* modulate onto carrier */
	float cx = carrier -> fnext();
	outsample(s*cx);
      }
  }

