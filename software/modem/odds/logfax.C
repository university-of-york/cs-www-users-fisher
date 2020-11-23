#include <stdio.h>

#define global
#define unless(x)    if(!(x))

typedef unsigned int uint;

extern "C"
  { char *getenv(char*), *ctime(int*), *strpbrk(char*, char*);
    void umask(uint);
    int gethostname(char*, int), time(int*);
  };

static void usage();
static void giveup(char*, char* = NULL);


global int main(int argc, char **argv)
  { unless (argc == 2) usage();
    umask(022);
    char hn[33]; int code = gethostname(hn, 32); hn[32] = '\0';
    if (code != 0) giveup("gethostname failed!");
    if (strpbrk(hn, "./") != NULL) giveup("bad host name! %s", hn); /* hn mustn't contain '.' or '/' */
    char *un = getenv("LOGNAME");
    if (un == NULL) giveup("LOGNAME not set!");
    char fn[256]; sprintf(fn, "/usr/fisher/faxlogs/%s", hn);
    FILE *fi = fopen(fn, "a");
    if (fi == NULL) giveup("can't append to log file");
    int clock = time(NULL); char *ctim = ctime(&clock);
    ctim[24] = '\0'; /* delete nl */
    fprintf(fi, "%s %-8s %s\n", &ctim[4], un, argv[1]);
    fclose(fi);
    return 0;
  }

static void usage()
  { fprintf(stderr, "Usage: logfax 'msg'\n");
    exit(2);
  }

static void giveup(char *msg, char *p1)
  { fprintf(stderr, "logfax: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

