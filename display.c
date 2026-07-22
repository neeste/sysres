/* display.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "display.h"

#define limit(mn,aa,mx) (((aa)<(mn))?(mn):((aa)>(mx))?(mx):(aa))

int dp_nc();
int nint(double);
void dp_fc(int, int, int *);
void dp_exp_fit(float *, PLTFMT *);
void get_crn_line(char *);
void gr_clrs();
void gr_clrb(int, int, int, int);
void gr_line(int, int, int, int, int);
void gr_update();
void gr_set_clip(int, int, int, int);
void gr_text(int, int, char *);
void gr_tpcl(int, int, int, int, int);

extern double splref;
extern int c0, c1, cw, ch, xpix, ypix;
extern int bx, by, bw, bh, ytop, ybot, ycmd;
extern FILE *mfp;

FRAME fr = {0};

static double tpi = 2 * 3.1415927;
static double dsp_x0 = 0, dsp_dx = 0, dsp_nf = 0;
static int fft_ci = 0;
static int fft_color[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
static int fft_ic = 0;
static int fft_nci = sizeof(fft_color) / sizeof(fft_color[0]);
static int dsp_i1 = 0, dsp_i2 = 0;
static int eml = 0;	/* extra message lines */
static int top_clr = 0;
static int mid_clr = 0;
static PGMCFG last_cf;
static PLTFMT last_pf;

void
adjust_layout(int n)
{
    eml = n;
    bx = 8 * cw;
    by = ypix - 4 * ch - eml * ch;
    bw = xpix - bx - 2 * cw;
    bh = by - 4 * ch;
    ytop = 3 * ch + ch / 2;
    ybot = by + ch + ch / 2;
    ycmd = ybot + ch + eml * ch;
}


void
clr_top()
{
    if (!top_clr)
	gr_clrb(0, 0, xpix - 1,  ytop - 1);
    top_clr = 1;
}

void
clr_mid()
{
    if (!mid_clr)
	gr_clrb(0, ytop, xpix - 1,  ybot - 1);
    mid_clr = 1;
    fft_ci = fft_ic;
    fr.nframe = 0;
}

void
clr_all()
{
    gr_clrs();
    top_clr = 1;
    mid_clr = 1;
    fft_ci = fft_ic;
    fr.nframe = 0;
}

void
set_ic(int c)
{
    fft_ic = c % fft_nci;
}

void
prn_crn()
{
    int x, y;
    char s[80];
    time_t t;

    y = cw / 4;
    x = xpix - 12 * cw;
    get_crn_line(s);
    gr_text(x, y, s);
    y += ch;
    (void) time(&t);
    strftime(s, 20, "%m/%d/%Y", localtime(&t));
    gr_text(x, y, s);
    y += ch;
    strftime(s, 20, "%H:%M:%S", localtime(&t));
    gr_text(x, y, s);
    top_clr = 0;
}

void
prn_msg(char *s)
{
    gr_clrb(0, ybot, xpix - 1,  ybot + ch + ch * eml);
    gr_text(cw, ybot, s);
    if (mfp) {
    	fprintf(mfp, "%s\n", s);
	fflush(mfp);
    }
    gr_update();
}

void
prn_msg2(char *s)
{
    if (eml > 0) {
	gr_text(cw, ybot + ch, s);
    }
    if (mfp) {
    	fprintf(mfp, "%s\n", s);
	fflush(mfp);
    }
    gr_update();
}

void
prn_top(PGMCFG *cf, char *comment)
{
    char s[80], *gu, dam[8], adm[8], *vu;
    double g, v;
    int x, y, stm, nd;
    static char *ms[] = {"N", "L", "R", "L&R"};
    static char *stnam[] = {
	"chirp", "impulse", "zero", "tone", "2tones", "noise", "user",
	"masking", "dbl.noise", "???"
    };
    static int nstm = 9;

    clr_top();
    if (cf->da_mode < 0 || cf->da_mode > 511) {
	strcpy(dam, "?");
    } else if (cf->da_mode == 259) {
	strcpy(dam, "L=R");
    } else if (cf->da_mode < 4) {
	strcpy(dam, ms[cf->da_mode]);
    } else {
	sprintf(dam, "%d", cf->da_mode);
    }
    if (cf->ad_mode < 0 || cf->ad_mode > 515) {
	strcpy(adm, "?");
    } else if (cf->ad_mode == 259) {
	strcpy(adm, "L/R");
    } else if (cf->ad_mode == 515) {
	strcpy(adm, "R/L");
    } else if (cf->ad_mode < 4) {
	strcpy(adm, ms[cf->ad_mode]);
    } else {
	sprintf(adm, "%d", cf->ad_mode);
    }
    v = cf->volts;
    if (v < 0 || v > 1e9)   // sanity check
	return;
    if (v < 1e-6) {
	v *= 1e9;
	vu = "nV";
    } else if (v < 1e-3) {
	v *= 1e6;
	vu = "uV";
    } else if (v < 1) {
	v *= 1e3;
	vu = "mV";
    } else {
	vu = "V";
    }
    if (v < 10)
	nd = 3;
    else if (v < 100)
	nd = 2;
    else
	nd = 1;
    stm = (cf->stm_typ < 0 || cf->stm_typ >= nstm) ? nstm : cf->stm_typ;
    sprintf(s, "amplitude=%.*f%s  d/a_mode=%s  a/d_mode=%s  stimulus=%s   ", 
        nd, v, vu, dam, adm, stnam[stm]);
    y = cw / 4;
    x = cw / 2;
    gr_text(x, y, s);
    y += ch;
    g = cf->gap;
    if (g < 0 || g > 1e9)   // sanity check
	return;
    if (g < 1) {
	g *= 1000;
	gu = "ms";
    } else {
	gu = "s";
    }
    sprintf(s, "#averages=%d  #samples=%d  rate=%.0fkHz  gap=%.4g%s   ", 
        cf->nav, cf->nsamp, cf->rate / 1000, g, gu);
    gr_text(x, y, s);
    y += ch;
    gr_text(x, y, comment);
    prn_crn();
}

void
reset_clip()
{
    gr_set_clip(0, 0, xpix - 1, ypix - 1);
}

void
display_box(int x1, int y1, int w, int h)
{
    int x2, y2;

    x2 = x1 + w;
    y2 = y1 - h;
    reset_clip();
    gr_line(x1, y1, x2, y1, c1);
    gr_line(x2, y1, x2, y2, c1);
    gr_line(x2, y2, x1, y2, c1);
    gr_line(x1, y2, x1, y1, c1);
    mid_clr = 0;
}

