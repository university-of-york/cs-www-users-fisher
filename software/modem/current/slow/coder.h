/* segments of training sequence */
#define SEG_1	    0
#define SEG_2	    48
#define SEG_3	    (SEG_2 + 128)
#define SEG_4	    (SEG_3 + 384)

struct traininggen
  { traininggen() { reset();	}
    void reset()  { reg = 0x2a; }
    complex get(int);

private:
    uchar reg;
  };

struct encoder
  { encoder()	 { reset();   }
    void reset() { state = 0; }
    complex encode(int);

private:
    int state;
  };

struct decoder
  { decoder()	 { reset();   }
    void reset() { state = 0; }
    int decode(complex);
    complex getez();

private:
    int state;
  };

