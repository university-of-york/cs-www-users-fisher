#define global
#define unless(x)   if(!(x))
#define until(x)    while(!(x))

typedef unsigned int uint;

extern "C"
  { void abort();
    int select(int, uint*, uint*, uint*, int*);
    int getpid(), read(int, char*, int), write(int, char*, int);
  };

