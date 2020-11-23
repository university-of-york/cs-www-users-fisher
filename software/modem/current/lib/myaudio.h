#define SAMPLERATE 9600

extern void openaudio(), closeaudio();
extern void discardinput(), discardoutput(), flushoutput();
extern void setduplex(int);

struct coroutine;			/* def'd in <coro.h> */

extern void inparallel(coroutine*, coroutine*);

extern float insample();
extern void outsample(float);

inline bool after(int t1, int t2) { return ((t2-t1) & (1 << 31)) != 0; }

extern int samplecount;

