/* sysres.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftlib.h>
#include "cardinfo.h"
#include "display.h"
#include "version.h"
#include "sysres.h" 
#include "sio.h"
#ifdef _MSC_VER
#include "uni.h"
#else 
#include <unistd.h>
#endif

double  cal_atten(double, double, int);
double  db_ref();
double  gr_etime();
int     chk_escape();
int     is_ascii(char *);
int     is_mat(char *);
void    sio_init_cardinfo();
void    chk_resp(int *);
void    flush_data();
void    flush_eventq();
void    gr_getline(char *, char *, int);
void    gr_pbstr(char *);
void    gr_prcl(char *);
void    gui();
void    init_chkrsp();
void    prn_msg(char *);
CARDINFO sio_get_cardinfo(int);

char  *no_dev = "No I/O device.";
double reject_thresh = 0, dpn_sb = 0;
double samp_val = 1;
double stime = 0;
double splref = 2.828427e-5; /* sqrt(2) * 20 uPa */
double swpskp = 1;
double tone_attn1 = 0;
double tone_attn2 = 0;
double tone_attn3 = 0;
FILE  *lfp[8] = {NULL};
FILE  *mfp = NULL;
float *rt = NULL, *rf = NULL, *rx = NULL;
float *st = NULL, *sf = NULL;
int    dc_remove = 0;
int    debug = 0;
int    gr_mode = 0;
int    inclev = 0;
int    logflg = 0;
int    nrej = 0, reject_mode = 0, wtav_mode = 0;
int    nsab = 4;
int    nsch = 0, nrch = 0, nrcp = 0, rchk = 0;
int    ntwb = 0;
int    sio_present = 0;

PLTFMT pf = {
    {1, 1, 1, 1, 0},
    10, 100, 0, 0, 0, 0, 0, {0, 0, 0, 0},
    4, 5, 0,
    2, 4, 0, 
    0, 0, 0, 1.5, 0.075, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    -1, 0
};
PGMCFG cf = {
    "", "$vn",
    1, 44100, 0, 0, 1000, 1200, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 9,
    {1, 1, 1}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0},
    32, -15, 5, -25, 60, 0, 0, 0,
    1024, 0,
    0, 0, 0, 1, 1,
    20, 0, 1, 0, 0, 0, 1,
    20, 2, 6, 1,
};
MSKSTM tm = {5, 0, 50, 5, 5, 1, 0.1};

static int n_allocated = 0;
static int resp_computed = 0;

void
defpar()
{
    int i;

    // PLTFMT
    for (i = 0; i < 4; i++)
	pf.dp_dsp[i] = 1;
    pf.dp_dsp[4] = 0;
    pf.dbvref = 10;
    pf.dbrange = 100;
    pf.dbvpu = 0;
    pf.zf1 = 0;
    pf.zf2 = 0;
    pf.zt1 = 0;
    pf.zt2 = 0;
    for (i = 0; i < 4; i++)
        pf.lv[i] = 0;
    pf.delay_max = 4;
    pf.delay_range = 5;
    pf.delay_off = 0;
    pf.phase_max = 2;
    pf.phase_range = 4;
    pf.phase_off = 0;
    pf.df_khz = 0;
    pf.dpbw = 0;
    pf.dpsb = 0;
    pf.dpt1 = 1.5;
    pf.dpt2 = 0.075;
    for (i = 0; i < 10; i++)
	pf.efc[i] = 0;
    pf.nfft = 0;
    pf.dpen = 0;
    pf.dpft = 0;
    pf.ofst = 0;
    pf.nnsb = 0;
    pf.npfit = 0;
    pf.log_axis = 0;
    pf.show_phase = 0;
    pf.show_delay = 0;
    pf.clear_screen = 0;
    pf.time_axis = 0;
    pf.i1 = 0;
    pf.i2 = 0;
    pf.nc = 0;
    pf.dp = 0;
    pf.dpef = 0;
    pf.dpfc = 0;
    pf.dpfw = 0;
    pf.zoom = 0;
    pf.tic_dir = 0;
    pf.eml = 0;
    pf.dpch = 0;
    pf.dpon = -1;
    pf.dprm = 0;
    // PGMCFG
    strncpy(cf.prn_name,"",80);
    strncpy(cf.prn_label,"$vn",80);
    cf.volts = 1;
    cf.rate = 44100;
    cf.delay = 0;
    cf.sens_mp = 0;
    cf.f1 = 1000;
    cf.f2 = 1200;
    cf.f3 = 0;
    cf.l1 = 0;
    cf.l2 = 0;
    cf.l3 = 0;
    cf.beg_dur = 0;
    cf.end_dur = 0;
    cf.rmp_dur = 0;
    cf.gap = 0;
    cf.pregap = 0;
    cf.olsnr = 9;
    for (i = 0; i < 3; i++) {
	cf.dbms[i] = 1;
	cf.lpnc[i] = 0;
	cf.lpph[i] = 0;
	cf.lpma[i] = 0;
	cf.lpmb[i] = 0;
	cf.lpmc[i] = 0;
	cf.amin[i] = 0;
	cf.amnc[i] = 0;
    }
    cf.lsavt = 32;
    cf.lsbeg = -15;
    cf.lsinc = 5;
    cf.lsnlv = -25;
    cf.lssnr = 60;
    cf.lsf3 = 0;
    cf.lsl3a = 0;
    cf.lsl3b = 0;
    cf.nsamp = 1024;
    cf.ncal = 0;
    cf.prn_type = 0;
    cf.prn_orient = 0;
    cf.ovrwrt = 0;
    cf.ad_mode = 1;
    cf.da_mode = 1;
    cf.nav = 20;
    cf.stm_typ = 0;
    cf.ntone = 1;
    cf.rmp_typ = 0;
    cf.cal_chn = 0;
    cf.dpsf = 0;
    cf.acsf = 1;
    cf.lsnum = 20;
    cf.lsrep = 2;
    cf.olmin = 6;
    cf.olord = 1;
    cf.choi = 0;
    cf.choo = 0;
    cf.pefr = 0;
    cf.pesl = 0;
}

