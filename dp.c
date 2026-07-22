/* dp.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftlib.h>
#include "display.h"
#include "sysres.h"

#define limit(mn,aa,mx) (((aa)<(mn))?(mn):((aa)>(mx))?(mx):(aa))

extern double splref;
extern float *st, *rt, *sf, *rf;
extern int nrcp;
extern PGMCFG cf;
extern PLTFMT pf;

int     stim_compute(int, char *);
int	is_resp();
void    fft(float *, int);
void    ifft(float *, int);
void    resp_compute();

void
dp_fc(int i1, int i2, int *ii)
{
    int n;
    static int c1[8] = { 6,  5,  4,  3,  2, -1, -1, -2};
    static int c2[8] = {-5, -4, -3, -2, -1,  1,  2,  3};

    n = limit(-5, pf.dpon, 2) + 5;
    ii[0] = c1[n] * i1 + c2[n] * i2;
    ii[1] = i1;
    ii[2] = i2;
    ii[3] = 2 * i1 - (i1 + i2) / 2;
    ii[4] = 3 * i1 - 2 * i2;
}

static void
dp_ft(int o, int n)
{
    float *rfc, *rtc;
    int i, ch, nc;

    nc = nrcp;
    if (nc == 1) {
        memcpy(rf + o, rt + o, n * sizeof(float));
        fast(rf + o, n);
    } else {		// if more than one A/D channel is recorded
	ch = pf.dpch;   // analyze only the selected channel
        rfc = &rf[o];
        rtc = &rt[o * nc + ch];
	for (i = 0; i < n; i++) {
	    rfc[i] = rtc[nc * i];
	}
	fast(rfc, n);
    }
}

void
dp_stft()
{
    int i, n;

    if (cf.ncal < 64 || cf.nsamp < cf.ncal)
        return;
    if (rt == NULL || rf == NULL)
        return;
    n = pf.nc;
    for (i = 0; i < n; i++)
        dp_ft(i * cf.ncal + pf.ofst, cf.ncal);
    pf.dbvpu = -20 * log10(2.0 / cf.ncal);
}

void
dp_filt_f(float *ff, int n, int ft)
{
    double d, p, w, c, nsb;
    int i, j, ii;

    memset(ff, 0, 2 * n * sizeof(float));
    nsb = 0.001 * n * pf.dpbw / pf.df_khz;
    if (nsb < 1)
        nsb = 1;
    c = (double) n / cf.nsamp;
    j = 2 * n;
    //k = 2 * n + 1;
    ff[0] = (float) c;
    for (i = 1; i < n / 2; i++) {
    	if (ft == 0) {                          /* 24 dB/oct */
            d = i / nsb;
            d = d * d;                          /* = (i/nsb)^2 */
            d = d * d;                          /* = (i/nsb)^4 */
            w = 1 / (1 + d);
    	} else if (ft == 1) {                   /* Blackman */
            if (i > 3 * nsb) {
                w = 0;
            } else {
                p = i * M_PI / (3 * nsb + 1);
                w = 0.42 + 0.5 * cos(p) + 0.08 * cos(p * 2);
            }
    	} else if (ft == 2) {                   /* Gaussian */
            d = i / nsb;
            w = exp(-d * d * log(2.0));
        } else {
            w = (i > nsb) ? 0 : 1;
        }
        ii = i * 2;
        ff[ii] = ff[j - ii] = (float) (w * c);
    }
}

void
dp_filt_t(float *ff, int n, int ft)
{
    float c;
    int i, ir, ii, m;

    m = pf.dpfw;
    c = n * ((float) n / cf.nsamp);
    if (m > 0)
    	c /= m;
    for (i = 0; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        ff[ir] = (i < m) ? c : 0;
        ff[ii] = 0;
    }
    fft(ff, n);
}

void
dp_filt(float *ff, int n)
{
    int ft = pf.dpft;
    if (ft >= 0 && ft <= 3) {
        dp_filt_f(ff, n, ft);    /* frequency-domain filter */
    } else {
        dp_filt_t(ff, n, ft);    /* time-domain filter */
    }
}

