/* Modem for Mips   AJF	  January 1996
   Set/reset tty modes (raw or cooked)
*/

#include <sgtty.h>
#include "modem.h"

static termios cmode, rmode;

static void getmode(termios*), setmode(termios*);


global void init_ttymodes()
  { getmode(&cmode);					/* get current mode		   */
    rmode = cmode;					/* copy the struct		   */
    rmode.c_iflag = rmode.c_oflag = rmode.c_lflag = 0;	/* modify for raw mode		   */
    rmode.c_cc[VMIN] = rmode.c_cc[VTIME] = 1;		/* set timeout params		   */
    atexit(setcooked);					/* make sure mode is reset on exit */
  }

global void setraw()
  { setmode(&rmode);
  }

global void setcooked()
  { setmode(&cmode);
  }

static void getmode(termios *tm)
  { int code = tcgetattr(0, tm);
    unless (code == 0) giveup("tcgetattr failed!");
  }

static void setmode(termios *tm)
  { tcsetattr(0, TCSADRAIN, tm);	/* ignore errors */
  }

