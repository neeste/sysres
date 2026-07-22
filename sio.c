/* sio.c - depends on functions in wa.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "sio.h"
#include "bio.h"
#include "cardinfo.h"

#ifdef WIN32
void     wa_set_cardinfo(CARDINFO, int);
CARDINFO wa_get_cardinfo(int);
CARDINFO wa_get_cardtype();
#endif // WIN32

BIO bio_;

static double atten[2] = {0, 0};
static double mxsmp[2] = {0, 0};
static int iodev = 0;
static int mxndc[2] = {2, 2};
static int ndc[2] = {2, 2};

/**************************************************************************/

static void
bank_put(int bo, int is, int ns, int doit, int os)
{
#ifdef WIN32
    if(iodev == WA_DEV)
	wa_bank_put(bo, is, ns, doit, os);
#endif // WIN32
}

static void
bank_get(int bo, int is, int ns)
{
#ifdef WIN32
    if(iodev == WA_DEV)
	wa_bank_get(bo, is, ns);
#endif // WIN32
}

static void
bank_ready(int bo)
{
#ifdef WIN32
    if(iodev == WA_DEV)
	wa_bank_ready(bo);
#endif // WIN32
}

static void
bank_avg(int ns, int os)
{
    float v;
    int c, i, j, d, n, nc;
    long *b;
    long *r, *a;

    nc = ndc[0];    /* number of input channels */
    n = ns * nc;
    b = bio_.bank_b;
    for (c = 0; c < nc; c++) {
	d = bio_.resp_i[c];
	switch(bio_.avmode[c]) {
	case 1:
	    r = bio_.resp_b[c] + os * d;
	    for (i = c, j = 0; i < n; i += nc, j += d) {
		r[j] = b[i];
	    }
	    break;
	case 2:
	    a = bio_.resp_a[c] + os * d;
	    for (i = c, j = 0; i < n; i += nc, j += d) {
		a[j] += b[i];
	    }
	    break;
	case 3:
	    r = bio_.resp_b[c] + os * d;
	    a = bio_.resp_a[c] + os * d;
	    for (i = c, j = 0; i < n; i += nc, j += d) {
		r[j] = b[i];
		a[j] += b[i];
	    }
	    break;
	case 7:
	    r = bio_.resp_b[c] + os * d;
	    a = bio_.resp_a[c] + os * d;
	    for (i = c, j = 0; i < n; i += nc, j += d) {
		v = (float) b[i];
		r[j] = b[i];
		a[j] += b[i];
	    }
	    break;
	}
    }
}

/**************************************************************************/

static float **abptr = NULL;
static float ad_vpc[MAXNDC] = {1,1};
static float da_vpc[MAXNDC] = {1,1};
static float **ibptr = NULL;
static float **obptr = NULL;
static int avcnt = 0;
static int *avgcp = NULL;
static int bnk_ic = 0;
static int bnk_oc = 0;
static int bnksiz = 1024;
static int gapflg = 0;
static int *ibinc = NULL;
static int nbpc = 2;
static int nrbc = 0;
static int nsbc = 0;
static int *obinc = NULL;
static int ofst0 = 0;
static int rspbuf = 0;
static int smpskp = 0;
static int smpstm = 0;
static int smpswp = 0;
static int smptot = 0;
static int swpcnt = 0;
static int swptot = 0;

static void
set_stim_buff(int b)
{
    int c, k;

    for (c = 0; c < ndc[1]; c++) {
	k = c * nsbc + b;
	bio_.stim_i[c] = obinc ? obinc[k] : 1;
	bio_.stim_b[c] = obptr ? (long *) obptr[k] : NULL;
    }
}

static void
set_resp_buff(int b)
{
    int c, k;
    long *bp, *ap;

    for (c = 0; c < ndc[0]; c++) {
	k = c * nrbc + b;
	bio_.resp_i[c] = ibinc ? ibinc[k] : 1;
	bio_.resp_b[c] = bp = ibptr ? (long *) ibptr[k] : NULL;
	bio_.resp_a[c] = ap = abptr ? (long *) abptr[k] : NULL;
 	bio_.avmode[c] = ap ? (bp ? 3 : 2) : (bp ? 1 : 0);
    }
    rspbuf = b;
}

