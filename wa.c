/* wa.c - sio functions for Win32 wave-audio device */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <math.h>
#include <ks.h>
#include <ksmedia.h>
#include "bio.h"
#include "cardinfo.h"

#define MAXBNKSIZ   16384	/* max bank size */
#define NBNK	    2		/* max banks */
#define SRLSTSZ     27		/* sample rate list size */
#define BIT23	    (1<<23)

#define WODM_USER            0
#define ECHO_GET_WAVE_SYNC  (WODM_USER + 0)
#define ECHO_SET_WAVE_SYNC  (WODM_USER + 1)

static double ad_vfs_sv[MAXNDC] = {0};
static double cur_att_in = 0;
static double cur_att_out = 0;
static double da_vfs_sv[MAXNDC] = {0};
static double mxsv = 0;
static int bits = 0;		/* number of I/O bits */
static int bnkchk = 0;
static int bnkrdy[NBNK] = {0, 0};
static int bnksiz = 0;
static int ct = 0;	/* card type */	
static int iocnt = 0;	/* count of I/O transactions */
static int left = 0;	/* number of left-shift bits within sample */
static int nbps = 2;	/* number of bytes per sample */
static int ncad = 2;	/* number of channels of A/D */
static int ncda = 2;	/* number of channels of D/A */
static int mid = 0;	/* manufacturer ID number */
static int pid = 0;	/* product ID number */
static int wa_dev_opened = 0;
static int wav_fmt_ext = 0;
static long *lbanki[NBNK];
static long *lbanko[NBNK];
static short *sbanki[NBNK];
static short *sbanko[NBNK];
static unsigned long rate = 44100;
static unsigned long goodSR[SRLSTSZ];
static unsigned long SRlist[SRLSTSZ] = {
    4000, 5512, 6000, 8000, 8269, 10000, 11025, 12000, 16000, 16538, 20000, 22050, 
    24000, 25000, 32000, 33075, 44100, 48000, 50000, 64000, 88200, 96000, 100000,
    128000, 176400, 192000, 200000
};
static void *vbanki[NBNK];
static void *vbanko[NBNK];
static HWAVEIN hWaveIn;
static HWAVEOUT hWaveOut;
static MMRESULT Result;
static UINT cardNumber = 0;
static WAVEHDR whdri[NBNK];
static WAVEHDR whdro[NBNK];
static WAVEINCAPS WavInCaps;
static CARDINFO card_info[NCT] = {
    { "Creative Sound",      16, 0, 2, 2, 2, 0, {2.660}, {2.560} },
    { "Crystal SoundFusion", 16, 0, 2, 2, 2, 0, {0.420}, {0.630} }, // vol=4
    { "M-Audio Delta",       24, 8, 4, 2, 2, 0, {1.826}, {1.767} },
    { "CardDeluxe Analog",   24, 8, 4, 2, 2, 0, {10.20}, {4.875} }, // +4
    { "Gina24",              24, 8, 4, 2, 8, 0, {10.37}, {10.25} },
    { "Indigo io",           24, 8, 4, 2, 2, 0, {2.357}, {2.190} },
    { "5-Waveterminal",      24, 8, 4, 2, 6, 0, {9.655}, {4.351} },
    { "Juli@",               24, 8, 4, 2, 2, 0, {10.33}, {5.458} }, // +4
};
static int nct = NCT;

/***************************************************************************/

/* get_devcaps - get device capabilities */

static void
get_devcaps()
{
    Result = waveInGetDevCaps(cardNumber, &WavInCaps, sizeof(WAVEINCAPS));
    mid = WavInCaps.wMid;
    pid = WavInCaps.wPid;
/*
 * Values encountered for mid & pid:
 *
 * "Creative Sound",        1,  101
 * "TBS Pro Series",       21,   31
 * "CardDeluxe Analog",   136,   19
 * "Crystal SoundFusion", 132,    2
 */
}

/* card_type - return type of "card" from structure */

static int
card_type()
{
    int     i, j, n, c = 0;

    // distribute vfs values to all channels
    for (i = 0; i < NCT; i++) {
	if(*card_info[i].name == '\0') {
	    nct = i;
	    break;
	}
	if (card_info[i].ad_vfs[0] > 0) {
	    for (j = 1; j < card_info[i].ncad; j++) {
		if (card_info[i].ad_vfs[j] == 0)
		    card_info[i].ad_vfs[j] = card_info[i].ad_vfs[0];
	    }
	}
	if (card_info[i].da_vfs[0] > 0) {
	    for (j = 1; j < card_info[i].ncda; j++) {
		if (card_info[i].da_vfs[j] == 0)
		    card_info[i].da_vfs[j] = card_info[i].da_vfs[0];
	    }
	}
    }
    // lookup names of currently installed cards
    get_devcaps();
    for (i = 0; i < nct; i++) {
	if (card_info[i].name) {
	    n = strlen(card_info[i].name);
	    if (n && strncmp(WavInCaps.szPname, card_info[i].name, n) == 0) {
		c = i;
		break;
	    }
	}
    }

    return (c);
}