void
dp_ht(int icf, float *fb, float *ff, int n)
{
    float fbr, fbi, *fe;
    int i, ir, ii, na, n1, n2;

    memset(fb, 0, 2 * n * sizeof(float));
    n1 = (n > (cf.nsamp - icf)) ? ((cf.nsamp - icf) % 2) : n;
    n2 = (n > icf) ? (icf % 2) : n;
    fe = fb + n * 2 - n2;
    if (n1 > 0)
	memcpy(fb, &rf[icf * 2], n1 * sizeof(float));
    if (n2 > 0)
	memcpy(fe, &rf[icf * 2 - n2], n2 * sizeof(float));

    // remove mean value
    if (pf.dprm) {
	fbr = 0;
	fbi = 0;
	na = 0;
	for (i = 1; i < n1 / 2; i++) {
	    fbr += fb[2 * i];
	    fbi += fb[2 * i + 1];
	    na++;
	}
	for (i = 1; i < n2 / 2; i++) {
	    fbr += fe[2 * i];
	    fbi += fe[2 * i + 1];
	    na++;
	}
	if (na > 0) {
	    fbr /= na;
	    fbi /= na;
	    for (i = 0; i < n1 / 2; i++) {
		fb[2 * i] -= fbr;
		fb[2 * i + 1] -= fbi;
	    }
	    for (i = 0; i < n2 / 2; i++) {
		fe[2 * i] -= fbr;
		fe[2 * i + 1] -= fbi;
	    }
	}
    }

    for (i = 0; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        fbr = ff[ir] * fb[ir] - ff[ii] * fb[ii];
        fbi = ff[ir] * fb[ii] + ff[ii] * fb[ir];
        fb[ir] = fbr;
        fb[ii] = fbi;
    }
    ifft(fb, n);
}

void
dp_hetr()
{
    float *fbuf, *filt;
    int i, m, n, ns, i1, i2, i3, ii[NDP];

    if (rt == NULL || rf == NULL)
        return;
    ns = cf.nsamp;
    dp_ft(0, ns);
    i1 = (int) (ns * cf.f1 / cf.rate + 0.5);
    i2 = (int) (ns * cf.f2 / cf.rate + 0.5);
    i3 = (int) (ns * cf.f3 / cf.rate + 0.5);
    dp_fc(i1, i2, ii);
    if (i3 > 0)
	ii[4] = i3;
    m = NDP * 2 * pf.dpen;
    if (m > ns) {
	return;
    }
    fbuf = (float *) calloc(m, sizeof(float));
    if (fbuf == NULL) {
        return;
    }
    filt = (float *) calloc(m, sizeof(float));
    if (filt == NULL) {
        free(fbuf);
        return;
    }
    n = pf.dpen;
    dp_filt(filt, n);
    for (i = 0; i < NDP; i++)
        dp_ht(ii[i], &fbuf[i * 2 * n], filt, n);
    memcpy(rf, fbuf, m * sizeof(float));
    free(filt);
    free(fbuf);
    pf.dbvpu = -20 * log10(2.0 / n);
}

void
dp_compute()
{
    if (pf.dpen == 0)
        dp_stft();
    else
        dp_hetr();
}

void
dp_stim(char *msg)
{
    int i, ns, nc;

    nc = cf.ncal;
    ns = cf.nsamp;
    cf.ncal = 0;
    if (stim_compute(cf.stm_typ, msg)) {
	if (cf.da_mode == 1 || cf.da_mode == 2) {
            memcpy(rt, st, ns * sizeof(float));
	} else if (cf.da_mode == 3) {
	    for (i = 0; i < ns; i++) {
		rt[i] = st[i * 2 + 0] + st[i * 2 + 1];
	    }
	}
        cf.ad_mode = 1;
	resp_compute();
        sprintf(msg, "DP stimulus");
    }
    cf.ncal = nc;
}

/*******************************************************************************/

/* DP Lissajous-path optimal level */

static double
dbc(int o, int m, int ad, double dbrf, int fold)
{
    double rr, ri, rs, lv;
    int i1, j1, i2, j2, i3, j3, i4, j4;

    if (fold == 1) {
        i1 = 2 * o;
	j1 = i1 + 1;
	i2 = 2 * (o + m);
	j2 = i2 + 1;
	if (ad) {
	    rr = (rf[i1] + rf[i2]) / 2;
	    ri = (rf[j1] + rf[j2]) / 2;
	} else {
	    rr = (rf[i1] - rf[i2]) / 2;
	    ri = (rf[j1] - rf[j2]) / 2;
	}
    } else if (fold == 2) {
        i1 = 2 * o;
	j1 = i1 + 1;
	i2 = 2 * (o + m);
	j2 = i2 + 1;
	i3 = 2 * (o + m * 2);
	j3 = i3 + 1;
	i4 = 2 * (o + m * 3);
	j4 = i4 + 1;
	if (ad) {
	    rr = ((rf[i1] + rf[i3]) + (rf[i2] + rf[i4])) / 4;
	    ri = ((rf[j1] + rf[j3]) + (rf[j2] + rf[j4])) / 4;
	} else {
	    rr = ((rf[i1] + rf[i3]) - (rf[i2] + rf[i4])) / 4;
	    ri = ((rf[j1] + rf[j3]) - (rf[j2] + rf[j4])) / 4;
	}
    } else {
	rr = ri = 0;
    }
    rs = rr * rr + ri * ri;
    if (rs < 1e-30)
	rs = 1e-30;
    lv = 10 * log10(rs) - dbrf;
    return (lv);
}

