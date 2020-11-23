#include <fishaudio.h>
#include <stdio.h>

#define global
#define unless(x)  if(!(x))
#define until(x)   while(!(x))

#define MAXAMPL	   32768.0

static int header[8] =
  { 0x2e736e64,			/* ".snd" magic number */
    32,				/* length of hdr */
    0,				/* length of body, filled in later */
    1, 8000, 1, 0, 0,		/* don't ask me what this means! */
  };

static void usage(), recordfile(char*), giveup(char*, char* = NULL);


global void main(int argc, char **argv)
  { unless (argc == 2) usage();
    recordfile(argv[1]);
    exit(0);
  }

static void usage()
  { fprintf(stderr, "Usage: autac fn\n");
    exit(2);
  }

static void recordfile(char *fn)
  { FILE *fi = fopen(fn, "w");
    if (fi == NULL) giveup("can't create %s", fn);
    fwrite(header, sizeof(int), 8, fi); /* write header */
    double x; int nb = 0;
    int ni = scanf("%lg", &x);
    while (ni == 1)
      { if (x < -1.0 || x > +1.0) giveup("audio data out of range");
	uchar c = mu_compress((int) (x * MAXAMPL));
	putc(c, fi);
	nb++;
	ni = scanf("%lg", &x);
      }
    fflush(fi);
    header[2] = nb;	/* fill in byte count in header */
    fseek(fi, 0, 0);	/* rewind file */
    fwrite(header, sizeof(int), 8, fi); /* re-write header */
    fclose(fi);
  }

static void giveup(char *msg, char *p1)
  { fprintf(stderr, "autac: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