/* list_rates - create a list of available sample rates */

static int
list_rates()
{
    int     i, gdsr;
    struct {
	WAVEFORMATEX  f;
	WORD    ValidBits;	/* bits of precision */
	DWORD   ChannelMask;	/* which channels are present in stream */
	GUID    SubFormat;	/* globally unique identifier */
    } w;

/* set WAVEFORMATEX structure with proposed value and see if they're
 * accepted 
 */

    // Initialize the waveformat structure
    memset(&w, 0, sizeof(w));
    w.f.cbSize = sizeof(w) - sizeof(WAVEFORMATEX);
    w.f.nChannels = ncad;
    w.f.wBitsPerSample = nbps * 8;
    w.f.nBlockAlign = ncad * nbps;
    w.f.nAvgBytesPerSec = (w.f.nBlockAlign * w.f.nSamplesPerSec);
    w.ValidBits = bits;
    w.ChannelMask = 3;
    w.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    if (wav_fmt_ext) {
        w.f.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    } else {
        w.f.wFormatTag = WAVE_FORMAT_PCM;
    }
    gdsr = 0;
    for (i = 0; i < SRLSTSZ; i++) {
	if (SRlist[i] <= 0)
	    continue;
        w.f.nSamplesPerSec = SRlist[i];
	//wfmt.nAvgBytesPerSec = (wfmt.nBlockAlign * wfmt.nSamplesPerSec);
	// Although one would think that the preceeding line might be necessary,
        // in fact, when included, Result returns zero even if an absurd
	// sampling rate is requested
        Result = waveInOpen(0, cardNumber, (PWAVEFORMATEX)&w, 0L, 0L, WAVE_FORMAT_QUERY);
	if (Result == 0)
	    gdsr |= 1 << i;
    }

    return (gdsr);
}

/* open_io - open input and ouput devices */

static void
open_io()
{
    struct {
	WAVEFORMATEX  f;
	WORD    ValidBits;	/* bits of precision */
	DWORD   ChannelMask;	/* which channels are present in stream */
	GUID    SubFormat;	/* globally unique identifier */
    } w;

    if (!wa_dev_opened) {
	memset(&w, 0, sizeof(w));
	w.f.cbSize = sizeof(w) - sizeof(WAVEFORMATEX);
	w.f.nSamplesPerSec = rate;
	w.f.nChannels = ncad;
	w.f.wBitsPerSample = nbps * 8;
	w.f.nBlockAlign = ncad * nbps;
        w.f.nAvgBytesPerSec = (w.f.nBlockAlign * w.f.nSamplesPerSec);
        w.ValidBits = bits;
        w.ChannelMask = 3;
        w.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
	if (wav_fmt_ext) {
 	    w.f.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	} else {
	    w.f.wFormatTag = WAVE_FORMAT_PCM;
	}
	// open "In" before "Out" to avoid "allocated" error
	if (ncad > 0) {
	    Result = waveInOpen(&hWaveIn, cardNumber, (PWAVEFORMATEX)&w, 0L, 0L, 0);
	    if (Result != 0) {
		return;
	    }
	}
	if (ncda > 0) {
	    Result = waveOutOpen(&hWaveOut, cardNumber, (PWAVEFORMATEX)&w, 0L, 0L, 
		WAVE_ALLOWSYNC);
	    if (Result == 32) {	// if old format fails, try new format
		w.f.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		Result = waveOutOpen(&hWaveOut, cardNumber, (PWAVEFORMATEX)&w, 0L, 0L, 
		    WAVE_ALLOWSYNC);
		if (Result == 0)
		    wav_fmt_ext = 1;
	    }
	    if (Result != 0) {
		if (ncad > 0)
		    waveInClose(hWaveIn);
		return;
	    }
	}
	/*
	 * MMSYSERR codes:
	 *  1 = unspecified error
	 *  2 = device ID out of range 
	 *  3 = driver enable failed 
	 *  4 = device already allocated 
	 *  6 = no device driver present
	 *  7 = memory allocation error
	 * 10 = invalied flag passed
	 * WAVERR codes:
	 * 32 = unsupported wave format 
	 */
	// check if Echo wavesync is set
	Result = waveOutMessage(hWaveOut, ECHO_GET_WAVE_SYNC, 0, 0);

	wa_dev_opened = 1;
    }
}

