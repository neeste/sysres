/* calibr.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "display.h"
#include "sysres.h"

int	read_data_mat(char *, char *, PGMCFG *);
int     stim_compute(int, char *);
int	write_data_mat(char *, char *, PGMCFG *);
void    display_frame(PGMCFG *, PLTFMT *, char *, int);
void    display_fft(float *, PLTFMT *, int);
void    resp_ft(int);

extern char *no_dev;
extern double splref;
extern float *st, *rt, *sf, *rf;
extern int nrcp, sio_present;
extern PGMCFG cf;
extern PLTFMT pf;

static char *calfn = "sysres.cal";
static char comment[80] = "";
static double fcal = 0;
static int ncal = 0;
static float *ca = NULL;

void
cal_init() 
{
    double dbr;
    float *rf2;
    int i, n;
    
    if (ca != NULL)
        free(ca);
    n = cf.ncal / 2 + 1;
    ca = (float *) calloc(2 * n, sizeof(float));
    dbr = 20 * log10(cf.sens_mp * splref);
    rf2 = &rf[cf.ncal + 2];
    for (i = 0; i < n; i++) {
        ca[i] = (float) (cdb(&rf[2 * i]) - dbr);
        ca[i + n] = (float) (cdb(&rf2[2 * i]) - dbr);
    }
    ncal = n;
    fcal = cf.rate / cf.ncal;
}

void
cal_read(char *msg) 
{
    if (!read_data_mat(msg, calfn, &cf)) {
    	ncal = 0;
    	cf.ncal = 0;
	sprintf(msg, "can't read calibration file: %s", calfn);
    	return;
    }
    if (cf.nsamp != cf.ncal) {
	sprintf(msg, "invalid calibration format: %s", calfn);
    	return;
    }
    display_frame(&cf, &pf, comment, 1);
    resp_ft(nrcp);
    pf.nfft = cf.ncal;
    pf.df_khz = (cf.rate / pf.nfft) / 1000;
    pf.dbvpu = 0;
    display_fft(rf, &pf, nrcp);
    cal_init();
}

void
cal_create(char *msg) 
{
    float rp;
    int i, a, c;

    if (!sio_present) {
	strcpy(msg, no_dev);
    	ncal = 0;
        cf.ncal = 0;
	return;
    }
    if (cf.sens_mp <= 0) {
	strcpy(msg, "Please set microphone sensitivity first. (sm=?)");
    	ncal = 0;
        cf.ncal = 0;
	return;
    }
    cf.nsamp = cf.ncal;
    cf.ad_mode = 3;
    cf.da_mode = 1;
    if (!stim_compute(0, msg)) {        // chirp stimulus
    	ncal = 0;
        cf.ncal = 0;
        return;
    }
    play_stim(0, 0, 1, &a);
    c = cf.cal_chn;			// select A/D channel
    for (i = 0; i < cf.nsamp; i++)
        rf[i] = rt[2 * i + c];		// save selected A/D channel
    cf.da_mode = 2;
    play_stim(0, 0, 1, &a);
    for (i = 0; i < cf.nsamp; i++) {
        rp = rt[2 * i + c];		// save selected A/D channel
        rt[2 * i] = rf[i];
        rt[2 * i + 1] = rp;
    }
    if (cf.volts > 0) {
	rp = (float) (1 / cf.volts);
        for (i = 0; i < 2 * cf.nsamp; i++) {
            rt[i] = rt[i] * rp;
        }
    }
    resp_compute();
    cf.ad_mode = 1;
    cf.da_mode = 3;
    if (!write_data_mat(msg, calfn, &cf)) {
    	ncal = 0;
    	cf.ncal = 0;
    	return;
    }
    cal_read(msg);
}

void
cal_tones(double f1, double f2, char *msg) 
{
    int i1, i2;

    if (ncal < MIN_NSAMP) {
	strcpy(msg, "Please calibrate microphone first.");
	return;
    }
    i1 = (int) (f1 / fcal + 0.5);
    i2 = (int) (f2 / fcal + 0.5) + cf.ncal / 2 + 1;
    sprintf(msg, "calibration tones (L,R): %.3f, %.3f kHz @ %.1f, %.1f dBSPL",
	f1 / 1000, f2 / 1000, ca[i1], ca[i2]);
}

double
cal_atten(double f, double lv, int ch)
{
    int i;
    double at, dbv;

    if (ncal < MIN_NSAMP)
        return (0);    
    i = (int) (f / fcal + 0.5);
    if (ch)
        i += ncal;
    dbv = (cf.volts > 0) ? 20 * log10(cf.volts) : 0;
    at = ca[i] - lv + dbv;
    return (at);
}
