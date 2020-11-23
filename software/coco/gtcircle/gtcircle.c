/* gtcircle -- give great circle distance from A to B
   Calls the Automated Data Service from U.S. Naval Observatory
   A.J. Fisher	 September 1995 */

#include <stdio.h>
#include <hdr.h>

static FILE *pfile, *qfile;
static char fifofn[16];

extern double fmod();

forward delfifo();


global main(argc, argv) int argc; char *argv[];
  { struct pco pco1, pco2;
    unless (argc == 5) usage();
    getpco(&argv[1], &pco1);
    getpco(&argv[3], &pco2);
    printf("Loc A: "); printpco(&pco1, "WGS72"); putchar('\n');
    printf("Loc B: "); printpco(&pco2, "WGS72"); putchar('\n');
    askads(&pco1, &pco2);
    exit(0);
  }

static usage()
  { fprintf(stderr, "Usage: gtcircle lon1 lat1 lon2 lat2\n");
    fprintf(stderr, "See README for details of input and output formats.\n");
    exit(1);
  }

static askads(pc1, pc2) struct pco *pc1, *pc2;
  { int code, i; char temps[256];
    strcpy(fifofn, "/tmp/gtc_XXXXXX"); mktemp(fifofn);
    atexit(delfifo);
    delfifo();
    code = mkfifo(fifofn, 0600);
    if (code != 0) fail("mkfifo failed!");
    sprintf(temps, "telnet tycho.usno.navy.mil 1>%s 2>/dev/null", fifofn);
    pfile = popen(temps, "w");
    if (pfile == NULL) fail("popen failed!");
    qfile = fopen(fifofn, "r");
    if (qfile == NULL) fail("fopen failed! (fifo)");
    exchange("login: ", "ads");
    exchange("==> ", "loran");
    exchange("==> ", "Ldx");
    formatpco(pc1, temps); exchange("Position:1", temps);
    formatpco(pc2, temps); exchange("Position:2", temps);
    for (i=0; i<5; i++) checkreply("\n", false);
    for (i=0; i<9; i++) checkreply("\n", true); /* copy to stdout */
    exchange("==> ", "bye");
    code = pclose(pfile);
    unless (code == 0 || code == 256) fail("telnet cmd failed! (pclose)"); /* telnet normally returns exit code 1 */
    code = fclose(qfile);
    if (code != 0) fail("fclose failed! (fifo)");
  }

static delfifo()
  { unlink(fifofn);
  }

static formatpco(pc, str) struct pco *pc; char *str;
  { formpco(&str[0], "%02d %02d %05.2f %c ", pc -> lat, "NS");
    formpco(&str[14], "%03d %02d %05.2f %c ", pc -> lon, "EW");
  }

static formpco(str, fmt, x, dir) char *str, *fmt; double x; char *dir;
  { char d; int deg, min;
    if (x >= 0.0) d = dir[0]; else { d = dir[1]; x = -x; }
    deg = (int) x;
    x = fmod(x, 1.0) * 60.0;
    min = (int) x;
    x = fmod(x, 1.0) * 60.0;
    sprintf(str, fmt, deg, min, x, d);
  }

static exchange(s1, s2) char *s1, *s2;
  { checkreply(s1, false);
    fprintf(pfile, s2); putc('\n', pfile); fflush(pfile);
  }

static checkreply(msg, copy) char *msg; bool copy;
  { int k = 0;
    until (msg[k] == '\0')
      { int ch = getc(qfile);
	if (ch < 0) fail("premature eof from host!");
	if (copy && ch != '\r') putchar(ch);
	if (ch != msg[k]) k = 0;
	if (ch == msg[k]) k++;
      }
  }

static fail(msg, p1) char *msg, *p1;
  { fprintf(stderr, "gtcircle: "); fprintf(stderr, msg, p1); putc('\n', stderr);
    exit(1);
  }

