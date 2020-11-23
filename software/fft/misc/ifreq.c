#include <stdio.h>

main ()
  { int time = 0, prevtime = 0;
    int prevbit = 0;
    int bit = getbit ();
    double freq = 0.0;
    while (bit >= 0)
      { if ((prevbit == 0) && (bit == 1) && (time>prevtime))
	  { double f = 800.0/(time-prevtime);
	    if (f <= 20.0) freq = f;
	    prevtime = time;
	  }
	printf ("%lf\n", freq);
	time++; prevbit = bit; bit = getbit ();
      }
    fprintf (stderr, "ndata %d\n", time);
  }

int getbit ()
  { int ch, bit;
    do ch = getchar (); while (ch == ' ' || ch == '\t' || ch == '\n');
    if (ch<0) bit = -1;
    else if (ch == '0') bit = 0;
    else if (ch == '1') bit = 1;
    else
      { fprintf (stderr, "ifreq: ilgl input char '%c'\n", ch);
	exit (1);
      }
    return (bit);
  }