static void
put_bank()
{
    int     bnko, chnk, ibnk, doit, ofst, nrem, nsmp, smpo;

    bnko = bnk_oc % nbpc;
    for (chnk = bnksiz; chnk > 0; chnk -= nsmp) {
	ibnk = bnksiz - chnk;
	smpo = bnk_oc * bnksiz + ibnk;
	ofst = (smpo + ofst0) % smpswp;
	if ((ofst == 0 && nsbc > 0) || smpo == 0)
	    set_stim_buff((smpo / smpswp) % nsbc);
	nrem = (ofst < smpstm) ? (smpstm - ofst) : (smpswp - ofst);
	nsmp = (chnk < nrem) ? chnk : nrem;
	doit = (smpo < smptot && ofst < smpstm);
	bank_put(bnko, ibnk, nsmp, doit, ofst);
    }
    bank_ready(bnko);
    bnk_oc++;
}

static void
get_bank()
{
    int     bnko, chnk, ibnk, doit, ofst, nrem, nsmp, smpi;

    bnko = bnk_ic % nbpc;
    for (chnk = bnksiz; chnk > 0; chnk -= nsmp) {
	ibnk = bnksiz - chnk;
	smpi = bnk_ic * bnksiz + ibnk;
	ofst = (smpi +ofst0) % smpswp;
	if ((ofst == 0 && nrbc > 0) || smpi == 0)
	    set_resp_buff((smpi / smpswp) % nrbc);
	nrem = (ofst < smpstm) ? (smpstm - ofst) : (smpswp - ofst);
	nsmp = (chnk < nrem) ? chnk : nrem;
	doit = (smpi >= smpskp && ofst < smpstm && swpcnt < swptot);
	if (doit) {
	    bank_get(bnko, ibnk, nsmp);
	    bank_avg(nsmp, ofst);
	}
	ofst += nsmp;
	if (ofst == smpstm) {
	    swpcnt++;
	    if (doit && (rspbuf == (nrbc - 1))) {
		avcnt++;
		if (avgcp)
		    *avgcp = avcnt;
	    }
	}
	gapflg = (ofst >= smpstm);
    }
    bnk_ic++;
}

static int check_bank()
{
    int bank = 0;

#ifdef WIN32
    if (iodev == WA_DEV)
	bank = wa_check_bank();
#endif // WIN32

    return (bank);
}

/**************************************************************************/

static int
reset_io(int bs)
{
#ifdef WIN32
    if (iodev == WA_DEV)
        bs = wa_reset_io(bs);
#endif // WIN32
    bio_.bank_b = (long *) calloc(bs * ndc[0], sizeof(long));

    return (bs);
}

static void
begin_io()
{
#ifdef WIN32
    if (iodev == WA_DEV)
	wa_begin_io();
#endif // WIN32
}

/**************************************************************************/

static int (*escape)() = NULL;
static int escflg = 0;

static void
setup_io(long nstm, long ngap, int nskp, int nswp, double pregap)
{
    time_t t = 0;

    if (nswp <= 0 || nstm <= 0) {
	escflg = 1;
	return;
    }
#ifdef WIN32
    if (wa_open(mxndc, atten, mxsmp))
	return;
#endif // WIN32
    smpstm = nstm;
    smpswp = nstm + ngap;
    smpskp = nskp;
    smptot = smpswp * nswp - ngap + smpskp;
    swptot = nswp + (smpskp + smpswp - 1) / smpswp;
    ofst0 = swptot * smpswp - smptot;
    bnksiz = reset_io(smptot);
    escflg = 0;
    if (pregap > 0) {
	bank_ready(0);
	t = clock() + (time_t)(pregap * CLOCKS_PER_SEC);
	while(clock() < t)
	    continue;
    }
}

static void
perform_io(int navg, int nrej)
{
    int     b, stop = 0;

    if (escape)
        if (escape())
            return;
    swpcnt = 0;
    bnk_ic = 0;
    bnk_oc = 0;
    for (b = 0; b < nbpc; b++)
	put_bank();
    begin_io();
    while (!stop) {
	if (check_bank()) {
	    get_bank();
	    put_bank();
	}
	if (escape)
	    if (escape())
		escflg++;
	if (escflg > 1 || (escflg && gapflg))
	    stop++;
	if (swpcnt >= swptot)
	    stop++;
	if (gapflg) {
	    if (navg && avcnt >= navg)
		stop++;
	}
    }
}