void
drop_suid()
{
#ifdef linux
    setuid(getuid());		/* drop root priveleges */
    setgid(getgid());
#endif /* linux */
}

void
free_mem()
{
    if (st != NULL)
        free(st);
    if (sf != NULL)
        free(sf);
    if (rt != NULL)
        free(rt);
    if (rf != NULL)
        free(rf);
    if (rx != NULL)
        free(rx);
    st = sf = rt = rf = rx = (float *) NULL;
}

int
alloc_mem(int n)
{
    int ns, nr;
    static int lnsch = 0, lnrch = 0, lrchk = 0;

    if ((n_allocated >= n) 
        && (nsch == lnsch) 
        && (nrch == lnrch) 
        && (rchk == lrchk))
        return (n_allocated);
    if (n > MAX_NSAMP)
        n = MAX_NSAMP;
    if ((n_allocated < n) || (nsch != lnsch)) {
        if (st != NULL) {
            free(st);
	    st = (float *) NULL;
	}
        if (sf != NULL) {
            free(sf);
	    sf = (float *) sf;
	}
	if (nsch) {
            ns = (n + 2) * nsch;
            st = (float *) calloc(ns, sizeof(float));
            sf = (float *) calloc(ns, sizeof(float));
            if (st == NULL || sf == NULL) {
                free_mem();
                return (0);
            }
	}
        lnsch = nsch;
    }
    if ((n_allocated < n) || (nrch != lnrch)) {
        if (rt != NULL) {
            free(rt);
	    rt = (float *) NULL;
	}
        if (rf != NULL) {
            free(rf);
	    rf = (float *) NULL;
	}
	if (nrch) {
            nr = (n + 2) * nrch;
            rt = (float *) calloc(nr, sizeof(float));
            rf = (float *) calloc(nr, sizeof(float));
            if (rt == NULL || rf == NULL) {
		free_mem();
                return (0);
            }
	}
        lnrch = nrch;
    }
    if ((n_allocated < n) || (rchk != lrchk)) {
        if (rx != NULL) {
            free(rx);
            rx = (float *) NULL;
        }
	if (nrch && rchk) {
            nr = (n + 2) * nrch;
            rx = (float *) calloc(nr, sizeof(float));
            if (rx == NULL) {
                free_mem();
                return (0);
            }
        }
        lrchk = rchk;
    }
    n_allocated = n;
    return (n_allocated);
}

void
stim_scale(int dir)
{
    float sc;
    int i, j;

    for (j = 0; j < nsch; j++) {
        sc = (float) (cf.volts / samp_val);
	if (cf.ncal > 0 && cf.acsf) {
	    if (j == 0)
		sc = (float) (sc / pow(10, cal_atten(cf.f1, cf.l1, 0) / 20));
	    else if (j == 1)
		sc = (float) (sc / pow(10, cal_atten(cf.f2, cf.l2, 1) / 20));
	    else if (j == 2)
		sc = (float) (sc / pow(10, cal_atten(cf.f3, cf.l3, 1) / 20));
	}
	if (sc != 1) {
	    if (dir) {
		sc = 1 / sc;
	    }
	    for (i = 0; i < cf.nsamp; i++) {
		st[i * nsch + j] *= sc;
	    }
	}
    }
    if (dir == 0 && cf.ncal > 0 && nsch == 3) {	// fold channel 2 into channel 1 ?
        for (i = 0; i < cf.nsamp; i++) {
	    st[i * 2 + 1] = st[i * 3 + 1] + st[i * 3 + 2];
	    nsch = 2;
        }
    }
}

static void
stim_zero(int c, int n)
{
    int i;

    for (i = 0; i < n; i++)
        st[i * c] = 0;
}

int
nint(double x)
{
    return((int)floor(x + 0.5));
}

/*
 * unirand - uniform random number generator
 *
 * From "Random Number Generators: Good Ones are Hard to Find",
 * by Stephen K. Park and Keith W. Miller,
 * Communications of the ACM, 31, 10 (Oct. 1988) pp. 1192-1201.
 *
 */
static double
unirand()
{
    long    hi, lo;
    static long seed = 1;
    static long a = 16807;
    static long m = 2147483647;	/* Mersenne prime 2^31 -1 */
    static long q = 127773;	/* M div A (M / A) */
    static long r = 2836;	/* M mod A (M % A) */

    hi = seed / q;
    lo = seed % q;
    if ((seed = a * lo - r * hi) <= 0)
	seed += m;
    return ((double) seed / m);
}

static void
compute_chirp_pe(int c, int n, double a, double f, double s)
{
    int i;
    double dp, ph, am, mx;
    CARDINFO ci;

    ci = sio_get_cardinfo(-1);
    mx = ci.da_vfs[c];
    dp = M_PI / (n - 1);
    ph = 0;
    for (i = 0; i < n; i++) {
        am = a;
        if (s > 0) {
            am *= sqrt(1 + pow(i / f, s));
        } else if (s < 0) {
            am /= sqrt(1 + pow(i / f, -s));
        }
        am = (am < mx) ? am : mx;
        ph += i * dp;
        st[i * c] = (float) (am * sin(ph));
    }
}

