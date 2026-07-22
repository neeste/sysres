/* simp.c - simplex parameter fitting routine
 ****************************************************************************
 *
 *
 * usage:
 *          void simpfit(iniv, npv, mxiter, mniter, pvar, prep, peex)
 *          float iniv[npv];
 *          int npv, mxiter, mniter;
 *
 * where:
 *          iniv   - array of initial parameter values
 *                   (final values will be returned here also)
 *          npv    - number of values in the parameter array
 *          mxiter - maximum number of iterations before convergence
 *          mniter - minimum number of iterations between reports
 *          pvar - pointer to function returning variance
 *          prep - pointer to function producing reports
 *          peex - pointer to function checking for "early exit"
 *
 * The user must also supply a function called "variance" that
 * will compute the residual variance for a given set of parameters
 * and a function called "report" to handle reports of intermediate
 * parameter values prior to convergence:
 *
 *         float pv[npv];
 *         double variance(pv)
 *         int early_exit()
 *         void report(pv)
 *
 ****************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MXNV	21		/* maximum number of parameters + 1 */
#define ALFA	1.0		/* reflection coefficient */
#define BETA	0.5		/* contraction coefficient */
#define GAMA	2.0		/* expansion coefficient */
#define IPER	0.01		/* initial fraction */
#define FPER	0.0001		/* final fraction */

double  (*variance) (float *pv);
void    (*report) (float *pv);
int     (*early_exit) ();

static int sfconvergence(void);
static void sfinflate(float *);
static void sfhilo(int, int);
static void sfcentroid(float *);
static void sfreflect(float *, float *);
static void sfaccept(float *);
static void sfsavelo(float *);
static void sfexpansion(float *, float *);
static void sfcontraction(float *, float *);
static void sfshrink(void);

static double lores, hires, lstres;
static int npar, nval, hiidx, loidx, lastii;
static float simp[MXNV][MXNV];

void 
simpfit(float *iniv, int npv, int mxiter, int mniter,
    double  (*pvar) (float *),
    void    (*prep) (float *),
    int     (*peex) ())
{
    int     i;
    float   cent[MXNV], nxtv[MXNV], nxres;

    if (npv >= MXNV)
	return;
    variance = pvar;
    report = prep;
    early_exit = peex;
    lores = hires = lstres = 0;
    lastii = 0;
    npar = npv;
    nval = npv + 1;
    sfinflate(iniv);
    sfhilo(0, 0);
    for (i = 0; i < mxiter; i++) {
        if (early_exit)
            if (early_exit())
                break;
	sfhilo(i, mniter);
	sfcentroid(cent);
	sfreflect(cent, nxtv);
	if (nxtv[npar] < hires) {
	    sfaccept(nxtv);
	    if (nxtv[npar] < lores) {
		nxres = nxtv[npar];
		sfexpansion(cent, nxtv);
		if (nxtv[npar] < nxres)
		    sfaccept(nxtv);
	    }
	} else {
	    sfcontraction(cent, nxtv);
	    if (nxtv[npar] < hires)
		sfaccept(nxtv);
	    else
		sfshrink();
	    if (sfconvergence()!= 0)
		break;
	}
    }
    sfhilo(i, 0);
    sfsavelo(iniv);

    return;
}

static void 
sfinflate(float *iniv)
{
    int     i, j;
    float   d;

    for (i = 0; i < npar; i++)
	simp[0][i] = iniv[i];
    simp[0][npar] = (float) variance(simp[0]);
    if (report) {
        report(simp[0]);
    }

    for (j = 1; j < nval; j++) {
	for (i = 0; i < npar; i++)
	    simp[j][i] = simp[0][i];
	d = simp[0][j - 1];
	simp[j][j - 1] = (float) ((d == 0) ? 1 : d * (1 + IPER));
	simp[j][npar] = (float) variance(simp[j]);
    }

    return;
}