static int
db_round(float *w, float *v, int n)
{
    float tmp;
    int i, j;

    // round to nearest dB
    for (i = 0; i < n; i++) {
	w[i] = (float) floor(v[i] + 0.5);
    }
    // sort
    for (i = 0; i < n; i++) {
        for (j = i; j < n; j++) {
	    if (w[j] < w[i]) {
		tmp = w[i];
		w[i] = w[j];
		w[j] = tmp;
	    }
	}
    }
    // make unique
    for (i = 1, j = 1; i < n; i++) {
	if (w[j-1] != w[i])
	    w[j++] = w[i];
    }
    return (j);
}

static double
median(float *v, int n)
{
    double med;
    int i, j;
    float *s, temp;

    s = calloc(n, sizeof(float));
    // copy input array to local array
    for (i = 0; i < n; i++) {
	s[i] = v[i];
    }
    // sort local array
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
	    if (s[i] > s[j]) {
		temp = s[i];
		s[i] = s[j];
		s[j] = temp;
	    }
	}
    }
    // median in middle of local array
    med = s[n / 2 - 1];
    free(s);

    return (med);
}

static int
maxnset(float *u, float *v, float *w, int n, double uref, double snr)
{
    double urnd;
    int j, j1, j2, jmx;

    // find maximum v for the subset u ~= uref
    j1 = j2 = -1;
    for (j = 0; j < n; j++) {
        urnd = floor(u[j] + 0.5);
        if (urnd == uref) {
	    if ((j1 < 0) || (v[j1] < v[j])) {
		j1 = j;
	    }
	    if ((v[j] - w[j]) >= snr) {
		if ((j2 < 0) || (v[j2] < v[j])) {
		    j2 = j;
		}
	    }
	}
    }
    jmx = (j2 >= 0) ? j2 : j1;
    return (jmx);
}

static void
regress(float *x, float *y, int n, double *c, int o)
{
    double *v, *w, *z, p;
    int   i, j, k, m = o + 1;

    v = c;
    w = calloc(m * m, sizeof(double));
    z = calloc(n * m, sizeof(double));
    // fill regression arrays
    for (k = 0; k < n; k++) {
	z[k * m] = 1;
	for (j = 1; j < m; j++)
	    z[k * m + j] = z[k * m + j - 1] * x[k];
    }
    for (i = 0; i < m; i++) {
	v[i] = 0;
        for (k = 0; k < n; k++) {
	    v[i] += z[k * m + i] * y[k];
	}
	for (j = 0; j < m; j++) {
	    w[i * m + j] = 0;
	    for (k = 0; k < n; k++) {
		w[i * m + j] += z[k * m + i] * z[k * m + j];
	    }
	}
    }
    // solve regression equation by elimination
    for (j = 0; j < m; j++) {
	for (i = 0; i < j; i++) {
	    p = w[j * m + i];
	    v[j] -= p * v[i];
	    for (k = i; k < m; k++) {
	        w[j * m + k] -= p * w[i * m + k];
	    }
	}
	p = w[j * m + j];
	v[j] /= p;
	for (i = j; i < m; i++) {
	    w[j * m + i] /= p;
	}
    }
    for (j = m - 2; j >= 0 ; j--) {
	for (i = (j + 1); i < m; i++) {
	    v[j] -= w[j * m + i] * v[i];
	}
    }
    free(w);
    free(z);
}

int
ilog2(int a)
{
    int b, c;

    for (b = 0, c = 1; c < a; b++) {
	c *= 2;
    }
    return (b);
}

int
gcd(int a, int b)
{
    return b ? gcd(b, a % b) : a;
}

