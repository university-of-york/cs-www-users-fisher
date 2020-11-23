#define RBLEN	   (1 << 15)

#define AU_OUT	   1
#define AU_IN	   2

struct ringbuf
  { int head, tail, intreq, fillpt;
    int buf[RBLEN];
  };

struct audio
  { short dir;		    /* AU_IN or AU_OUT					  */
    short fd;		    /* file descriptor used in memory-mapping hdsp device */
    struct ringbuf *rb;	    /* ptr to ring buffer				  */
  };

/* params to ControlAudio; for a full list see /usr/include/sys/hdsp.h
   "HDSP" has been changed to "AU" to avoid name clashes. */

#define AU_INPUT_SOURCE	      0
#define AU_LEFT_INPUT_ATTEN   1
#define AU_RIGHT_INPUT_ATTEN  2
#define AU_INPUT_RATE	      3
#define AU_OUTPUT_RATE	      4
#define AU_LEFT_SPEAKER_GAIN  5
#define AU_RIGHT_SPEAKER_GAIN 6
#define AU_MONITOR_CTL	      12
#define AU_SPEAKER_MUTE_CTL   16

/* args to AU_INPUT_RATE, AU_OUTPUT_RATE */

#define AU_RATE_8000	      0x50
#define AU_RATE_24000	      0x10

/* args to AU_INPUT_SOURCE */

#define AU_INPUT_LINE	      0	    /* Line In Jack	*/
#define AU_INPUT_MIC	      1	    /* Microphone Jack	*/
#define AU_INPUT_DIGITAL      2	    /* Digital I/O Jack */