static int
numdec(double v1, double v2)
{
    int nd;

    if (v1 < -99.9 || v2 > 99.9) {
        nd = 1;
    } else if (v1 < -9.99 || v2 > 9.99) {
        nd = 2;
    } else {
        nd = 3;
    }
    return (nd);
}

static double
time_label(double t1, double t2)
{
    char s[20], *units;
    double sc;
    int nd;

    if (t2 > 0.999) {
        sc = 1;
        units = "time (sec)";
    } else {
        sc = 1e3;
        units = "time (msec)";
    }
    nd = numdec(t1 * sc, t2 * sc);
    gr_text(fr.bx + (fr.bw - 10 * cw) / 2, fr.cy, units);
    sprintf(s, "%.*f", nd, t1 * sc);
    gr_text(fr.bx - 4, fr.cy, s);
    sprintf(s, "%6.*f", nd, t2 * sc);
    gr_text(fr.bx + fr.bw + cw / 2 - 6 * cw, fr.cy, s);

    return (sc);
}

static void
time_axis(double t1, double t2, int ay, int ah)
{
    double dx, tmn, tmx, sc, ts, x0;
    int dy, xx, i, j, i1, i2, aa;
    static double s[3] = {0.5, 0.75, 1.0};
    static int m[3] = {1, 5, 10};

    sc = time_label(t1, t2);
    tmn = t1 * sc;
    tmx = t2 * sc;
    if (tmx <= tmn)
    	return;
    while (fr.bw / (tmx - tmn) < cw * 2) {
        tmx /= 10;
        tmn /= 10;
    }
    while (fr.bw / (tmx - tmn) > cw * 5) {
        tmx *= 10;
        tmn *= 10;
    }
    aa = ay - ah;
    ts = fr.td ? -cw : cw;
    x0 = fr.bx - fr.bw * tmn / (tmx - tmn);
    for (j = 0; j < 3; j++) {
        dx = m[j] * fr.bw / (tmx - tmn);
        if (dx > cw / 2) {
            dy = (int) (ts * s[j]);
            i1 = (int) floor(tmn / m[j]);
            i2 = (int) ceil(tmx / m[j]);
            for (i = i1; i <= i2; i++) {
                xx = nint(x0 + i * dx);
                if (xx >= fr.bx && xx <= fr.bx + fr.bw) {
                    gr_line(xx, ay, xx, ay - dy, c1);
                    gr_line(xx, aa + dy, xx, aa, c1);
                }
            }
        }
    }
}

static double
volt_label(double v1, double v2, int unit_type)
{
    char s[20], *pref, *unit;
    double sc;
    int nd, h;

    h = ch / 2;
    if (v1 < -0.999 || v2 > 0.999) {
        sc = 1;
        pref = "";
    } else if (v1 < -0.999e-3 || v2 > 0.999e-3) {
        sc = 1e3;
        pref = "milli-";
    } else {
        sc = 1e6;
        pref = "micro-";
    }
    if (unit_type == 3) {
	unit = "vpp ";
    } if (unit_type == 2) {
	unit = " Pa ";
    } else if (unit_type == 1) {
	unit = "volt";
    } else {
	unit = "unit";
    }
    nd = numdec(v1 * sc, v2 * sc);
    sprintf(s, "%6.*f", nd, v1 * sc);
    gr_text(fr.cx, fr.by - h, s);
    sprintf(s, "%6.*f", nd, v2 * sc);
    gr_text(fr.cx, fr.by - fr.bh - h, s);
    gr_text(fr.cx, fr.by - fr.bh / 2 - h, pref);
    gr_text(fr.cx, fr.by - fr.bh / 2 + h, unit);

    return (sc);
}

static void
volt_axis(double v1, double v2, int ay, int ah)
{
    double dx, dy, vmn, vmx, sc, ts;
    int xx, yy, i, j, i1, i2, y0;
    static double s[2] = {0.5, 1.0};
    static int m[2] = {1, 10};

    sc = volt_label(v1, v2, fr.unit_type);
    vmn = v1 * sc;
    vmx = v2 * sc;
    while (ah / (vmx - vmn) < cw / 2) {
        vmx /= 10;
        vmn /= 10;
    }
    while (ah / (vmx - vmn) > cw * 5) {
        vmx *= 10;
        vmn *= 10;
    }
    ts = fr.td ? -cw : cw;
    y0 = (int) (ay + ah * vmn / (vmx - vmn));
    for (j = 0; j < 2; j++) {
        dy = m[j] * ah / (vmx - vmn);
        dx = ts * s[j];
        i1 = (int) floor(vmn / m[j]);
        i2 = (int) ceil(vmx / m[j]);
        for (i = i1; i <= i2; i++) {
            yy = y0 - nint(i * dy);
            if (yy <= ay && yy >= ay - ah) {
		xx = (int) (fr.bx + dx);
                gr_line(fr.bx, yy, xx, yy, c1);
		xx = (int) (fr.bx + fr.bw - dx);
                gr_line(fr.bx + fr.bw, yy, xx, yy, c1);
            }
        }
    }
    gr_line(fr.bx, ay, fr.bx, ay - ah, c1);
    gr_line(fr.bx + fr.bw, ay, fr.bx + fr.bw, ay - ah, c1);
}

static void
wave_frame(PLTFMT *pf, double a, double tmx)
{
    int tax;
    double lga, lgd;
    static double last_lga = 0;
    static double last_tmx = 0;

    lga = log10(a);
    lgd = lga - last_lga;
    last_lga = lga;
    tax = (tmx == last_tmx);
    last_tmx = tmx;
    if (pf->clear_screen == 0
	&& ((!pf->zoom && tax)
	    ||	(pf->zoom
	    	 && pf->zt1 == last_pf.zt1
	         && pf->zt2 == last_pf.zt2))
	&& pf->zoom == last_pf.zoom
	&& pf->tic_dir == last_pf.tic_dir
	&& pf->eml == last_pf.eml
	&& lgd < 0.1
	&& lgd > -0.3
	) {
        return;
    }
    clr_mid();
    fr.wave = 1;
    fr.bx = bx;
    fr.by = by;
    fr.bw = bw;
    fr.bh = bh;
    fr.cx = bx - 6 * cw - cw / 2;
    fr.cy = by + cw / 4;
    if (fr.td) {
        fr.bx += cw;
        fr.by -= cw;
        fr.bw -= cw + cw / 4;
        fr.bh -= cw + cw / 4;
    }
    display_box(fr.bx, fr.by, fr.bw, fr.bh);
}

