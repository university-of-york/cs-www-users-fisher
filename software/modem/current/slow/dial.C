/* Modem for MIPS   AJF	  January 1995
   Dialling routines */

#include <string.h>

#include <complex.h>
#include <sinegen.h>

#include "modem.h"

#define MAXSTRLEN  256

static void collapse(char*, char*, char*), senddtmf(char*);


global void dialnumber(char *telno)
  { char vec[MAXSTRLEN+1];
    strcpy(vec, &telno[1]);
    if (telno[0] == '+')
      { collapse(vec, "", "00");
	collapse(vec, "0044", "0");
	collapse(vec, "01904", "");
	collapse(vec, "", "791");       /* route via B.T. */
	if (vec[3] >= '2' && vec[3] <= '4') collapse(vec, "79143", ""); /* internal extension */
      }
    waitfortone(DIAL_TONE);
    infomsg("Dialling %s", vec);
    senddtmf(vec);
  }

static void collapse(char *ovec, char *s1, char *s2)
  { int n1 = strlen(s1), n2 = strlen(s2);
    if (strncmp(ovec, s1, n1) == 0)
      { char nvec[MAXSTRLEN+1];
	strcpy(nvec, s2); strcpy(&nvec[n2], &ovec[n1]);
	strcpy(ovec, nvec);
      }
  }

static float rowtab[4] = {  697.0,  770.0,  852.0,  941.0 };
static float coltab[4] = { 1209.0, 1336.0, 1477.0, 1633.0 };

static void senddtmf(char *s)
  { int k = 0;
    until (s[k] == '\0')
      { char *dstr = "123A456B789C*0#D";
	char *p = strchr(dstr, s[k++]);
	unless (p == NULL)
	  { int n = p - dstr;
	    sendfreqs(rowtab[n >> 2], coltab[n & 3], 0.1);  /* tones for 100 ms	  */
	    sendpause(0.1);				    /* silence for 100 ms */
	  }
      }
  }

