#include <fishaudio.h>
#include <stdio.h>

#define global
#define unless(x)  if(!(x))
#define until(x)   while(!(x))

static audio *Audio;

static void usage(), playfile(FILE*), giveup(char*, char* = NULL);


global void main(int argc, char **argv)
  { unless (argc == 1 || argc == 2) usage();
    FILE *fi;
    if (argc == 2)
      { fi = fopen(argv[1], "r");
	if (fi == NULL) giveup("can't open %s", argv[1]);
      }
    else fi = stdin;
    Audio = new audio(AU_OUT);
    Audio -> control(AU_OUTPUT_RATE, AU_RATE_8000);
    playfile(fi);
    delete Audio;
    if (argc == 2) fclose(fi);
    exit(0);
  }

static void usage()
  { fprintf(stderr, "Usage: play fn\n");
    exit(2);
  }

static void playfile(FILE *fi)
  { int header[2];
    int ni = fread(header, sizeof(int), 2, fi);		/* read first 2 wds of header */
    unless (ni == 2) giveup("fread failed");
    unless (header[0] == 0x2e736e64 && header[1] >= 8) giveup("bad header");
    for (int i = 8; i < header[1]; i++) getc(fi);	/* skip to start of data */
    int val = getc(fi);
    while (val >= 0)
      { int x = mu_expand(val);
	Audio -> write(x << 8);
	val = getc(fi);
      }
    fclose(fi);
  }

static void giveup(char *msg, char *p1)
  { fprintf(stderr, "play: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