static void
draw_seg(int x1, int y1, int x2, int y2, int cc)
{
    static int ss = 0, sx = 0, sy1 = 0, sy2 = 0, sc = 0;

    if (cc == -1) {
	ss = sx = sy1 = sy2 = sc = 0;
    } else if (cc == -2) {
	if (ss) {
            gr_tpcl(sx, sy1, sx, sy2, sc);
	    ss = 0;
	}
    } else if (x1 == sx && x2 == sx) {
	if (!ss) {
	    sc = cc;
	    ss = 1;
	}
        if (sy1 > y2)
	    sy1 = y2;
	if (sy2 < y2)
	    sy2 = y2;
    } else {
	if (ss) {
            gr_tpcl(sx, sy1, sx, sy2, sc);
	    ss = 0;
	}
        gr_tpcl(x1, y1, x2, y2, cc);
	if (!ss) {
	    sx = x2;
	    sy1 = sy2 = y1;
	}
    }
}

int
display_wave(float *w, int n, int c, double r, PLTFMT *pf)
{
    double v1, v2, t1, t2, tmn, tmx;
    double mx, mn, a;
    int i, j, x = 0, y = 0, xx, yy, cc, i1, i2;
    static float eps = (float) 1e-6;

    prn_crn();
    pf->time_axis = 1;
    fr.td = pf->tic_dir;
    if (w == NULL || n <= 0 || r <= 0)
        return (0);
    mx = mn = 0;
    for (i = 0; i < n * c; i++) {
        if (mx < w[i])
            mx = w[i];
        if (mn > w[i])
            mn = w[i];
    }
    if (_isnan(mn) || _isnan(mx)) {
	return (0);
    }
    if (mx < mn + eps)
        mx = mn + eps;
    v1 = fr.vmn = mn;
    v2 = fr.vmx = mx;
    a = fr.bh / (mx - mn);
    t1 = 0;
    t2 = n / r;
    wave_frame(pf, a, t2);
    last_pf = *pf;
    if (pf->zoom) {
        tmn = t1;
        tmx = t2;
        t1 = limit(tmn, pf->zt1, tmx);
        t2 = limit(tmn, pf->zt2, tmx);
        if (t1 >= t2) {		/* If zoom values are not good, */
            t1 = tmn;           /* then restore default values. */
            t2 = tmx;
        }
    }
    i1 = (int) floor(t1 * r);
    i2 = (int) ceil(t2 * r);
    dsp_dx = fr.bw / ((t2 - t1) * r);
    dsp_x0 = fr.bx - t1 * r * dsp_dx;
    gr_set_clip(fr.bx, fr.by - fr.bh, fr.bx + fr.bw, fr.by);
    for (j = 0; j < c; j++) {
	cc = fft_color[fft_ci];
        draw_seg(0, 0, 0, 0, -1);
	for (i = i1; i < i2; i++) {
	    xx = nint(dsp_x0 + i * dsp_dx);
	    yy = fr.by - nint(a * (w[i * c + j] - mn));
	    if (i > i1)
                draw_seg(x, y, xx, yy, cc);
	    x = xx;
	    y = yy;
	}
        draw_seg(0, 0, 0, 0, -2);
	fft_ci++;
	fft_ci %= fft_nci;
    }
    fr.tmn = t1;
    fr.tmx = t2;
    display_box(fr.bx, fr.by, fr.bw, fr.bh);
    volt_axis(v1, v2, fr.by, fr.bh);
    time_axis(t1, t2, fr.by, fr.bh);

    return(1);
}

void
display_rte(float *w, int n, int c, double r, PLTFMT *pf)
{
    char s[20], *units;
    double t1, t2, tmn, tmx, v, nrg, nrgtot, nrgdb, nrgmn, sc, dy;
    double dbmx = 0, dbmn = -90, a, av, eps = 1e-30;
    int i, j, k, i1, i2, x, y, xx, yy, h, cc, dx, yi, nd;

    prn_crn();
    display_box(fr.bx, fr.by, fr.bw, fr.bh);
    if (w == NULL || n <= 0 || r <= 0)
        return;
    clr_mid();
    nrgmn = pow(10.0, dbmn / 10);
    h = ch / 2;
    a = fr.bh / (dbmx - dbmn);
    t1 = 0;
    t2 = n / r;
    if (pf->zoom) {
        tmn = t1;
        tmx = t2;
        t1 = limit(tmn, pf->zt1, tmx);
        t2 = limit(tmn, pf->zt2, tmx);
        if (t1 >= t2) {     /* If zoom values are not good, */
            t1 = tmn;           /* then restore default values. */
            t2 = tmx;
        }
    }
    i1 = (int) floor(t1 * r);
    i2 = (int) ceil(t2 * r);
    dsp_dx = fr.bw / ((t2 - t1) * r);
    dsp_x0 = fr.bx - t1 * r * dsp_dx;
    gr_set_clip(fr.bx, fr.by - fr.bh, fr.bx + fr.bw, fr.by);
    for (j = 0; j < c; j++) {
	av = 0;
	k = 0;
	for (i = (n - n / 4); i < n; i++) {   /* average over last fourth only */
	    v = w[i * c + j];
	    av += v * v;
	    k++;
	}
        av = (k > 0) ? (av / k) / 2 : 0;
	nrg = 0;
	for (i = n - 1; i >= 0; i--) {
	    v = w[i * c + j];
	    nrg += (v * v - av);
	}
	nrgtot = nrg;
	if (nrgtot < eps)
	    nrgtot = eps;
	nrg = 0;
	x = fr.bx + fr.bw;
	y = fr.by;
	cc = fft_color[fft_ci];
        draw_seg(0, 0, 0, 0, -1);
	for (i = n - 1; i >= 0; i--) {
	    v = w[i * c + j];
	    nrg += (v * v - av) / nrgtot;
	    nrgdb = (nrg < nrgmn) ? dbmn : 10 * log10(nrg);
	    xx = nint(dsp_x0 + i * dsp_dx);
	    yy = fr.by - nint(a * (nrgdb - dbmn));
	    if (i >= i1 && i <= i2)
		draw_seg(x, y, xx, yy, cc);
	    x = xx;
	    y = yy;
	}
        draw_seg(0, 0, 0, 0, -2);
	fft_ci++;
	fft_ci %= fft_nci;
    }
    display_box(fr.bx, fr.by, fr.bw, fr.bh);
    yi = nint((dbmx - dbmn) / 10);
    dx = cw / 2;
    dy = fr.bh / (float) yi;
    draw_seg(0, 0, 0, 0, -1);
    for (i = 1; i < yi; i++) {
    	yy = fr.by - nint(i * dy);
        draw_seg(fr.bx, yy, fr.bx + dx, yy, c1);
        draw_seg(fr.bx + fr.bw - dx, yy, fr.bx + fr.bw, yy, c1);
    }
    draw_seg(0, 0, 0, 0, -2);
    sprintf(s, "%6.0f", dbmn);
    gr_text(cw, fr.by - h, s);
    sprintf(s, "%6.0f", dbmx);
    gr_text(cw, fr.by - fr.bh - h, s);
    gr_text(cw, fr.by - fr.bh / 2 - h, "   dB");
    if (t2 > 0.999) {
        sc = 1;
        units = "time (sec)";
    } else {
        sc = 1e3;
        units = "time (msec)";
    }
    nd = numdec(t1 * sc, t2 * sc);
    gr_text(fr.bx + (fr.bw - 10 * cw) / 2, fr.cy, units);
    sprintf(s, "%.*f", nd, t1 * sc);
    gr_text(fr.bx - 4, fr.cy, s);
    sprintf(s, "%6.*f", nd, t2 * sc);
    gr_text(fr.bx + fr.bw + cw / 2 - 6 * cw, fr.cy, s);
    time_axis(t1 * sc, t2 * sc, fr.by, fr.bh);
}

