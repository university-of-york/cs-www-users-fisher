#include <stdio.h>

#define global

typedef unsigned char uchar;

global main()
  { uchar a[3], b[3];
    for (int i = 0; i < 8; i++)
      { printf("%d :", i);
	a[0] = i >> 2; a[1] = (i >> 1) & 1; a[2] = i & 1;
	for (int j=0; j < 4; j++)
	  { b[0] = a[2];
	    b[1] = a[0] ^ (j == 1 || j == 2);
	    b[2] = a[1] ^ (j & 1);
	    if (a[2])
	      { b[1] ^= b[2];
		b[2] ^= (j >> 1);
	      }
	    printf(" %d", (b[0] <<  2) | (b[1] << 1) | b[2]);
	  }
	putchar('\n');
      }
  }

