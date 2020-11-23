struct equalizer
  { equalizer(float d)		  { delta = d; reset(); }
    void reset();
    void insert(complex);				  /* put new raw value into equalizer		    */
    complex get();					  /* get equalized value			    */
    void update(complex eps)	   { upd(eps, np); }	  /* given eps, update coeffs			    */
    void short_update(complex eps) { upd(eps, 2);  }	  /* ditto, use short window	// WAS 1	    */
    int getdt();					  /* get timing offset				    */
    void shift(int);					  /* shift coefficients vector			    */
    void print(char*);					  /* print coefficients vector			    */

private:
    void upd(complex, int);
    const int size = 16;				  /* power of 2 .ge. (2*np+1)			    */
    const int np = 7;					  /* np to the left of me, np to the right of me    */
    complex coeffs[2*np+1];				  /* vector of coefficients			    */
    complex in[size];					  /* circular buffer				    */
    int next;						  /* ptr to next place to insert		    */
    float delta;
  };

