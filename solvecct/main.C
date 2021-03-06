/* solvecct -- equation solver
   AJF	 May 1997
*/

#include <stdio.h>
#include <string.h>
#include <new.h>
#include <math.h>
#include <libcgi.h>

#include "solvecct.h"
#include "complex.h"

#define TWOPI	    (2.0 * M_PI)
#define MAXCOMPS    200
#define NUMFSTEPS   101
#define MAGICR	    1e4		/* for cvting V src to I src */
#define OPINRES	    1e7		/* op-amp input resistance */
#define OPGAIN	    1e6		/* op-amp transconductance */

static FILE *datain, *htmlout;
static int numcomps, numnodes, numvisnodes;
static component **components;
static double minfreq, maxfreq, logmin;
static complex **nvmat;
static complex *ivector;	/* current vector */

static void newhandler(), parsecmdline(char**);
static FILE *openinput(char*), *openoutput(char*);
static void usage(), writeheader(), writetrailer();
static void readcct();
static bool getline(char*);
static void formaterror(int, char*);
static void assignnames(), mkcctdiag(), writenames();
static void scancomps(), checknotshorted(component*);
static int allocnode();
static void addisrc(double, int, int);
static void addcomp(int, double, int, int, int = 0, int = 0, int = 0, int = 0, int = 0);
static void compute();
static complex admittance(int, double, double);
static void mkvmtables(), mkvmgraphs(), outputimage(char*);
static void appendf(char*, int&, char*, int);


global void main(int argc, char *argv[])
  { set_new_handler(newhandler);
    parsecmdline(argv);
    writeheader();
    readcct();
    assignnames();
    mkcctdiag();
    writenames();
    scancomps();
    compute();
    mkvmgraphs();	/* voltmeter graphs */
    mkvmtables();	/* voltmeter tables */
    writetrailer();
    fclose(datain); fclose(htmlout);
    exit(0);
  }

static void newhandler()
  { giveup("No room!");
  }

static void parsecmdline(char **argv)
  { int ap = 0;
    if (argv[ap] != NULL) ap++;
    datain = openinput(argv[ap++]);
    htmlout = openoutput(argv[ap++]);
    unless (argv[ap] == NULL) usage();
  }

static FILE *openinput(char *fn)
  { if (fn == NULL) usage();
    FILE *fi = fopen(fn, "r");
    if (fi == NULL) giveup("can't open %s", fn);
    return fi;
  }

static FILE *openoutput(char *fn)
  { if (fn == NULL) usage();
    FILE *fi = fopen(fn, "w");
    if (fi == NULL) giveup("can't create %s", fn);
    return fi;
  }

static void usage()
  { fprintf(stderr, "Usage: solvecct in.dat out.html\n");
    exit(1);
  }

static void writeheader()
  { fprintf(htmlout, "<HTML>\n\n");
    fprintf(htmlout, "<title> Circuit Simulation Results </title>\n\n");
    fprintf(htmlout, "<h1> Circuit Simulation Results </h1>\n\n");
  }

static void writetrailer()
  { fprintf(htmlout, "<hr>\n");
    fprintf(htmlout, "<address>\n");
    fprintf(htmlout, "   <a href=http://www-users.cs.york.ac.uk/~fisher>Tony Fisher</a> /\n");
    fprintf(htmlout, "   fisher@minster.york.ac.uk\n");
    fprintf(htmlout, "</address>\n");
  }

