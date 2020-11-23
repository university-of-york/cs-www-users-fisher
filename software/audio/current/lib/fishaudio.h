#define NULL	   0

#define AU_OUT	   1
#define AU_IN	   2
#define AU_LOCK	   4
#define AU_SAVE	   8

#define MAXSAMPLES 0x3fff	/* power of 2 minus 1 */

struct ringbuf;
struct audio_plist;

struct audio_pval { int code, nval; };

typedef unsigned int uint;
typedef unsigned char uchar;
typedef signed char schar;

struct audio
  { audio(uint, audio_pval[] = NULL);	/* constructor */
    ~audio();				/* destructor */
    int control(int, int);		/* set control params, e.g. rates; return old value */
    void oflush();			/* wait for output buffer to empty */
    void idiscard(), odiscard();	/* discard pending input or output */
    void iwait(int), owait(int);	/* wait for input queue to become half full, or output queue to become half empty */
    int icount(), ocount();		/* num. of samples in input or output buffer */
    int read();				/* read a mono value */
    void write(int);			/* write a mono value */
    void reread(int), replay(int);	/* replay last "ns" samples of input or output */
    void setduplex(int);		/* set duplex mode; d = delay in samples from input to output */

private:
    void gdelay(int&, int&, int&);	/* get delay from electrical input through s/ware to electrical output (samples) */
    uint bits;				/* AU_IN or AU_OUT, or'd with AU_LOCK or AU_SAVE       */
    schar ifd, ofd, mfd;		/* file descriptors used in memory-mapping hdsp device */
    ringbuf *irb, *orb;			/* ptr to input & output ring buffers		       */
    audio_plist *pl;			/* parameters to restore			       */
    int ifill, ofill;			/* input, output fill points			       */
    int srate;				/* sample rate if explicitly set		       */
  };

/* params to audio::control; for a full list see /usr/include/sys/hdsp.h
   "HDSP" has been changed to "AU" to avoid name clashes. */

#define AU_INPUT_SOURCE	      0
#define AU_LEFT_INPUT_ATTEN   1
#define AU_RIGHT_INPUT_ATTEN  2
#define AU_INPUT_RATE	      3
#define AU_OUTPUT_RATE	      4
#define AU_LEFT_SPEAKER_GAIN  5
#define AU_RIGHT_SPEAKER_GAIN 6
#define AU_INPUT_RBCOUNT      7
#define AU_OUTPUT_RBCOUNT     8
#define AU_MONITOR_CTL	      12
#define AU_SPEAKER_MUTE_CTL   16

#define AU_SETIFILL	      1001
#define AU_SETOFILL	      1002

/* args to AU_INPUT_RATE, AU_OUTPUT_RATE */

#define AU_RATE_8000	      0x50
#define AU_RATE_9600	      0x40
#define AU_RATE_12000	      0x30
#define AU_RATE_24000	      0x10

/* args to AU_INPUT_SOURCE */

#define AU_INPUT_LINE	      0	    /* Line In Jack	*/
#define AU_INPUT_MIC	      1	    /* Microphone Jack	*/
#define AU_INPUT_DIGITAL      2	    /* Digital I/O Jack */

/* result code returned by audio_inuse() */

#define AU_NOTINUSE	  0
#define AU_INUSEBYME	  1
#define AU_INUSEBYOTHER	  2

extern int audio_inuse();
extern int mu_expand(uchar);	/* mu-law expansion   */
extern uchar mu_compress(int);	/* mu-law compression */

