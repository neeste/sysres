/* expfit.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "display.h"

#define limit(mn,aa,mx) (((aa)<(mn))?(mn):((aa)>(mx))?(mx):(aa))

double  cln(float *);
double  cmg(float *);
double  cph(float *);
int     nint(double);
void    prn_msg(char *);
void    simpfit(float *, int, ...);

extern FRAME fr;

static double ftc = 1.5;
static double var0 = 0;
static int dpef = 0, dpnp = 0, dpnc = 0;
static int penalty_flag = 0;

static double
avg(float *a, int i1, int i2)
{
    double s = 0;
    int i;
    
    for (i = i1; i < i2; i++)
        s += a[i];

    return ((i2 > i1) ? s / (i2 - i1) : 0);
}

static double
avgph(float *a, int i1, int i2)
{
    double s1 = 0, s2 = 0, av;
    int i, n;
    
    for (i = i1; i < i2; i++) {
        s1 += cos(a[i]);
        s2 += sin(a[i]);
    }
    n = i2 - i1;
    av = (n > 0) ? atan2(s2 / n, s1 / n) : 0;
    return (av);
}

static double
avgexp(double a0, double a1, double a2, int i1, int i2)
{
    double s = 0;
    int i;
    
    for (i = i1; i < i2; i++)
        s += a0 + a1 * exp(-i * a2);

    return ((i2 > i1) ? s / (i2 - i1) : 0);
}

static double
penalty(int nc, double t1, double t2, int np)
{
    double c, p = 0, v = var0;
    static double mn = 0.001, mx = 2, tr = 2;

    if (nc > 2 && t1 != 0) {
        c = t1 * np;
        if (c < 1 / mx || c > 1 / mn)
            p += v;
    }
    if (nc > 4 && t2 != 0) {
        c = t2 * np;
        if (c < 1 / mx || c > 1 / mn)
            p += v;
    }
    if (nc > 4 && t1 > 0 && t2 > 0) {
        c = fabs(log(t1 / t2));
        if (c < log(tr))
            p += v;
    }

    return (p);
}

static double
dpef_var_1(float *a)
{
    double er, dr, dprmx, dprmn, var = 0;
    int    i;
    static double dprn = 2.3;

    dprmn = a[0] - dprn;
    dprmx = a[0] + dprn;
    for (i = 0; i < dpnp; i++) {
        er = a[0];
        if (dpnc > 2)
            er += a[1] * exp(-i * a[2]);
        if (dpnc > 4)
            er += a[3] * exp(-i * a[4]);
        dr = er - limit(dprmn, fr.dpdb[i], dprmx);
        var  += dr * dr;
    }
    if (penalty_flag)
        var += penalty(dpnc, a[2], a[4], dpnp);

    return (var);
}

static void
dp_exp_fit_1(float *w, PLTFMT *pf)
{
    double dt_s, var1, var2, vaf, sr, t1, t2;
    double sm1, sm2, ss1, ss2, msq, var;
    float  a[6], b[3], c[1], *ww, *www;
    int i, j1, j2, n;
    static int nit = 1000;
    static double eps = 1e-30;

    prn_msg("Performing exponential fit...");
    n = fr.nn;
    ww = w + fr.wo + (fr.mg * fr.wi + fr.wj[0]) * 2;
    sm1 = sm2 = ss1 = ss2 = 0;
    for (i = 0; i < n; i++) {
	www = ww + i * fr.wi * 2;
        fr.dpdb[i] = (float) (cln(www));
	sm1 += www[0];
	sm2 += www[1];
	ss1 += www[0] * www[0];
	ss2 += www[1] * www[1];
    }
    msq = (sm1 / n) * (sm1 / n) + (sm2 / n) * (sm2 / n);
    var = (ss1 - sm1 * sm1 / n) / n + (ss2 - sm2 * sm2 / n) / n;
    dt_s = 0.001 / pf->df_khz;
    t1 = limit(1e-6, pf->dpt1, 100) / dt_s;
    t2 = limit(1e-6, pf->dpt2, 100) / dt_s;
    dpef = pf->dpef;
    dpnp = n;
    for (i = 0; i < 5; i++)
        a[i] = 0;
    a[0] = c[0] = (float) (avg(fr.dpdb, 0, n));
    dpnc = 1;
    penalty_flag = 0;
    var0 = dpef_var_1(a);
    if (dpef < 2) {
        j1 = 0;
        j2 = nint(t1 / ftc);
    } else if (pf->dpt1 > pf->dpt2) {
        j1 = nint(t2 / ftc);
        j2 = nint(t1 / ftc);
    } else {
        j1 = nint(t1 / ftc);
        j2 = nint(t2 / ftc);
    }
    j1 = limit(0, j1, n / 2);
    j2 = limit(1, j2, n / 2);
    a[0] = b[0] = (float) avg(fr.dpdb, j2, n);
    a[1] = b[1] = (float) (avg(fr.dpdb, j1, j2) - a[0]);
    a[2] = b[2] = (float) (ftc / j2);
    penalty_flag = 1;
    simpfit(a, dpnc = 3, nit, nit, dpef_var_1, NULL, NULL);
    penalty_flag = 0;
    var1 = dpef_var_1(a);
    if (dpef > 1) {
	a[3] = (float) (avg(fr.dpdb, 0, j1) - avgexp(a[0], a[1], a[2], 0, j1));
        a[4] = (float) (ftc / j1);
        penalty_flag = 1;
        simpfit(a, dpnc = 5, nit, nit, dpef_var_1, NULL, NULL);
        penalty_flag = 0;
	var2 = dpef_var_1(a);
    } else {
	var2 = var0 + var1; /* make var2 larger than either var0 or var1 */
    }
    if (var2 < var0 && var2 < var1) {
	vaf = (var0 > 0) ? 1 - var2 / var0 : 0;
    } else if (var1 < var0 && var1 < var2) {
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
	a[3] = 0;
	a[4] = 0;
	vaf = (var0 > 0) ? 1 - var1 / var0 : 0;
    } else {
	a[0] = c[0];
	a[1] = 0;
	a[2] = 0;
	a[3] = 0;
	a[4] = 0;
	vaf = 0;
    }
    sr = 20 / log(10);
    pf->efc[0] = a[0] * sr - fr.dbref - pf->dbvpu;
    pf->efc[1] = a[1] * sr;
    pf->efc[3] = a[3] * sr;
    pf->efc[2] = dt_s / a[2];
    pf->efc[4] = dt_s / a[4];
    pf->efc[5] = vaf;
    pf->efc[9] = (msq > eps && var > eps) ? 10 * log10(msq / var) : -300;
    for (i = 0; i < n; i++) {
        fr.dpdb[i] = (float) (a[0] * sr - pf->dbvpu);
        fr.dpdb[i] += (float) (a[1] * exp(-i * a[2]) * sr);
        fr.dpdb[i] += (float) (a[3] * exp(-i * a[4]) * sr);
    }
}