static void
stop_io()
{
#ifdef WIN32
    if (iodev == WA_DEV) {
	wa_stop_io();
	wa_close();
    }
#endif // WIN32
    if (bio_.bank_b) {
	free(bio_.bank_b);
	bio_.bank_b = NULL;
    }
}

/**************************************************************************/

static double prgap = 0;
static int nsgap = 0;
static int nsstm = 0;

static void
init_vpc()
{
    double  da_vfs[MAXNDC], ad_vfs[MAXNDC];
    int     c;

#ifdef WIN32
    if (iodev == WA_DEV)
        wa_get_vfs(da_vfs, ad_vfs);
#endif // WIN32
    for (c = 0; c < ndc[1]; c++) {
	da_vpc[c] = (float) ((da_vfs[c] / mxsmp[1]));
    }
    for (c = 0; c < ndc[0]; c++) {
	ad_vpc[c] = (float) ((ad_vfs[c] / mxsmp[0]));
    }
}


static void
stim_fix()
{
    float  *ob, sc;
    int     c, b, i, j, k, dj;
    long    s, *sl;

    for (c = 0; c < ndc[1]; c++) {
	for (b = 0; b < nsbc; b++) {
	    k = c * nsbc + b;
	    if (obptr && obptr[k]) {
		ob = obptr[k];
		dj = obinc[k];
		sl = (long *) ob;
		sc = da_vpc[c];
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    s = (long) (ob[j] / sc);
		    sl[j] = (long) limit(-mxsmp[1], s, mxsmp[1]);
		}
	    }
	}
    }
}

static void
stim_float()
{
    float  *ob, sc;
    int     c, b, i, j, k, dj;
    long   *sl;

    for (c = 0; c < ndc[1]; c++) {
	for (b = 0; b < nsbc; b++) {
	    k = c * nsbc + b;
	    if (obptr && obptr[k]) {
		ob = obptr[k];
		dj = obinc[k];
		sl = (long *) ob;
		sc = da_vpc[c];
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    ob[j] = sl[j] * sc;
		}
	    }
	}
    }
}

static void
resp_zero()
{
    float  *ib;
    int     c, b, i, j, k, dj;
    long   *rl;

    for (c = 0; c < ndc[0]; c++) {
	for (b = 0; b < nrbc; b++) {
	    k = c * nrbc + b;
	    if (abptr && abptr[k]) {
		ib = abptr[k];
		dj = ibinc[k];
		rl = (long *) ib;
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    rl[j] = 0;
		}
	    }
	}
    }
    if (avgcp)
	*avgcp = 0;
    escflg = bio_.accept = avcnt = 0;
}

static void
resp_scale_average()
{
    float  *ab, sc;
    int     c, b, i, j, k, dj, n;
    long   *rb;

    for (c = 0; c < ndc[0]; c++) {
	for (b = 0; b < nrbc; b++) {
	    n = avcnt;
	    k = c * nrbc + b;
	    if (abptr && abptr[k]) {
		ab = abptr[k];
		dj = ibinc[k];
		rb = (long *) ab;
		sc = (n > 0) ? (float) (ad_vpc[c] / n) : 0;
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    ab[j] = rb[j] * sc;
		}
	    }
	}
    }
}

static void
resp_scale_input()
{
    float  *ib, sc;
    int     c, b, i, j, k, dj;
    long   *rb;

    for (c = 0; c < ndc[0]; c++) {
	for (b = 0; b < nrbc; b++) {
	    k = c * nrbc + b;
	    if (ibptr && ibptr[k]) {
		ib = ibptr[k];
		dj = ibinc[k];
		rb = (long *) ib;
		sc = ad_vpc[c];
		for (i = j = 0; i < nsstm; i++, j += dj) {
		    ib[j] = rb[j] * sc;
		}
	    }
	}
    }
}

/**************************************************************************/
/* SIO library routines */
/**************************************************************************/

// sio_open - Initializes soundcard and internal SIO variables.

int sio_open(	// returns non-zero if successful
)
{
#ifdef WIN32
    if (!iodev && wa_test()) {
        if (wa_open(mxndc, atten, mxsmp))
	    return (0);
        wa_close();
	iodev = WA_DEV;
    }
#endif // WIN32
    ndc[1] = 0;
    ndc[0] = 0;
    nsstm = 0;
    nsgap = 0;
    prgap = 0;
    nsbc = 0;
    nrbc = 0;
    obptr = NULL;
    obinc = NULL;
    abptr = NULL;
    ibptr = NULL;
    ibinc = NULL;
    escape = NULL;
    bio_.bank_b = NULL;

    return (iodev);
}

