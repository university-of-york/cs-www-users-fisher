/* phoneman -- telephone manager
   obey -- obey script	 A.J. Fisher   March 1996 */

#include <stdio.h>
#include <fcntl.h>
#include <fishaudio.h>

#include "filters.h"
#include "phoneman.h"

#define NU_DURATION	20		/* play NU for 20 secs	*/
#define AU_MAGIC	0x2e736e64	/* ".snd" magic number  */
#define TOFN		"/dev/ttyd2"

static fspec *lpf_fs = mkfilter("-Bu -Lp -o 2 -a 0.00125");     /* 10 Hz */

static int seizefd;
static tone *nu_tone, *beep_tone;
static char *numval;

static void obey(pnode*);
static char *substp(char*);
static void obeyaccept(pnode*);
static pnode *findbranch(pnode*, int, bool&);
static char dig_to_char(int);
static pnode *findbr(pnode*, char*);
static bool nummatch(char*, char*);
static void playnu();
static void record(char*, int), speak(char*, int), readheader(int[], FILE*), writeheader(int[], FILE*);
static void seizeline(), releaseline(), waitforring();
static bool ringstep(filter*);


global void obeyscript(pnode *x)
  { initdtmf();
    nu_tone = new tone(400.0);
    beep_tone = new tone(440.0);
    numval = copystr("");
    obey(x);
    delete nu_tone; delete beep_tone;
    tidydtmf();
  }

static void obey(pnode *x)
  { unless (x == NULL) switch (x -> h1)
      { default:
	    giveup("bug: ilgl h1 in obey: %d", x -> h1);

	case s_seq:
	    obey(x -> h2);
	    obey(x -> h3);
	    break;

	case s_print:
	    if (x -> str1 != NULL) fputs(substp(x -> str1), stderr);
	    putc('\n', stderr);
	    break;

	case s_speak:
	    speak(substp(x -> str1), x -> num);
	    Audio -> oflush();
	    break;

	case s_record:
	    Audio -> idiscard();
	    record(substp(x -> str1), x -> num);
	    break;

	case s_seize:
	    seizeline();
	    break;

	case s_release:
	    releaseline();
	    break;

	case s_wtring:
	    Audio -> idiscard();
	    waitforring();
	    break;

	case s_sleep:
	    sleep(x -> num);
	    break;

	case s_loop:
	    for (;;) obey(x -> h2);

	case s_accept:
	    obeyaccept(x);
	    break;

	case s_run:
	  { char *s1 = substp(x -> str1);
	    char *s2 = (x -> str2 != NULL) ? substp(x -> str2) : NULL;
	    runprog(s1, s2, false);
	    break;
	  }

	case s_connect:
	    runprog(substp(x -> str1), NULL, true);
	    break;
      }
  }

static char *substp(char *s)
  { int t = time(NULL);
    char uid[9]; sprintf(uid, "%08x", t);   /* sub for $uid */
    char *ct = ctime(&t); ct[19] = '\0';
    return substparams(s, giveup, numval, uid, ct);
  }

static void obeyaccept(pnode *x)
  { bool dt = true;	/* send dial tone on first digit only */
    putc('[', stderr);
    while (x != NULL && x -> h1 == s_accept) x = findbranch(x -> h2, x -> num, dt);
    fprintf(stderr, "]\n");
    if (x != NULL) obey(x);
    else
      { playnu();
	Audio -> oflush();
      }
  }

static pnode *findbranch(pnode *x, int maxd, bool &dt)
  { char vec[MAXSTR+1]; int nd = 0; pnode *br = NULL; bool tmo = false;
    until (br != NULL || nd >= maxd || tmo)
      { if (dt) Audio -> idiscard();	/* if first, get rid of echoed speech */
	int dig = getdtmf(dt);
	if (dig < 0)			/* timeout */
	  { putc('T', stderr);
	    tmo = true;
	  }
	else
	  { char c = dig_to_char(dig);
	    vec[nd++] = c; vec[nd] = '\0';
	    putc(c, stderr);
	    if (dt)
	      { Audio -> odiscard();	/* terminate dial tone */
		dt = false;
	      }
	    br = findbr(x, vec);
	  }
      }
    return br;
  }

static char dig_to_char(int dig)
  { static char *tab = "0123456789*#ABCDf";     /* 'f' means 'fax' */
    return (dig >= 0 && dig <= 16) ? tab[dig] : '?';
  }

static pnode *findbr(pnode *x, char *vec)
  { switch (x -> h1)
      { default:
	    giveup("bug: ilgl h1 in findbr: %d", x -> h1);

	case s_or:
	  { pnode *y = findbr(x -> h2, vec);
	    return (y != NULL) ? y : findbr(x -> h3, vec);
	  }

	case s_branch:
	    return (nummatch(x -> str1, vec)) ? x -> h2 : NULL;
      }
  }