static void
compute_chirp(int c, int n, double a)
{
    int i;
    double dp, ph;

    dp = -2 * M_PI / n;
    for (i = 0; i <= (n / 2); i++) {
	ph = i * dp * i;
	sf[2 * i] = (float) cos(ph);
	sf[2 * i + 1] = (float) sin(ph);
    }
    a *= sqrt(n / 2.0);
    fsst(sf, n);
    for (i = 0; i < n; i++) {
        st[i * c] = (float) (a * sf[i]);
    }
}

static void
compute_impulse(int c, int n, double a)
{
    int i;

    st[0] = (float) a;
    for (i = 1; i < n; i++)
        st[i * c] = 0;
}

static double
phase_incr(int n, double f)
{
    int nc;
    double dp, tp;

    if (cf.ncal > 0)
        n = cf.ncal;
    nc = nint(n * f / cf.rate);
    tp = 2 * M_PI;
    dp = nc * tp / n;
    return (dp);
}

static double
max_volts()
{
    double adv[8], dav[8];

    if (sio_present) {
        sio_get_vfs(adv, dav);
        return (dav[0]);
    }
    return (1);
}

static void
tone_atten()
{
    double d, vmx, lmx, lmn;

    if (cf.ncal > 0) {
	tone_attn1 = cal_atten(cf.f1, cf.l1, 0);
	tone_attn2 = cal_atten(cf.f2, cf.l2, 1);
	tone_attn3 = cal_atten(cf.f3, cf.l3, 1);
    } else if ((cf.volts > 0) & (cf.sens_mp > 0)) {
        vmx = max_volts();
	lmx = 20 * log10(vmx) - db_ref();
	lmn = lmx - 200;
	cf.l1 = limit(lmn, cf.l1, lmx);
	cf.l2 = limit(lmn, cf.l2, lmx);
	cf.l3 = limit(lmn, cf.l3, lmx);
	d = 20 * log10(cf.volts) - db_ref();
	tone_attn1 = d - cf.l1;
	tone_attn2 = d - cf.l2;
	tone_attn3 = d - cf.l3;
//    } else {
//	tone_attn1 = 0;
//	tone_attn2 = 0;
//	tone_attn3 = 0;
    }
}

static void
ramp_stim(int c, int n, int o, int r)
{
    int i, j, k, m, b, e;
    double w = 0, s = 0, p, d;
    /* Blackman window coefficients */
    static double bw[3] = {0.42, 0.5, 0.08};
    /* Nuttall window coefficients */
    static double nw[4] = {0.355768, 0.487396, 0.144232, 0.012604};

    if (r <= 0)
        return;
    b = nint(cf.beg_dur * cf.rate * 0.001);
    e = nint(cf.end_dur * cf.rate * 0.001);
    m = nint(cf.rmp_dur * cf.rate * 0.001);
    d = cf.dbms[2] * 1000;
    if (r == 3)
        m = 0;
    else if (m > n / 2)
        m = n / 2;
    if (e > n - 2 * m)
        e = n - 2 * m;
    if (b > n - 2 * m - e)
        b = n - 2 * m - e;
    if (r == 1) {
        s = 1.0 / m;
    } else if (r == 2) {
        s = M_PI / m;
    } else if (r == 3 || r == 7) {
        m = (n - b - e) / 2;
        s = m ? M_PI / m : 0;
        e = n - b - 2 * m;
    }

    for (i = 0; i < b; i++) {
        st[i * c + o] = 0;
    }
    for (k = 0; k < m; k++) {
        i = k + b;
        j = n - e - k;
        if (cf.rmp_typ == 1) {
            w = k * s;
        } else if (r == 2) {
            w = (1 - cos(k * s)) / 2;
        } else if (r == 3) {	// Blackman window
            p = (m - k) * s;
            w = bw[0] + bw[1] * cos(p) + bw[2] * cos(p * 2);
        } else if (r == 4) {
            s = (m + 1 - k) / cf.rate;	// seconds
	    p = s * d;			// dB atten
            w = pow(10.0, -p / 20);  	// atten multiplier
        } else if (r == 7) {	// Nuttall window
            p = (m - k) * s;
            w = nw[0] + nw[1] * cos(p)
                + nw[2] * cos(p * 2) + nw[3] * cos(p * 3);
        }
        st[i * c + o] *= (float) w;
        st[j * c + o] *= (float) w;
    }
    for (i = n - e; i < n; i++) {
            st[i * c + o] = 0;
    }
}

static void
add_tone_exp(int c, int n, int o, double am, double dp, int r, int mpi)
{
    int i, m, b, e;
    double s = 0, w, d, a;

    b = nint(cf.beg_dur * cf.rate * 0.001);
    e = nint(cf.end_dur * cf.rate * 0.001);
    m = nint(cf.rmp_dur * cf.rate * 0.001);
    d = cf.dbms[mpi] * 1000;
    for (i = 0; i < n; i++) {
	if ((i < b) || (i >= (n - e))) {
	    s = m / cf.rate;
	} else if ((i >= b) &&  (i < (b + m))) {
	    s = (b + m - i) / cf.rate;
	} else if ((i >= (n - e - m)) &&  (i < (n - e))) {
	    s = (i - (n - e - m)) / cf.rate;
	}
	a = s * d;			// dB atten
	w = pow(10.0, -a / 20);  	// atten multiplier
	st[i * c + o] += (float) (w * am * cos(dp * i));
    }
}