static void
freq_label(double fmn, double fmx)
{
    char    s[80];

    if (fmx < 1) {
        gr_text(fr.bx + (fr.bw - 15 * cw) / 2, fr.cy, "frequency (Hz)");
	sprintf(s, "%-6.4g", fmn * 1000);
	gr_text(fr.bx - cw / 2, fr.cy, s);
	sprintf(s, "%6.4g", fmx * 1000);
	gr_text(fr.bx + fr.bw + cw / 2 - 6 * cw, fr.cy, s);
    } else {
        gr_text(fr.bx + (fr.bw - 15 * cw) / 2, fr.cy, "frequency (kHz)");
	sprintf(s, "%-6.4g", fmn);
	gr_text(fr.bx - cw / 2, fr.cy, s);
	sprintf(s, "%6.4g", fmx);
	gr_text(fr.bx + fr.bw + cw / 2 - 6 * cw, fr.cy, s);
    }
}

static void
freq_axis(double fmn, double fmx, int ay, int ah, int logax)
{
    double dx, dy, df, lb, x0, ts, fs, fe;
    int xx, yy, i, j, y1, y2;

    freq_label(fmn, fmx);
    y1 = ay;
    y2 = ay - ah;
    ts = fr.td ? -cw : cw;
    if (logax) {
	if (fmn < 1e-9)
	    fmn = 1e-9;
        lb = log(fmx / fmn);
        dy = (2 * ts) / 3;
	fs = pow(10.0, floor(log10(fmn)));
	fe = fmx * 1.01;
        for (df = fs; df < fe; df *= 10) {
            for (i = 1; i < 9; i++) {
                dx = nint(fr.bw * log(i * df / fmn) / lb);
                if (dx >= 0 && dx <= fr.bw) {
                    xx = (int) (fr.bx + dx);
                    yy = (int) ((i == 1) ? 2 * dy : dy);
                    gr_line(xx, y1, xx, y1 - yy, c1);
                    gr_line(xx, y2 + yy, xx, y2, c1);
                }
            }
        }
    } else {
        for (j = 0; j < 5; j++) {
            df = pow(10.0, 1.0 - j);
            dx = fr.bw * df / (fmx - fmn);
            x0 = fr.bw * fmn / (fmx - fmn);
            if (dx <= cw / 2 || (fr.bw / dx) > 200)
                break;
            dy = (ts * (5 - j)) / 4;
            for (i = (int) ceil(fmn / df); i <= (fmx / df); i++) {
                xx = fr.bx + nint(i * dx - x0);
		yy = (int) (y1 - dy);
                gr_line(xx, y1, xx, yy, c1);
		yy = (int) (y2 + dy);
                gr_line(xx, yy, xx, y2, c1);
            }
        }
    }
    gr_line(fr.bx, y1, fr.bx + fr.bw, y1, c1);
    gr_line(fr.bx, y2, fr.bx + fr.bw, y2, c1);
}

static void
db_axis(int ay, int ah, double ymx, double ymn)
{
    double dy, ts;
    int yy, i, imn, imx, tx;

    imx = nint(ymx);
    imn = nint(ymn);
    dy = ah / (double) (imx - imn);
    ts = fr.td ? -cw : cw;
    for (i = imn; i <= imx; i++) {
    	if ((i % 10) == 0) {
            tx = (int) ts;
    	} else if ((i % 5) == 0 && dy > cw / 4) {
            tx = (int) ((3 * ts) / 4);
    	} else if (dy > cw) {
            tx = (int) (ts / 2);
    	} else {
            tx = 0;
    	}
    	if (tx != 0) {
            yy = ay - nint((i - imn) * dy);
            gr_line(fr.bx, yy, fr.bx + tx, yy, c1);
            gr_line(fr.bx + fr.bw - tx, yy, fr.bx + fr.bw, yy, c1);
        }
    }
}

static void
db_label(int y, int h, double ymx, double ymn, int unit_type)
{
    char s[20], *labl;
    double dbsf;
    int hch;

    if (unit_type == 3) {
        labl = "dBvpp";
        dbsf = 0;
    } else if (unit_type == 2) {
        labl = "dBSPL";
        dbsf = 0;
    } else if (unit_type == 1) {
        if (((ymx + ymn) / 2) < -120) {
            labl = " dBuV";
            dbsf = 120;
        } else if (((ymx + ymn) / 2) < -60) {
            labl = " dBmV";
            dbsf = 60;
        } else {
            labl = " dBV";
            dbsf = 0;
        }
    } else {
        labl = " dB";
        dbsf = 0;
    }
    hch = ch / 2;
    gr_text(fr.cx, y - h / 2 - hch, labl);
    sprintf(s, "%6.0f", ymn + dbsf);
    gr_text(fr.cx, y - hch, s);
    sprintf(s, "%6.0f", ymx + dbsf);
    gr_text(fr.cx, y - h - hch, s);
}