static bool nummatch(char *s1, char *s2)
  { int k = 0, p = 0; bool ok = true;
    char sub[MAXSTR+1];
    until ((s1[k] == '\0' && s2[k] == '\0') || !ok)
      { if (s1[k] == 'x' && s2[k] >= '0' && s2[k] <= '9') sub[p++] = s2[k];	/* 'x' matches any digit */
	else if (s1[k] != s2[k]) ok = false;
	k++;
      }
    if (ok)
      { sub[p] = '\0';
	delete numval;
	numval = copystr(sub);
      }
    return ok;
  }

static void playnu()
  { int n = 0;
    while (n < NU_DURATION*SAMPLERATE)
      { for (int i=0; i < TONELEN; i++) Audio -> write(mu_expand(nu_tone -> vec[i]) << 8);
	n += TONELEN;
      }
  }

static int new_header[8] =	/* header for empty message file */
  { AU_MAGIC,			/* ".snd" magic number */
    32,				/* length of hdr */
    0,				/* length of body */
    1, SAMPLERATE, 1, 0, 0,	/* don't ask me what this means! */
  };

static void record(char *fn, int dur)
  { FILE *fi = fopen(fn, "r+"); /* open for update */
    if (fi == NULL)
      { fprintf(stderr, "Creating new %s\n", fn);
	fi = fopen(fn, "w+");   /* create for update */
	if (fi == NULL) giveup("can't create new msg file %s", fn);
	writeheader(new_header, fi);
	fseek(fi, 0, 0);	/* rewind */
      }
    int header[8];
    readheader(header, fi);
    int code = fseek(fi, header[1] + header[2], 0); /* position to end of data */
    if (code < 0) giveup("fseek failed (1)");
    int nb = 0, eomcnt = 0;
    filter *lpf = new filter(lpf_fs);
    for (int i=0; i < dur*SAMPLERATE; i++)
      { int val = Audio -> read() / 300;
	float x = (float) val / 32768.0;
	float y = lpf -> fstep(x*x);
	// if (i%100 == 0) printf("%14.4e\n", y);
	if (y > 0.1) eomcnt++; else eomcnt = 0;
	if (eomcnt > SAMPLERATE/2) break;	/* stop recording if 0.5 secs of DT or NU detected */
	if (val < -32768 || val > +32767)
	  { fprintf(stderr, "Out of range: %d\n", val);
	    if (val < -32768) val = -32768;
	    if (val > +32767) val = +32767;
	  }
	putc(mu_compress(val), fi); nb++;
      }
    delete lpf;
    for (int i=0; i < TONELEN; i++) { putc(beep_tone -> vec[i], fi); nb++; } /* record beep */
    fflush(fi);
    header[2] += nb;		/* update byte count in header */
    fseek(fi, 0, 0);		/* rewind file */
    writeheader(header, fi);	/* re-write header */
    fclose(fi);
  }

static void speak(char *fn, int num)
  { FILE *fi = fopen(fn, "r");
    if (fi == NULL) giveup("can't open %s", fn);
    int header[8];
    readheader(header, fi);
    for (int i=0; i < num; i++)
      { int code = fseek(fi, header[1], 0); /* position to start of data */
	if (code < 0) giveup("fseek failed (2)");
	int val = getc(fi);
	while (val >= 0)
	  { Audio -> write(mu_expand(val) << 8);
	    val = getc(fi);
	  }
      }
    fclose(fi);
  }

static void readheader(int hdr[], FILE *fi)
  { int ni = fread(hdr, sizeof(int), 8, fi);
    unless (ni == 8) giveup("fread failed");
    unless (hdr[0] == AU_MAGIC && hdr[1] >= 28 && hdr[2] >= 0) giveup("bad header");
  }

static void writeheader(int hdr[], FILE *fi)
  { int ni = fwrite(hdr, sizeof(int), 8, fi);
    unless (ni == 8) giveup("fwrite failed");
  }

static void seizeline()
  { sleep(2); /* wait in case line has only just been dropped */
    seizefd = open(TOFN, O_RDWR); /* this asserts DTR */
    if (seizefd < 0) giveup("can't open %s", TOFN);
  }

static void releaseline()
  { close(seizefd);
  }

static void waitforring()
  { filter *lpf = new filter(lpf_fs);
    bool sig;
    for (int i=0; i < SAMPLERATE/4; i++) sig = ringstep(lpf);	/* let filter stabilize */
    while (sig) sig = ringstep(lpf);				/* wait for no ring	*/
    until (sig) sig = ringstep(lpf);				/* wait for ring	*/
    delete lpf;
  }

static bool ringstep(filter *lpf)
  { float x = (float) Audio -> read() / 32768.0;
    float y = lpf -> fstep(x*x);
    static int n = 0;
    return (y > 5000.0);
  }