/* close_io - close input and ouput devices */

static void
close_io()
{
    if (wa_dev_opened) {
	(void) waveOutReset(hWaveOut);
	(void) waveOutClose(hWaveOut);
	hWaveOut = 0;
	(void) waveInReset(hWaveIn);
	(void) waveInClose(hWaveIn);
	hWaveIn = 0;
	wa_dev_opened = 0;
    }

}

/* devsel_bybits - select I/O device by number of bits */

static void
devsel_bybits(int bits)
{
    char   *cn;
    int     i, j, nc, nd;

    nd = (int) waveInGetNumDevs();
    for (i = 0; i < nd; i++) {
        Result = waveInGetDevCaps(i, &WavInCaps, sizeof(WAVEINCAPS));
	for (j = 0; j < nct && card_info[j].bits; j++) {
	    if (card_info[j].bits >= bits) {
    		cn = card_info[j].name;
		nc = strlen(cn);
		if (strncmp(WavInCaps.szPname, cn, nc) == 0) {
		    cardNumber = i;
		    nd = 0;
		    break;
		}
	    }
	}
    }
}

/***************************************************************************/

/* wa_devnam - return name of I/O device */

char   *
wa_devnam()
{
    get_devcaps();
    return (WavInCaps.szPname);
}

/* wa_devsel - select I/O device */

int
wa_devsel(int n)
{
    int nd;

    if (n == -16 || n == -24) {
	devsel_bybits(-n);
    } else if (n > 0) {
	nd = (int) waveInGetNumDevs();
	cardNumber = limit(1, n, nd) - 1;
    }
    return (cardNumber + 1);
}

/* wa_devsel_byname - select I/O device by name */

int
wa_devsel_byname(char *cn)
{
    int     i, nc, nd;

    if (cn == NULL)
	return (0);
    nc = strlen(cn);
    if (nc <= 0)
	return (0);
    nd = (int) waveInGetNumDevs();
    for (i = 0; i < nd; i++) {
        Result = waveInGetDevCaps(i, &WavInCaps, sizeof(WAVEINCAPS));
	if (strncmp(WavInCaps.szPname, cn, nc) == 0) {
	    cardNumber = i;
	    break;
	}
    }
    return (cardNumber + 1);
}

/* wa_info - return soundcard information */

void
wa_info(char *s)
{
    sprintf(s, "id=%d/%d, WaveAudio=%d/%d", mid, pid, 
	cardNumber + 1, waveInGetNumDevs());
}

double
wa_adjust_rate(double r)
{
    int     i, bestindex;
    unsigned long rt;
    double mindiff, diff;

    if (r <= 0)
	return (0);
    if (!card_info[ct].gdsr)
        card_info[ct].gdsr = list_rates();
    bestindex = 0;
    mindiff = fabs(log10(r));
    for (i = 0; i < SRLSTSZ; i++) {
	if (card_info[ct].gdsr & (1 << i)) {
	    diff = fabs(log10(r / SRlist[i]));
	    if (mindiff > diff) {
		mindiff = diff;
		bestindex = i;
	    }
	}
    }
    rt = SRlist[bestindex];

    return (rt);
}

/* wa_set_rate - set sampling rate to nearest possible value */

double
wa_set_rate(double r)
{
    unsigned long rt;

    rt = (unsigned long) wa_adjust_rate(r);
    if ((rt > 0) && (rt != (unsigned long) rate)) {
	rate = rt;
        if (wa_dev_opened) {
	    close_io();
	    open_io();
	}
    }
    return (rate);
}

/* wa_atten_input - set input attenuation */

double
wa_atten_input(double a)
{
    cur_att_in = 0.0;
    return (0);
}

/* wa_atten_output - set output attenuation */

double
wa_atten_output(double a)
{
    int err;
    double  returnval = 0;

    if (wa_dev_opened) {
        err = waveOutSetVolume(hWaveOut, 0xFFFFFFFF);
        returnval = 0.0;
    }

    return (cur_att_out = returnval);
}

/* wa_test - test for presence of wave audio device */

int
wa_test()
{
    return (waveInGetNumDevs() * waveOutGetNumDevs());
}

/* wa_set_ndc - set number of device channels */

