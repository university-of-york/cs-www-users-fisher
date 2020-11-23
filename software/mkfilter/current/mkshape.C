/* mkshape -- design raised cosine FIR filter
   A.J. Fisher, University of York   <fisher@minster.york.ac.uk>
   November 1996 */

#include <stdio.h>
#include <math.h>

#include "mkfilter.h"
#include "complex.h"

#define SEQLEN 2048

#define opt_l	0x01
#define opt_c	0x02	/* raised cosine */
#define opt_r	0x04	/* root raised cosine */
#define opt_h	0x08	/* Hilbert transformer */
#define opt_i	0x10	/* identity function */
#define opt_w	0x20	/* apply windowing */
#define opt_x	0x40	/* x / sin x compensation */
#define opt_b	0x80	/* truncate coeffs */

static uint options;
static int numcoeffs, truncbits;
static double alpha, beta;
static double xcoeffs[MAXPZ+1];
static complex circle[SEQLEN/2];

static void readcmdline(char**);
static double getf(char*);
static int geti(char*);
static void usage(), initcircle(), computefilter(), compute_rc(), compute_ht();
static void apply_window(), trunc_coeffs();
static void printresults(char**), printcmdline(char**);
static void fft(complex*, complex*, int);
static void giveup(char*, int = 0);


global void main(int argc, char **argv)
  { readcmdline(argv);
    if (options & opt_l)
      { computefilter();
	printresults(argv);
      }
    else printf("OK!\n");
    exit(0);
  }

static void readcmdline(char **argv)
  { int ap = 0;
    unless (argv[ap] == NULL) ap++;	/* skip program name */
    options = numcoeffs = truncbits = 0;
    while (argv[ap] != NULL && argv[ap][0] == '-')
      { char *s = argv[ap++]; s++;
	uint opts = 0;
	until (*s == '\0')
	  { char ch = *(s++);
	    if (ch == 'c') opts |= opt_c;
	    else if (ch == 'r') opts |= opt_r;
	    else if (ch == 'h') opts |= opt_h;
	    else if (ch == 'i') opts |= opt_i;
	    else if (ch == 'l') opts |= opt_l;
	    else if (ch == 'x') opts |= opt_x;
	    else if (ch == 'w') opts |= opt_w;
	    else if (ch == 'b') opts |= opt_b;
	    else usage();
	  }
	if (opts & (opt_c | opt_r))
	  { alpha = getf(argv[ap++]);
	    beta = getf(argv[ap++]);
	  }
	if (opts & (opt_c | opt_r | opt_h | opt_i))
	  { numcoeffs = geti(argv[ap++]);
	    unless (numcoeffs & 1) giveup("sorry, filter length must be odd");
	    /* (num.zeros) = (num.coeffs) - 1 */
	    if (numcoeffs < 1 || numcoeffs > MAXPZ+1) giveup("filter length must be in range 1 .. %d", MAXPZ+1);
	  }
	if (opts & opt_b)
	  { truncbits = geti(argv[ap++]);
	    if (truncbits <= 0) giveup("Num. bits after -b must be .gt. 0");
	  }
	options |= opts;
      }
    unless (argv[ap] == NULL) usage();
  }

static double getf(char *s)
  { if (s == NULL) usage();
    return atof(s);
  }

static int geti(char *s)
  { if (s == NULL) usage();
    return atoi(s);
  }

static void usage()
  { fprintf(stderr, "Mkshape V.%s from <fisher@minster.york.ac.uk>\n", VERSION);
    fprintf(stderr, "Usage: mkshape -{cr} <alpha> <beta> <len> [-{lwx}] [-b nb]\n");
    fprintf(stderr, "       mkshape -i <len> [-{lwx}] [-b nb]\n");
    fprintf(stderr, "       mkshape -h <len> [-{lw}] [-b nb]\n");
    exit(1);
  }

static void initcircle()
  { for (int i = 0; i < SEQLEN/2; i++)
      { double x = (TWOPI * i) / SEQLEN;
	circle[i] = expj(x);	/* inverse fft */
      }
  }

static void computefilter()
  { switch (options & (opt_c | opt_r | opt_h | opt_i))
      { default:
	    giveup("Must have exactly one of -{crhi}");

	case opt_c:	case opt_r:	case opt_i:
	    initcircle();
	    compute_rc();
	    apply_window();
	    trunc_coeffs();
	    break;

	case opt_h:
	    if (options & opt_x) giveup("Can't combine -h with -x");
	    compute_ht();
	    apply_window();
	    trunc_coeffs();
	    break;
      }
  }

