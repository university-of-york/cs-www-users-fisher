/* Version of getpass() which works with a 13-char buffer */

#include <stdio.h>
#include <sgtty.h>

#include "modem.h"

#define BUFSIZE 13

static void getbuf(FILE*, char[], int);


global char *getpassword(char *prompt)
  { termios otmode, ntmode;
    static char pbuf[BUFSIZE+1];
    FILE *fi = fopen("/dev/tty", "r+");
    if (fi == NULL) return NULL;
    setbuf(fi, NULL);
    ioctl(fileno(fi), TCGETA, &otmode);			/* fetch current tty mode */
    ntmode = otmode;					/* copy the struct	  */
    ntmode.c_lflag &= ~ECHO;				/* disable echo		  */
    ioctl(fileno(fi), TCSETAW, &ntmode);		/* set new mode		  */
    fprintf(stderr, "%s", prompt); fflush(stderr);
    getbuf(fi, pbuf, BUFSIZE);
    putc('\n', stderr);
    ioctl(fileno(fi), TCSETAW, &otmode);		/* restore previous mode  */
    fclose(fi);
    return pbuf;
  }

static void getbuf(FILE *fi, char pbuf[], int max)
  { int k = 0; int ch = getc(fi);
    until (ch == '\n' || ch < 0)
      { if (k < max) pbuf[k++] = ch;
	ch = getc(fi);
      }
    pbuf[k++] = '\0';
  }