void
wa_set_ndc(int *ndc)
{
    ndc[0] = ncad;  /* number of open A/D channels */
    ndc[1] = ncda;  /* number of open D/A channels */
}

/* wa_open - open wave-audio device */

int
wa_open(int *nc, double *at, double *ms)
{
    int j;

    // If a device is already opened, close it!
    if (wa_dev_opened)
	close_io();

    // Initialize card-specific values
    ct = card_type();
    bits = card_info[ct].bits;	/* I/O bits */
    left = card_info[ct].left;	/* left-shift bits within sample */
    nbps = card_info[ct].nbps;	/* number of bytes per sample */
    for (j = 0; j < MAXNDC; j++) {
	ad_vfs_sv[j] = card_info[ct].ad_vfs[j];
        da_vfs_sv[j] = card_info[ct].da_vfs[j];
    }
    mxsv = (int) (((unsigned long) 1 << (bits - 1)) - 1);

    // open i/o device
    open_io();
    if (!wa_dev_opened)
	return (Result);

    // Set attenuation to zero
    (void) wa_atten_input(0.0);
    (void) wa_atten_output(0.0);

    // Initialize device info
    get_devcaps();
    if (!card_info[ct].gdsr)
        card_info[ct].gdsr = list_rates();

    // return values
    if (nc) {
	nc[0] = card_info[ct].ncad;	/* max number of A/D channels */
	nc[1] = card_info[ct].ncda;	/* max number of D/A channels */
    }
    if (at)
	at[0] = at[1] = 0.0;	/* zero attenuation */
    if (ms)
	ms[0] = ms[1] = mxsv;	/* maximum I/O sample value */

    return (0);
}

/* wa_close - close wave-audio device */

void
wa_close()
{
    close_io();
}

/* wa_reset_io - prepare device and buffers for I/O */

int
wa_reset_io(int bs)
{
    int     b;

    if (wa_dev_opened) {
        Result = waveOutReset(hWaveOut);
        Result = waveInReset(hWaveIn);
    
        bnksiz = limit(1, bs, MAXBNKSIZ);
        for (b = 0; b < NBNK; b++) {

            // Allocate memory for the input and output buffers
	    vbanki[b] = calloc(bnksiz, ncad * nbps); /* input bank */
	    vbanko[b] = calloc(bnksiz, ncda * nbps); /* output bank */
	    if (vbanki[b] == NULL || vbanko[b] == NULL)
		return (0);

	    // Initial various buffer pointer types
	    lbanki[b] = (long *) vbanki[b];
	    lbanko[b] = (long *) vbanko[b];
	    sbanki[b] = (short *) vbanki[b];
	    sbanko[b] = (short *) vbanko[b];

	    // Prepare wave_out header.
            whdro[b].lpData = (HPSTR) vbanko[b];
            whdro[b].dwBufferLength = bnksiz * ncda * nbps;
            whdro[b].dwFlags = 0L;
            whdro[b].dwLoops = 0L;
            Result = waveOutPrepareHeader(hWaveOut, &whdro[b], sizeof(WAVEHDR));

            // Prepare wave_in header.
            whdri[b].lpData = (HPSTR) vbanki[b];
            whdri[b].dwBufferLength = bnksiz * ncad * nbps;
            whdri[b].dwFlags = 0L;
            whdri[b].dwLoops = 0L;
            Result = waveInPrepareHeader(hWaveIn, &whdri[b], sizeof(WAVEHDR));
        }
        bnkchk = 0;
        iocnt = 0;
        return (bnksiz);
    }
    return (0);
}

/* wa_bank_ready - indicated bank is ready to go */

void
wa_bank_ready(int b)
{

    if (wa_dev_opened) {
	if (iocnt == 0) {
	    Result = waveInStop(hWaveIn);
	    Result = waveOutPause(hWaveOut);
	}
        // input data buffer added to the input device
	Result = waveInAddBuffer(hWaveIn, &whdri[b], sizeof(WAVEHDR));
	// ouput data buffer sent to the output device.
	Result = waveOutWrite(hWaveOut, &whdro[b], sizeof(WAVEHDR));
    }

}

/* wa_begin_io - begin I/O */

void
wa_begin_io()
{
    if (wa_dev_opened) {
        // start output and input at the same time
	if (iocnt == 0) {
	    Result = waveInStart(hWaveIn);
	    Result = waveOutRestart(hWaveOut);
	}
        iocnt++;
    }
}

/* wa_check_bank - check for bank completion */

