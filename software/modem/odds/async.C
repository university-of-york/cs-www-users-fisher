static uchar async_statab[9][4] =
  { { 1, 1, 5, 0 }, { 2, 2, 2, 2 }, { 3, 3, 3, 3 }, { 4, 4, 4, 4 }, { 5, 0, 5, 0 },
    { 6, 6, 6, 6 }, { 7, 7, 7, 7 }, { 8, 8, 8, 8 }, { 0, 0, 0, 0 },
  };

static uchar async_acttab[9][4] =
  { { 1, 1, 0, 0 }, { 3, 3, 3, 3 }, { 3, 3, 3, 3 }, { 3, 3, 3, 3 }, { 6, 6, 6, 6 },
    { 3, 3, 3, 3 }, { 3, 3, 3, 3 }, { 3, 3, 3, 3 }, { 7, 7, 7, 7 },
  };

static void advance_state(uint k)
  { static uchar async_state = 0;
    static uchar ch;
    uchar act = async_acttab[async_state][k];
    if (act & 2) ch = (ch >> 1) | ((k & 2) << 6);
    if (act & 1) ch = (ch >> 1) | (k << 7);
    if (act & 4) putchar(ch);
    async_state = async_statab[async_state][k];
  }