static void
add_tone_lis(int c, int n, int o, double am, double dp, int r, int mpi)
{
    int i;
    double w, tpi;
    double m1 = 0, m2 = 0, m3 = 0;
    double d1, d2, d3, p1, p2, p3, s1, s2, s3;
    double da, in, wa, a;

    tpi = 2 * M_PI;
    if (mpi == 0) {
	m1 = cf.lpma[0];
	m2 = cf.lpmb[0];
	m3 = cf.lpmc[0];
    } else if (mpi == 1) {
	m2 = cf.lpma[1];
	m1 = cf.lpmb[1];
	m3 = cf.lpmc[1];
    } else if (mpi == 2) {
	m3 = cf.lpma[2];
	m1 = cf.lpmb[2];
	m2 = cf.lpmc[2];
    }
    d1 = cf.lpnc[0] * tpi / n;
    d2 = cf.lpnc[1] * tpi / n;
    d3 = cf.lpnc[2] * tpi / n;
    p1 = cf.lpph[0] * tpi / 360;
    p2 = cf.lpph[1] * tpi / 360;
    p3 = cf.lpph[2] * tpi / 360;
    da = cf.amnc[mpi] * tpi / n;
    in = cf.amin[mpi];
    for (i = 0; i < n; i++) {
	s1 = sin(d1 * i + p1);	     // sinusoid #1
	s2 = sin(d2 * i + p2);	     // sinusoid #2
	s3 = sin(d3 * i + p3);	     // sinusoid #3
	wa = da ? 1 - in * (1 + cos(da * i)) / 2 : 1;
	a = m1 * s1 + m2 * s2 + m3 * s3;    // dB atten
	w = pow(10.0, -a / 20) * wa;  	    // atten multiplier
	st[i * c + o] += (float) (w * am * cos(dp * i));
    }
}

static void
add_tone_pro(int c, int n, int o, double am, double dp, int r, int mpi)
{
    int i;
    double w;
    double m1 = 0, m2 = 0;
    double d1, n1, n2, s1, s2;
    double dn, a;

    dn = (double) n;
    m1 = cf.lpma[mpi];
    m2 = cf.lpmb[mpi];
    n1 = (cf.lpnc[0] > 0) ? cf.lpnc[0] : 1;
    n2 = (cf.lpnc[1] > 0) ? cf.lpnc[1] : 1;
    d1 = n1 * n2 / dn;
    //d2 = n2 / dn;
    for (i = 0; i < n; i++) {
	s1 = 1 - 2 * fmod(i * d1, 1);
	if (n1 < 2) {
	    s2 = 1;
	} else {
	    s2 = 1 - 2 * fmod(floor(i * d1) / n1, 1) * n1 / (n1 - 1);
	}
	a = m1 * s1 + m2 * s2;		    // dB atten
	w = pow(10.0, -a / 20);  	    // atten multiplier
	st[i * c + o] += (float) (w * am * cos(dp * i));
    }
}

static void
add_tone(int c, int n, int o, double a, double attn, double f, int r, int mpi)
{
    int i;
    double am, dp;

    if (f > 0) {
        am = a / pow(10.0, attn / 20);
        dp = phase_incr(n, f);
	if (r == 4) {		 // exponential ramp
	    add_tone_exp(c, n, o, am, dp, r, mpi);
	} else if (r == 5) {	// Lissajous ramp
	    add_tone_lis(c, n, o, am, dp, r, mpi);
	} else if (r == 6) {	// proscan ramp
	    add_tone_pro(c, n, o, am, dp, r, mpi);
	} else {
	    for (i = 0; i < n; i++)
		st[i * c + o] += (float) (am * cos(dp * i));
	}
    }
}

static void
compute_tones(int c, int n, double a, int r, int nt)
{
    int i, o;

    tone_atten();
    o = (c > 1) ? 1 : 0;
    if (nt >= 3) {
	add_tone(c, n, o, a, tone_attn3, cf.f3, r, 2);
	if (r < 0)
	    ramp_stim(c, n, o, -r);
    }
    if (nt >= 2) {
	add_tone(c, n, o, a, tone_attn2, cf.f2, r, 1);
    }
    if (nt >= 1) {
	add_tone(c, n, 0, a, tone_attn1, cf.f1, r, 0);
    }
    if ((r > 0 && r < 4) || (r == 7)) {  
	// ramp: 1=Linear, 2=Cosine, 3=Blackman, 7=Nuttall
	for (i = 0; i < c; i++)
	    ramp_stim(c, n, i, r);
    }
}

static void
compute_noise1(int c, int n, double a)
{
    int i;
    double tpi, ph;

    tpi = 2 * M_PI;
    sf[0] = sf[n] = 1;
    sf[1] = sf[n + 1] = 0;
    for (i = 1; i < n / 2; i++) {
        ph = tpi * unirand();
        sf[2 * i] = (float) cos(ph);
        sf[2 * i + 1] = (float) sin(ph);
    }
    fsst(sf, n);
    a *= sqrt(0.1 * n);
    for (i = 0; i < n; i++)
        st[i * c] = (float) (a * sf[i]);
}

