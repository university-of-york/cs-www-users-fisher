typedef unsigned short ushort;
typedef signed short sshort;

struct tableentry
  { ushort code;
    ushort len;
    sshort count;
  };

extern tableentry twtable[], mwtable[], tbtable[], mbtable[], extable[];

