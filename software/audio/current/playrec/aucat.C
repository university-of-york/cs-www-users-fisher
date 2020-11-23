#include <fishaudio.h>
#include <stdio.h>

#define global
#define unless(x)  if(!(x))
#define until(x)   while(!(x))

#define MAXAMPL	   32768.0

static void usage(), playfile(char*), giveup(char*, char* = NULL);


global void main(int argc, char **argv)
  { if (argc < 2) usage();
    for (int k = 1; k < argc; k++) playfile(argv[k]);
    exit(0);
  }

static void usage()
  { fprintf(stderr, "Usage: aucat fn ...\n");
    exit(2);
  }

static void playfile(char *fn)
  { FILE *fi = fopen(fn, "r");
    if (fi == NULL) giveup("can't open %s", fn);
    int header[8];
    int ni = fread(header, sizeof(int), 8, fi); /* read header */
    unless (ni == 8) giveup("fread failed");
    unless (header[0] == 0x2e736e64 && header[1] >= 28) giveup("bad header");
    int code = fseek(fi, header[1], 0); /* position to start of data */
    if (code < 0) giveup("fseek failed");
    int val = getc(fi);
    while (val >= 0)
      { float x = (float) mu_expand(val) / MAXAMPL;
	if (x < -1.0 || x > +1.0) giveup("audio data out of range");
	printf("%10.6f\n", x);
	val = getc(fi);
      }
    fclose(fi);
  }

static void giveup(char *msg, char *p1)
  { fprintf(stderr, "aucat: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

