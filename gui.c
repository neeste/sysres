/* gui.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "display.h"
#include "version.h"
#include "sysres.h"
#include "sio.h"
#include "fk.h"

#ifndef WIN32
#define _strdup strdup
#endif

#define MAXNF       256

double  eval_f(char *);
double  gr_etime(void);
double  resp_rms(double *, double *);
int	access(char *, int);
int	alloc_mem(int);
int     chk_overwrite(char *);
int     describe_position(char *, int, int);
int     display_wave(float *, int, int, double, PLTFMT *);
int     eval_i(char *);
int	getnum(char *, double *, char *);
int     get_usr_var(char *, double *, char *);
int     gr_can_print(void);
int     gr_edit(char *);
int     gr_find(char *, char *, int);
int     gr_pbstr(char *);
int     print_screen(char *, char *, int, int);
int	read_config(char *, char *, PGMCFG *);
int	read_data_ascii(char *, char *, PGMCFG *);
int	read_data_mat(char *, char *, PGMCFG *);
int	read_stim_mat(char *, char *, PGMCFG *);
int     set_usr_var(char *, char *, char *);
int     spectral_average(int);
int     stim_compute(int, char *);
int	write_config(char *, char *, PGMCFG *);
int	write_data_ascii(char *, char *, PGMCFG *);
int	write_data_mat(char *, char *, PGMCFG *);
int	write_stim_mat(char *, char *, PGMCFG *);
void    cal_create(char *);
void    cal_read(char *);
void    cal_tones(double, double, char *);
void    clr_all();
void    clr_hist();
void    dp_cp(char *);
void    dp_mp(char *);
void    display_fft(float *, PLTFMT *, int);
void    display_frame(PGMCFG *, PLTFMT *, char *, int);
void    display_rte(float *, int, int, double, PLTFMT *);
void    dp_compute();
void    dp_stim(char *);
void    drop_suid();
void    dump_hist(FILE *);
void    free_mem();
void    gr_beep();
void    gr_close();
void    gr_exit_incl();
void    gr_get_cwd(char *, int);
void    gr_getline(char *, char *, int);
void    gr_getprompt(char *, char *, int);
void    gr_init();
void    gr_prcl(char *);
void    gr_set_cwd(char *);
void    gr_sleep(double);
void    log_file(int);
void    masking_msg(char *, char *);
void    prn_devlst();
void    prn_help();
void    prn_msg(char *);
void    prn_msg2(char *);
void    prn_top(PGMCFG *, char *);
void    resp_bp(int);
void    resp_ft(int);
void    resp_norm();
void    resp_vpu();
void    set_ic(int);
void    stim_ft(int);
void    stim_scale(int);
void    test_tone(char *msg);
void    write_help(char *, char *);
void    write_nfo_file(char *);

extern char *no_dev;
extern double reject_thresh, dpn_sb, splref, stime, samp_val;
extern double tone_attn1, tone_attn2, tone_attn3, swpskp;
extern float *st, *rt, *sf, *rf;
extern int dc_remove, nsab, logflg, debug;
extern int nrej, reject_mode, wtav_mode;
extern int nsch, nrch, nrcp, sio_present, inclev, ntwb;
extern FILE *lfp[8];
extern PGMCFG cf;
extern PLTFMT pf;

extern int ycmd, xpix, ypix, cw, ch;

static char cfgfn[80] = "";
static char comment[80] = "";
static char crn[20] = "";
static char datfn[80] = "";
static char *def_ascfn = "sysres.txt";
static char *def_cfgfn = "sysres.cfg";
static char *def_datfn = "sysres.mat";
static char *def_nrmfn = "sysres.mat";
static char *def_prnfn = "sysres.prn";
static char *help_fn = "help.txt";
static char idstr[20] = "";
static char lstfn[80] = "";
static char msg[300] = "";
static char nrmfn[80] = "";
static char *po[2] = {"Landscape", "Portrait"};
static char *pt[3] = {"PostScript", "PCL", "Color PostScript"};
static char *ramp_name[] = {
    "None", "Linear", "Cosine", "Blackman", "Exponential", 
    "Lissajous", "Proscan", "Nuttall"
};
static char nfofn[80] = DEFNFO;
static char stmfn[80] = "";
static int nrt = sizeof(ramp_name) / sizeof(ramp_name[0]);
static double ad_atten = 0;
static double da_atten = 0;

void
init_sio(char *s)
{
    prn_msg("locate sio device...");
    sio_set_device(SIO_PREF_SYNC); // look for a 24-bit card
    prn_msg("open sio...");
    sio_set_channel_offset(cf.choi, cf.choo);
    sio_present = sio_open();
    if (sio_present) {
	sio_get_device(s);
    } else {
	strcpy(s,"No sio device.");
    }
    stim_compute(2, msg);	// zero stimulus
    play_zero(8);
}

void
newext(char *f, char *e)
{
    char   *s = f;

    while (*s >= ' ')		/* locate end of filename */
	s++;
    while (*s != '\\' && *s != '/' && s > f)	/* backup to start of name */
	s--;
    while (*s >= ' ' && *s != '.')	/* locate end of name */
	s++;
    while (*e >= ' ')		/* copy extension */
	*s++ = *e++;
    *s = '\0';
}

int
hasext(char *f)
{
    char   *s = f;
    int     e = 0;

    while (*s >= ' ')		/* locate end of filename */
	s++;
    while (*s != '\\' && *s != '/' && s > f) { /* backup to start of name */
	s--;
	if (*s == '.')
	    e++;
    }
    return (e);
}

void
trim(char *s)
{
    char *p;

    for (p = s; *p >= ' '; p++)	    /* locate end of string */
	continue;
    while (p > s && p[-1] <= ' ')   /* backup over trailing spaces */
	p--;
    if (*p)
        *p = '\0';                  /* truncate string */
}

void
strip_comment(char *s)
{
    char *p;

    for (p = s; *p && *p != ';'; p++)	    /* locate comment or EOL */
	continue;
    while (p > s && p[-1] <= ' ')	    /* backup over trailing spaces */
	p--;
    if (*p)
        *p = '\0';			    /* truncate string */
}

void
defext(char *f, char *e)
{
    trim(f);
    if (!hasext(f))
        newext(f, e);
}

void
basename(char *b, char *f, int n)
{
    char   *s = f;

    while (*s >= ' ')		/* locate end of filename */
	s++;
    while (*s != '\\' && *s != '/' && s > f) /* backup to start of name */
	s--;
    while (*s == '\\' && *s == '/') /* advance past slash */
	s++;
    f = s;
    while (*f >= ' ' && *f != '.' && (f - s) < n) /* copy name */
	*b++ = *f++;
    *b = '\0';
}

int
read_list(char *msg)
{
    lfp[inclev] = fopen(lstfn, "rt");
    if (lfp[inclev] == NULL) {
        sprintf(msg, "Can't read list file (%s).", lstfn);
        return (0);
    }
    inclev++;
    return (1);
}

void
write_list(char *msg)
{
    FILE *fp;
    static char *fn = "sysres.lst";

    fp = fopen(fn, "wt");
    if (fp == NULL) {
        sprintf(msg, "Can't open list file for writing (%s).", fn);
    }
    dump_hist(fp);
    fclose(fp);

    sprintf(msg, "Wrote command list to file (%s).", fn);
}

void
exit_list()
{
    while (inclev > 0) {
        fclose(lfp[--inclev]);
    }
}

void
masking_query(char *s)
{
    int c, d, e;
    double v;

    c = s[1];
    d = s[2];
    e = s[3];
    if (e == '=') {
        v = eval_f(s + 4);
        switch(c) {
        case 'a':
            if (d == '1') {
                tone_attn1 = v;
            } else if (d == '2') {
                tone_attn2 = v;
            }
            break;
        case 'f':
            if (d == '1') {
                cf.f1 = v;
            } else if (d == '2') {
                cf.f2 = v;
            }
            break;
        case 'm':
            if (d == 'r') {
                tm.mr = v;
            } else if (d == 's') {
                tm.ms = v;
            } else if (d == 'd') {
                tm.md = v;
            }
            break;
        case 'p':
            if (d == 'g') {
                tm.pg = v;
            } else if (d == 'd') {
                tm.pd = v;
            }
            break;
        case 't':
            if (d == 'v') {
                tm.tv = v;
            } else if (d == 'd') {
                tm.td = v;
            }
            break;
        }
    } else if (c == 's' && d <= ' ') {
        cf.f1 = 4000;
        cf.f2 = 4000;
        cf.f3 = 0;
        tm.mr = 2;
        tm.pd = 2;
        swpskp = 0;
    }
}

static double
adjust_f(double f)
{
    int n;

    if (cf.rate > 0 && cf.ncal > 0) {
        n = (int) (cf.ncal * f / cf.rate + 0.5);
        f = n * cf.rate / cf.ncal;
    }
    return (f);
}

static void
attn_msg(int flg)
{
    if (flg == 3 || cf.f3 > 0)
        sprintf(msg, "att1=%.1f  att2=%.1f  att3=%.1f  (dB)", 
            tone_attn1, tone_attn2, tone_attn3);
    else
        sprintf(msg, "att1=%.1f  att2=%.1f  (dB)", tone_attn1, tone_attn2);
    prn_msg(msg);
}

static void
cho_msg()
{
    sprintf(msg, "channel offset: in=%d out=%d", cf.choi, cf.choo);
    prn_msg(msg);
    sio_set_channel_offset(cf.choi, cf.choo);
    sio_present = sio_open();
}

static void
cd_msg(char *s)
{
    int n;
    static int m = 58;

    n = strlen(s);
    if (n > m) {
	strcpy(s, "...");
	strcat(s, s + (n - m + 4));
    }

    sprintf(msg, "current directory = %.*s", n, s);
    prn_msg(msg);
}

static void
freq_msg(int flg)
{
    if (flg == 3 || cf.f3 > 0) {
        sprintf(msg, "f1=%.0f  f2=%.0f  f3=%.0f  Hz", 
            cf.f1, cf.f2, cf.f3);
    } else {
        sprintf(msg, "f1=%.0f  f2=%.0f  Hz", cf.f1, cf.f2);
    }
    prn_msg(msg);
}