static void
compute_masking_stimulus(int c, int n, double a)
{
    int i, i1, i2, i3, i4, m;
    double dp1, dp2, tpi;
    double t1, t2, d1, d2, r1, w, dpw, mw, p;
    float  a1, a2, aa;
    /* Blackman window coefficients */
    static double wc[3] = {0.42, 0.5, 0.08};

    t1 = tm.ms;
    d1 = tm.md;
    t2 = tm.ms + tm.md + tm.pg;
    d2 = tm.pd;
    a1 = (float) (a / pow(10.0, tone_attn1 / 20));
    a2 = (float) (a / pow(10.0, tone_attn2 / 20));
    tpi = 2 * M_PI;
    dp1 = tpi * cf.f1 / cf.rate;
    dp2 = tpi * cf.f2 / cf.rate;
    i1 = (int) limit(0, 0.001 * t1 * cf.rate, n);
    i2 = (int) limit(0, 0.001 * (t1 + d1) * cf.rate + 1, n);
    i3 = (int) limit(0, 0.001 * t2 * cf.rate, n);
    i4 = (int) limit(0, 0.001 * (t2 + d2) * cf.rate + 1, n);
    for (i = 0; i < n; i++)
        st[i * c] = 0;
    for (i = i1; i < i2; i++)
        st[i * c] = (float) (a1 * cos(dp1 * i));      /* masker tone */
    m = i4 - i3;
    dpw = tpi / m;
    mw = (i3 + i4) / 2.0;
    for (i = i3; i < i4; i++) {
	p = (i - mw) * dpw;
	w = wc[0] + wc[1] * cos(p) + wc[2] * cos(p * 2);
        st[i * c] = (float) (w * a2 * cos(dp2 * (i - mw)));  /* probe tone */
    }

    /* masker ramps */
    r1 = tm.mr;
    i1 = (int) limit(0, 0.001 * t1 * cf.rate, n);
    i2 = (int) limit(0, 0.001 * (t1 + r1) * cf.rate + 1, n) + 1;
    i3 = (int) limit(0, 0.001 * (t1 + d1 - r1) * cf.rate, n);
    i4 = (int) limit(0, 0.001 * (t1 + d1) * cf.rate + 1, n) + 1;
    if (i2 > i1)
        for (i = i1; i < i2; i++) {
	    aa = (float) ((1 + cos(tpi * (i2 - i) / (2.0 * (i2 - i1)))) / 2);
            st[i * c] *= aa;
	}
    if (i4 > i3)
        for (i = i3; i < i4; i++) {
            aa = (float) ((1 + cos(tpi * (i - i3) / (2.0 * (i4 - i3)))) / 2);
	    st[i * c] *= aa;
	}

    /* trigger */
    if (c > 1) {
        t1 = tm.ms + tm.md + tm.pg;
        d1 = tm.td;
        a1 = (float) tm.tv;
        i1 = (int) limit(0, 0.001 * t1 * cf.rate, n);
        i2 = (int) limit(0, 0.001 * (t1 + d1) * cf.rate + 1, n);
        for (i = 0; i < n; i++)
            st[i * c + 1] = 0;
        for (i = i1; i < i2; i++)
            st[i * c + 1] = a1;             /* trigger pulse */
    }
}

void
masking_msg(char *comment, char *msg)
{
    sprintf(comment, "f1=%.0f f2=%.0f Hz | a1=%.0f a2=%.0f dB", 
        cf.f1, cf.f2, tone_attn1, tone_attn2);
    sprintf(msg, "ms=%.0f md=%.0f mr=%.0f pg=%.0f pd=%.0f ms"
        " | tv=%.1f V td=%.1f ms", 
        tm.ms, tm.md, tm.mr, tm.pg, tm.pd, tm.tv, tm.td);
}

void
stim_ft(int c)
{
    int i, j, n;
    float *sfc;

    n = cf.nsamp;
    if (c < 2) {
	memcpy(sf, st, n * sizeof(float));
	fast(sf, n);
    } else {
	for (j = 0; j < c; j++) {
	    sfc = &sf[(n + 2) * j];
	    for (i = 0; i < n; i++) {
		sfc[i] = st[c * i + j];
	    }
	    fast(sfc, n);
	}
    }
}

static void
stim_filt(int c, int m)
{
    double df;
    int i, j, n, i1, i2, ia, ib, ic, iw;
    float *sfc;

    n = cf.nsamp;
    df = cf.rate / n;
    i1 = (int) (cf.f1 / df);		// f1
    i2 = (int) (cf.f2 / df);		// f2
    iw = (int) ((pf.dpbw * 3) / df);	// dpbw * 3
    ic = 2 * i1 - i2;			// 2f1-f2
    ia = (ic - iw) * 2;			// start of zeros
    ib = (ic + iw) * 2;			// end of zeros
    if (c < 2) {
	for (i = ia; i <= ib; i++) {
	    sf[i] = 0;
	}
	fsst(sf, n);
	memcpy(st, sf, n * sizeof(float));
	fast(sf, n);
    } else if (m == 2) {	// special mode: bandpass primaries
        ia = (2 * i1 - i2 + iw) * 2;	// start of zeros
        ib = (2 * i2 - i1 - iw) * 2;	// end of zeros
	for (j = 0; j < c; j++) {
	    sfc = &sf[(n + 2) * j];
	    for (i = 0; i <= ia; i++) {	// zero low freqs
		sfc[i] = 0;
	    }
	    for (i = ib; i < n; i++) {	// zero high freqs
		sfc[i] = 0;
	    }
	    fsst(sfc, n);
	    for (i = 0; i < n; i++) {
		st[c * i + j] = sfc[i];
	    }
	    fast(sfc, n);
	}
    } else if (m < 0) {	    // other special modes:
	if (m == -1) {	    // (1) only filter ch2 around f1
            j = 1;
	    ic = i1;
	} else {	    // (2) only filter ch1 around f2
            j = 0;
	    ic = i2;
	}
        ia = (ic - iw) * 2;	// start of zeros
        ib = (ic + iw) * 2;	// end of zeros
        sfc = &sf[(n + 2) * j];
        for (i = ia; i <= ib; i++) {
	    sfc[i] = 0;
	}
	fsst(sfc, n);
	for (i = 0; i < n; i++) {
	    st[c * i + j] = sfc[i];
	}
        fast(sfc, n);
    } else {
	for (j = 0; j < c; j++) {
	    sfc = &sf[(n + 2) * j];
	    for (i = ia; i <= ib; i++) {
		sfc[i] = 0;
	    }
	    fsst(sfc, n);
	    for (i = 0; i < n; i++) {
		st[c * i + j] = sfc[i];
	    }
	    fast(sfc, n);
	}
    }
}

