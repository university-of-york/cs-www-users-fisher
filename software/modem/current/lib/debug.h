struct co_debugger
  { /* for debugging constellations */
    co_debugger(int);
    ~co_debugger();
    void insert(complex);
    void reset()	{ ptr = 0;    }
    int getcount()	{ return ptr; }
    void print(char*);

private:
    complex *vec;
    int len, ptr;
  };

struct db_tick;

struct debugger
  { debugger(int, int);
    ~debugger();
    void insert(float = 0.0, float = 0.0, float = 0.0, float = 0.0);
    void tick(char);
    void reset()	{ ptr = nticks = 0; }
    int getcount()	{ return ptr;	    }
    void print(char*);

private:
    float *vec[4]; db_tick *ticks;
    int nrows, ncols, ptr, nticks;
  };