static void
ph_axis(int ay, int ah, double ymx, double ymn)
{
    double dy, y, y0, y1, y2;
    int    i, dx, xx, yy;
    static double ti[] = {1, 2, 10, 20, 100, 200, 1000};
    static int ni = sizeof(ti) / sizeof(ti[0]);

    for (i = 0; i < ni ; i++) {
	y1 = ceil(ymn * ti[i]);
	y2 = floor(ymx * ti[i]);
	if ((y2 - y1) >= 20)
            break;
        dx = fr.td ? i - cw : cw - i;
        dy = ah / ((ymx - ymn) * ti[i]);
        y0 = ymn * ti[i];
        for (y = y1; y <= y2; y += 1) {
	    yy = ay - nint((y - y0) * dy);
	    xx = fr.bx + dx;
	    gr_line(fr.bx, yy, xx, yy, c1);
	    xx = fr.bx + fr.bw - dx;
	    gr_line(xx, yy, fr.bx + fr.bw, yy, c1);
	}
    }
}

static void
ph_label(int y, int h, double ymx, double ymn, int dlyflg, int taxflg)
{
    char    s[80], *lab1, *lab2;
    int     hch, dp;
    
    if (dlyflg) {
        if (taxflg) {
	    lab1 = "freq.";
	    lab2 = "(Hz)";
	} else {
	    lab1 = "delay";
	    lab2 = "(ms)";
	}
    } else {
	lab1 = "phase";
	lab2 = "(cyc)";
    }
    hch = ch / 2;
    if (ymx > 1000 || ymn < -100) {
        dp = 0;
    } else if (ymx > 100 || ymn < -10) {
        dp = 1;
    } else if (ymx > 10 || ymn < -1) {
        dp = 2;
    } else {
        dp = 3;
    }
    sprintf(s, "%6.*f", dp, ymn);
    gr_text(fr.cx, y - hch, s);
    sprintf(s, "%6.*f", dp, ymx);
    gr_text(fr.cx, y - h - hch, s);
    gr_text(fr.cx, y - h / 2 - hch, lab1);
    gr_text(fr.cx, y - h / 2 + hch, lab2);
    fr.phdp = dp;
}

static void
spec_frame_1(PLTFMT *pf)
{
    double ymn, ymx;

    display_box(fr.bx, fr.ay, fr.bw, fr.bh);
    ymx = nint(pf->dbvref - fr.dbref);
    ymn = ymx - nint(pf->dbrange);
    if (ymn >= ymx)
    	ymn = ymx - 1;
    fr.dbvmx = ymx + fr.dbref;
    fr.dbvmn = ymn + fr.dbref;
    db_axis(fr.ay, fr.bh, ymx, ymn);
    db_label(fr.ay, fr.bh, ymx, ymn, fr.unit_type);
    if (fr.time_axis) {
        time_axis(fr.tmn, fr.tmx, fr.ay, fr.bh);
    } else {
        freq_axis(fr.fmn, fr.fmx, fr.ay, fr.bh, fr.log_axis);
    }
}

static void
spec_frame_2(PLTFMT *pf)
{
    display_box(fr.bx, fr.by, fr.bw, fr.bh);
    ph_axis(fr.by, fr.bh, fr.phmx, fr.phmn);
    ph_label(fr.by, fr.bh, fr.phmx, fr.phmn, fr.sh_d, fr.time_axis);
    if (fr.time_axis) {
        time_axis(fr.tmn, fr.tmx, fr.by, fr.bh);
    } else {
        freq_axis(fr.fmn, fr.fmx, fr.by, fr.bh, fr.log_axis);
    }
}

void
display_frame(PGMCFG *cf, PLTFMT *pf, char *comment, int vpflg)
{
    double fmn, fmx, tmn, tmx;
    int nframe, unit_type;

    nframe = (pf->show_phase || pf->show_delay) ? 2 : 1;
    if (vpflg && cf->sens_mp > 0) {
        unit_type = (cf->ntone > 0) ? 2 : 3;
    } else {
        unit_type = (cf->ntone > 0) ? 1 : 0;
    }

    if (nframe == fr.nframe
	&& unit_type == fr.unit_type
	&& pf->clear_screen == 0
	&& pf->dbvref == last_pf.dbvref
	&& pf->dbrange == last_pf.dbrange
	&& pf->log_axis == last_pf.log_axis
	&& pf->time_axis == last_pf.time_axis
	&& (pf->show_phase == 0 
            || (pf->show_phase == last_pf.show_phase
                && pf->phase_max == last_pf.phase_max
                && pf->phase_range == last_pf.phase_range))
	&& (pf->show_delay == 0
            || (pf->show_delay == last_pf.show_delay
                && pf->delay_max == last_pf.delay_max
                && pf->delay_range == last_pf.delay_range))
	&& (pf->show_phase == 0 
            || pf->show_delay == last_pf.show_delay)
	&& (pf->zoom == 0
	    ||	(!pf->time_axis
	    	 && pf->zf1 == last_pf.zf1
	         && pf->zf2 == last_pf.zf2)
	    ||	(pf->time_axis
	    	 && pf->zt1 == last_pf.zt1
	         && pf->zt2 == last_pf.zt2))
	&& pf->zoom == last_pf.zoom
	&& pf->tic_dir == last_pf.tic_dir
	&& pf->eml == last_pf.eml
	&& cf->sens_mp == last_cf.sens_mp
	)
        return;
    clr_all();
    adjust_layout(pf->eml);
    prn_top(cf, comment);

    fr.wave = 0;
    fr.nframe = nframe;
    fr.nfft = (cf->ncal > 0) ? cf->ncal : cf->nsamp;
    fr.sh_b = (pf->show_phase || pf->show_delay);
    fr.sh_d = pf->show_delay;
    fr.sh_p = pf->show_phase;
    fr.td = pf->tic_dir;
    if (fr.td) {
        fr.bx = bx + cw;
        fr.by = by - cw;
        fr.bw = bw - cw - cw / 4;
        fr.bh = bh - cw - cw / 4;
    } else {
        fr.bx = bx;
        fr.by = by;
        fr.bh = bh;
        fr.bw = bw;
    }
    fr.ax = fr.bx;
    fr.ay = fr.by;
    if (fr.sh_b) {
        if (fr.td) {
            fr.bh = (fr.bh - ch - cw) / 2;
            fr.ay = (fr.by - (fr.bh + ch + cw));
        } else {
            fr.bh = (bh - ch) / 2;
            fr.ay = (fr.by - (fr.bh + ch));
        }
    }
    fr.cx = bx - 6 * cw - cw / 2;
    fr.cy = by + cw / 4;
    fr.log_axis = pf->log_axis;
    fr.time_axis = pf->time_axis;
    if (fr.time_axis) {
        fr.tmn = 0;
        fr.tmx = cf->nsamp / cf->rate;
        if (pf->zoom) {
            tmn = fr.tmn;
            tmx = fr.tmx;
            fr.tmn = limit(tmn, pf->zt1, tmx);
            fr.tmx = limit(tmn, pf->zt2, tmx);
            if (fr.tmn >= fr.tmx) {     /* If zoom values are not good, */
                fr.tmn = tmn;           /* then restore default values. */
                fr.tmx = tmx;
            }
        }
    } else {
        fr.fmx = cf->rate / 2000;
        fr.fmn = (fr.log_axis) ? fr.fmx / (fr.nfft / 2): 0;
        if (pf->zoom) {
            fmn = fr.fmn;
            fmx = fr.fmx;
            fr.fmn = limit(fmn, pf->zf1 / 1000, fmx);
            fr.fmx = limit(fmn, pf->zf2 / 1000, fmx);
            if (fr.fmn >= fr.fmx) {     /* If zoom values are not good, */
                fr.fmn = fmn;           /* then restore default values. */
                fr.fmx = fmx;
            }
        }
    }
    fr.unit_type = unit_type;
    if (unit_type == 3) {
	fr.vref = 1 / cf->sens_mp;
        fr.dbref = 20 * log10(fr.vref);
    } else if (unit_type == 2) {
	fr.vref = cf->sens_mp * splref / sqrt(2.0);
        fr.dbref = 20 * log10(cf->sens_mp * splref);
    } else if (unit_type == 1) {
	fr.vref = 1;
        fr.dbref = 10 * log10(2.0);
    } else {
	fr.vref = 1;
        fr.dbref = 0;
    }
    //df_khz = (cf->rate / fr.nfft) / 1000;
    if (fr.sh_d) {
	fr.phmx = pf->delay_max;
	fr.phmn = fr.phmx - pf->delay_range;
    } else {
	fr.phmx = pf->phase_max;
	fr.phmn = fr.phmx - pf->phase_range;
    }

    spec_frame_1(pf);
    if (fr.sh_b)
        spec_frame_2(pf);
    last_pf = *pf;
    last_cf = *cf;
    pf->clear_screen = 0;
}