static void compute_rc()	/* (Root?) Raised Cosine */
  { complex vec[SEQLEN], temp[SEQLEN];
    double f1 = (options & opt_i) ? 0.5 : (1.0-beta)*alpha;
    double f2 = (options & opt_i) ? 0.5 : (1.0+beta)*alpha;
    double tau = (options & opt_i) ? 1.0 : 0.5/alpha;
    for (int i = 0; i <= SEQLEN/2; i++)
      { double f = (double) i / (double) SEQLEN;
	vec[i] = (f <= f1) ? 1.0 :
		 (f <= f2) ? 0.5 * (1.0 + cos((PI*tau/beta) * (f-f1))) :
		 0.0;
      }
    if (options & opt_r)
      { for (int i = 0; i <= SEQLEN/2; i++) vec[i].re = sqrt(vec[i].re);
      }
    if (options & opt_x)
      { for (int i = 1; i <= SEQLEN/2; i++)
	  { double x = PI * (double) i / (double) SEQLEN;
	    vec[i].re *= (x / sin(x));
	  }
      }
    for (int i = 0; i <= SEQLEN/2; i++) vec[i].re *= tau;
    for (int i = 1; i < SEQLEN/2; i++) vec[SEQLEN-i] = vec[i];
    fft(vec, temp, SEQLEN);	/* inverse fft */
    int h = (numcoeffs-1)/2;
    for (int i = 0; i < numcoeffs; i++)
      { int j = (SEQLEN-h+i) % SEQLEN;
	xcoeffs[i] = vec[j].re / (double) SEQLEN;
      }
  }

static void compute_ht()	/* Hilbert Transformer */
  { int h = (numcoeffs-1) / 2;
    xcoeffs[h] = 0.0;
    for (int i = 1; i <= h; i++)
      { double x = (i & 1) ? (1.0 / (double) i) : 0.0;
	xcoeffs[h+i] = -x;
	xcoeffs[h-i] = +x;
      }
  }

static void apply_window()	/* apply Hamming window to impulse response */
  { if (options & opt_w)
      { int h = (numcoeffs-1) / 2;
	for (int i = 1; i <= h; i++)
	  { double w = 0.54 - 0.46 * cos(TWOPI * (double) (h+i) / (double) (numcoeffs-1));
	    xcoeffs[h+i] *= w;
	    xcoeffs[h-i] *= w;
	  }
      }
  }

static void trunc_coeffs()
  { if (options & opt_b)
      { double fac = pow(2.0, (double) (truncbits-1));
	int h = (numcoeffs-1) / 2;
	double max = (options & opt_h) ? xcoeffs[h-1] : xcoeffs[h];	/* max coeff */
	double scale = (fac-1.0) / (fac*max);
	for (int i = 0; i < numcoeffs; i++)
	  { double x = xcoeffs[i] * scale;	/* scale coeffs so max is (fac-1.0)/fac */
	    xcoeffs[i] = fix(x*fac) / fac;	/* truncate */
	  }
      }
  }

static void printresults(char **argv)
  { printcmdline(argv);
    double gain = 0.0;
    if (options & opt_h)
      { int p = ((numcoeffs-1)/2 & 1) ? 0 : 1;
	bool odd = false;
	for (int i = p; i < numcoeffs && xcoeffs[i] > 0.0; i += 2)
	  { if (odd) gain += xcoeffs[i]; else gain -= xcoeffs[i];
	    odd ^= true;
	  }
	if (odd) gain = -gain;
	gain *= 2.0;
      }
    else
      { for (int i = 0; i < numcoeffs; i++) gain += xcoeffs[i];
      }
    printf("G  = %18.10e\n", gain);
    printf("NZ = %d\n", numcoeffs-1);
    for (int i = 0; i < numcoeffs; i++) printf("%18.10e\n", xcoeffs[i]);
    printf("NP = %d\n", numcoeffs-1);
    for (int i = 0; i < numcoeffs-1; i++) printf("0\n");
    printf("-1\n");
  }

static void printcmdline(char **argv)
  { int k = 0;
    until (argv[k] == NULL)
      { if (k > 0) putchar(' ');
	fputs(argv[k++], stdout);
      }
    putchar('\n');
 }

static void fft(complex *data, complex *temp, int n)
  { if (n > 1)
      { int h = n/2;
	for (int i = 0; i < h; i++)
	  { int i2 = i*2;
	    temp[i] = data[i2];		/* even */
	    temp[h+i] = data[i2+1];	/* odd	*/
	  }
	fft(&temp[0], &data[0], h);
	fft(&temp[h], &data[h], h);
	int p = 0, t = SEQLEN/n;
	for (int i = 0; i < h; i++)
	  { complex wkt = circle[p] * temp[h+i];
	    data[i] = temp[i] + wkt;
	    data[h+i] = temp[i] - wkt;
	    p += t;
	  }
      }
  }

static void giveup(char *msg, int p1)
  { fprintf(stderr, "mkshape: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