static void
levl_msg(int flg)
{
    if (flg == 3 || cf.f3 > 0)
        sprintf(msg, "levl1=%.1f  levl2=%.1f  levl3=%.1f  (dB)", 
            cf.l1, cf.l2, cf.l3);
    else
        sprintf(msg, "levl1=%.1f  levl2=%.1f  (dB)", cf.l1, cf.l2);
    prn_msg(msg);
}

void
l1eq(char *s, int set)
{
    char *p;
    double a0, a1;
    int i;
    static double c[4] = {0};
    static int o = 0;

    if (strlen(s) > 0) {
	for (i = 0; i < 4; i++)
	    c[i] = 0;
	o = 0;
	p = s;
	while (*p && o < 4) {
	    while (*p == ' ' || *p == '+')
		p++;
	    c[o++] = eval_f(p);
	    while (*p && *p != ' ' && *p != '+')
		p++;
	}
    }
    if (set && o) {
        a0 = cf.l2;
        a1 = cf.lpma[1];
        cf.l1 = c[0] + c[1] * a0;
        cf.lpma[0] = c[1] * a1;
        cf.lpmb[0] = c[2];
        cf.lpmc[0] = c[3];
        cf.lpnc[0] = cf.lpnc[1];
        cf.lpph[0] = cf.lpph[1];
    }
    sprintf(msg, "L1 =");
    for (i = 0; i < o; i++) {
	sprintf(msg + strlen(msg), " %.3g", c[i]);
    }
    prn_msg(msg);
}

void
l3eq(char *s, int set)
{
    char *p;
    double a0, a1;
    int i;
    static double c[4] = {0};
    static int o = 0;

    if (strlen(s) > 0) {
	for (i = 0; i < 4; i++)
	    c[i] = 0;
	o = 0;
	p = s;
	while (*p && o < 4) {
	    while (*p == ' ' || *p == '+')
		p++;
	    c[o++] = eval_f(p);
	    while (*p && *p != ' ' && *p != '+')
		p++;
	}
    }
    if (set && o) {
        a0 = cf.l2;
        a1 = cf.lpma[1];
        cf.l3 = c[0] + c[1] * a0;
        cf.lpma[2] = c[1] * a1;
        cf.lpmb[2] = c[2];
        cf.lpmc[2] = c[3];
        cf.lpnc[2] = cf.lpnc[1];
        cf.lpph[2] = cf.lpph[1];
    }
    sprintf(msg, "L3 =");
    for (i = 0; i < o; i++) {
	sprintf(msg + strlen(msg), " %.3g", c[i]);
    }
    prn_msg(msg);
}

static void
lp_msg(int mpi)
{
    sprintf(msg, 
	"Lissajous path %d:  nc=%.0f  ph=%.0f (deg)  ma,mb,mc=%.0f,%.0f,%.0f (dB)",
	mpi + 1, cf.lpnc[mpi], cf.lpph[mpi], 
	cf.lpma[mpi], cf.lpmb[mpi], cf.lpmc[mpi]);
    prn_msg(msg);
}

static void
am_msg()
{
    sprintf(msg, 
	"amplitude modulation:  in=%.2f %.2f %.2f;  nc=%.0f %.0f %.0f  (Hz)",
	cf.amin[0], cf.amin[1], cf.amin[2],
	cf.amnc[0], cf.amnc[1], cf.amnc[2]); 
    prn_msg(msg);
}

static void
dbms_msg(int flg)
{
    if (flg == 3 || cf.f3 > 0)
        sprintf(msg, "dbms = %.4f, %.4f, %.4f dB/msec",
	cf.dbms[0], cf.dbms[1], cf.dbms[2]);
    else
        sprintf(msg, "dbms = %.4f, %.4f msec", cf.dbms[0], cf.dbms[1]);
    prn_msg(msg);
}

static void
dpon_msg()
{
    int n;
    static char *dpon_name[8] = {
        "6*F1-5*F2", "5*F1-4*F2", "4*F1-3*F2", "3*F1-2*F2",
        "2*F1-F2", "F2-F1", "2*F2-F1", "3*F2-2*F1"
    };

    n = limit(-5, pf.dpon, 2) + 5;
    sprintf(msg, "DP order number = %d (%s)",  pf.dpon, dpon_name[n]);
    prn_msg(msg);
}

static void
edit_fn(char *fn)
{
    if (!gr_edit(fn))
	prn_msg("Can't edit file.");
}

static void
list_fn(char *s)
{
    char fn[80];
    double to = 0;
    int nf, c;

    for (nf = 0; nf < MAXNF && gr_find(s, fn, nf); nf++) {
        c = gr_getkey(fn, to); 
        if (c == CTRL_C || c == ESC)
            break;
    }
}

static void
dp_exp_fit_msg(char *fn, double dbmx, double dbrn, int dpon)
{
    char bn[80];
    double t;

    if (pf.dpef <= 0)
	return;
    if (pf.dpef == 1) {
	pf.efc[3] = 0.00;
	pf.efc[4] = 0.001;
    } else if (pf.efc[2] > pf.efc[4]) { /* put smallest time-constant first */
	t = pf.efc[1];
	pf.efc[1] = pf.efc[3];
	pf.efc[3] = t;
	t = pf.efc[2];
	pf.efc[2] = pf.efc[4];
	pf.efc[4] = t;
	t = pf.efc[7];
	pf.efc[7] = pf.efc[8];
	pf.efc[8] = t;
    }
    strcpy(bn, fn);
    newext(bn, "");
    if (pf.dpfc) {
        sprintf(msg, "dpenv('%s', [%.2f %.3f %.2f %.3f %.2f %.4f %.4f %.4f]"
	    ", %.0f, %.0f, %d, '%s.txt')",
	    bn, pf.efc[1], pf.efc[2], pf.efc[3], pf.efc[4], pf.efc[0], 
	    pf.efc[7], pf.efc[8], pf.efc[6], dbmx, dbrn, dpon, bn);
    } else {
        sprintf(msg, "dpenv('%s', [%.2f %.3f %.2f %.3f %.2f]"
	    ", %.0f, %.0f, %d, '%s.txt')",
	    bn, pf.efc[1], pf.efc[2], pf.efc[3], pf.efc[4], pf.efc[0], 
	    dbmx, dbrn, dpon, bn);
    }
    prn_msg(msg);
    if (pf.dpef == 1) {
	sprintf(msg, "DP exp fit: %.2f, %.2f dB"
	    ";  %.3f sec (VAF=%.3f,SNR=%.1fdB)",
	     pf.efc[1], pf.efc[0], pf.efc[2], pf.efc[5], pf.efc[9]);
    } else if (pf.dpef == 2) {
	sprintf(msg, "DP exp fit: %.2f, %.2f, %.2f dB"
	    "; %.3f, %.3f sec (VAF=%.3f,SNR=%.1fdB)",
	    pf.efc[1], pf.efc[3], pf.efc[0], pf.efc[2], pf.efc[4], 
	    pf.efc[5], pf.efc[9]);
    }
    prn_msg(msg);
    if (pf.dpfc) {
        if (pf.dpef == 1) {
    	    sprintf(msg, "     phase:  %.4f, %.4f cyc",
	        pf.efc[7], pf.efc[6]);
        } else if (pf.dpef == 2) {
    	    sprintf(msg, "     phase:  %.4f, %.4f, %.4f cyc",
	        pf.efc[7], pf.efc[8], pf.efc[6]);
	}
        prn_msg2(msg);
    }
}

void
dp_dsp_msg()
{
    sprintf(msg, "DP display = %d %d %d %d %d", 
	pf.dp_dsp[0], pf.dp_dsp[1], pf.dp_dsp[2], 
	pf.dp_dsp[3], pf.dp_dsp[4]);
    prn_msg(msg);
}

double
db_ref() 
{
    double dbref;

    if (cf.sens_mp > 0) {
	dbref = (cf.ntone > 0) ? 20 * log10(cf.sens_mp * splref) : -20 * log10(cf.sens_mp) ;
    } else {
        dbref = (cf.ntone > 0) ? 10 * log10(2.0) : 0;
    }
    return (dbref);
}

void
db_msg()
{
    char *units, msg[80];
    double dbmin, dbmax;


    if (cf.sens_mp > 0) {
        units = (cf.ntone > 0) ? "dBSPL" : "dBvpp";
    } else {
        units = (cf.ntone > 0) ? "dBV" : "dB";
    }
    dbmax = pf.dbvref - db_ref();
    dbmin  = dbmax - pf.dbrange;
    sprintf(msg, "dBmax=%.0f dBmin=%.0f (%s) ", dbmax, dbmin, units);
    prn_msg(msg);
}

