#include <fishaudio.h>
#include <stdio.h>

#define global
#define unless(x)  if(!(x))
#define until(x)   while(!(x))

static audio *Audio;

static int header[8] =
  { 0x2e736e64,			/* ".snd" magic number */
    32,				/* length of hdr */
    0,				/* length of body, filled in later */
    1, 8000, 1, 0, 0,		/* don't ask me what this means! */
  };

extern "C" int select(int, uint*, uint*, uint*, int*);

static void usage(), recordfile(char*), record(int, FILE*), giveup(char*, char* = NULL);
static bool charwaiting();


global void main(int argc, char **argv)
  { unless (argc == 2) usage();
    Audio = new audio(AU_IN);
    Audio -> control(AU_INPUT_RATE, AU_RATE_8000);
    Audio -> control(AU_SETIFILL, 1);
    recordfile(argv[1]);
    delete Audio;
    exit(0);
  }

static void usage()
  { fprintf(stderr, "Usage: record fn\n");
    exit(2);
  }

static void recordfile(char *fn)
  { FILE *fi = fopen(fn, "w");
    if (fi == NULL) giveup("can't create %s", fn);
    fwrite(header, sizeof(int), 8, fi); /* write header */
    printf("Press CR to stop recording: "); fflush(stdout);
    int nb = 0;
    until (charwaiting())
      { record(800, fi);	/* 0.1 sec */
	nb += 800;
      }
    int nc = Audio -> icount();
    record(nc, fi); nb += nc;	/* process remainder of buffer */
    fflush(fi);
    header[2] = nb;	/* fill in byte count in header */
    fseek(fi, 0, 0);	/* rewind file */
    fwrite(header, sizeof(int), 8, fi); /* re-write header */
    fclose(fi);
    getchar(); /* discard CR */
  }

static void record(int nc, FILE *fi)
  { for (int i=0; i < nc; i++)
      { int val = Audio -> read() >> 8;
	putc(mu_compress(val), fi);
      }
  }

static bool charwaiting()
  { uint ib = 1; int tmo[] = { 0, 0 };
    int nb = select(32, &ib, NULL, NULL, tmo);
    if (nb < 0) giveup("select failed");
    return (nb > 0);
  }

static void giveup(char *msg, char *p1)
  { fprintf(stderr, "record: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