static void
io_chan()
{
    int i;

    if ((cf.da_mode & 256)) {
	nsch = 1;
    } else {
	nsch = 0;
	for (i = (cf.da_mode % 256); i > 0; i >>= 1) {
	    if ((i & 1))
		nsch++;
	}
    }
    if ((cf.ad_mode == 259) || (cf.ad_mode == 515)) {
	nrch = 2;
	nrcp = 1;
    } else {
	nrch = 0;
	for (i = (cf.ad_mode % 256); i > 0; i >>= 1) {
	    if ((i & 1))
		nrch++;
	}
	nrcp = nrch;
    }
}

int
stim_compute(int stmtyp, char *msg)
{
    double a, vmx, pef, pes;
    int i, c, n, r, m;

    io_chan();
    if (!alloc_mem(cf.nsamp)) {
	if (msg)
	    sprintf(msg, "Can't allocate memory for %d samples.", cf.nsamp);
        return (0);
    }
    if (nsch == 0) {
	return (1);
    }
    vmx = max_volts();
    if (cf.volts > vmx)
	cf.volts = vmx;
    c = nsch;
    a = cf.volts;
    n = cf.nsamp;
    r = cf.rmp_typ;
    pef = (n * cf.pefr) / (cf.rate / 2 / 1000);
    pes = cf.pesl / (10 * log10(2));

    if (stmtyp != 6)	/* zero stimulus, if not user-defined */
	memset(st, 0, n * c * sizeof(float));

    if (stmtyp == 0) {                      /* chirp */
        if (pef > 0) {
            compute_chirp_pe(c, n, a, pef, pes);
        } else {
            compute_chirp(c, n, a);
        }
        cf.ntone = ntwb;
    } else if (stmtyp == 1) {               /* impulse */
        compute_impulse(c, n, a);
        cf.ntone = ntwb;
    } else if (stmtyp == 2) {               /* zero */
        stim_zero(c, n);
        cf.ntone = 1;           
    } else if (stmtyp == 3) {               /* tone */
        compute_tones(c, n, a, r, 1);
        cf.ntone = 1;
    } else if (stmtyp == 4) {
    	if (cf.f3 > 0)
            compute_tones(c, n, a, r, 3);   /* tone pair + suppressor */
        else
            compute_tones(c, n, a, r, 2);   /* tone pair */
        cf.ntone = 2 / c;
    } else if (stmtyp == 5) {
        compute_noise1(c, n, a);            /* noise */
	for (i = 0; i < c; i++)
            ramp_stim(c, n, i, r);
        cf.ntone = ntwb;
    } else if (stmtyp == 6) {               /* user-defined */
        ;
    } else if (stmtyp == 7) {
        compute_masking_stimulus(c, n, a);  /* masking */
        cf.ntone = 1;
    } else if (stmtyp == 8) {
        compute_noise1(c, n, a);            /* double noise */
	m = n / 2;
	for (i = 0; i < m; i++)
	    st[c * (i + m)] = st[c * i];
        cf.ntone = ntwb;
    }
    if (c == 2 && stmtyp != 4 && stmtyp != 6 && stmtyp != 7)
	for (i = 0; i < n; i++)
	    st[2 * i + 1] = st[2 * i];
    stim_ft(c);
    if (cf.dpsf)
	stim_filt(c, cf.dpsf);

    return (1);
}

void
ft_div(float *rf, float *sf)
{
    float sms, rfr, rfi, sfr, sfi;
    int i, n, ii, ir;
    static float eps = (float) 1e-30;

    n = cf.nsamp / 2;
    for (i = 0; i <= n; i++) {
        ir = i * 2;
        ii = i * 2 + 1;
        rfr = rf[ir];
        rfi = rf[ii];
        sfr = sf[ir];
        sfi = sf[ii];
        sms = sfr * sfr + sfi * sfi;
        if (sms < eps)
            sms = eps;
        rf[ir] = (rfr * sfr + rfi * sfi) / sms;
        rf[ii] = (rfi * sfr - rfr * sfi) / sms;
    }
}

void
remove_delay(float *rf)
{
    double dp, tpi;
    float  rfr, rfi, di, dr;
    int i, n, ii, ir;

    tpi = 2 * M_PI;
    dp = cf.delay * tpi * cf.rate / cf.nsamp;
    n = cf.nsamp / 2;
    for (i = 0; i <= n; i++) {
        ir = i * 2;
        ii = i * 2 + 1;
        rfr = rf[ir];
        rfi = rf[ii];
        dr = (float) cos(i * dp);
        di = (float) sin(i * dp);
        rf[ir] = rfr * dr - rfi * di;
        rf[ii] = rfi * dr + rfr * di;
    }
}

void
resp_norm()
{
    ft_div(rf, sf);
    memcpy(rt, rf, (cf.nsamp + 2) * sizeof(float));
    fsst(rt, cf.nsamp);
}

