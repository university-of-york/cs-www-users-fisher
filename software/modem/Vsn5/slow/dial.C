/* Modem for MIPS   AJF	  January 1995
   Dialling routines */

#include <string.h>
#include <fishaudio.h>
#include "modem.h"

#define MAXSTRLEN  256

static void insertpin(char*), collapse(char*, char*, char*), senddtmf(char*), sendfreqs(int, int, int);


global void dialnumber()
  { char vec[MAXSTRLEN+1];
    strcpy(vec, &telno[1]);
    if (telno[0] == '+')
      { collapse(vec, "", "00");
	collapse(vec, "0044", "0");
	if (options & opt_m) collapse(vec, "", "131,x");  /* add Mercury prefix & pin */
	collapse(vec, "01904", "");
	collapse(vec, "", "9");
	if (vec[3] >= '2' && vec[3] <= '4') collapse(vec, "943", "");   /* internal extension */
      }
    waitfortone(DIAL_TONE);
    infomsg("Dialling %s", vec);
    insertpin(vec);
    senddtmf(vec);
    Audio -> oflush();
  }

static void insertpin(char *vec)
  { for (int k=0; vec[k] != '\0'; k++)
      if (vec[k] == 'x') collapse(&vec[k], "x", mercurypin);
  }

static void collapse(char *ovec, char *s1, char *s2)
  { int n1 = strlen(s1), n2 = strlen(s2);
    if (strncmp(ovec, s1, n1) == 0)
      { char nvec[MAXSTRLEN+1];
	strcpy(nvec, s2); strcpy(&nvec[n2], &ovec[n1]);
	strcpy(ovec, nvec);
      }
  }

static short rowtab[4] = {  697,  770,	852,  941 };
static short coltab[4] = { 1209, 1336, 1477, 1633 };

static void senddtmf(char *s)
  { int k = 0;
    until (s[k] == '\0')
      { char c = s[k++];
	if (c == ',') waitfortone(MERC_TONE);	/* wait for secondary DT */
	else
	  { char *dstr = "123A456B789C*0#D";
	    char *p = strchr(dstr, c);
	    unless (p == NULL)
	      { uchar n = p - dstr;
		sendfreqs(rowtab[n >> 2], coltab[n & 3], SAMPLERATE/10);  /* tones for 100 ms	*/
		sendfreqs(0, 0, SAMPLERATE/10);				  /* silence for 100 ms */
	      }
	  }
      }
  }

static void sendfreqs(int f1, int f2, int ns)
  { int dph1 = (f1 * SINELEN) / SAMPLERATE;
    int dph2 = (f2 * SINELEN) / SAMPLERATE;
    int n = 0; uint sp1 = 0, sp2 = 0;
    while (n < ns)
      { int p1 = sp1 & (SINELEN-1), p2 = sp2 & (SINELEN-1);
	float val = (0.5 * MAXAMPL) * (sinetab[p1] + sinetab[p2]);
	if (val > MAXAMPL || val < -MAXAMPL) giveup("Bug! out of range (dial): %08x", (int) val);
	Audio -> write((int) val);
	sp1 += dph1; sp2 += dph2;
	n++;
      }
  }

