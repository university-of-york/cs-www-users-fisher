struct phr_detector
  { phr_detector(fspec*, fspec*);
    ~phr_detector();
    void insert(float);
    bool reversal() { return pol1 != pol2; }
    float phase;
private:
    filter *bpf, *lpf;
    int pol1, pol2;
  };