double 
cmg(float *w)
{
    double wms;
    static double eps = 1e-30;
    
    wms = w[0] * w[0] + w[1] * w[1];
    if (wms < eps)
        wms = eps;
    return (wms);
}

double 
cdb(float *w)
{
    double wms;
    static double eps = 1e-30;
    
    wms = w[0] * w[0] + w[1] * w[1];
    if (wms < eps)
        wms = eps;
    return (10 * log10(wms));
}

double 
cln(float *w)
{
    double wms;
    static double eps = 1e-30;
    
    wms = w[0] * w[0] + w[1] * w[1];
    if (wms < eps)
        wms = eps;
    return (log(wms) / 2);
}

double cph(float *w)
{
    double wms;
    static double eps = 1e-30;
    
    wms = w[0] * w[0] + w[1] * w[1];
    if (wms < eps)
        return (0);
    return (atan2(w[1], w[0]));
}

double
uw_ph(double ph, double lp)                             /* unwrap phase */
{
    while((ph - lp) > 0.5)
        ph -= 1;
    while((lp - ph) > 0.5)
        ph += 1;

    return (ph);
}

double
wr_ph(double yp, double dy, double ymn, double ymx)     /* wrap phase */
{
    if (dy != 0) {
        while (yp < ymn)
            yp += dy;
        while (yp > ymx)
            yp -= dy;
    }
    return (yp);
}

void
dp_drw_fit_1()
{
    double dx, ymn, ymx;
    int i, n, cc, x, x0, x2, y, y0, yh;
    int xl = 0, yl = 0;
    
    n = fr.nn;
    dx = dsp_dx;
    x0 = nint(dsp_x0 + dsp_dx * fr.xo);
    x2 = fr.bx + fr.bw;
    y0 = fr.ay;
    yh = fr.bh;
    gr_set_clip(fr.bx, y0 - yh, fr.bx + fr.bw, y0);
    ymx = fr.dbvmx;
    ymn = fr.dbvmn;
    cc = fft_color[(4 + fft_ic) % fft_nci];
    draw_seg(0, 0, 0, 0, -1);
    for (i = 0; i < n; i++) {
        x = nint(x0 + dx * i);
        y = y0 - nint(yh * (fr.dpdb[i] - ymn) / (ymx - ymn));
        if (i > 0)
            draw_seg(xl, yl, x, y, cc);
        if (x > x2)
            break;
        xl = x;
	yl = y;
    }
    draw_seg(0, 0, 0, 0, -2);
}

void
dp_drw_fit_2()
{
    double dx, ymn, ymx;
    int i, n, cc, x, x0, x2, y, y0, yh;
    int xl = 0, yl = 0;
    
    n = fr.nn;
    dx = dsp_dx;
    x0 = nint(dsp_x0 + dsp_dx * fr.xo);
    x2 = fr.bx + fr.bw;
    y0 = fr.by;
    yh = fr.bh;
    gr_set_clip(fr.bx, y0 - yh, fr.bx + fr.bw, y0);
    ymx = fr.phmx;
    ymn = fr.phmn;
    cc = fft_color[(4 + fft_ic) % fft_nci];
    draw_seg(0, 0, 0, 0, -1);
    for (i = 0; i < n; i++) {
        x = nint(x0 + dx * i);
        y = y0 - nint(yh * (fr.dpph[i] - ymn) / (ymx - ymn));
        if (i > 0)
            draw_seg(xl, yl, x, y, cc);
        if (x > x2)
            break;
        xl = x;
	yl = y;
    }
    draw_seg(0, 0, 0, 0, -2);
}

void
display_ftvt_1(float *w, PLTFMT *pf)
{
    double yy, ymn, ymx;
    float *ww;
    int    cc, i, j, y0, x, y, yh, yl = 0, xx;

    y0 = fr.ay;
    yh = fr.bh;
    ymx = fr.dbvmx;
    ymn = fr.dbvmn;
    for (j = 0; j < NDP; j++) {
        if (pf->dp_dsp[j]) {
            cc = fft_color[(j + fft_ic) % fft_nci];
            draw_seg(0, 0, 0, 0, -1);
            for (i = 0; i < fr.np; i++) {
                ww = w + fr.wo + (i * fr.wi + fr.wj[j]) * 2;
                yy = cdb(ww) - pf->dbvpu;
                x = nint(dsp_x0 + dsp_dx * i);
                y = y0 - nint(yh * (yy - ymn) / (ymx - ymn));
                if (i >= dsp_i1 && i <= dsp_i2) {
                    if (fr.ss) {
                        if (i > dsp_i1)
                            draw_seg(x, yl, x, y, cc);
                        yl = y;
                    }
                    xx = nint(dsp_x0 + dsp_dx * (i + 1));
                    draw_seg(x, yl, xx, y, cc);
                }
                yl = y;
            }
            draw_seg(0, 0, 0, 0, -2);
        }
    }
}