static void readcct()
  { int ni, nc, i; char line[MAXSTR+1]; bool ok;
    components = new component*[MAXCOMPS];
    numcomps = numnodes = 0;
    ok = getline(line);
    while (ok && !seq(line, "End"))
      { int ty, or, len, ix, iy, n1, n2, n3; double val;
	ni = sscanf(line, "%d %d %d %lg %d %d %d %d %d", &ty, &or, &len, &val, &ix, &iy, &n1, &n2, &n3);
	if (ni < 8) formaterror(2, line);
	if (n1 > numnodes) numnodes = n1;
	if (n2 > numnodes) numnodes = n2;
	if (ty == 7 || ty == 8)
	  { /* op-amp */
	    unless (ni == 9) formaterror(3, line);
	    if (n3 > numnodes) numnodes = n3;
	  }
	else
	  { /* other */
	    unless (ni == 8) formaterror(5, line);
	    n3 = 0;
	  }
	addcomp(ty, val, n1, n2, n3, ix, iy, or, len);
	ok = getline(line);
      }
    if (ok) ok = getline(line);	    /* skip "End" */
    bool hadmin = false, hadmax = false, hadlog = false;
    logmin = 0.0;	/* default */
    while (ok)
      { if (starts(line, "minf=")) { minfreq = atof(&line[5]); hadmin = true; }
	else if (starts(line, "maxf=")) { maxfreq = atof(&line[5]); hadmax = true; }
	else if (starts(line, "logmin=")) { logmin = atof(&line[7]); hadlog = true; }
	else formaterror(1, line);
	ok = getline(line);
      }
    unless (hadmin) giveup("You must specify a minimum frequency.");
    unless (hadmax) giveup("You must specify a maximum frequency.");
    if (hadlog && logmin >= 0.0) giveup("logmin (if specified) must be negative.");
  }

static bool getline(char *line)
  { if (fgets(line, MAXSTR, datain) == NULL) return false;
    int len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = '\0';
    return true;
  }

static void formaterror(int n, char *line)
  { fprintf(stderr, "%s\n", line);
    giveup("Input format error %d", n);
  }

static void assignnames()
  { for (int ty = 1; ty <= 3; ty++)	/* RCL */
      { int num = 0;
	for (int j = 0; j < numcomps; j++)
	  { component *c = components[j];
	    if (c -> type == ty)
	      { static char *names = "RCL";
		char vec[16]; sprintf(vec, "%c%d", names[ty-1], ++num);
		c -> name = copystring(vec);
	      }
	  }
      }
  }

static void mkcctdiag()
  { fprintf(htmlout, "<h2> Circuit Diagram (schematic) </h2>\n");
    char fn[16], path[MAXSTR+1], loc[MAXSTR+1];
    sprintf(fn, "%07dX.gif", uniqueid());
    sprintf(path, "%s/%s", TEMP_DIR, fn);
    sprintf(loc, "%s/%s", TEMP_URL, fn);
    drawcircuit(components, numcomps, path);
    outputimage(loc);
  }

static void writenames()
  { fprintf(htmlout, "<h2> Component Values </h2>\n");
    fprintf(htmlout, "<ul> <table>\n");
    for (int i = 0; i < numcomps; i++)
      { component *c = components[i];
	unless (c -> name == NULL) fprintf(htmlout, "   <tr> <td> %s <td> %g\n", c -> name, c -> val);
      }
    fprintf(htmlout, "</table> </ul>\n");
  }

static void scancomps()
  { numnodes--;			/* disregard ground node */
    if (numnodes < 0) giveup("empty circuit!");
    numvisnodes = numnodes;	/* num. of "visible" nodes (discounting nodes we invent) */
    ivector = new complex[numnodes+numcomps];	/* allow extra for voltage sources */
    for (int i = 0; i < numnodes; i++) ivector[i] = 0.0;
    int nc = 0, nvm = 0;
    while (nc < numcomps)
      { component *c = components[nc++];
	switch (c -> type)
	  { default:
		giveup("Bad component type (1) %d", c -> type);

	    case 0:
		/* wire */
		components[--nc] = components[--numcomps];	/* delete this component */
		break;

	    case 1:	case 2:	    case 3:
		/* RCL */
		checknotshorted(c);
		break;

	    case 4:
		/* current source */
		checknotshorted(c);
		addisrc(c -> val, c -> n1, c -> n2);
		components[--nc] = components[--numcomps];	/* delete this component */
		break;

	    case 5:
	      { /* voltage source */
		checknotshorted(c);
		int n1 = c -> n1, n2 = c -> n2;
		int nn = allocnode();				/* allocate an extra node */
		addisrc(c -> val / MAGICR, nn, n2);
		components[--nc] = components[--numcomps];	/* delete this component */
		addcomp(1, MAGICR, nn, n2);			/* add a pair of magic resistors */
		addcomp(1, -MAGICR, nn, n1);
		break;
	      }

	    case 6:
		/* voltmeter */
		checknotshorted(c);
		nvm++;
		break;

	    case 7:	case 8:
	      { /* op-amp, V+ on top */
		int n1 = c -> n1, n2 = c -> n2, n3 = c -> n3;
		if (c -> type == 7) { n1 = c -> n2; n2 = c -> n1; } /* swap */
		components[--nc] = components[--numcomps];	/* delete this component */
		addcomp(1, OPINRES, n1, n2);			/* add a resistor between op-amp inputs */
		addcomp(9, OPGAIN, n1, n2, n3);			/* add a controlled source (transconductance) */
		break;
	      }

	    case 9:
		/* controlled source (transconductance) */
		break;
	  }
      }
    if (nvm == 0)
      { fprintf(htmlout, "If you want a graph or voltmeter reading "
			 "you'll have to include one or more voltmeters. <br>\n");
      }
    if (nvm > 26) giveup("Too many voltmeters!");       /* graphs are labelled A..Z */
  }

