#include <stdio.h>

#define global

#define unless(x) if(!(x))


global main(argc, argv) int argc; char *argv[];
  { FILE *fi; int i, ni; short wtab[256];
    unless (argc == 2) usage();
    fi = fopen(argv[1], "r");
    if (fi == NULL) giveup("can't open %s", argv[1]);
    ni = fread(wtab, sizeof(short), 256, fi);
    unless (ni == 256) giveup("fread failed");
    fclose(fi);
    for (i=0; i < 256; i++)
      { printf(" %03o:%4d", i, wtab[i]);
	if (i%8 == 7) putchar('\n');
      }
    exit(0);
  }

static usage()
  { fprintf(stderr, "Usage: shwid fn\n");
    exit(2);
  }

static giveup(msg, p1) char *msg, *p1;
  { fprintf(stderr, "shwid: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

