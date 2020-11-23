struct state		    /* an Earley state */
  { struct state *sflk, *shlk;
    struct hyperrule *pval;
    int jval, bval;
    struct statelist *treep;
    bool mark; char *how;
  };

struct stateset
  { struct state *sshd;
    struct state **sstl;
    struct state *hash[HASHSIZE];
  };

struct statelist
  { struct statelist *link;
    struct state *left, *down;
  };

