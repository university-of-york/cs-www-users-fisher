struct canceller
  { canceller(float d)	{ delta = d; reset(); }
    void reset();
    void insert(complex);		/* put new Tx value into canceller	     */
    complex get();			/* get predicted echo value		     */
    void update(complex);		/* given eps, update coeffs		     */
    void print(char*);			/* print coeffs				     */

private:
    const int size = 2048;			/* power of 2, large enough so (size + ebeg - TRDELAY) is +ve	*/
    const int ebeg = -60;			/* start of near-end echo response (samples)			*/
    const int eend =  30;			/* end of near-end echo response (samples)			*/
    const int ncs = (eend-ebeg) / (SYMBLEN/2);	/* num. of coeffs						*/
    complex coeffs[ncs];			/* vectors of coefficients					*/
    complex in[size];				/* circular buffer for input samples				*/
    int next;					/* ptr to next place to insert					*/
    float delta;
  };

