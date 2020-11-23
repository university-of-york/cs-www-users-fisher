#include "/usr/fisher/mipslib/audio.h"

#include <stdio.h>

#define global
#define unless(x)  if(!(x))
#define until(x)   while(!(x))

typedef int word;	/* or char* or ... */
typedef unsigned int uint;

static struct audio *audio;
static short expand[256];

extern struct audio *OpenAudio();
extern double pow();


global main(argc, argv) int argc; char *argv[];
  { unless (argc == 2) usage();
    InitAudio("play");
    makeexpand();
    audio = OpenAudio(AU_OUT);
    ControlAudio(audio, AU_OUTPUT_RATE, AU_RATE_8000);
    playfile(argv[1]);
    CloseAudio(audio);
    exit(0);
  }

static usage()
  { fprintf(stderr, "Usage: play fn\n");
    exit(2);
  }

static makeexpand()
  { /* make mu-law expanding table */
    int n;
    for (n=0; n < 128; n++)
      { double x = (pow(256.0, (double) n / 127.0) - 1.0) / 255.0;	/* result in range 0 .. 1 */
	int ix = (int) (x * 32767.0 + 0.5);				/* in range 0 .. 32767	  */
	expand[255-n] = ix;
	expand[127-n] = -ix;
      }
  }

static playfile(fn) char *fn;
  { FILE *fi; int val;
    fi = fopen(fn, "r");
    if (fi == NULL) giveup("can't open %s", fn);
    fseek(fi, 32, 0); /* skip header */
    val = getc(fi);
    while (val >= 0)
      { WaitAudio(audio, RBLEN/2); /* wait until half-full or less */
	while (NumFilled(audio) < RBLEN-2 && val >= 0)
	  { WriteMono(audio, expand[val] << 8);
	    val = getc(fi);
	  }
      }
    fclose(fi);
  }

static giveup(msg, p1) char *msg; word p1;
  { fprintf(stderr, "play: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

