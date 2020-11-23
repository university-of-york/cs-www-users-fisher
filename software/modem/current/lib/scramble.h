#define GPC 18
#define GPA 5

typedef unsigned int uint;

struct scrambler
  { scrambler(int pb);
    void reset();
    int fwd(int), rev(int);

private:
    int bit(int);
    int tap;
    uint wd;
  };

inline scrambler::scrambler(int xt)
  { tap = xt;
    wd = 0;
  }

inline void scrambler::reset()
  { wd = 0;
  }

inline int scrambler::bit(int bn)
  { return (wd >> 23-bn) & 1;
  }