void
display_ftvt_2(float *w, PLTFMT *pf)
{
    double dp = 0, dy, ph, po, lp, yy, ymn, ymx;
    float *ww;
    int    cc, i, j, y0, x, y, yh, yl = 0, xx;

    y0 = fr.by;
    yh = fr.bh;
    ymn = fr.phmn;
    ymx = fr.phmx;
    if (fr.sh_d) {
        dy = 1000 * pf->df_khz;
        dp = dy;
    } else {
        //sc = pow(10.0, (double) fr.phdp);
        dy = nint(ymx - ymn);
        if (dy < 1)
            dy = 1;
    }
    po = pf->phase_off;
    lp = 0;
    for (j = 0; j < NDP; j++) {
        if (pf->dp_dsp[j]) {
            cc = fft_color[(j + fft_ic) % fft_nci];
            draw_seg(0, 0, 0, 0, -1);
            for (i = 0; i < fr.np; i++) {
                ww = w + fr.wo + (i * fr.wi + fr.wj[j]) * 2;
                ph = uw_ph(cph(ww) / tpi - po, lp);
                yy = fr.sh_d ? (ph - lp) * dp : wr_ph(ph, dy, ymn, ymx);
                x = nint(dsp_x0 + dsp_dx * i);
                y = y0 - nint(yh * (yy - ymn) / (ymx - ymn));
                if (i >= dsp_i1 && i <= dsp_i2) {
                    if (fr.ss) {
                        if (i > dsp_i1)
                            draw_seg(x, yl, x, y, cc);
                        yl = y;
                    }
                    xx = nint(dsp_x0 + dsp_dx * (i + 1));
                    draw_seg(x, yl, xx, y, cc);
                }
                yl = y;
                lp = ph;
            }
        }
        draw_seg(0, 0, 0, 0, -2);
    }
}

void
display_fft_1(float *w, int nfft, PLTFMT *pf)
{
    double db, ymn, ymx, sn = 0;
    int i, n, os, x, y, xx, yy, cc, j, ii[NDP], y0, yh;
    int i1 = 0, i2 = 0, nn = 0;

    y0 = fr.ay;
    yh = fr.bh;
    gr_set_clip(fr.bx, y0 - yh, fr.bx + fr.bw, y0);
    ymx = fr.dbvmx;
    ymn = fr.dbvmn;
    dp_fc(pf->i1, pf->i2, ii);
    if (fr.time_axis) {
        display_ftvt_1(w, pf);
    } else {
        cc = fft_color[fft_ci];
        n = nfft / 2;
        x = fr.bx;
        y = fr.by;
        if (pf->dp) {
            os = pf->ofst;
            nn = nint(ii[0] * pf->dpsb / 100);
            i1 = limit(ii[2] - ii[1] + 1, ii[0] - nn, ii[1] - 1);
            i2 = limit(ii[2] - ii[1] + 1, ii[0] + nn, ii[1] - 1);
            pf->nnsb = nn;
            nn = 0;
            sn = 0;
        } else {
            os = 0;
        }
        draw_seg(0, 0, 0, 0, -1);
        for (i = 0; i < n; i++) {
            db = cdb(&w[i * 2 + os]) - pf->dbvpu;
            if (pf->dp) {
                for (j = 0; j < NDP; j++) {
                    if (i == ii[j]) {
                        pf->lv[j] = db - fr.dbref;
                    }
                }
                if (i >= i1 && i <= i2 && i != ii[0]) {
                    sn += pow(10.0, db / 10);
                    nn++;
                }
            }
            if (i < dsp_i1 || i > dsp_i2)
                continue;
            if (fr.log_axis)
                xx = (int) (dsp_x0 + dsp_dx * log(i * dsp_nf));
            else
                xx = (int) (dsp_x0 + dsp_dx * (i - dsp_i1));
            yy = y0 - nint(yh * (db - ymn) / (ymx - ymn));
            if (i > dsp_i1)
                draw_seg(x, y, xx, yy, cc);
            x = xx;
            y = yy;
        }
        draw_seg(0, 0, 0, 0, -2);
        if (pf->dp && nn > 0 && sn > 0) {
            pf->lv[3] = 10 * log10(sn / nn) - fr.dbref;
        }
    }
    display_box(fr.bx, y0, fr.bw, yh);
}

void
display_fft_2(float *w, int nfft, PLTFMT *pf)
{
    double ph, lp, yp, ymn, ymx, dp = 0, dy, sc;
    double po, pho, deo, df;
    int i, x, y, xx, yy, cc, y0, yh;

    y0 = fr.by;
    yh = fr.bh;
    gr_set_clip(fr.bx, y0 - yh, fr.bx + fr.bw, y0);
    ymn = fr.phmn;
    ymx = fr.phmx;
    if (fr.time_axis) {
        display_ftvt_2(w, pf); 
    } else {
        cc = fft_color[fft_ci];
        //n = nfft / 2;
        if (fr.sh_d) {
            dy = 1 / pf->df_khz;
            dp = -dy;
        } else {
            sc = pow(10.0, fr.phdp);
            dy = nint((ymx - ymn) * sc) / sc;
        }
	pho = pf->phase_off;
	deo = pf->delay_off;
	df = pf->df_khz;
        lp = 0;
        x = fr.bx;
        y = fr.by;
        draw_seg(0, 0, 0, 0, -1);
        for (i = dsp_i1; i <= dsp_i2; i++) {
            po = pho - i * deo * df;
            ph = uw_ph(cph(&w[i * 2]) / tpi - po, lp);
            yp = fr.sh_d ? (ph - lp) * dp : wr_ph(ph, dy, ymn, ymx);
            if (fr.log_axis)
                xx = (int) (dsp_x0 + dsp_dx * log(i * dsp_nf));
            else
                xx = (int) (dsp_x0 + dsp_dx * (i - dsp_i1));
            yy = y0 - nint(yh * (yp - ymn) / (ymx - ymn));
            if (i > dsp_i1)
                draw_seg(x, y, xx, yy, cc);
            x = xx;
            y = yy;
            lp = ph;
        }
        draw_seg(0, 0, 0, 0, -2);
        x = cw / 2;
        y = cw / 4 + ch * 2;
    }
    display_box(fr.bx, y0, fr.bw, yh);
}