static void
dp_optlev(double *c, double *L2r)
{
    double dbrf;
    float *fbuf, *L1, *L2, *Ld, *Ln;
    float SNR, *L1d, *L2d, *Ldd, *Lnd;
    float sig, snr;
    int i, j, k, m, n, o1, o2, od, imn, ord, fld, nen;

    c[0] = c[1] = L2r[0] = L2r[1] = 0;
    if (rt == NULL || rf == NULL)
        return;
    fld = ilog2(gcd(nint(cf.lpnc[0]), nint(cf.lpnc[1])));
    nen = pf.dpen;
    imn = cf.olmin;
    ord = cf.olord;
    snr = (float) cf.olsnr;
    if (fld == 1)
        m = nen / 2;
    else if (fld == 2)
	m = nen / 4;
    else 
	return;
    fbuf = (float *) calloc(8 * m, sizeof(float));
    if (fbuf == NULL)
	return;
    L1 = fbuf;
    L2 = fbuf + m;
    Ld = fbuf + m * 2;
    Ln = fbuf + m * 3;
    L1d = fbuf + m * 4;
    L2d = fbuf + m * 5;
    Ldd = fbuf + m * 6;
    Lnd = fbuf + m * 7;
    od = 0;	    // offset to Pd
    o1 = nen;	    // offset to P1
    o2 = nen * 2;   // offset to P2
    dbrf = 20 * log10(cf.sens_mp * splref) + pf.dbvpu;
    for (i = 0; i < m; i++) {
	L1[i] = (float) dbc(o1 + i, m, 1, dbrf, fld);
	L2[i] = (float) dbc(o2 + i, m, 1, dbrf, fld);
	Ld[i] = (float) dbc(od + i, m, 1, dbrf, fld);
	Ln[i] = (float) dbc(od + i, m, 0, dbrf, fld);
    }
    n = db_round(L2d, L2, m);
    sig = (float) (median(Ln, m) + snr);
    L2r[0] = 999;
    L2r[1] = -999;
    for (i = imn, j = 0; i < n; i++) {	// skip smallest L2
	k = maxnset(L2, Ld, Ln, m, L2d[i], snr);
	if (k >= 0) {
	    SNR = Ld[k] - Ln[k];
	    if ((SNR >= snr) && (Ld[k] >= sig)) {
		Ldd[j] = Ld[k];
		Lnd[j] = Ln[k];
		L1d[j] = L1[k];
		L2d[j] = L2d[i];
		if (L2r[0] > L2d[j])
		    L2r[0] = L2d[j];
		if (L2r[1] < L2d[j])
		    L2r[1] = L2d[j];
		j++;
	    }
	}
    }
    regress(L2d, L1d, j, c, ord);
}

static void
dp_olstr(char *s, double *c)
{
    int i, o;

    o = cf.olord;
    *s = '\0';
    for (i = 0; i <= o; i++)
	sprintf(s + strlen(s), " %.3g", c[i]);
}

void
dp_lpol(char *s, char *msg)
{
    char cv[80];
    double c[4], L2r[2];
    FILE *fp;

    cf.olord = limit(1, cf.olord, 3);
    dp_optlev(c, L2r);
    dp_olstr(cv, c);
    l1eq(cv, 0);
    if (strlen(s) > 0) {
	fp = fopen(s, "wt");
	fprintf(fp, "l1eq=%s ; L2=%.0f,%.0f\n", cv, L2r[0], L2r[1]);
	fclose(fp);
    }
    sprintf(msg, "L1eq=%s, L2=%.0f,%.0f", cv, L2r[0], L2r[1]);
}

