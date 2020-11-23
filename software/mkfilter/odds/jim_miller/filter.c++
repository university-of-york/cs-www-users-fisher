#include <stdio.h>
#include <math.h>

#include "defs.h"
#include "filter.h"

Filter::Filter(int ord, Float gainAtDC, Float *ycs) 
{ 
    order = ord;

    dcGain = gainAtDC;
    dcGainInv = 1.0/gainAtDC;

    // Load coefficients based on order
    switch (ord) {
        case 9: ycoeff8 = ycs[8];
        case 8: ycoeff7 = ycs[7];
        case 7: ycoeff6 = ycs[6];
        case 6: ycoeff5 = ycs[5];
        case 5: ycoeff4 = ycs[4];
        case 4: ycoeff3 = ycs[3];
        case 3: ycoeff2 = ycs[2];
        case 2: ycoeff1 = ycs[1];
        case 1: ycoeff0 = ycs[0];
    }

    // Choose iterate function
    switch (ord) {
	case 1: iterateFunc = iterate1;
		break;
	case 2: iterateFunc = iterate2;
		break;
	case 3: iterateFunc = iterate3;
		break;
	default: 
		printf("Filter order %d not supported\n");
		break;
    }

    // Start filters at 0
    xval0 = xval1 = xval2 = xval3 = xval4 = 
            xval5 = xval6 = xval7 = xval8 = xval9 = 0.0;
    yval0 = yval1 = yval2 = yval3 = yval4 = 
	    yval5 = yval6 = yval7 = yval8 = yval9 = 0.0;

}

void
Filter::iterate(Float *in, Float *out, int n)
{
    iterateFunc(this, in, out, n);
}
	
// First order iterator
void
Filter::iterate1(Filter *thisPtr, Float *in, Float *out, int n)
{
  register Float x0, x1, y0, y1;

  // Restore state
  x1 = thisPtr->xval1;
  y1 = thisPtr->yval1;

  // Get coefficient(s)
  Float c0 = thisPtr->ycoeff0;
  Float gaininv = thisPtr->dcGainInv;

  // Recurrance relation y[n] = x[n-1] + x[n] + (ycoeff * y[n-1])
  for (int i = 0; i < n; i++) {

    // shift
    x0 = x1; 
    x1 = *in++;

    // shift
    y0 = y1; 
    y1 = (x0 + x1) + (y0*c0);
    *out++ = gaininv * y1;	// unity gain
  }

  // Save state
  thisPtr->xval1 = x1;	
  thisPtr->yval1 = y1;
}

// Second order iterator
void
Filter::iterate2(Filter *thisPtr, Float *in, Float *out, int n)
{
  register Float x0, x1, x2, y0, y1, y2;

  // Restore state
  x1 = thisPtr->xval1;
  x2 = thisPtr->xval2;
  y1 = thisPtr->yval1;
  y2 = thisPtr->yval2;

  // Get coefficient(s)
  Float c0 = thisPtr->ycoeff0,
	c1 = thisPtr->ycoeff1,
        gaininv = thisPtr->dcGainInv;

  // Recurrance relation 
  //    y[n] = x[n-2] + 2x[n-1] + x[n] + (yc0 * y[n-2] + yc1*y[n-1])

  for (int i = 0; i < n; i++) {
    // shift
    x0 = x1; x1 = x2; 
    x2 = *in++;

    // shift
    y0 = y1; y1 = y2;
    y2 = (x0 + 2.0*x1 + x2) + (c0*y0 + c1*y1 );
    *out++ = gaininv * y2;	// unity gain
  }

  // Save state
  thisPtr->xval1 = x1;	
  thisPtr->xval2 = x2;
  thisPtr->yval1 = y1;
  thisPtr->yval2 = y2;
}

// Third order iterator
void
Filter::iterate3(Filter *thisPtr, Float *in, Float *out, int n)
{
  register Float x0, x1, x2, x3, y0, y1, y2, y3;

  // Restore state
  x1 = thisPtr->xval1;
  x2 = thisPtr->xval2;
  x3 = thisPtr->xval3;
  y1 = thisPtr->yval1;
  y2 = thisPtr->yval2;
  y3 = thisPtr->yval3;

  // Get coefficient(s)
  Float c0 = thisPtr->ycoeff0,
	c1 = thisPtr->ycoeff1,
	c2 = thisPtr->ycoeff2,
        gaininv = thisPtr->dcGainInv;

  // Recurrance relation 
  //    y[n] =   1*x[n-3] 
  //           + 3*x[n-2]
  //           + 3*x[n-1] 
  //           + 1*x[n]
  //           + yc0 * y[n-3] 
  //           + yc1 * y[n-2]
  //           + yc2 * y[n-1])

  for (int i = 0; i < n; i++) {
    // shift
    x0 = x1; x1 = x2; x2 = x3;
    x3 = *in++;

    // shift
    y0 = y1; y1 = y2; y2 = y3;
    y3 = (x0 + 3.0*(x1 + x2) + x3)
       + (c0*y0 + c1*y1 + c2*y2);
    *out++ = gaininv * y3;	// unity gain
  }

  // Save state
  thisPtr->xval1 = x1;	
  thisPtr->xval2 = x2;
  thisPtr->xval3 = x3;
  thisPtr->yval1 = y1;
  thisPtr->yval2 = y2;
  thisPtr->yval3 = y3;
}