static double
dpef_var_2(float *a)
{
    double er, ei, dr, di, dprmx, dprmn, var = 0;
    int    i;
    static double dprn = 2.3;
    static double pi = 3.1415927;

    dprmn = a[0] - dprn;
    dprmx = a[0] + dprn;
    for (i = 0; i < dpnp; i++) {
        er = a[0];
        ei = a[1];
        if (dpnc > 4) {
            er += a[2] * exp(-i * a[4]);
            ei += a[3] * exp(-i * a[4]);
        }
        if (dpnc > 7) {
            er += a[5] * exp(-i * a[7]);
            ei += a[6] * exp(-i * a[7]);
        }
        dr = er - limit(dprmn, fr.dpdb[i], dprmx);
        di = ei - fr.dpph[i];
	if (di > pi)
	    di -= (2 * pi);
	if (di < -pi)
	    di += (2 * pi);
        var  += dr * dr + di * di;
    }
    if (penalty_flag)
        var += penalty(dpnc, a[4], a[7], dpnp);

    return (var);
}

static void
dp_exp_fit_2(float *w, PLTFMT *pf)
{
    double dt_s, var1, var2, vaf, sr, si, t1, t2;
    double sm1, sm2, ss1, ss2, msq, var;
    float  a[9], b[5], c[2], *ww, *www;
    int i, j1, j2, n;
    static int nit = 2000;
    static double eps = 1e-30;
    
    prn_msg("Performing exponential fit (complex)...");
    n = fr.nn;
    ww = w + fr.wo + (fr.mg * fr.wi + fr.wj[0]) * 2;
    sm1 = sm2 = ss1 = ss2 = 0;
    for (i = 0; i < n; i++) {
	www = ww + i * fr.wi * 2;
        fr.dpdb[i] = (float) (cln(www));
        fr.dpph[i] = (float) (cph(www));
	sm1 += www[0];
	sm2 += www[1];
	ss1 += www[0] * www[0];
	ss2 += www[1] * www[1];
    }
    msq = (sm1 / n) * (sm1 / n) + (sm2 / n) * (sm2 / n);
    var = (ss1 - sm1 * sm1 / n) / n + (ss2 - sm2 * sm2 / n) / n;
    dt_s = 0.001 / pf->df_khz;
    t1 = limit(1e-6, pf->dpt1, 100) / dt_s;
    t2 = limit(1e-6, pf->dpt2, 100) / dt_s;
    dpef = pf->dpef;
    dpnp = n;
    for (i = 0; i < 8; i++)
        a[i] = 0;
    a[0] = c[0] = (float) (avg(fr.dpdb, 0, n));
    a[1] = c[1] = (float) (avgph(fr.dpph, 0, n));
    dpnc = 2;
    penalty_flag = 0;
    var0 = dpef_var_2(a);
    if (dpef < 2) {
        j1 = 0;
        j2 = nint(t1 / ftc);
    } else if (pf->dpt1 > pf->dpt2) {
        j1 = nint(t2 / ftc);
        j2 = nint(t1 / ftc);
    } else {
        j1 = nint(t1 / ftc);
        j2 = nint(t2 / ftc);
    }
    j1 = limit(0, j1, n / 2);
    j2 = limit(1, j2, n / 2);
    a[0] = b[0] = (float) avg(fr.dpdb, j2, n);
    a[1] = b[1] = (float) avgph(fr.dpph, j2, n);
    a[2] = b[2] = (float) (avg(fr.dpdb, j1, j2) - a[0]);
    a[3] = b[3] = (float) (avgph(fr.dpph, j1, j2) - a[1]);
    a[4] = b[4] = (float) (ftc / j2);
    penalty_flag = 1;
    simpfit(a, dpnc = 5, nit, nit, dpef_var_2, NULL, NULL);
    penalty_flag = 0;
    var1 = dpef_var_2(a);
    if (dpef > 1) {
	a[5] = (float) (avg(fr.dpdb, 0, j1) - avgexp(a[0], a[2], a[4], 0, j1));
	a[6] = (float) (avgph(fr.dpph, 0, j1) - avgexp(a[1], a[3], a[4], 0, j1));
        a[7] = (float) (ftc / j1);
        penalty_flag = 1;
        simpfit(a, dpnc = 8, nit, nit, dpef_var_2, NULL, NULL);
	penalty_flag = 0;
	var2 = dpef_var_2(a);
    } else {
	var2 = var0 + var1; /* make var2 larger than either var0 or var1 */
    }
    if (var2 < var0 && var2 < var1) {
	vaf = (var0 > 0) ? 1 - var2 / var0 : 0;
    } else if (var1 < var0 && var1 < var2) {
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
	a[3] = b[3];
	a[4] = b[4];
	a[5] = 0;
	a[6] = 0;
	a[7] = 0;
	vaf = (var0 > 0) ? 1 - var1 / var0 : 0;
    } else {
	a[0] = c[0];
	a[1] = c[1];
	a[2] = 0;
	a[3] = 0;
	a[4] = 0;
	a[5] = 0;
	a[6] = 0;
	a[7] = 0;
	vaf = 0;
    }
    sr = 20 / log(10);
    si = 1  / (2 * 3.1415927);
    pf->efc[0] = a[0] * sr - fr.dbref - pf->dbvpu;
    pf->efc[1] = a[2] * sr;
    pf->efc[2] = (a[4] > 0) ? dt_s / a[4] : 0;
    pf->efc[3] = a[5] * sr;
    pf->efc[4] = (a[7] > 0) ? dt_s / a[7] : 0;
    pf->efc[5] = vaf;
    pf->efc[6] = a[1] * si;
    pf->efc[7] = a[3] * si;
    pf->efc[8] = a[6] * si;
    pf->efc[9] = (msq > eps && var > eps) ? 10 * log10(msq / var) : -300;
    for (i = 0; i < n; i++) {
        fr.dpdb[i] = (float) (a[0] * sr - pf->dbvpu);
        fr.dpdb[i] += (float) (a[2] * exp(-i * a[4]) * sr);
        fr.dpdb[i] += (float) (a[5] * exp(-i * a[7]) * sr);
        fr.dpph[i] = (float) (a[1] * si - pf->phase_off);
        fr.dpph[i] += (float) (a[3] * exp(-i * a[4]) * si);
        fr.dpph[i] += (float) (a[6] * exp(-i * a[7]) * si);
    }
}

void
dp_exp_fit(float *w, PLTFMT *pf)
{
    if (pf->dpfc) {
        fr.dpph = fr.dpdb + fr.np;
        dp_exp_fit_2(w, pf);
    } else {
        dp_exp_fit_1(w, pf);
    }
}
