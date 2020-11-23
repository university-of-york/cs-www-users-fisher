#ifndef _FILTER_H
#define _FILTER_H

// Abstract base class
class FilterBase {
    public:
	virtual void iterate(Float *in, Float *out, int n) = 0;
};

struct FilterSpec {
    int order;
    Float dcGain;
    Float *ycoeffs;
};

class Filter: public FilterBase {
    public:
	Filter(int order, Float dcGain, Float *ycs);
	virtual void iterate(Float *in, Float *out, int n);

    private:
	// gain parameters
	Float dcGain, dcGainInv;
	int order;

	// State variables so we can resume later
	Float xval0, xval1, xval2, xval3, xval4,
	      xval5, xval6, xval7, xval8, xval9;

	Float yval0, yval1, yval2, yval3, yval4,
	      yval5, yval6, yval7, yval8, yval9;

	// Filtering coefficients for orders 1-10
	Float ycoeff0, ycoeff1, ycoeff2, ycoeff3, ycoeff4,
	      ycoeff5, ycoeff6, ycoeff7, ycoeff8, ycoeff9;

	// Hand-coded, speedy filters for 1st 3 orders
        static void iterate1(Filter *, Float *in, Float *out, int n);
        static void iterate2(Filter *, Float *in, Float *out, int n);
        static void iterate3(Filter *, Float *in, Float *out, int n);

	// This pointer will reference one of the above, dependin
	// on the order of the filter.
	void (*iterateFunc)(Filter *, Float *in, Float *out, int n);

};
	
#endif // _FILTER_H