static void checknotshorted(component *c)
  { int n1 = (c -> n1);
    int n2 = (c -> n2);
    if (n1 == n2) giveup("Short-circuited component! (node %d)", n1);
  }

static int allocnode()
  { int nn = numnodes++;
    ivector[nn] = 0.0;
    return nn+2;
  }

static void addisrc(double val, int n1, int n2)
  { if (n1 >= 2) ivector[n1-2] += val;
    if (n2 >= 2) ivector[n2-2] -= val;
  }

static void addcomp(int ty, double val, int n1, int n2,
		    int n3, int ix, int iy, int or, int len)	/* optional */
  { if (numcomps > MAXCOMPS) giveup("Too many components!");
    components[numcomps++] = new component(ty, or, 10*len, val, n1, n2, n3, 10*ix, 10*iy);
  }

static void compute()
  { int fs, i, j;
    complex **ymat = new complex*[numnodes];	/* nodal admittance matrix */
    for (i = 0; i < numnodes; i++) ymat[i] = new complex[numnodes];
    int ns = (minfreq == maxfreq) ? 1 : NUMFSTEPS;
    nvmat = new complex*[ns];
    int nfail = 0;
    for (fs = 0; fs < ns; fs++)
      { double alpha = fs / (double) (NUMFSTEPS-1);
	double freq = minfreq + alpha * (maxfreq-minfreq);
	for (i = 0; i < numnodes; i++)
	  { for (j = 0; j < numnodes; j++) ymat[i][j] = 0.0;
	  }
	for (i = 0; i < numcomps; i++)
	  { component *c = components[i];
	    complex y = admittance(c -> type, c -> val, freq);
	    if (c -> type == 9)
	      { /* controlled source */
		int n1 = (c -> n1);
		int n2 = (c -> n2);
		int n3 = (c -> n3);
		if (n3 >= 2)
		  { if (n1 >= 2) ymat[n3-2][n1-2] += y;
		    if (n2 >= 2) ymat[n3-2][n2-2] -= y;
		  }
	      }
	    else
	      { int n1 = (c -> n1);
		int n2 = (c -> n2);
		if (n1 >= 2 && n2 >= 2)
		  { ymat[n1-2][n2-2] -= y;
		    ymat[n2-2][n1-2] -= y;
		  }
		if (n1 >= 2) ymat[n1-2][n1-2] += y;
		if (n2 >= 2) ymat[n2-2][n2-2] += y;
	      }
	  }
	complex *ivec = new complex[numnodes];
	memcpy(ivec, ivector, numnodes * sizeof(complex));
	nvmat[fs] = new complex[numnodes];
	bool ok = solve_eqns(ymat, ivec, nvmat[fs], numnodes);	/* computes nvmat[fs]; overwrites ymat, ivec */
	unless (ok)
	  { fprintf(htmlout, "The nodal admittance matrix is singular at %g Hz. <br>\n", freq);
	    nfail++;
	  }
	delete ivec;
      }
    for (i = 0; i < numnodes; i++) delete ymat[i];
    delete ymat;
    if (ns >= 2 && nfail > ns-2)
      { rewind(htmlout); ftruncate(fileno(htmlout), 0);
	giveup((nfail > ns-1) ? "The nodal admittance matrix is singular at all frequencies." :
				"The nodal admittance matrix is singular at almost all frequencies.");
      }
  }

