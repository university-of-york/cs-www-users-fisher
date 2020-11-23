typedef unsigned int uint;

struct sinegen
  { sinegen(float);
    void setfreq(float);
    float fnext();
    complex cnext();
    void resetphase() { ptr = 0;	  }
    void flipphase()  { ptr ^= (1 << 31); }
private:
    uint ptr;
    int phinc;
  };