static void
response_msg(char *msg1, int tf)
{
    char *tfstr, *units, *rmsfmt, msg[320];
    int i, n;
    double vrms[MAXNDC], msgd[MAXNDC], rms[MAXNDC], dbr, maxrms = 0;

    resp_rms(vrms, msgd);
    tfstr = tf ? "frequency" : "time";
    if (tf) {
	tfstr = "frequency";
        if (cf.sens_mp > 0) {
	    dbr = 20 * log10(cf.sens_mp * splref / sqrt(2.0));
        } else if (cf.ntone > 0) {
	    dbr = 0;
        } else {
	    dbr = -10 * log10(cf.nsamp);
        }
	for (i = 0; i < nrch; i++)
	    rms[i] = (vrms[i] > 0) ? 20 * log10(vrms[i]) - dbr : -300;
        units = (cf.sens_mp > 0) ? "dBSPL" : (cf.ntone > 0) ? " dBV" : " dB";
    } else {
	tfstr = "time";
	for (i = 0; i < nrch; i++) {
	    rms[i] = (cf.sens_mp > 0) ? vrms[i] / cf.sens_mp :  
		(cf.ntone > 0) ? vrms[i] : vrms[i] * sqrt(cf.nsamp);
	    if (maxrms < rms[i])
		maxrms = rms[i];
	}
	if (maxrms < 1e-3) {
	    for (i = 0; i < nrch; i++)
		rms[i] *= 1e6;
	    units = (cf.sens_mp > 0) ? "uPa" : (cf.ntone > 0) ? " uV" : "";
	} else if (maxrms < 1) {
	    for (i = 0; i < nrch; i++)
		rms[i] *= 1e3;
	    units = (cf.sens_mp > 0) ? "mPa" : (cf.ntone > 0) ? " mV" : "";
	} else {
	    units = (cf.sens_mp > 0) ? "Pa" : (cf.ntone > 0) ? " V" : "";
	}
    }
    sprintf(msg, "%s response: %s", tfstr, msg1);
    n = strlen(msg);
    if (nrch == 1) {
	rmsfmt = tf ? "; rms=%.2f%s" : "; rms=%.4g%s";
        sprintf(msg + n, rmsfmt, rms[0], units);
	if (cf.ntone == 0) {
            n = strlen(msg);
	    sprintf(msg + n, "; gd=%.3g%s", msgd[0], "ms");
	}
    } else {
	rmsfmt = tf ? "; rms=%.2f, %.2f%s" : "; rms=%.4g, %.4g%s";
	sprintf(msg + n, "; rms=%.4g, %.4g%s", rms[0], rms[1], units);
	if (cf.ntone == 0) {
            n = strlen(msg);
	    sprintf(msg + n, "; gd=%.3g, %.3g%s", msgd[0], msgd[1], "ms");
	}
    }
    prn_msg(msg);
}

int
ndp(double z)
{
    int d = 0;

    if (z == 0)
	d = 0;
    else if (z < 1)
	d = 3;
    else if (z < 10)
	d = 2;
    else if (z < 100)
	d = 1;

    return (d);
}

void
zf_msg()
{
    sprintf(msg, "zf1=%.*f zf2=%.*f (Hz) zoom=%s", 
	ndp(pf.zf1), pf.zf1, 
	ndp(pf.zf2), pf.zf2, 
	pf.zoom ? "on" : "off");
    prn_msg(msg);
}

void
zt_msg()
{
    sprintf(msg, "zt1=%.3f zt2=%.3f (sec) zoom=%s", 
	pf.zt1, pf.zt2, pf.zoom ? "on" : "off");
    prn_msg(msg);
}

void
set_crn_line(char *s)
{
    char *c;

    // remove leading spaces
    while (*s == ' ')
	s++;
    // remove comment
    c = strchr(s, ';');
    if (c != NULL)
	*c = '\0';
    // remove trailing blanks
    c = s + strlen(s);
    while (c[-1] == ' ')
	c--;
    *c = '\0';
    // save corner label
    strncpy(crn, s, 19);
}

void
get_crn_line(char *s)
{
    if (crn[0] == '\0') {
	strcpy(s, "SysRes");
    } else if (strncmp(crn, "$fn", 3) == 0) {
	basename(s, datfn, 20);
    } else {
	strncpy(s, crn, 20);
    }
}

void
delay_set_msg() 
{
    sprintf(msg, "delay_max=%g delay_range=%g delay_off=%g (ms)", 
	pf.delay_max, pf.delay_range, pf.delay_off);
    prn_msg(msg);
}

void
optlev_fit_msg() 
{
    sprintf(msg, "optlev fit: snr=%g min=%d ord=%d", 
	cf.olsnr, cf.olmin, cf.olord);
    prn_msg(msg);
}

void
optlev_lst_msg() 
{
    sprintf(msg, "optlev lst: avt=%g  beg=%g inc=%g  nlv=%g  num=%d  snr=%g  rep=%d ", 
	cf.lsavt, cf.lsbeg, cf.lsinc, cf.lsnlv, cf.lsnum, cf.lssnr, cf.lsrep);
    prn_msg(msg);
}

void
lst_sup_msg() 
{
    sprintf(msg, "optlev lst suppressor:  f3=%g  L3a=%g  L3b=%g", 
	cf.lsf3, cf.lsl3a, cf.lsl3b);
    prn_msg(msg);
}

void
ncal_set_msg() 
{
    char msg[80];
    double ms;
    int    nm;
    
    ms = pf.ofst * 1000.0 / cf.rate;
    sprintf(msg, "ncal=%d  ofst=%d (%.3g msec)", cf.ncal, pf.ofst, ms);
    if (pf.npfit > 0) {
	nm = strlen(msg);
        ms = pf.npfit * 1000.0 / cf.rate;
        sprintf(msg + nm, "  npfit=%d (%.3g msec)", pf.npfit, ms);
    }
    prn_msg(msg);
}

void
phase_set_msg() 
{
    sprintf(msg, "phase_max=%g phase_range=%g phase_off=%g (cyc)", 
    pf.phase_max, pf.phase_range, pf.phase_off);
    prn_msg(msg);
}

void
ramp_msg() 
{
    char typ[20];
    int  n;

    n = cf.rmp_typ;
    if (n < 0)
	sprintf(typ, "%s (f3 only)", ramp_name[-n]);
    else
	sprintf(typ, "%s", ramp_name[n]);
    sprintf(msg, "Ramp: bd,ed,od = %.1f, %.1f, %.1f msec; type=%s", 
	cf.beg_dur, cf.end_dur, cf.rmp_dur, typ);
    prn_msg(msg);
}

void
mouse_msg() 
{
#ifdef WIN32
    int x, y;

    mouse_position(&x, &y);
    if (describe_position(msg, x, y))
	prn_msg(msg);
#endif // WIN32
}

static void
expand_id(char *datfn)
{
    char *s, *t;

    s = strchr(datfn, '$');
    if (s) {
	if (strncmp(s, "$id", 3) == 0) {
	    t = _strdup(s + 3);
	    sprintf(s, "%s%s", idstr, t);
	    free(t);
	}
    }
}

int
get_tokens(char *s, char **left, char **right, int *eq)
{
    int nch = 0;

    while (*s && *s <= ' ') // skip over leading blanks
	s++;
    *left = s;
    while (isalnum(*s))	    // skip over first token
	s++;
    while (*s == '?')
	s++;
    nch = (int) (s - *left);
    while (*s && *s <= ' ') // skip over trailing blanks
	s++;
    if (*s == '=') {	    // if next character is equals, skip over it
	s++;
        while (*s && *s <= ' ')
	    s++;
	*eq = 1;
    } else {
	*eq = 0;
    }
    *right = s;

    return (nch);
}

int
lookup(char *string, int e)
{
    int i, rval = 0, c = 0;
    static char *commands[] = {
        // Configuration commands #0-21
        "ad", "da", "dc", "ia", "na", "ns", "oa", "sr", "st",
        "vo", "f1", "f2", "fc", "fi", "ft", "fp", "fn", "fu", "fr", "nf", 
        "no", "sa", 
        // Plot commands #22-46
        "cs", "crn", "dr", "dn", "do", "eml", "pl", "pm", "pn",
        "po", "pr", "ps", "shd", "shp", "tr", "ts", "td", "te", "vr", "SR",
        "zf1", "zf2", "zt1", "zt2", "zo", 
        // File commands #47-58
        "lf", "ra", "rc", "rd", "rl", "rs", "sv", "wa", "wc", "wd", "wh", "ws",
        // printer commands #59-63
        "PL", "PN", "PO", "PS", "PT",
        // DPOAE commands #64-82
        "avm", "ca", "cn", "dp", "dpbw", "dpdp", "dpef", "dpen", "dpfc",
        "dpft", "dpno", "dpon", "dpsb", "dpt1", "om", "os", "rjm", "rjn",
        "rjt",
        // Masking commands #83-94
        "ma1", "ma2", "mf1", "mf2", "mmd", "mmr", "mms", "mpd", "mpg", "ms",
        "mtd", "mtv",
        // Other commands, #95-119
        "a1", "a2", "be", "bp", "bd", "cd", "co", "di", "ed", "et", "ga",
        "go", "l1", "l2", "ls", "sm", "od", "pa", "pg", "q", "rt", "sk", "sl",
        "to", "vn",
        // other commands that aren't in the help #120-140
        "at", "a3", "de", "dm", "dptw", "ex", "fz", "f3", "h", "help", "ic",
        "kc", "l3", "pe", "quit", "wl", "?", "ct", "nt", "dpch", "cach",
        // new commands #141-150
	"dpst", "dprm", "dpsf", "dpfd", "dbms1", "dbms2", "dbms3", "ow", "dbg",
	"edit",
        // new commands #151-167
	"lpma1", "lpma2", "lpma3", "lpmb1", "lpmb2", "lpmb3", 
	"lpmc1", "lpmc2", "lpmc3", "lpnc1", "lpnc2", "lpnc3", 
	"lpph1", "lpph2", "lpph3", "l1eq", "acsf",
        // new commands #168-196
	"lpol", "lsol", "olmin", "olord", "olsnr",
	"lsavt", "lsbeg", "lsinc", "lsnlv", "lsnum", "lssnr", "l3eq",
	"dpcp", "dpmp", "wci", "msg", 
	"amin1", "amin2", "amin3", "amnc1", "amnc2", "amnc3",
	"after", "id", "ckfn", "lsrep", "lsf3", "lsl3a", "lsl3b",
        // new commands #197-203
	"dl", "late", "rp", "oi", "oo", "pef", "pes"
    };
    static int numcom = sizeof(commands) / sizeof(commands[0]);

    for (i = 0; (i < numcom); i++) {
        c = strlen(commands[i]);
        if((c == e) && !strncmp(commands[i],string, e)) {
            rval = i;
            break;
        }
    }

    if (i == numcom)
        rval = -1;

    return rval;
}

int
prepare_stimulus(char *msg) 
{
    if (cf.stm_typ == 6) {
        if (!*stmfn) {
	    sprintf(msg, "Please use 'rs' to specify stimulus file name.");
	    return (0);
	}
	if (!read_stim_mat(msg, stmfn, &cf)) {
	    return (0);
	}
        stim_scale(0);
	stim_ft(nsch);
	cf.da_mode = (1 << nsch) - 1;
    }
    if (!stim_compute(cf.stm_typ, msg)) {
	return (0);
    }
    return (1);
}