void
dp_lsol(char *s, char *msg)
{
    char cv[80], *tn;
    double c[4], L2r[2];
    double f1, f2, L2, L1, L3, L2beg, L2inc;
    double avtim, noise, snrat;
    int i, nc;
    FILE *ifp, *ofp;
    static char *hdr[] = {
	"Octave=0",
	"Abscissa=L2",
	"DP_freq=2*F1-F2",
	"NNSB=0",
	"Signal=DP+",
	"Datafmt=Extended",
	"SaveBin=No",
	"Units=dB",
	"  F2      F1      L2      L1       T    Noise    SNR      F3      L3"
    };
    static int hdrsiz = sizeof(hdr) / sizeof(hdr[0]);

    cf.olord = limit(1, cf.olord, 3);
    dp_optlev(c, L2r);
    dp_olstr(cv, c);
    l1eq(cv, 0);
    if (strlen(s) > 0) {
	if (strchr(s, ',') == NULL) {
	    nc = cf.lsnum;
	    L2beg = cf.lsbeg;
	    L2inc = cf.lsinc;
	    avtim = cf.lsavt;
	    noise = cf.lsnlv;
	    snrat = cf.lssnr;
	    ofp = fopen(s, "wt");
	    fprintf(ofp, "; %s - optimum levels generated by SysRes\n", s);
	    fprintf(ofp, "; L1eq = %s\n", cv);
	    fprintf(ofp, "; L2range = %.0f,%.0f (dB SPL)\n", L2r[0], L2r[1]);
	    fprintf(ofp, ";Repeat=%d\n", cf.lsrep);
	    fprintf(ofp, ";Suppressor=%s\n", cf.lsf3 > 0 ? "Yes" : "No");
	    for (i = 0; i < hdrsiz; i++) {
		fprintf(ofp, ";%s\n", hdr[i]);
	    }
	    for (i = 0; i < nc; i++) {
		L2 = L2beg + i * L2inc;
		L1 = c[0] + c[1] * L2;
		L3 = cf.lsl3a + cf.lsl3b * L2;
		fprintf(ofp, "%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f \n", 
		    cf.f2, cf.f1, L2, L1, avtim, noise, snrat, cf.lsf3, L3);
	    }
	    fclose(ofp);
	} else {
	    tn = strchr(s, ',') + 1;
	    tn[-1] = '\0';
	    ofp = fopen(s, "wt");
	    fprintf(ofp, "; %s - optimum levels generated by SysRes\n", s);
	    fprintf(ofp, "; L1 = %s\n", cv);
	    fprintf(ofp, "; template = %s\n", tn);
	    ifp = fopen(tn, "rt");
	    while (fgets(cv, 80, ifp) != NULL) {
		if (*cv == ';') {
		    fputs(cv, ofp);
		} else {
		    sscanf(cv, "%lf %lf %lf %lf %lf %lf %lf", 
			&f2, &f1, &L2, &L1, &avtim, &noise, &snrat);
		    // replace L1, f2, & f1 values
		    L1 = c[0] + c[1] * L2;
		    fprintf(ofp, "%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f \n", 
			cf.f2, cf.f1, L2, L1, avtim, noise, snrat);
		}
	    }
	    fclose(ifp);
	    fclose(ofp);
	}
    }
    sprintf(msg, "L1eq=%s, L2=%.0f,%.0f", cv, L2r[0], L2r[1]);
}

/*******************************************************************************/

void
dp_cp(char *msg)
{
    double d, dcp;
    int i, j, jcp, k, kcp, m, n;
    static char *fn = "DP closest pair";

    n = 4;
    m = cf.nsamp / n;
    if (m < MIN_NSAMP) {
        sprintf(msg, "%s: too few samples", fn);
    } else if (!is_resp()) {
        sprintf(msg, "%s: no response", fn);
    } else if (cf.nsamp % n) {
	sprintf(msg, "%s: number of samples should be multiple of %d", fn, n);
    } else {
	for (i = 0; i < m; i++) {
	    jcp = 0;
	    kcp = 1;
	    dcp = fabs(rt[i + jcp * m] - rt[i + kcp * m]);
	    for (j = 0; j < n; j++) {
		for (k = j + 1; k < n; k++) {
		    d = fabs(rt[i + j * m] - rt[i + k * m]);
		    if (d < dcp) {
			jcp = j;
			kcp = k;
			dcp = d;
		    }
		}
	    }
	    rt[i] = (rt[i + jcp * m] + rt[i + kcp * m]) / 2;
	}
        cf.nsamp /= n;
	for (i = 0; i < 3; i++) {
	    cf.amnc[i] /= n;
	    cf.lpnc[i] /= n;
	}
	resp_compute();
        sprintf(msg, "%s: from %d samples", fn, n);
    }
}

void
dp_mp(char *msg)
{
    float temp, r[4];
    int i, j, k, m, n, o;
    static char *fn = "DP middle pair";

    n = 4;
    o = n / 2;
    m = cf.nsamp / n;
    if (m < MIN_NSAMP) {
        sprintf(msg, "%s: too few samples", fn);
    } else if (!is_resp()) {
        sprintf(msg, "%s: no response", fn);
    } else if (cf.nsamp % n) {
	sprintf(msg, "%s: number of samples should be multiple of %d", fn, n);
    } else {
	for (i = 0; i < m; i++) {
	    for (j = 0; j < n; j++) {	// fetch n samples
		r[j] = rt[i + j * m];
	    }
	    for (j = 0; j < n; j++) {
		for (k = j + 1; k < n; k++) {
		    if (r[j] > r[k]) {	// sort values
			temp = r[j];
			r[j] = r[k];
			r[k] = temp;
		    }
		}
	    }
	    rt[i] = (r[o - 1] + r[o]) / 2;
	}
        cf.nsamp /= n;
	for (i = 0; i < 3; i++) {
	    cf.amnc[i] /= n;
	    cf.lpnc[i] /= n;
	}
	resp_compute();
        sprintf(msg, "%s: from %d samples", fn, n);
    }
}