static void 
sfhilo(int ii, int ir)
{
    int     i;

    hiidx = 0;
    hires = simp[hiidx][npar];
    loidx = 0;
    lores = simp[loidx][npar];
    for (i = 0; i < nval; i++) {
	if (hires < simp[i][npar]) {
	    hires = simp[i][npar];
	    hiidx = i;
	}
	if (lores > simp[i][npar]) {
	    lores = simp[i][npar];
	    loidx = i;
	}
    }

    if (ir > 0) {
	if (ii < lastii + ir)
	    return;
	if (lores >= lstres)
	    return;
    }
    lastii = ii;
    lstres = lores;
    if (report) {
        report(simp[loidx]);
    }

    return;
}

static void 
sfcentroid(float *cent)
{
    int     i, j;

    for (i = 0; i < npar; i++) {
	cent[i] = 0;
	for (j = 0; j < nval; j++)
	    if (j != hiidx)
		cent[i] = cent[i] + simp[j][i];
	cent[i] = cent[i] / npar;
    }

    return;
}

static void 
sfreflect(float *cent, float *nxtv)
{
    int     i;

    for (i = 0; i < npar; i++)
	nxtv[i] = (float) ((1 + ALFA) * cent[i] - ALFA * simp[hiidx][i]);
    nxtv[npar] = (float) variance(nxtv);

    return;
}

static void 
sfaccept(float *nxtv)
{
    int     i;

    for (i = 0; i < nval; i++)
	simp[hiidx][i] = nxtv[i];

    return;
}

static void 
sfsavelo(float *iniv)
{
    int     i;

    for (i = 0; i < npar; i++)
	iniv[i] = simp[loidx][i];

    return;
}

static void 
sfexpansion(float *cent, float *nxtv)
{
    int     i;

    for (i = 0; i < npar; i++)
	nxtv[i] = (float) ((1 - GAMA) * cent[i] + GAMA * simp[hiidx][i]);
    nxtv[npar] = (float) variance(nxtv);

    return;
}

static void 
sfcontraction(float *cent, float *nxtv)
{
    int     i;

    for (i = 0; i < npar; i++)
	nxtv[i] = (float) ((1 - BETA) * cent[i] + BETA * simp[hiidx][i]);
    nxtv[npar] = (float) variance(nxtv);

    return;
}

static void 
sfshrink()
{
    int     i, j;

    for (j = 0; j < nval; j++) {
	for (i = 0; i < npar; i++)
	    simp[j][i] = (float) ((1 - BETA) * simp[loidx][i] + BETA * simp[j][i]);
	simp[j][npar] = (float) variance(simp[j]);
    }

    return;
}

static int 
sfconvergence()
{
    int     i, j;
    double  error, hi[MXNV], lo[MXNV];

    for (i = 0; i < nval; i++) {
	hi[i] = fabs(simp[0][i]);
	lo[i] = fabs(simp[0][i]);
    }
    for (j = 1; j < nval; j++) {
	for (i = 0; i < nval; i++) {
	    if (hi[i] < fabs(simp[j][i]))
		hi[i] = fabs(simp[j][i]);
	    if (lo[i] > fabs(simp[j][i]))
		lo[i] = fabs(simp[j][i]);
	}
    }
    for (i = 0; i < npar; i++) {
	error = (hi[i] > lo[i]) ? (1 - lo[i] / hi[i]) : 0;
	if (error > FPER)
	    return (0);
    }

    return (1);
}

#ifdef TEST

#define NV 4

static float c[NV] = {11, 12, 13, 14};

double 
test_variance(float *x)
{
    int     i;
    double  y, z = 0;

    for (i = 0; i < NV; i++) {
	y = (x[i] - c[i]);
	z += y * y;
    }
    return (sqrt(z));
}

void 
test_report(float *x)
{
    printf(" x1=%.1f x2=%.1f x3=%.1f x4=%.1f\n", x[0], x[1], x[2], x[3]);
}

void 
main()
{
    static float x[4] = {3, 2, 1, 1};

    simpfit(x, 4, 1000, 50, &test_variance, &test_report, NULL);
    printf("should be:\n");
    report(c);
}

#endif				/* TEST */
