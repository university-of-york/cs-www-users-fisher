struct tone_detector
  { tone_detector(fspec*, fspec*, fspec*, bool);
    ~tone_detector();
    void insert(float);
    void debug();
    float pow;
    int prescount;
    bool present;
private:
    filter *fef, *bpf, *lpf;
    bool limit;
  };

