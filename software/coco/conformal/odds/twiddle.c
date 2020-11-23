#ifdef NOTDEF
	for (i=0; i < NUMPOINTS; i++)
	  { struct complex twiddle; double theta;
	    theta = TWOPI * (double) (OFFSET*i) / (double) NUMPOINTS;
	    twiddle.re = cos(theta);
	    twiddle.im = sin(-theta);
	    fftvec[i] = ctimes(twiddle, fftvec[i]);
	  }
#endif