// sio_close - Terminate I/O and free any allocated resources.

void sio_close(
)
{
    iodev = 0;
}

// sio_get_nioch - Obtain number of I/O channels.

int sio_get_nioch(
    int *ni, 		// Number of input channels returned here
    int *no		// Number of output channels returned here
)
{
    *ni = mxndc[0];
    *no = mxndc[1];

    return (iodev >= 0);
}

// sio_get_device - Obtain short description of I/O device.

int sio_get_device(
    char *s		// Device description returned here
)
{
#ifdef WIN32
    strncpy(s, wa_devnam(), 40);
#endif // WIN32

    return (iodev >= 0);
}

// sio_set_device - select I/O device.

int sio_set_device(	// returns card number
    int n		// card number
)
{
#ifdef WIN32
    return (wa_devsel(n));
#else // WIN32
    return (0);
#endif // WIN32
}

// sio_get_info - Obtain one-line of information about I/O device.

int sio_get_info(
    char *s		// Device information returned here
)
{
#ifdef WIN32
    char info[40];

    wa_info(info);
    sprintf(s, "%s device: %s, %s", (iodev == WA_DEV) ? "I/O" : "Invalid",
	wa_devnam(), info);
#else // WIN32
    sprintf(s, "No I/O device.");
#endif // WIN32

    return (iodev >= 0);
}

// sio_get_vfs - Obtain full-scale voltage of ADCs and DACs.

int sio_get_vfs(
    double *ad_vfs, 	// volts-full-scale for each ADC channel returned 
    double *da_vfs 	// volts-full-scale for each DAC channel returned
)
{
#ifdef WIN32
    if (iodev == WA_DEV)
        wa_get_vfs(da_vfs, ad_vfs);
#endif // WIN32

    return (iodev >= 0);
}

// sio_set_size - Specify size of buffers and gap between buffers.

void sio_set_size(
    int ns, 		// size of I/O buffers (samples)
    int ng,		// size of gap between I/O buffers (samples)
    double pg		// size of gap before I/O (seconds)
)
{
    nsstm = ns;
    nsgap = ng;
    prgap = pg;
}

// sio_set_output - Specify output buffers.

void sio_set_output(
    int nch, 		// number of output channels to be used
    int bpc, 		// number of buffers per output channel
    int *inc, 		// array of increments for each output buffer
    float **bpt		// array of pointers to each output buffer
)
{
    ndc[1] = nch;
#ifdef WIN32
    wa_set_ndc(ndc);
#endif // WIN32
    nsbc = bpc;
    obinc = inc;
    obptr = bpt;
    while (nch < ndc[1]) {
	inc[nch] = 0;
	bpt[nch] = NULL;
	nch++;
    }
}

// sio_set_input - Specify input buffers.

void sio_set_input(
    int nch, 		// number of input channels to be used
    int bpc, 		// number of buffers per input channel
    int *inc, 		// array of increments for each input buffer
    float **bpt 	// array of pointers to each input buffer
)
{
    ndc[0] = nch;
#ifdef WIN32
    wa_set_ndc(ndc);
#endif // WIN32
    nrbc = bpc;
    ibinc = inc;
    ibptr = bpt;
    while (nch < ndc[0]) {
	inc[nch] = 0;
	bpt[nch] = NULL;
	nch++;
    }
}

// sio_set_average - Specify averaging.

void sio_set_average(
    float **apt, 	// array of pointers to accumulate buffers
    int *acp		// array set to num of swps averaged in each buffer
)
{
    abptr = apt;
    avgcp = acp;
}

// sio_set_rate - Specify sample rate.

double sio_set_rate(	// returns actual sample rate (samples/second)
    double r		// desired sample rate (samples/second)
)
{
#ifdef WIN32
    if (iodev == WA_DEV)
        r = wa_set_rate(r);
    else
#endif // WIN32
	r = 0;
    return (r);
}

// sio_set_att_in - Specify attenuation on input all channels.