void
display_fft(float *w, PLTFMT *pf, int nch)
{
    double  df, fw, os;
    float  *ww;
    int     i, j, n, nf, nn, i1 = 0, i2 = 0, j1 = 0, j2 = 0;

    nf = pf->nfft;
    if (w == NULL || nf <= 0)
        return;
    df = pf->df_khz;
    if (fr.time_axis) {
        if (pf->dpen) {
            nn = pf->dpen;
            os = 0;
            fr.np = nn;
            fr.ss = 0;
            fr.mg = (pf->ofst + nf / 2) / nf;
            fr.wo = 0;
            fr.wi = 1;
            for (j = 0; j < NDP; j++)
                fr.wj[j] = j * nn;
            fr.xo = fr.mg;
	    if (pf->npfit > 0)
		fr.nn = (pf->npfit + nf / 2) / nf;
	    else
		fr.nn = fr.np - fr.mg * 2;
        } else {
            nn = pf->nc;
            os = (double) pf->ofst / nf;
            fr.np = pf->nc;
            fr.ss = 1;
            fr.mg = 0;
            fr.wo = pf->ofst;
            fr.wi = nf / 2;
            dp_fc(pf->i1, pf->i2, fr.wj);
            fr.xo = 0.5;
            fr.nn = fr.np - 1;
        }
        i1 = (int) floor(fr.tmn * df * 1000);
        i2 = (int) ceil(fr.tmx * df * 1000);
        dsp_i1 = limit(0, i1 - 1, nn - 1);
        dsp_i2 = limit(dsp_i1 + 1, i2 + 1, nn);
        dsp_dx = fr.bw / ((fr.tmx - fr.tmn) * (df * 1000));
        dsp_x0 = fr.bx + dsp_dx * (os - fr.tmn * (df * 1000));
        dsp_nf = 0;
    } else if (!fr.log_axis) {
        i1 = (int) floor(fr.fmn / df);
        i2 = (int) ceil(fr.fmx / df);
        j1 = 0;
        j2 = nf / 2 + 1;
        dsp_i1 = limit(j1, i1, j2 - 1);
        dsp_i2 = limit(dsp_i1 + 1, i2, j2);
        fw = fr.fmx - fr.fmn;
        dsp_dx = fr.bw * (df / fw);
        dsp_x0 = fr.bx + fr.bw * (dsp_i1 * df - fr.fmn) / fw;
        dsp_nf = 0;
    } else {
        i1 = (int) floor(fr.fmn / df);
        i2 = (int) ceil(fr.fmx / df);
        j1 = 0;
        if (nf >= fr.nfft) {
            j1 = (int) (((double) nf / fr.nfft) + 0.9999);
        } else {
            j1 = 1;
        }
        j2 = nf / 2 + 1;
        dsp_i1 = limit(j1, i1, j2 - 1);
        dsp_i2 = limit(dsp_i1 + 1, i2, j2);
        dsp_dx = fr.bw / log(fr.fmx / fr.fmn);
        dsp_nf = df / fr.fmn;
        dsp_x0 = fr.bx;
    }
    for (i = 0; i < nch; i++) {
	ww = &w[(nf + 2) * i];
	display_fft_1(ww, nf, pf);
	if (fr.sh_b)
	    display_fft_2(ww, nf, pf); 
        if (fr.time_axis)
            break;
	fft_ci++;
	fft_ci %= fft_nci;
    }

    if (fr.time_axis && pf->dpef && fr.nn > 1) {
        n = pf->dpfc ? fr.np * 2 : fr.np;
        fr.dpdb = (float *) calloc(n, sizeof(float));
        if (fr.dpdb) {
            dp_exp_fit(w, pf);
            dp_drw_fit_1();
            if (fr.sh_b && pf->dpfc)
                dp_drw_fit_2();
	    reset_clip();
            free(fr.dpdb);
        }
    }
}

int
describe_position(char *s, int x, int y)
{
    char *xl, *xu, *yl, *yu;
    double xn, xv, xm, yn, yv;
    int ina,inb, inx;

    inx = (x >= fr.bx && x <= fr.bx + fr.bw);
    ina = (y >= fr.ay - fr.bh && y <= fr.ay);
    inb = (y >= fr.by - fr.bh && y <= fr.by);
    if (inx && (ina || inb)) {
	xn = (x - fr.bx) / (double) fr.bw;
        if (last_pf.time_axis) {
            xl =  "time";
	    xv = fr.tmn + (fr.tmx - fr.tmn) * xn;
	    if (xv < 1) {
		xm = 1000;
		xu = "msec";
	    } else {
		xm = 1;
		xu = "sec";
	    }
	} else {
            xl = "frequency";
	    if (fr.log_axis) {
		xv = fr.fmn * pow(fr.fmx / fr.fmn, xn);
	    } else {
		xv = fr.fmn + (fr.fmx - fr.fmn) * xn;
	    }
	    if (xv < 1) {
		xm = 1000;
		xu = "Hz";
	    } else {
		xm = 1;
		xu = "kHz";
	    }
	}
	if (fr.wave) {
	    yl = "wave";
	    if (fr.unit_type == 3) {
		yu = "vpp ";
	    } else if (fr.unit_type == 2) {
		yu = " Pa ";
	    } else if (fr.unit_type == 1) {
		yu = "volt";
	    } else {
		yu = "unit";
	    }
	    yn = (fr.by - y) / (double) fr.bh;
	    yv = fr.vmn + yn * (fr.vmx - fr.vmn);
	} else if (ina) {
	    yl = "magnitude";
	    yn = (fr.ay - y) / (double) fr.bh;
	    yv = fr.dbvmn + (fr.dbvmx - fr.dbvmn) * yn - fr.dbref;
	    if (fr.unit_type == 3) {
		yu = "dBvpp";
	    } else if (fr.unit_type == 2) {
		yu = "dBSPL";
	    } else if (fr.unit_type == 1) {
		if (yv > -60) {
		    yu = "dBV";
		} else {
		    yu = "dBmV";
		    yv += 60;
		}
	    } else {
		yu = "dB";
	    }
	} else {
	    yl = fr.sh_d ? "delay" : "phase";
	    yn = (fr.by - y) / (double) fr.bh;
	    yv = fr.phmn + (fr.phmx - fr.phmn) * yn;
	    yu = fr.sh_d ? "msec" : "cycl";
	}
	sprintf(s, "%s = %.4g %s, %s = %.4g %s", xl, xv * xm, xu, yl, yv, yu);
    } else {
	*s = '\0';
    }

    return (strlen(s));
}