void
resp_ft(int c)
{
    int i, n;
    float *rf2;

    n = cf.nsamp;
    if (c < 2) {
	memcpy(rf, rt, n * sizeof(float));
	fast(rf, n);
        if (dc_remove) {
            rf[0] = rf[n] = 0;
        }
    } else {
	rf2 = &rf[n + 2];
	for (i = 0; i < n; i++) {
	    rf[i] = rt[c * i];
	    rf2[i] = rt[c * i + 1];
	}
	fast(rf, n);
	fast(rf2, n);
        if (dc_remove) {
            rf[0] = rf[n] = 0;
            rf2[0] = rf2[n] = 0;
        }
    }
}

void
resp_bp(int c)
{
    double df;
    float *rf2;
    int i, n, i1, i2;

    n = cf.nsamp;
    df = cf.rate / n;
    i1 = (int) (2 * cf.f1 / df);
    i2 = (int) (2 * cf.f2 / df);
    if (c < 2) {
	for (i = 0; i < i1; i++) {
            rf[i] = 0;
	}
	for (i = i2; i <= n; i++) {
            rf[i] = 0;
	}
        fsst(rf, n);
	for (i = 0; i < n; i++) {
	    rt[i] = rf[i];
	}
	fast(rf, n);
    } else {
	rf2 = &rf[n + 2];
	for (i = 0; i < i1; i++) {
            rf[i] = 0;
            rf2[i] = 0;
	}
	for (i = 0; i < i1; i++) {
            rf[i] = 0;
            rf2[i] = 0;
	}
        fsst(rf, n);
	fsst(rf2, n);
	for (i = 0; i < n; i++) {
	    rt[2 * i] = rf[i];
	    rt[2 * i + 1] = rf2[i];
	}
	fast(rf, n);
	fast(rf2, n);
    }
}

static void
resp_scale(double sc)
{
    int i, n;
    
    n = cf.nsamp * nrch;
    for (i = 0; i < n; i++)
        rt[i] *= (float) sc;
}

void
resp_rms(double *rms, double *gd)
{
    double isq, ssq, vsq, v;
    int i, j, n;
    
    n = cf.nsamp;
    for (j = 0; j < nrch; j++) {
	ssq = 0;
	isq = 0;
	for (i = 0; i < n; i++) {
	    v = rt[i * nrch + j];
	    vsq = v * v;
	    ssq += vsq;
	    isq += vsq * i;
	}
        rms[j] = (n > 0) ? sqrt(ssq / n) : 0;
        gd[j] = (n > 0) ? ((isq / ssq) * (1000.0 / cf.rate)) : 0;
    }
}

void
resp_vpu()
{
    double vpu;

    if (cf.ntone > 0) {
        vpu = 2.0 / cf.nsamp;
    } else {
        vpu = 1;
    }
    pf.dbvpu = -20 * log10(vpu);
}

void
resp_compute()
{
    int i, j, inc, fmod = 0;
    float *rfi, *sfi;

    resp_vpu();
    if (cf.ntone > 0) {
        resp_ft(nrch);
    } else {
        resp_scale(cf.volts);
        resp_ft(nrch);
	inc = cf.nsamp + 2;
        if (cf.ntone == 0 && nsch > 0) {
	    if (cf.ad_mode < 3) {		// single out/in mode
		ft_div(rf, sf);
		remove_delay(rf);
		memcpy(rt, rf, inc * sizeof(float));
		fsst(rt, cf.nsamp);
	    } else if (cf.ad_mode < 256) {	// multiple out/in mode
		for (j = 0; j < nrch; j++) {
		    rfi = rf + inc * j;
		    sfi = sf + inc * ((j < nsch) ? j : (nsch - 1));
		    ft_div(rfi, sfi);
		    remove_delay(rfi);
		    fsst(rfi, cf.nsamp);
		    for (i = 0; i < cf.nsamp; i++) {
			rt[nrch * i + j] = rfi[i];
		    }
		    fast(rfi, cf.nsamp);
		}
	    } else if (cf.ad_mode == 259) {	// single L/R mode
		rfi = rf + inc;
		ft_div(rf, rfi);
		remove_delay(rf);
		memcpy(rt, rf, inc * sizeof(float));
		fsst(rt, cf.nsamp);
	    } else if (cf.ad_mode == 515) {	// single R/L mode
		rfi = rf + inc;
		ft_div(rfi, rf);
		remove_delay(rfi);
		memcpy(rf, rfi, inc * sizeof(float));
		memcpy(rt, rf, inc * sizeof(float));
		fsst(rt, cf.nsamp);
	    }
        }
    }
    if (dc_remove || fmod) {
    }
    resp_computed++;
}

int
is_resp()
{
    return (resp_computed);
}

static void
set_output(int da_mode, float *ob)
{
    int hc = 2;
    static float *obp[2];
    static int obi[2];

    if (da_mode == 0) {
	obp[0] = obp[1] = NULL;
	obi[0] = obi[1] = 0;
    } else if (da_mode == 1) {
	obp[0] = ob;
	obp[1] = NULL;
	obi[0] = 1;
	obi[1] = 0;
    } else if (da_mode == 2) {
	obp[0] = NULL;
	obp[1] = ob;
	obi[0] = 0;
	obi[1] = 1;
    } else if (da_mode == 3) {
	obp[0] = ob;
	obp[1] = ob + 1;
	obi[0] = obi[1] = 2;
    } else if (da_mode == 4) {
	obp[0] = obp[1] = ob;
	obi[0] = obi[1] = 1;
    }
    sio_set_output(hc, 1, obi, obp);
}