double sio_set_att_in(	// returns actual input attenuation (dB)
    double a		// desired input attenutation (dB)
)
{
#ifdef WIN32
    if (iodev == WA_DEV)
	atten[0] = wa_atten_input(a);
    else
#endif // WIN32
	atten[0] = 0;
    return (atten[0]);
}

// sio_set_att_out - Specify attenuation on output all channels.

double sio_set_att_out(	// returns actual output attenuation (dB)
    double a		// desired output attenutation (dB)
)
{
#ifdef WIN32
    if (iodev == WA_DEV)
	atten[1] = wa_atten_output(a);
    else
#endif // WIN32
	atten[1] = 0;
    return (atten[1]);
}

// sio_set_vfs - Specify full-scale voltage on ADCs and DACs

void sio_set_vfs(
    double *ad_vfs, 	// array set to volts-full-scale for each ADC
    double *da_vfs	// array set to volts-full-scale for each DAC
)
{
#ifdef WIN32
    if (iodev == WA_DEV)
        wa_set_vfs(da_vfs, ad_vfs);
#endif // WIN32
}

// sio_set_escape - Specify callback function for early I/O termination.

void sio_set_escape(
    int (*esc)()	// escape callback function
)
{
    escape = esc;
}

// sio_io - Perform input/output/averaging.

void sio_io(
    int nskip, 		// number of samples to skip before averaging
    int nswps, 		// maximum number of sweeps while averaging
    int navgs, 		// maximum number of sweeps to average
    int nrejs		// maximum number of rejects allowed
)
{
    init_vpc();
    stim_fix();
    resp_zero();
    setup_io(nsstm, nsgap, nskip, nswps, prgap);
    perform_io(navgs, nrejs);
    stop_io();
    resp_scale_average();
    stim_float();
}

// sio_io_chk - Perform input/output/averaging with callback.

void sio_io_chk(
    void (*resp_check)(int *)	// calllback function at end of each sweep
)
{
    init_vpc();
    stim_fix();
    resp_zero();
    while (!escflg) {
        setup_io(nsstm, 0, 0, 1, prgap);
	perform_io(0, 0);
	if (resp_check) {
	    resp_scale_input();
	    resp_check(&escflg);
	}
        stop_io();
    }
    resp_scale_average();
    stim_float();
}

/**************************************************************************/

// sio_set_latency - Set devcice latency (dummy version)

int sio_set_latency(
    int n		// number of samples
)
{
    return (0);
}

// sio_set_channel_offset - Set input & output channel offsets

void sio_set_channel_offset(
    int chnoff_i,
    int chnoff_o
)
{
}

/**************************************************************************/

// sio_init_cardinfo - get initialize info

void sio_init_cardinfo(
)
{
#ifdef WIN32
    extern int check_registry();
    check_registry();
#endif
}

/* sio_get_cardinfo - get card info */

CARDINFO
sio_get_cardinfo(int ct)
{
#ifdef WIN32
    if (ct < 0)
	return (wa_get_cardtype(iodev));
    return (wa_get_cardinfo(ct));
#else // WIN32
static CARDINFO card_info[NCT] = {
    { "Creative Sound",      16, 0, 2, 2, 2, 0, {2.660}, {2.560} },
    { "Crystal SoundFusion", 16, 0, 2, 2, 2, 0, {0.420}, {0.630} }, // vol=4
    { "M-Audio Delta",       24, 8, 4, 2, 2, 0, {1.826}, {1.767} },
    { "CardDeluxe Analog",   24, 8, 4, 2, 2, 0, {10.20}, {4.875} }, //  +4
    { "Gina24",              24, 8, 4, 2, 8, 0, {10.37}, {10.25} },
    { "Indigo io",           24, 8, 4, 2, 2, 0, {2.357}, {2.190} },
    { "5-Waveterminal",      24, 8, 4, 2, 6, 0, {9.655}, {4.351} },
    { "Juli@",               24, 8, 4, 2, 2, 0, {10.33}, {5.458} },
};
    return (card_info[limit(0,ct,NCT-1)]);
#endif // WIN32
}

/* sio_set_cardinfo - set card info */

void
sio_set_cardinfo(CARDINFO ci, int ct)
{
#ifdef WIN32
    wa_set_cardinfo(ci, ct);
#endif // WIN32
}

/**************************************************************************/
// sio_version - ARSC version

char *sio_version(void)
{
    return ("SIO version: ???");
}

/**************************************************************************/