static complex admittance(int ty, double val, double freq)
  { switch (ty)
      { default:
	    giveup("Bad component type (2) %d", ty);

	case 1:
	    /* resistor */
	    return complex(1.0 / val);

	case 2:
	    /* capacitor */
	    return complex(0.0, TWOPI * freq * val);

	case 3:
	    /* inductor */
	    return complex(0.0, -1.0 / (TWOPI * freq * val));

	case 6:
	    /* voltmeter */
	    return 0.0;

	case 9:
	    /* conductance */
	    return val;
      }
  }

static void mkvmtables()
  { fprintf(htmlout, "<h2> Voltmeter Readings </h2>\n");
    for (int i = 0; i < numcomps; i++)
      { component *c = components[i];
	if (c -> type == 6)	/* if voltage meter */
	  { int n1 = (c -> n1), n2 = (c -> n2);
	    char tit[MAXSTR+1]; tit[0] = '\0'; int p = 0;
	    if (n1 > 1) appendf(tit, p, "<i>V</i><sub>%d</sub> ", n1);
	    if (n2 > 1) appendf(tit, p, "- <i>V</i><sub>%d</sub> ", n2);
	    fprintf(htmlout, "<ul> <table border>\n");
	    fprintf(htmlout, "   <tr> <th rowspan=2> Frequency (Hz) <th colspan=2> %s\n", tit);
	    fprintf(htmlout, "   <tr> <th> Magnitude <th> Phase\n");
	    int ns = (minfreq == maxfreq) ? 1 : NUMFSTEPS;
	    for (int fs = 0; fs < ns; fs += 2)	/* tabulate every other value */
	      { double alpha = fs / (double) (NUMFSTEPS-1);
		double freq = minfreq + alpha * (maxfreq-minfreq);
		complex z = 0.0;
		if (n1 > 1) z += nvmat[fs][n1-2];
		if (n2 > 1) z -= nvmat[fs][n2-2];
		fprintf(htmlout, "   <tr align=center> "
				 "<td> &nbsp; %11.4e Hz &nbsp; <td> &nbsp; %11.4e &nbsp; <td> &nbsp; %+6.3f PI &nbsp;\n",
			freq, hypot(z), atan2(z) / M_PI);
	      }
	    fprintf(htmlout, "</table> </ul>\n");
	  }
      }
  }

static void mkvmgraphs()
  { if (minfreq < maxfreq)
      { int gnum = 0;
	for (int i = 0; i < numcomps; i++)
	  { component *c = components[i];
	    if (c -> type == 6)	    /* if voltage meter */
	      { int n1 = (c -> n1);
		int n2 = (c -> n2);
		char tit[MAXSTR+1]; tit[0] = '\0'; int p = 0;
		if (n1 > 1) appendf(tit, p, "<i>V</i><sub>%d</sub> ", n1);
		if (n2 > 1) appendf(tit, p, "- <i>V</i><sub>%d</sub> ", n2);
		fprintf(htmlout, "<h2> Graph of %s(<i>y</i> axis) vs. frequency (<i>x</i> axis) </h2>\n", tit);
		fprintf(htmlout, "(<i>y</i> axis is %s and normalized) <p>\n", (logmin != 0.0) ? "logarithmic" : "linear");
		char fn[12], path[MAXSTR+1], loc[MAXSTR+1];
		sprintf(fn, "%07d-%d.gif", uniqueid(), ++gnum);   /* make unique filename */
		sprintf(path, "%s/%s", TEMP_DIR, fn);
		sprintf(loc, "%s/%s", TEMP_URL, fn);
		complex vec[NUMFSTEPS];
		for (int fs = 0; fs < NUMFSTEPS; fs++)
		  { vec[fs] = 0.0;
		    if (n1 > 1) vec[fs] += nvmat[fs][n1-2];
		    if (n2 > 1) vec[fs] -= nvmat[fs][n2-2];
		  }
		drawgraph(vec, NUMFSTEPS, minfreq, maxfreq, logmin, path);
		outputimage(loc);
	      }
	  }
      }
  }

static void outputimage(char *loc)
  { fprintf(htmlout, "<ul>\n");
    fprintf(htmlout, "   <img src=%s>\n", loc);
    fprintf(htmlout, "</ul>\n");
  }

static void appendf(char *vec, int &p, char *fmt, int p1)
  { sprintf(&vec[p], fmt, p1);
    until (vec[p] == '\0') p++;
  }