static void
set_input(int ad_mode, float *ib, int inpflg, int *avcp)
{
    float **ibp, **abp;
    int hc = 2;
    static float *tbp[2];
    static int ibi[2];

    if (ad_mode == 0) {
	tbp[0] = tbp[1] = NULL;
	ibi[0] = ibi[1] = 0;
    } else if (ad_mode == 1) {
	tbp[0] = ib;
	tbp[1] = NULL;
	ibi[0] = 1;
	ibi[1] = 0;
    } else if (ad_mode == 2) {
	tbp[0] = NULL;
	tbp[1] = ib;
	ibi[0] = 0;
	ibi[1] = 1;
    } else if (ad_mode >= 3) {
	tbp[0] = ib;
	tbp[1] = ib + 1;
	ibi[0] = ibi[1] = 2;
    }
    ibp = inpflg ? tbp : NULL;
    abp = inpflg ? NULL : tbp;
    sio_set_input(hc, 1, ibi, ibp);
    sio_set_average(abp, avcp);
}

void
play_stim(int i, int r, int s, int *a)
{
    int     ngap, nskp, dam;

    if (!sio_present)
        return;
    cf.rate = sio_set_rate(cf.rate);
    dam = s ? cf.da_mode : 0;
    if (!i) {
        if (r && (reject_mode || wtav_mode)) {
            rchk = 1;
            rchk = (alloc_mem(cf.nsamp) > 0);
        }
	ngap = r ? nint(cf.gap * cf.rate) : 0;
        sio_set_size(cf.nsamp, ngap, cf.pregap);
        sio_set_escape(chk_escape);
        set_output(dam, st);
        flush_eventq();
    }
    gr_prcl("~~~");
    if (!r || cf.gap < 0.1) {
        set_input(cf.ad_mode, rt, 0, a);
	nskp = dam ? nint(swpskp * cf.nsamp) : 0;
	sio_io(nskp, cf.nav, 0, 0);
    } else {
	init_chkrsp();
        set_input(cf.ad_mode, rt, rchk, a);
	sio_io_chk(chk_resp);
    }
    rchk = 0;
}

void
play_zero(int nav)
{
    int cnav;

    cnav = cf.nav;		// save current # averages
    stim_compute(2, NULL);	// zero stimulus
    cf.nav = nav;
    play_stim(0, 1, 1, NULL);
    cf.nav = cnav;
}

int
spectral_average(int stim)
{
    double rfr, rfi, *p1, *p2, *p3, p1m, p2m, mg1, mg2;
    int i, j, n, b = 0, a;

    n = cf.nsamp / 2 + 1;
    p1 = (double *) calloc(n * 3, sizeof(double));
    if (p1 == NULL)
        return (0);
    p2 = p1 + n;
    p3 = p2 + n;
    for (j = 0; j < nsab; j++) {
        play_stim(j, 1, stim, &a);
	if (a < cf.nav)
	    break;
        resp_compute();
        for (i = 0; i < n; i++) {
            rfr = rf[2 * i];
            rfi = rf[2 * i + 1];
            p1[i] += rfr;
            p2[i] += rfi;
            p3[i] += rfr * rfr + rfi * rfi;
        }
        b++;
    }
    if (b > 0) {
        for (i = 0; i < n; i++) {
            p1m = p1[i] / b;
            p2m = p2[i] / b;
            mg1 = sqrt(p1m * p1m + p2m * p2m);
            mg2 = sqrt(p3[i] / b);
            if (mg1 > 0) {
                rf[2 * i] = (float) (mg2 * p1m / mg1);
                rf[2 * i + 1] = (float) (mg2 * p2m / mg1);
            } else {
                rf[2 * i] = (float) mg2;
                rf[2 * i + 1] = 0;
            }
        }
    }
    if (dc_remove) {
	rf[0] = rf[cf.nsamp] = 0;
    }
    free(p1);
    memcpy(rt, rf, (cf.nsamp + 2) * sizeof(float));
    fsst(rt, cf.nsamp);

    return (b);
}

int
chk_overwrite(char *fn)
{
    char s[80];
    
    if (!cf.ovrwrt) {
	if (access(fn, 0) == 0) {
	    sprintf(s, "Overwrite existing file (%s) [y/N] ? ", fn);
	    gr_getline(s, s, 80);
	    if (*s != 'y' && *s != 'Y')
		return (0);
        }
    }
    return (1);
}

void
sysres(int ac, char *av[])
{
    char cmd[128];

    defpar();
    while (ac > 1) {
        if (av[1][0] == '-') {
            if (av[1][1] == 'd') {
                debug = 1;
            } else if (av[1][1] == 'l') {
                logflg = 1;
            }
        } else {
            if (is_mat(av[1])) {
                gr_pbstr("pr");
                gr_pbstr("cs");
                sprintf(cmd, "rd=%s", av[1]);
                gr_pbstr(cmd);
            } else if (is_ascii(av[1])) {
                sprintf(cmd, "rl=%s", av[1]);
                gr_pbstr(cmd);
            }
        }
        ac--;
        av++;
    }
    sio_init_cardinfo();
    printf("%s\n", VER);
    stime = gr_etime();
    gui();
}

#ifndef WIN32
int
main(int ac, char *av[])
{
#ifdef linux
    if (strcmp(getenv("TERM"), "linux") == 0)
	gr_mode = 0;
    else if (getenv("DISPLAY") != NULL)
	gr_mode = 2;
    else
	gr_mode = 1;
#endif /* linux */

    sysres(ac, av);

    return(0);
}
#endif /* WIN32 */
