void canceller::interpolate()
  { complex *co0 = coeffs[0];
    for (int i=1; i < spc; i++)
      { complex *co = coeffs[i];
	float w = (float) i / (float) spc;
	co[0] = (1.0-w)*co0[0];
	for (int j=1; j < ncs; j++) co[j] = w*co0[j-1] + (1.0-w)*co0[j];
      }
  }

