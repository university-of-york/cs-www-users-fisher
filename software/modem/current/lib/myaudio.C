#include <stdio.h>
#include <fishaudio.h>
#include <coro.h>

#include "private.h"
#include "myaudio.h"

static audio *Audio;
static coroutine *pco;

static audio_pval pvals[] =
  { { AU_INPUT_RATE, AU_RATE_9600    },
    { AU_OUTPUT_RATE, AU_RATE_9600   },
    { AU_INPUT_SOURCE, AU_INPUT_LINE },
    { AU_LEFT_INPUT_ATTEN, 0	     },
    { AU_RIGHT_INPUT_ATTEN, 0	     },
    { AU_SPEAKER_MUTE_CTL, 1	     },
    { AU_MONITOR_CTL, 0		     },
    { -1, -1			     },
  };

global int samplecount;

global void openaudio()
  { Audio = new audio(AU_IN | AU_OUT | AU_LOCK | AU_SAVE, pvals);
    pco = NULL; samplecount = 0;
  }

global void closeaudio()
  { delete Audio;
  }

global void setduplex(int n)
  { Audio -> setduplex(n);
  }

global void discardinput()
  { Audio -> idiscard();
  }

global void discardoutput()
  { Audio -> odiscard();
  }

global void flushoutput()
  { Audio -> oflush();
  }

global void inparallel(coroutine *co1, coroutine *co2)
  { coroutine *spco = pco;
    pco = co1;
    callco(co2);
    pco = spco;
  }

inline void swapit()
  { coroutine *p = pco;
    if (p != NULL)
      { pco = currentco;
	callco(p);
      }
  }

global float insample()
  { swapit();
    samplecount++;
    return (float) (Audio -> read() * 2e-6f);
  }

global void outsample(float x)
  { swapit();
    if (x > 2.0f || x < -2.0f)
      { fprintf(stderr, "outsample value out of range: %f\r\n", x);
	abort();
      }
    Audio -> write((int) (x * 2e6f));	/* hardware accepts signed 24-bit but reduce to limit power to line */
  }