int
set_stimulus(int stmtyp, char *msg) 
{
    cf.stm_typ = stmtyp;
    prn_top(&cf, comment);
    if (!prepare_stimulus(msg)) {
	return (0);
    }
    return (1);
}

void
gui() 
{
    char s[256], *tu;
    char *right;
    char *left;
    double ms, sm, sr = 0, ss, t1, t2, dbmx, f, v;
    int gc = 1, n = 0, ical, avcnt = 0, eq = 0;
    int stmtyp = 0;
    int command, nch, c;
    static char *hlp_msg = "Type '?' for help.";
    static double pause_timeout = 0;    // sec 
    static double print_timeout = 2;    // sec 

    gr_init();
    drop_suid();
    if (logflg)
	log_file(1);
    if (!alloc_mem(cf.nsamp))
        prn_msg("can't allocate buffer memory");
    (void) read_config(msg, def_cfgfn, &cf);
    *comment = '\0';
    pf.zoom = 0;
    pf.ofst = 0;

    display_frame(&cf, &pf, comment, 1);
    init_sio(s);
    sprintf(msg,"%s    %s",VER, s);
    prn_msg(msg);
    while (gc) {

        gr_getline("? ", s, 80);
        if (*s == '\1') {			// EOF
            continue;
        } else if (*s == ';' || *s == '\0') {	// comment line
            continue;
	} else if (*s == CTRL_C) {		// Quit
	    gc = 0;
	    continue;
	} else if (*s == CTRL_O) {		// Left Click
	    mouse_msg();
	    continue;
	}

	nch = get_tokens(s, &left, &right, &eq);
        command = lookup(left, nch);	// call lookup function to find
					// command number
        switch(command) {
        case 167:     // the acsf command 
            if(eq) {
                cf.acsf = eval_i(right);
	    }
            sprintf(msg, "acsf=%d", cf.acsf);
            prn_msg(msg);
            break;
        case 0:     // the ad command 
            if(eq) {
                n = eval_i(right);
		if (n >= 0 && n <= 515)
		    cf.ad_mode = n;
                prn_top(&cf, comment);
	    }
            break;
        case 190:   // after pause before execution
	    while (*left && *left != ' ')
		left++;
	    while (*left == ' ')
		left++;
	    n = getnum(left, &v, " ");
	    if (n <= 0) {
		v = 0;	    // syntax error
	    } else {
		left += n;
		while (*left == ' ')
		    left++;
	    }
	    if (strlen(left) > 0) {
    		sprintf(msg, "After %.0f sec, execute %s ...", v, left);
    		c = gr_getkey(msg, v);
		if (c != CTRL_C && c != ESC)
		    gr_pbstr(left);
	    }
           break;
         case 184:     // amin1 for mod. ind. of tone 1
            if (eq)
                cf.amin[0] = eval_f(right);
            am_msg();
            break; 
        case 185:     // amin2 for mod. ind. of tone 2
            if (eq)
                cf.amin[1] = eval_f(right);
            am_msg();
            break; 
        case 186:     // amin3 for mod. ind. of tone 3
            if (eq)
                cf.amin[2] = eval_f(right);
            am_msg();
            break; 
        case 187:     // amnc1 for num. cyc. of tone 1
            if (eq)
                cf.amnc[0] = eval_f(right);
            am_msg();
            break; 
        case 188:     // amnc2 for num. cyc. of tone 2
            if (eq)
                cf.amnc[1] = eval_f(right);
            am_msg();
            break; 
        case 189:     // amnc3 for num. cyc. of tone 3
            if (eq)
                cf.amnc[2] = eval_f(right);
            am_msg();
            break; 
        case 120:     // at for attenuation something 
            if (eq) {
                ad_atten = eval_f(right);
                if (sio_present)
                    ad_atten = sio_set_att_in(ad_atten);
            }
            sprintf(msg, "input attenuation=%.1f (dB)", ad_atten);
            prn_msg(msg);
            break; 
        case 64:    // avm or average weighting mode
            if (eq)
                wtav_mode = eval_i(right);
            sprintf(msg, "weighted average mode=%d", wtav_mode);
            prn_msg(msg);
            break;
        case 95:    // a1 or attenuation of tone 1 (db)
            if (eq)
                tone_attn1 = eval_f(right);
            attn_msg(1);
            break;
        case 96:    // a2
            if (eq)
                tone_attn2 = eval_f(right);
            attn_msg(2);
            break;
        case 121:   // a3
            if (eq)
                tone_attn3 = eval_f(right);
            attn_msg(3);
            break;
        case 99:   // bd or beginning duration
            if (eq)
                cf.beg_dur = eval_f(right);
            ramp_msg();
            break;
        case 97:    // be for beep  
            gr_beep();
            break;
        case 98:    // bp for band-pass filter
            resp_bp(nrcp);
            display_frame(&cf, &pf, comment, 1);
            pf.nfft = cf.nsamp;
            pf.df_khz = (cf.rate / pf.nfft) / 1000;
	    display_fft(rf, &pf, nrcp);
            sprintf(msg, "band-pass filter from %.0f to %.0f", 
                cf.f1, cf.f2);
            prn_msg(msg);
            break;
        case 65:    // ca for calibrate
            if (eq) {
                n = eval_i(right);
  	        cf.ncal = 64;
  	        while((n > cf.ncal) && (cf.ncal < MAX_NSAMP))
  	            cf.ncal *= 2;
  	        if (cf.ncal > 0)
  	    	    cal_create(msg);
            } else {
	        cal_read(msg);
            } 
            prn_msg(msg);
            break;
        case 140:    // cach for DP envelope bandwidth (Hz)
            if (eq) {
                cf.cal_chn = eval_i(right);
                cf.cal_chn = limit(0, cf.cal_chn, nrcp - 1);
	    }
            sprintf(msg, "calibration channel = %d", cf.cal_chn);
            prn_msg(msg);
            break;
        case 100:   // cd for get or set current directory        
            if (*right >= ' ') {
                trim(s);
                gr_set_cwd(right);
	    }
	    gr_get_cwd(s, 256);
	    cd_msg(s);
            break;
        case 192:   // ckfn - check file name  
            if (*right >= ' ') {
                strip_comment(right);
	        expand_id(right);
                if (access(right, 0) == 0) {
		    exit_list();
		    sprintf(msg, "*** ERROR: file '%s' already exists!", right);
		    prn_msg(msg);
		}
	    }
            break;
        case 66:    // cn for calibrate buffer size 
            if (eq) {
                n = eval_i(right);
		if (n <= 0) {
		    cf.ncal = 0;
		} else {
  	            cf.ncal = 64;
  	            while((n > cf.ncal) && (cf.ncal < MAX_NSAMP))
  	                cf.ncal *= 2;
		}
	    }
            ncal_set_msg();
            break;
        case 101:   // co for comment
            if (eq) {
                strncpy(comment, right, 79);
                trim(comment);
	        prn_top(&cf, comment);
	    }
            break;
        case 23:    // crn for specify upper-right corner
	    if (eq) 
	        set_crn_line(right);
	    sprintf(msg, "crn = %s", crn);
	    prn_msg(msg);
            break;
        case 22:    // cs for clear screen
            pf.clear_screen = 1;
            display_frame(&cf, &pf, comment, 1);
	    break;
        case 137:    // ct for calibration tones
            if (eq) {
                f = eval_f(right);
		cal_tones(f, f, msg);
            } else {
	        cal_tones(cf.f1, cf.f2, msg);
            } 
            prn_msg(msg);
            break;
        case 1:     // da for D/A mode
            if (eq) {
                n = eval_i(right);
	        if (n >= 0 && n <= 511)
	            cf.da_mode = n;
		prn_top(&cf, comment);
            }
            break;
        case 149:    // dbg for debug mode
            if (eq)
                debug = eval_i(right);
            sprintf(msg, "debug mode=%d", debug);
            prn_msg(msg);
            break;
        case 145:   // dbms1 sets L1 msec per dB for exp. ramp
            if (eq)
                cf.dbms[0] = eval_f(right);
	    dbms_msg(1 );
            break;
        case 146:   // dbms2 sets L2 msec per dB for exp. ramp
            if (eq)
                cf.dbms[1] = eval_f(right);
	    dbms_msg(2);
            break;
        case 147:   // dbms3 sets L3 msec per dB for exp. ramp
            if (eq)
                cf.dbms[2] = eval_f(right);
	    dbms_msg(3);
            break;
        case 2:     // dc for DC component removed from response    
            if (eq)
                dc_remove = (eval_i(right) != 0);
            sprintf(msg, "dc_remove=%d", dc_remove);
            prn_msg(msg);
            break;
        case 122:     // de for delay or something
            if (eq)
                cf.delay = eval_f(s + 3) / 1000;
	    prn_top(&cf, comment);
            break;
        case 102:   // di for SIO device info
            if (eq) {
		n = eval_i(right);
		if (sio_present) {
		    sio_close();
		}
		if (n > 0) {
		    sio_set_device(n);
		    sio_set_channel_offset(cf.choi, cf.choo);
		    sio_present = sio_open();
		} else {
		    sio_close();
		    sio_present = 0;
		}
            }
	    if (sio_present) {
                sio_get_info(msg);
	    } else {
		strcpy(msg, no_dev);
	    }
            prn_msg(msg);
            break;
        case 197:    // dl - device list
            prn_devlst();
            display_frame(&cf, &pf, comment, 1);
            break;
        case 123:     // dm for delay axis maximum
            if (eq)
                pf.delay_max = eval_f(right);
            delay_set_msg();
            break;
        case 25:    // dn for for delay axis range 
            if (eq)
                pf.delay_range = eval_f(right);
            delay_set_msg();
            break;
        case 26:    // do for delay offset removed
            if (eq)
                pf.delay_off = eval_f(right);
            delay_set_msg();
            break;
        case 68:    // dpbw for DP envelope bandwidth (Hz)
            if (eq)
                pf.dpbw = eval_f(right);
            sprintf(msg, "DP envelope bandwidth = %.3f", pf.dpbw);
            prn_msg(msg);
            break;
        case 139:    // dpch for DP envelope bandwidth (Hz)
            if (eq) {
                pf.dpch = eval_i(right);
                pf.dpch = limit(0, pf.dpch, nrcp - 1);
	    }
            sprintf(msg, "DP analysis channel = %d", pf.dpch);
            prn_msg(msg);
            break;
        case 180:    // dpcp for DP closest pair
	    dp_cp(msg);
            prn_msg(msg);
	    prn_top(&cf, comment);
            break;
        case 69:    // dpdp for display 3*F1-2*F2 component also
            if (eq) {
                if (right[0] == '0' || right[0] == '1')
                    pf.dp_dsp[4] = right[0] - '0';
            }
            dp_dsp_msg();
            break;
        case 70:    // dpef for DP exponential fit type
            if (eq)
                pf.dpef = eval_i(right);
            sprintf(msg, "DP exponential fit type = %d", pf.dpef);
            prn_msg(msg);
            break;
        case 71:    // dpen for DP envelope size
            if (eq) {
                n = eval_i(right);
                if (n <= 0) {
                    pf.dpen = 0;
		} else {
                    pf.dpen = 4;
  		    while((n > pf.dpen) && (pf.dpen < cf.nsamp / 4))
  		        pf.dpen *= 2;
  		}
            }
            sprintf(msg, "DP envelope size = %d", pf.dpen);
            prn_msg(msg);
            break;
        case 72:    // dpfc for DP fit complex
            if (eq)
                pf.dpfc = eval_i(right);
            sprintf(msg, "DP fit complex = %d", pf.dpfc);
            prn_msg(msg);
            break;
        case 181:    // dpmp for DP middle pair
	    dp_mp(msg);
            prn_msg(msg);
	    prn_top(&cf, comment);
            break;
        case 143:   // dpsf for DP filter stimulus
            if (eq)
                cf.dpsf = eval_i(right);
            sprintf(msg, "DP stimulus filter = %d", cf.dpsf);
            prn_msg(msg);
            break;
        case 144:   // dpfd for DP fit duration
            if (eq) {
                ms = eval_f(right);
	        pf.npfit = nint(ms * cf.rate / 1000);
            }
	    ncal_set_msg();
            break;
        case 73:    // dpft for DP filter type
            if (eq)
                 pf.dpft = eval_i(right);
            sprintf(msg, "DP filter type = %d", pf.dpft);
            prn_msg(msg);
            break;
        case 124:   // dpfw for DP filter width
            if (eq) {
                pf.dpfw = eval_i(right);
                sprintf(msg, "DP filter width = %d", pf.dpfw);
                prn_msg(msg);
            }
            break;
        case 74:    // dpno for set DP noise display
            if (eq) {
                if (right[0] == '0' || right[0] == '1')
                    pf.dp_dsp[3] = right[0] - '0';
            }
            dp_dsp_msg();
            break;
        case 75:    // dpon for set DP order number
            if (eq) {
                pf.dpon = (short) eval_f(right);
                pf.dpon = limit(-5, pf.dpon, 2);
            }
            dpon_msg();
            break;
        case 76:    // dpsb for DP noise sidebands
            if (eq) 
		pf.dpsb = dpn_sb = eval_f(right);
            sprintf(msg, "DP noise sideband = %.1f%%", dpn_sb);
            prn_msg(msg);
            break;
        case 142:    // dprm for DP remove mean
            if (eq) 
		pf.dprm = eval_i(right);
            sprintf(msg, "DP remove mean = %d", pf.dprm);
            prn_msg(msg);
            break;
        case 141:    // dpst for DP stimulus
	    dp_stim(msg);
            prn_top(&cf, comment);
            prn_msg(msg);
            break;
        case 77:    // dpt1 for DP initial time-constant (sec)
            if (eq)
                pf.dpt1 = eval_f(right);
            sprintf(msg, "DP initial time-constant = %.3f sec", pf.dpt1);
            prn_msg(msg);
            break;
        case 67:    // dp for display distortion product and stuff
            if (cf.ncal <= 0) {
                sprintf(msg, "Please calibrate first using 'ca'.");
                prn_msg(msg);
                break;
            }
	    if (pf.dpen > (cf.nsamp / (NDP * 2))) {
                sprintf(msg, "Please reduce 'dpen'.");
                prn_msg(msg);
                break;
            }
            pf.nc = (cf.nsamp - pf.ofst) / cf.ncal;
            pf.i1 = nint(cf.ncal * cf.f1 / cf.rate);
            pf.i2 = nint(cf.ncal * cf.f2 / cf.rate);
            pf.dp = 1;
            pf.nfft = pf.dpen ? cf.nsamp / pf.dpen : cf.ncal;
            pf.df_khz = (cf.rate / pf.nfft) / 1000;
            dp_compute();
            if (eq) {
                ical = eval_i(right);
                ical = limit(0, ical, pf.nc - 1);
                pf.nc = 1;
                display_frame(&cf, &pf, comment, 1);
                display_fft(rf + ical * cf.ncal, &pf, nrcp);
            } else {
                ical = 0;
                pf.time_axis = 1;
	        if (pf.dpef > 0 && pf.dpfc > 0 && pf.eml == 0)
	            pf.eml = 1;
                display_frame(&cf, &pf, comment, 1);
                display_fft(rf, &pf, nrcp);
            }
            if (pf.time_axis) {
                if (pf.dpef == 1 || pf.dpef == 2) {
	            dbmx = pf.dbvref - db_ref();
                    dp_exp_fit_msg(datfn, dbmx, pf.dbrange, pf.dpon);
	        }
            } else if (pf.nc == 1) {
                 n = pf.ofst + cf.ncal * ical;
                 t1 = n * 1000 / cf.rate;
                 t2 = t1 + cf.ncal * 1000 / cf.rate;
                 tu = "msec";
                 if (t2 > 1) 
                 {
                    t1 /= 1000;
                     t2 /= 1000;
                     tu = "sec";
                 }
                 sprintf(msg, 
                        "L1=%.1f L2=%.1f Ld=%.1f Ln=%.1f %s "
                        "at %.4g-%.4g %s (nn=%d)",
                        pf.lv[1], pf.lv[2], pf.lv[0], pf.lv[3], 
                        cf.sens_mp ? "dBSPL" : "dBV", 
                        t1, t2, tu, pf.nnsb);
                 prn_msg(msg);
            }
            pf.time_axis = 0;
            pf.dp = 0;
            break;
        case 24:    // dr for dynamic range
            if (eq)
                pf.dbrange = eval_f(right);
            db_msg();
            break;
        case 103:   // ed for end duration (msc)
            if (eq)
                cf.end_dur = eval_f(right);
            ramp_msg();
            break;
        case 150:   // edit - edit filename
            if (right[0] >= ' ')
                edit_fn(right);
            break;
        case 27:    // eml for extra message lines
            if (eq)
			    pf.eml = (short) eval_i(right);
		    sprintf(msg, "extra message lines = %d", pf.eml);
		    prn_msg(msg);
            break;
        case 104:   // et forelapsed time
            if (eq)
                stime = gr_etime() - eval_f(right);
            sprintf(msg, "elapsed time = %.3f sec", gr_etime() - stime);
            prn_msg(msg);
        case 125:     // ex and exit
            gr_exit_incl();
	        break;
       case 10:     // f1
            if (eq)
                cf.f1 = eval_f(right);
            if (cf.ncal > 0)
                cf.f1 = adjust_f(cf.f1);
            freq_msg(1);
            break;
       case 11:     // f2
            if (eq)
                cf.f2 = eval_f(right);
            if (cf.ncal > 0)
                cf.f2 = adjust_f(cf.f2);
            freq_msg(2);
            break;
       case 127:      // f3
            if (eq)
                cf.f3 = eval_f(right);
            if (cf.ncal > 0)
                cf.f3 = adjust_f(cf.f3);
            freq_msg(3);
            break;
        case 18:    // fr for frequency response (use last stim)
        case 12:    // fc    
            if (left[1] == 'c')
                stmtyp = 0;		// chirp stimulus
        case 13:    // fi
            if (left[1] == 'i')
                stmtyp = 1;		// impulse stimulus
        case 126:     // fz for the z stim
            if (left[1] == 'z')
                stmtyp = 2;		// zero stimulus */
        case 14:    // ft
            if (left[1] == 't')
                stmtyp = 3;		// tone stimulus 
        case 15:    // fp
            if (left[1] == 'p')
                stmtyp = 4;		// tone-pair stimulus 
        case 16:    // fn
            if (left[1] == 'n')
                stmtyp = 5;		// random-noise stimulus 
        case 17:    // fu
            if (left[1] == 'u') {
                stmtyp = 6;		// used-defined stimulus 
                if (eq) {
                    strcpy(stmfn, right);
                    defext(stmfn, ".mat");
		}
            }
            // here we start playing the selected stim type
	    if (!set_stimulus(stmtyp, msg)) {
                prn_msg(msg);
                break;
            }
            if (!sio_present) {
                prn_msg(no_dev);
                break;
            }
            pf.time_axis = 0;
            display_frame(&cf, &pf, comment, 1);
            play_stim(0, 1, 1, &avcnt);
            resp_compute();
            pf.nfft = cf.nsamp;
            pf.df_khz = (cf.rate / pf.nfft) / 1000;
	    display_fft(rf, &pf, nrcp);
	    sprintf(msg, "avcnt=%d", avcnt);
            response_msg(msg, 1);
            break;  // now we finally break
        case 105:   // ga for gap between stimulus repetitions
            if (eq)
	    {
                cf.gap = eval_f(right);
    	        prn_top(&cf, comment);
	    }
            break;
        case 106:   // go for average and show time-domain response
            if (!prepare_stimulus(msg)) {
                prn_msg(msg);
                break;
            }
            if (!sio_present) {
	        prn_msg(no_dev);
                break;
	    }
	    play_stim(0, 1, 1, &avcnt);
            resp_compute();
            clr_all();
	    if (display_wave(rt, cf.nsamp, nrcp, cf.rate, &pf)) {
		if (cf.stm_typ == 7) {
		    masking_msg(comment, msg);
		    prn_msg(msg);
		} else {
		    sprintf(msg, "avcnt=%d", avcnt);
		    response_msg(msg, 0);
		} 
		prn_top(&cf, comment);
		if (cf.stm_typ == 7)
		    strcpy(comment, "");
	    } else {
		prn_msg("can't display response");
	    }
            break;
        case 136:     // for the "?"
        case 129:     // also one for help
        case 128:     // h
            prn_help();
            display_frame(&cf, &pf, comment, 1);
            break;
        case 3:     // ia for input attenuation
            if (eq) 
            {
                ad_atten = eval_f(right);
                if (sio_present)
                    ad_atten = sio_set_att_in(ad_atten);
            }
            sprintf(msg, "input attenuation=%.1f (dB)", ad_atten);
            prn_msg(msg);
            break;
        case 130:     // ic
            if (eq)
                n = eval_i(right);
            else
	        n = 0;
	    set_ic(n);
            sprintf(msg, "initial color=%d", n);
            prn_msg(msg);
            break; 
        case 191:     // id
	    if (eq) {
		if (*right == '?') {
		    gr_getprompt("? id=", right, 20);
		}
		strip_comment(right);
	        strncpy(idstr, right, 19);
	    }
            sprintf(msg, "id=%s", idstr);
            prn_msg(msg);
            break; 
        case 131:     // kc for key-code...this does something I guess
            if (eq) {
	        sprintf(msg, "key-code=%d", s[3]);
	        prn_msg(msg);
	    }
            break;
            
        case 107:   // l1 for level of tone 1 (dB)
            if (eq)
                cf.l1 = eval_f(right);
            levl_msg(1);
            break;
        case 166:   // l1eq for tone level 1 as function of 2
	    l1eq(right, 1);
            break;
        case 179:   // l3eq for tone level 3 as function of 2
	    l3eq(right, 1);
            break;
        case 108:   // l2 for level of tone 2
            if (eq)
                cf.l2 = eval_f(right);
            levl_msg(2);
            break;
        case 132:     // l3 for level of tone 3
            if (eq)
                cf.l3 = eval_f(right);
            levl_msg(3);
            break; 
        case 198:     // late - set device latency
            if (eq) {
                n = eval_i(right);
	    } else {
		n = SIO_GET_LATENCY;
	    }
            n = sio_set_latency(n);
            sprintf(msg, "latency=%d", n);
            prn_msg(msg);
            break; 
        case 151:     // lpma1 for mod. amp. of tone 1
            if (eq)
                cf.lpma[0] = eval_f(right);
            lp_msg(0);
            break; 
        case 152:     // lpma2 for mod. amp. of tone 2
            if (eq)
                cf.lpma[1] = eval_f(right);
            lp_msg(1);
            break; 
        case 153:     // lpma3 for mod. amp. of tone 3
            if (eq)
                cf.lpma[2] = eval_f(right);
            lp_msg(2);
            break; 
        case 154:     // lpmb1 for mod. amp. of tone 1
            if (eq)
                cf.lpmb[0] = eval_f(right);
            lp_msg(0);
            break; 
        case 155:     // lpmb2 for mod. amp. of tone 2
            if (eq)
                cf.lpmb[1] = eval_f(right);
            lp_msg(1);
            break; 
        case 156:     // lpmb3 for mod. amp. of tone 3
            if (eq)
                cf.lpmb[2] = eval_f(right);
            lp_msg(2);
            break; 
        case 157:     // lpmc1 for mod. amp. of tone 1
            if (eq)
                cf.lpmc[0] = eval_f(right);
            lp_msg(0);
            break; 
        case 158:     // lpmc2 for mod. amp. of tone 2
            if (eq)
                cf.lpmc[1] = eval_f(right);
            lp_msg(1);
            break; 
        case 159:     // lpmc3 for mod. amp. of tone 3
            if (eq)
                cf.lpmc[2] = eval_f(right);
            lp_msg(2);
            break; 
        case 160:     // lpnc1 for num. cyc. of tone 1
            if (eq)
                cf.lpnc[0] = eval_f(right);
            lp_msg(0);
            break; 
        case 161:     // lpnc2 for num. cyc. of tone 2
            if (eq)
                cf.lpnc[1] = eval_f(right);
            lp_msg(1);
            break; 
        case 162:     // lpnc3 for num. cyc. of tone 3
            if (eq)
                cf.lpnc[2] = eval_f(right);
            lp_msg(2);
            break; 
        case 168:     // lpol - find optimal level
            dp_lpol(right, msg);
	    prn_msg(msg);
            break; 
        case 163:     // lpph1 for mod. pha. of tone 1
            if (eq)
                cf.lpph[0] = eval_f(right);
            lp_msg(0);
            break; 
        case 164:     // lpph2 for mod. pha. of tone 2
            if (eq)
                cf.lpph[1] = eval_f(right);
            lp_msg(1);
            break; 
        case 165:     // lpph3 for mod. pha. of tone 3
            if (eq)
                cf.lpph[2] = eval_f(right);
            lp_msg(2);
            break; 
        case 47:    // lf - log file
            if (eq)
                log_file(eval_i(right));
            break;
        case 109:   // ls - list file names
            if (right[0] >= ' ')
                list_fn(right);
	    else
                list_fn("*.*");
            break;
        case 173:   // lsavt - opt. lev. lst averaging time
            if (eq)
                cf.lsavt = eval_f(right);
            optlev_lst_msg();
            break;
        case 174:   // lsbeg - opt. lev. lst L2 begin
            if (eq)
                cf.lsbeg = eval_f(right);
            optlev_lst_msg();
            break;
        case 194:   // lsf3 - emav lst file suppr. freq.
            if (eq)
                cf.lsf3 = eval_f(right);
            lst_sup_msg();
            break;
        case 175:   // lsinc - opt. lev. lst L2 increment
            if (eq)
                cf.lsinc = eval_f(right);
            optlev_lst_msg();
            break;
        case 176:   // lsnlv - opt. lev. lst noise level
            if (eq)
                cf.lsnlv = eval_f(right);
            optlev_lst_msg();
            break;
        case 195:   // lsl3a - emav lst file suppr. level a
            if (eq)
                cf.lsl3a = eval_f(right);
            lst_sup_msg();
            break;
        case 196:   // lsl3b - emav lst file suppr. level b
            if (eq)
                cf.lsl3b = eval_f(right);
            lst_sup_msg();
            break;
        case 177:   // lsnum - opt. lev. list number of conditions
            if (eq)
                cf.lsnum = (short) eval_i(right);
            optlev_lst_msg();
            break;
        case 193:   // lsrep - emav lst repeats
            if (eq)
                cf.lsrep = (short) eval_i(right);
            optlev_lst_msg();
            break;
        case 169:     // lsol - list optimal level
	    trim(right);
            dp_lsol(right, msg);
	    prn_msg(msg);
            break; 
        case 178:   // lssnr - opt. lev. lst SNR
            if (eq)
                cf.lssnr = eval_f(right);
            optlev_lst_msg();
            break;
        case 183:   // msg - display message
            if (eq) {
                strcpy(msg, right);
	    } else {
		*msg = '\0';
	    }
                prn_msg(msg);
            break;
        case 4:     // na for number of averages
            if (eq) {
                n = eval_i(right);
		    if (n > 0)
		        cf.nav = n;
		    prn_top(&cf, comment);
            }
            break;
        case 19:    // nf for noise floor
            if (eq)
                nsab = eval_i(right);
            if (cf.nsamp > 0) {
                if (sio_present) {
                    if (!prepare_stimulus(msg)) {
                        prn_msg(msg);
                        break;
                    }
		    pf.time_axis = 0;
		    display_frame(&cf, &pf, comment, 1);
                    n = spectral_average(0);
                    pf.nfft = cf.nsamp;
                    pf.df_khz = (cf.rate / pf.nfft) / 1000;
                    display_fft(rf, &pf, nrcp);
                    sprintf(msg, "#spec_avg_blks=%d/%d", n, nsab);
	            response_msg(msg, 1);
                } else {
	            strcpy(msg, no_dev);
	            prn_msg(msg);
                }
	    }
            break;
        case 20:    // no for normalize to save response
            strcpy(nrmfn, (eq) ? (right) : def_nrmfn);
            defext(nrmfn, ".mat");
            if (!read_stim_mat(msg, nrmfn, &cf)) {
                prn_msg(msg);
                break;
            }
	    stim_ft(nsch);
	    resp_norm();
            pf.nfft = cf.nsamp;
            pf.df_khz = (cf.rate / pf.nfft) / 1000;
	    display_fft(rf, &pf, nrcp);
            prn_msg("normalized response");
	    break;
        case 5:     // ns for number of samples
            if (eq) {
                n = eval_i(right);
	        cf.nsamp = 64;
	        while((n > cf.nsamp) && (cf.nsamp < MAX_NSAMP))
	            cf.nsamp *= 2;
	        prn_top(&cf, comment);
                if (!alloc_mem(cf.nsamp))
                    prn_msg("can't allocate buffer memory");
            }
            break;
        case 138:    // nt
            if (eq) {
                ntwb = eval_i(right);
            }
            sprintf(msg, "nt=%d", ntwb);
            prn_msg(msg);
            break;        
	case 6:     // oa for output attenuation (dB)
            if (eq) {
                da_atten = eval_f(right);
                if (sio_present)
                    da_atten = sio_set_att_out(da_atten);
            }
            sprintf(msg, "output attenuation=%.1f (dB)", da_atten);
            prn_msg(msg);
            break;
        case 111:   // od for ramp onset duration
            if (eq)
                cf.rmp_dur = eval_f(right);
            ramp_msg();
            break;
        case 200:   // oi for channel offset input
            if (eq)
                cf.choi = eval_i(right);
            cho_msg();
            break;
        case 201:   // oo for channel offset output
            if (eq)
                cf.choo = eval_i(right);
            cho_msg();
            break;
        case 170:   // olmin - opt. levl. L2 minimum
            if (eq)
                cf.olmin = (short) eval_i(right);
            optlev_fit_msg();
            break;
        case 171:   // olord - opt. levl. polynomial order
            if (eq)
                cf.olord = (short) eval_i(right);
            optlev_fit_msg();
            break;
        case 172:   // olsnr - opt. levl. SNR threshold
            if (eq)
                cf.olsnr = eval_f(right);
            optlev_fit_msg();
            break;
        case 78:    // om for offset for DP envelope analysis
            if (eq) {
                ms = eval_f(right);
	        pf.ofst = nint(ms * cf.rate / 1000);
            }
	    ncal_set_msg();
            break;
        case 79:    // os for offset for DP envelope analysis
            if (eq) 
                pf.ofst = eval_i(right);
	    ncal_set_msg();
            break;
        case 148:   // ow for overwriting data files
            if (eq) 
                cf.ovrwrt = eval_i(right);
            sprintf(msg, "over write (data file) = %d", cf.ovrwrt);
            prn_msg(msg);
            break;
        case 112:   // pa for pause (with message)
           if (eq) {
	       trim(right);
               sprintf(msg, "[pause]  %s", right);
            } else {
               sprintf(msg, "[pause]");
	    }
           c = gr_getkey(msg, pause_timeout);
           if (c == CTRL_C || c == ESC)     // might have a problem here
               gr_exit_incl();
           gr_prcl("...");
           break;
       case 133:     // pe
           pf.clear_screen = 1;
           display_frame(&cf, &pf, comment, 1);
           break; 
       case 202:     // pef
            if (eq) 
                cf.pefr = eval_f(right);
            sprintf(msg, "chirp pre-ephasis: f=%.0f (kHz) s=%.0f (dB/oct)", 
                cf.pefr, cf.pesl);
            prn_msg(msg);
            break;
           break; 
       case 203:     // pes
            if (eq) 
                cf.pesl = eval_f(right);
            sprintf(msg, "chirp pre-ephasis: f=%.0f (kHz) s=%.0f (dB/oct)", 
                cf.pefr, cf.pesl);
            prn_msg(msg);
           break; 
        case 113:   // pg for pregap before I/O
           if (eq)
                cf.pregap = eval_f(right);
            sprintf(msg, "pregap= %.3g sec", cf.pregap);
            prn_msg(msg);
            break;
        case 28:    // pl for toggle log/linear frequency scale
	    if (eq)
		pf.log_axis = eval_i(right);
	    else
		pf.log_axis = !pf.log_axis;
            display_frame(&cf, &pf, comment, 1);
            pf.nfft = cf.nsamp;
            pf.df_khz = (cf.rate / pf.nfft) / 1000;
	    display_fft(rf, &pf, nrcp);
            break;
        case 29:    // pm for phase axis maximum
            if (eq)
                pf.phase_max = eval_f(right);
	    phase_set_msg();
            break;
        case 30:    // pn for phase axis range
            if (eq)
                pf.phase_range = eval_f(right);
            phase_set_msg();
            break;
        case 31:    // po for phase offset removed
            if (eq)
                pf.phase_off = eval_f(right);
            phase_set_msg();
            break;
        case 32:    // pr for plot response vs frequency
            display_frame(&cf, &pf, comment, 1);
            pf.nfft = cf.nsamp;
            pf.df_khz = (cf.rate / pf.nfft) / 1000;
	    display_fft(rf, &pf, nrcp);
            break;
        case 33:    // ps for plot stimulus vs frequency
            display_frame(&cf, &pf, comment, 0);
            pf.nfft = cf.nsamp;
            pf.df_khz = (cf.rate / pf.nfft) / 1000;
            pf.dbvpu = -20 * log10(2.0 / cf.nsamp);
            display_fft(sf, &pf, nsch);
            break;
        case 59:    // PL for printer lebel
            if (eq)
                strcpy(cf.prn_label, right);
            trim(cf.prn_label);
            if (*cf.prn_label == '\0')
                strcpy(cf.prn_label, "                 ");
            sprintf(msg, "Print Label: %s", cf.prn_label);
            prn_msg(msg);
            break;
        case 60:    // PN for printer name
            if (eq)
                strcpy(cf.prn_name, right);
            trim(cf.prn_name);
            if (*cf.prn_name == '\0')
                strcpy(cf.prn_name, def_prnfn);
            sprintf(msg, "Printer: name=%s orientation=%s type=%s", 
                    cf.prn_name, po[cf.prn_orient], pt[cf.prn_type]);
            prn_msg(msg);
            break;
        case 61:    // PO for print orientation
            if (eq)
                cf.prn_orient = eval_i(right);
            cf.prn_orient = limit(0, cf.prn_orient, 1);
            if (*cf.prn_name == '\0')
                strcpy(cf.prn_name, def_prnfn);
            sprintf(msg, "Printer: name=%s orientation=%s type=%s", 
                    cf.prn_name, po[cf.prn_orient], pt[cf.prn_type]);
            prn_msg(msg);
            break;
        case 62:    // PS for print screen (=printer_name)
            if (!gr_can_print()) {
                strcpy(msg, "Can't print screen in this graphics mode.");
	    } else {
                if (eq)
                    strcpy(cf.prn_name, right);
                trim(cf.prn_name);
                if (*cf.prn_name == '\0')
                    strcpy(cf.prn_name, def_prnfn);
                if (strncmp("$vn", cf.prn_label, 3) == 0)
                    strcpy(msg, VER);
                else
                    strcpy(msg, cf.prn_label);
                gr_prcl(msg);
                if (print_screen(cf.prn_name, "SysRes screen dump", 
                    cf.prn_orient, cf.prn_type))
                    sprintf(msg, "Error printing screen to '%s'.", cf.prn_name);
                else
                    sprintf(msg, "Printed screen to '%s'.", cf.prn_name);
                c = gr_getkey(msg, print_timeout);
                if (c == CTRL_C || c == ESC)
                    break;
            }
            break;
        case 63:     // PT for printer type
            if (eq)
		cf.prn_type = eval_i(right);
            cf.prn_type = limit(0, cf.prn_type, 2);
            sprintf(msg, "Printer: name=%s orientation=%s type=%s", 
                    cf.prn_name, po[cf.prn_orient], pt[cf.prn_type]);
	    prn_msg(msg);
            break;
        case 134:   // for "quit"
        case 114:   // q
            gc = 0;
            break;
        case 48:    // ra for read ASCII data file
            strcpy(datfn, (eq) ? (right) : def_ascfn);
	    strip_comment(datfn);
	    expand_id(datfn);
            defext(datfn, ".txt");
            if (!read_data_ascii(msg, datfn, &cf)) {
                prn_msg(msg);
                break;
            }
            resp_vpu();
	    resp_ft(nrcp);
            pf.nfft = cf.nsamp;
            pf.df_khz = (cf.rate / pf.nfft) / 1000;
            prn_top(&cf, comment);
            prn_msg(msg);
            break;
        case 49:    // rc for read configuration
            strcpy(cfgfn, (eq) ? (right) : def_cfgfn);
            if (*cfgfn == '\0')
                strcpy(cfgfn, def_cfgfn);
            defext(cfgfn, ".cfg");
            if (!read_config(msg, cfgfn, &cf)) {
                prn_msg(msg);
                break;
            }
            prn_top(&cf, comment);
            prn_msg(msg);
            break;
        case 50:    // rd for read data file (matlab)
            strcpy(datfn, (eq) ? (right) : def_datfn);
	    strip_comment(datfn);
	    expand_id(datfn);
            defext(datfn, ".mat");
            if (!read_data_mat(msg, datfn, &cf)) {
                prn_msg(msg);
                break;
            }
            resp_vpu();
            resp_ft(nrcp);
            pf.nfft = cf.nsamp;
            pf.df_khz = (cf.rate / pf.nfft) / 1000;
            prn_top(&cf, comment);
            prn_msg(msg);
            break;
        case 80:    // rjm for reject mode
            if (right[0] !='\0')
                reject_mode = eval_i(right);
            sprintf(msg, "reject: nrej=%d, mode=%d thresh=%.1f", 
                    nrej, reject_mode, reject_thresh);
            prn_msg(msg);
            break;
        case 81:    // rjn for max num of rejects
            if (eq)
                nrej = eval_i(right);
            sprintf(msg, "reject: nrej=%d, mode=%d thresh=%.1f", 
                    nrej, reject_mode, reject_thresh);
            prn_msg(msg);
            break;
        case 82:    // rjt for reject threshold
            if (eq)
                reject_thresh = eval_f(right);
            sprintf(msg, "reject: nrej=%d, mode=%d thresh=%.1f", 
                    nrej, reject_mode, reject_thresh);
            prn_msg(msg);
            break;
        case 51:    // rl for read list of commands from file
            if (eq) {
                strcpy(lstfn, right);
                defext(lstfn, ".lst");
		if (!read_list(msg))
		    prn_msg(msg);
	    } else {
		prn_msg("supply a valid list filename");
	    }
            break;
        case 199:   // rp reset parameters
	    defpar();
	    init_sio(s);
            pf.clear_screen = 1;
            display_frame(&cf, &pf, comment, 1);
	    sprintf(msg,"%s    %s",VER, s);
	    prn_msg(msg);
            break;
        case 52:    // for rs for read stimulus file (matlab)
            if (eq) {
                strcpy(stmfn, right);
                defext(stmfn, ".mat");
            }
            stmtyp = 6;		// used-defined stimulus
	    if (!set_stimulus(stmtyp, msg)) {
		prn_msg(msg);
		break;
	    }
            clr_all();
            prn_top(&cf, comment);
            display_wave(st, cf.nsamp, nsch, cf.rate, &pf);
            break;
        case 115:   // rt for ramp type
            if (eq) {
                n = eval_i(right);
		cf.rmp_typ = limit(1 - nrt, n, nrt - 1);
	    }
            ramp_msg();
            break;
        case 21:    // sa for spectral average
            if (eq)
                nsab = eval_i(right);
            display_frame(&cf, &pf, comment, 1);
            if (cf.nsamp > 0) {
                if (sio_present) {
                    if (!prepare_stimulus(msg)) {
                        prn_msg(msg);
                        break;
                    }
		    pf.time_axis = 0;
		    display_frame(&cf, &pf, comment, 1);
                    n = spectral_average(1);
                    pf.nfft = cf.nsamp;
                    pf.df_khz = (cf.rate / pf.nfft) / 1000;
                    display_fft(rf, &pf, nrcp);
                    sprintf(msg, "#spec_avg_blks=%d/%d", n, nsab);
	            response_msg(msg, 1);
                } else {
                    strcpy(msg, no_dev);
                    prn_msg(msg);
                }
            }
            break;
        case 34:    // shd for show delay
            if (eq)
                pf.show_delay = eval_i(right);
            sprintf(msg, "show_delay=%d  show_phase=%d", 
                    pf.show_delay, pf.show_phase);
            prn_msg(msg);
            break;
        case 35:    // shp for show phase
            if (eq)
                pf.show_phase = eval_i(right);
            sprintf(msg, "show_delay=%d  show_phase=%d", 
                    pf.show_delay, pf.show_phase);
            prn_msg(msg);
            break;
        case 116:   // sk for number of reps to skip
            if (eq)
                swpskp = eval_f(right);
            sprintf(msg, "skip %.2f sweeps", swpskp);
            prn_msg(msg);
            break;
        case 117:   // sl for sleep for N seconds
            if (*right != '\0') {
                ss = eval_f(right);
                if (ss > 0) {
	                gr_prcl("...");
	                gr_sleep(ss);
	            }
            } else {
                sprintf(msg, "No seconds specified");
                prn_msg(msg);
            }
            break;
        case 110:   // sm for sensitivity of microphone
            if (eq) {
                sm = eval_f(right);
                if (cf.sens_mp != sm) {
		    if ((cf.sens_mp > 0) && (sm > 0) && (cf.ntone > 0))
			 pf.dbvref -= 20 * log10(cf.sens_mp / sm);
                    cf.sens_mp = sm;
                    pf.clear_screen = 1;
                    display_frame(&cf, &pf, comment, 1);
                }
            }
            sprintf(msg, "microphone sensitivity=%.3f (V/Pa)", cf.sens_mp);
            prn_msg(msg);
            break;
        case 7:     // sr for sample rate
            if (eq) {
                sr = eval_f(right);
                if (sr > 0)
		    cf.rate = sio_present ? sio_set_rate(sr) : sr;
                pf.clear_screen = 1;
                display_frame(&cf, &pf, comment, 1);
            }
            break;
        case 8:     // st for stimulus type
            if (eq) {
                stmtyp = eval_i(right);
	    }
            if (!set_stimulus(stmtyp, msg)) {
                prn_msg(msg);
            }
            break;
        case 53:    // sv for sample value for 1 volt
            if (eq)
                samp_val = eval_f(right);
            if (samp_val == 0)
                samp_val = 1;
            sprintf(msg, "sample_value %.0f = 1 V", samp_val);
            prn_msg(msg);
            break;
        case 41:    // SR for SPL reference (dBSPL)
            if (cf.sens_mp <= 0) {
                sprintf(msg, "Set 'sm' (sensitivity of microphone) first.");
                prn_msg(msg);
            } else {
		cf.ntone = 1;
                if (eq) 
                    pf.dbvref = eval_f(right) + db_ref();
                db_msg();
            }
            break;
        case 38:    // td for tick direction
            if (eq)
                pf.tic_dir = eval_i(right);
            sprintf(msg, "tick direction = %d (%sward)", pf.tic_dir,
                    pf.tic_dir ? "out" : "in");
            prn_msg(msg);
            break;
        case 39:    // te for plot reverse-time-energy vs time
            clr_all();
            prn_top(&cf, comment);
            display_rte(rt, cf.nsamp, nrcp, cf.rate, &pf);
            break;
        case 118:   // to for time-out pause after N seconds
            if (eq)
                pause_timeout = eval_f(right);
            sprintf(msg, "pause time-out = %.3f sec", pause_timeout);
            prn_msg(msg);
            break;
        case 36:    // tr for plot response vs time
            clr_all();
            prn_top(&cf, comment);
            display_wave(rt, cf.nsamp, nrcp, cf.rate, &pf);
            break;
        case 37:    // ts for plot stimulus vs time
            clr_all();
            prn_top(&cf, comment);
            display_wave(st, cf.nsamp, nsch, cf.rate, &pf);
            break;
        case 119:   // vn for version number
            prn_msg(VER);
            break;
        case 9:     // vo for volts (stim amplitude)
            if (eq) {
                cf.volts = eval_f(right);
                prn_top(&cf, comment);
            }
            break;
        case 40:    // vr for voltage reference (dBV)
            if (eq)
                pf.dbvref = eval_f(right) + db_ref();
            db_msg();
            break;
        case 54:    // wa for write ASCII data file
            if (eq) {
                strcpy(datfn, right);
		strip_comment(datfn);
		expand_id(datfn);
                defext(datfn, ".txt");
                if (!chk_overwrite(datfn))
                 	break;
            } else {
                strcpy(datfn, def_ascfn);
	    }
            if (!write_data_ascii(msg, datfn, &cf)) {
                prn_msg(msg);
                break;
            }
            prn_msg(msg);
            break;
        case 55:    // wc for write configuration
            strcpy(cfgfn, (eq) ? (right) : def_cfgfn);
            if (*cfgfn == '\0')
                strcpy(cfgfn, def_cfgfn);
            defext(datfn, ".cfg");
            if (!chk_overwrite(cfgfn))
                break;
            if (!write_config(msg, cfgfn, &cf)) {
                prn_msg(msg);
                break;
            }
            prn_msg(msg);
            break;
        case 56:    // wd for write data file (Matlab format)
            if (eq) {
                strcpy(datfn, right);
		strip_comment(datfn);
		expand_id(datfn);
                defext(datfn, ".mat");
                if (!chk_overwrite(datfn))
                    break;
            } else {
                strcpy(datfn, def_datfn);
	    }
            if (!write_data_mat(msg, datfn, &cf)) {
                prn_msg(msg);
                break;
            }
            prn_msg(msg);
            break;
        case 57:    // wh is write help file (help.txt)
            write_help(help_fn, msg);
            prn_msg(msg);
            break;
        case 135:     // w1
            write_list(msg);
            prn_msg(msg);
            break; 
        case 182:     // wci
            if (eq) {
                strcpy(nfofn, right);
            }
            write_nfo_file(nfofn);
	    sprintf(msg, "cardinfo written to %s", nfofn);
            prn_msg(msg);
            break; 
        case 58:    // ws for write stim file (matlab)
            if (eq) {
                strcpy(stmfn, right);
                defext(stmfn, ".mat");
            }
            if (!*stmfn) {
                prn_msg("Please specify stimulus file name.");
                break;
            }
            if (!chk_overwrite(stmfn))
                break;
            stim_scale(1);
            if (!write_stim_mat(msg, stmfn, &cf)) {
                prn_msg(msg);
                break;
            }
            stim_scale(0);
            prn_msg(msg);
            break;
        case 42:    // zf1 for zoom frequency 1
            if (eq) {
                 pf.zf1 = eval_f(right);
                 pf.zoom = 1;
            }
            zf_msg();
            break;
        case 43:    // zf2 for zoom freq 2
            if (eq) {
                pf.zf2 = eval_f(right);
                pf.zoom = 1;
            }
            zf_msg();
            break;
        case 44:    // zt1 for zoom time 1
            if (eq) {
                pf.zt1 = eval_f(right);
                pf.zoom = 1;
            }
            zt_msg();
            break;
        case 45:    // zt2 for zoom time 2
            if (eq) {
                pf.zt2 = eval_f(right);
                pf.zoom = 1;
            }
            zt_msg();
            break;
        case 46:    // zo for zoom on/off (toggle)
            if (eq) {
                if (right[0] == '0')
                    pf.zoom = 0;
                else if (right[0] == '1') 
                    pf.zoom = 1;
            } else {
	            pf.zoom = !pf.zoom;
	    }
            sprintf(msg, "zoom=%s", pf.zoom ? "on" : "off");
            prn_msg(msg);
            break;
        case 83:    // ma1
        case 84:    // ma2
        case 85:    // mf1
        case 86:    // mf2
        case 87:    // mmd
        case 88:    // mmr
        case 89:    // mms
        case 90:    // mpd
        case 91:    // mpg
        case 92:    // ms
        case 93:    // mtd
        case 94:    // mtv
	    masking_query(s);
	    cf.nsamp = 4096;
	    cf.da_mode = 3;
	    stmtyp = 7;
	    if (!set_stimulus(stmtyp, msg)) 
		prn_msg(msg);
	    clr_all();
	    display_wave(st, cf.nsamp, nsch, cf.rate, &pf);
	    masking_msg(comment, msg);
	    prn_top(&cf, comment);
            break;
        default:
	    if (!eq && get_usr_var(left, &v, msg)) {
		prn_msg(msg);
	    } else if (eq && set_usr_var(left, right, msg)) {
		prn_msg(msg);
	    } else {
		strcpy(msg, s);
		defext(msg, ".lst");
		if (access(msg, 0) == 0) {
		    strcpy(lstfn, msg);
		    if (!read_list(msg))
			prn_msg(msg);
		} else {
		    prn_msg(hlp_msg);
		}
	    }
        }   // PHEWWW!....end of the switch

    }   // end of the while loop

    log_file(0);
    gr_close();
    if (sio_present) {
        sio_close();
    }
    free_mem();
    clr_hist();
}

