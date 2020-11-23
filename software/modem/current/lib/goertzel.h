struct goertzel
  { goertzel(float);
    void insert(float);
    complex result();
private:
    float v1, v2, v3, fac;
    complex w;
  };