int
wa_check_bank()
{
    bnkchk %= NBNK;
    if ((whdri[bnkchk].dwFlags & WHDR_DONE)
	&& (whdro[bnkchk].dwFlags & WHDR_DONE)) {
	return (++bnkchk);
    }
    return (0);
}

/* wa_stop_io - stop I/O */

void
wa_stop_io()
{
    int     b;

    if (wa_dev_opened) {
	waveOutReset(hWaveOut);
	waveInReset(hWaveIn);
        for (b = 0; b < NBNK; b++) {
	    waveOutUnprepareHeader(hWaveOut, &whdro[b], sizeof(WAVEHDR));
	    waveInUnprepareHeader(hWaveIn, &whdri[b], sizeof(WAVEHDR));
	    free(vbanki[b]);
	    free(vbanko[b]);
	}
    }
}

/* wa_get_vfs - get volts full scale */

void
wa_get_vfs(double *da_vfs, double *ad_vfs)
{
    int     i;

    for (i = 0; i < MAXNDC; i++) {
	da_vfs[i] = da_vfs_sv[i] / pow(10.0, cur_att_out / 20);
	ad_vfs[i] = ad_vfs_sv[i] * pow(10.0, cur_att_in / 20);

    }
}

/* wa_set_vfs - set volts full scale */

void
wa_set_vfs(double *da_vfs, double *ad_vfs)
{
    int     i;

    for (i = 0; i < MAXNDC; i++) {
	da_vfs_sv[i] = da_vfs[i] * pow(10.0, cur_att_out / 20);
	ad_vfs_sv[i] = ad_vfs[i] / pow(10.0, cur_att_in / 20);
    }
}

/* wa_get_cardinfo - get card info */

CARDINFO
wa_get_cardinfo(int i)
{
    CARDINFO ci = {0};

    if (i >= 0 && i < nct)
        ci = card_info[i];

    return (ci);
}

/* wa_set_cardinfo - set card info */

void
wa_set_cardinfo(CARDINFO ci, int i)
{
    card_info[i] = ci;
}

/* wa_get_cardinfo - get card info */

CARDINFO
wa_get_cardtype()
{
    return (wa_get_cardinfo(card_type()));
}

/**************************************************************************/

void
wa_bank_put(int bo, int is, int ns, int doit, int os)
{
    int     c, i, j, d, n;
    long   *s, z = 0;
    short  *sbuf;
    long   *lbuf;

    n = ns * ncda;
    if (nbps == 4) {
	lbuf = lbanko[bo] + is * ncda;
	for (c = 0; c < ncda; c++) {
	    if (bio_.stim_b[c] && doit) {
		d = bio_.stim_i[c];
		s = bio_.stim_b[c] + os * d;
	    } else {
		d = 0;
		s = &z;
	    }
	    if (left == 8) {
		for (i = c, j = 0; i < n; i += ncda, j += d) {
		    lbuf[i] = s[j] << 8;
		}
	    } else {	/* assume left == 0 */
		for (i = c, j = 0; i < n; i += ncda, j += d) {
		    lbuf[i] = s[j] ;
		}
	    }
	}
    } else {
	sbuf = sbanko[bo] + is * ncda;
	for (c = 0; c < ncda; c++) {
	    if (bio_.stim_b[c] && doit) {
		d = bio_.stim_i[c];
		s = bio_.stim_b[c] + os * d;
	    } else {
		d = 0;
		s = &z;
	    }
	    /* assume left == 0 */
	    for (i = c, j = 0; i < n; i += ncda, j += d) {
		sbuf[i] = (short) s[j];
	    }
	}
    }
}

void
wa_bank_get(int bo, int is, int ns)
{
    int     i, n;
    long   *lbuf, *r;
    short  *sbuf;

    n = ns * ncad;
    r = bio_.bank_b;
    if (nbps == 4) {
	lbuf = lbanki[bo] + is * ncad;
	if (bits == 24 && left == 8) {
	    for (i = 0; i < n; i++)
		r[i] = lbuf[i] >> 8;
	} else if (bits == 24 && left == 0) {
	    for (i = 0; i < n; i++)
		r[i] = (lbuf[i] & BIT23) ? lbuf[i] | 0xFF000000 : lbuf[i] & 0x00FFFFFF;

	} else {    /* assume bits == 32 */
	    for (i = 0; i < n; i++)
		r[i] = lbuf[i];
	}
    } else {
	sbuf = sbanki[bo] + is * ncad;
	for (i = 0; i < n; i++)
	    r[i] = sbuf[i];
    }
}
