/* dsp.c */

#include <stdio.h>
#include <math.h>
#include "bio.h"
#ifdef linux
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <asm/io.h>
#include <asm/mman.h>
#define ptr_enable()
#define ptr_disable()
#else  /* linux */
#ifdef WIN32
#include <windows.h>
#include <winioctl.h>
#include <WinRTctl.h>
#define ptr_enable()
#define ptr_disable()
int     LoadDriver(int);
extern HANDLE hWinRT;                 // device handle
HANDLE hWinRT;                 // device handle
#else  /* WIN32 */
#include <unistd.h>
#include <sys/nearptr.h>
#define ptr_enable()    __djgpp_nearptr_enable();
#define ptr_disable()   __djgpp_nearptr_disable();
#endif /* WIN32 */
#endif /* linux */

#define MAX_SAMP    (32767.0)	    /* max sample value */
#define MSND_PAGE   0xD000          /* shared memory area */
#define MSND_PORT   0x290           /* DSP host base port (set on card) */
#define MSND_CFGP   0x260           /* configuration port (set on card) */

char *dsp_dev1();
char *dsp_dev2();
double dsp_scale1(double, int);
double dsp_scale2(double, int);
int dsp_atten1(double *, double *);
int dsp_atten2(double *, double *);
int dsp_bank1(int);
int dsp_bank2(int);
int dsp_rate1(int);
int dsp_rate2(int);
int init_dsp1();
int init_dsp2();
int rxd_rdy1();
int rxd_rdy2();
void dsp_bnkful1(int);
void dsp_bnkful2(int);
void dsp_play1();
void dsp_play2();
void dsp_stop1();
void dsp_stop2();
void flush_data1();
void flush_data2();
void get_dsp_data1(int *, int *, int *);
void get_dsp_data2(int *, int *, int *);
void mem_blk_select1(int);
void mem_blk_select2(int);
void read_cfg1();
void read_cfg2();


int tbflg = 1;
unsigned int port = MSND_PORT;
unsigned int page = MSND_PAGE;
unsigned int cfgp = MSND_CFGP;

static int dsp_select = 0;

/**************************************************************************/

static int
rxd_rdy()
{
    if (dsp_select == 1)
        return (rxd_rdy1());
    else if (dsp_select == 2)
        return (rxd_rdy2());
    return (-1);
}

static void
get_dsp_data(int *w0, int *w1, int *w2)
{
    if (dsp_select == 1)
        get_dsp_data1(w0, w1, w2);
    else if (dsp_select == 2)
        get_dsp_data2(w0, w1, w2);
}

static void
mem_blk_select(int p)
{
    if (dsp_select == 1)
        mem_blk_select1(p);
    else if (dsp_select == 2)
        mem_blk_select2(p);
}

/**************************************************************************/

static void
dsp_flush_data()
{
    if (dsp_select == 1)
        flush_data1();
    else if (dsp_select == 2)
        flush_data2();
}

static int
dsp_bank(int np)
{
    if (dsp_select == 1)
        return (dsp_bank1(np));
    else if (dsp_select == 2)
        return (dsp_bank2(np));
    return (-1);
}

/**************************************************************************/

double
dsp_adjust_rate(double r)
{
    double emn, err;
    float *sr;
    int *rc, nr, i, imn;
 
    static float sr1[15] = {11025, 22050, 44100};
    static float sr2[15] = {
        5512, 6000, 8000, 8269, 11025, 12000, 16000, 16538, 
        22050, 24000, 32000, 33075, 44100, 48000, 66150
    };
    static int nr1 = 3;
    static int nr2 = 15;
    static int rc1[3] = {0, 1, 2};
    static int rc2[15] = {
        8, 4, 1, 12, 9, 5, 2, 13, 10, 6, 3, 14, 11, 7, 15
    };

    if (dsp_select == 1) {
        nr = nr1;
        sr = sr1;
        rc = rc1;
    } else {
        nr = nr2;
        sr = sr2;
        rc = rc2;
    }
    imn = 0;
    emn = fabs(log10(r / sr[0]));
    for (i = 1; i < nr; i++) {
        err = fabs(log10(r / sr[i]));
        if (emn > err) {
            emn = err;
            imn = i;
        }
    }

    return (sr[imn]);
}

double
dsp_set_rate(double r)
{
    double emn, err;
    float *sr;
    int *rc, nr, i, imn;
 
    static float sr1[15] = {11025, 22050, 44100};
    static float sr2[15] = {
        5512, 6000, 8000, 8269, 11025, 12000, 16000, 16538, 
        22050, 24000, 32000, 33075, 44100, 48000, 66150
    };
    static int nr1 = 3;
    static int nr2 = 15;
    static int rc1[3] = {0, 1, 2};
    static int rc2[15] = {
        8, 4, 1, 12, 9, 5, 2, 13, 10, 6, 3, 14, 11, 7, 15
    };

    if (dsp_select == 1) {
        nr = nr1;
        sr = sr1;
        rc = rc1;
    } else {
        nr = nr2;
        sr = sr2;
        rc = rc2;
    }
    imn = 0;
    emn = fabs(log10(r / sr[0]));
    for (i = 1; i < nr; i++) {
        err = fabs(log10(r / sr[i]));
        if (emn > err) {
            emn = err;
            imn = i;
        }
    }
    if (dsp_select == 1)
        (void) dsp_rate1(rc[imn]);
    else if (dsp_select == 2)
        (void) dsp_rate2(rc[imn]);

#ifdef WIN32    
    Sleep(500);
#else
    usleep(5e5);
#endif
    return (sr[imn]);
}

char *
dsp_devnam()
{
    if (dsp_select == 1)
        return (dsp_dev1());
    else if (dsp_select == 2)
        return (dsp_dev2());
    return ("");
}

int
dsp_devnum()
{
    return(dsp_select);
}

int
dsp_test()
{
    if (!tbflg)
	return (0);
#ifdef WIN32
        // Get exclusive handle to device 0   
    hWinRT = WinRTOpenDevice(0, FALSE);
    if (hWinRT == INVALID_HANDLE_VALUE)  // Was the device opened?
    {
        if (LoadDriver(0)) {
            printf("Could not load WinRTdev0.\n");
            return(-1);
        }
        hWinRT = WinRTOpenDevice(0, FALSE);
        if (hWinRT == INVALID_HANDLE_VALUE)  // Was the device opened?
        {
            printf("Could not open WinRTdev0, error code %d.\n", 
                GetLastError());
            return(-1);
        }
    }
#endif /* WIN32 */
    read_cfg1();
    if (init_dsp1() == 0)
        return (dsp_select = 1);
    read_cfg2();
    if (init_dsp2() == 0)
        return (dsp_select = 2);
    dsp_select = -1;
    return (dsp_select);
}

static double cur_att_in = 0;
static double cur_att_out = 0;

double
dsp_atten_input(double at)
{
    if (dsp_select == 1) {
        if (dsp_atten1(&at, &at));
	    cur_att_in = at;
    } else if (dsp_select == 2) {
        if (dsp_atten2(&at, &at));
	    cur_att_in = at;
    } else {
	cur_att_in = 0;
    }
    return (cur_att_in);
}

double
dsp_atten_output(double at)
{
    double sf = 1;

    if (dsp_select == 2) {
	sf = pow(10.0, limit(0, at, 30) / -20);
        dsp_scale2(sf, 0);
        dsp_scale2(sf, 1);
	at = -20 * log10(sf);
	cur_att_out = at;
    } else {
	cur_att_out = 0;
    }
    return (cur_att_out);
}

/* Monterey 20-Jun-97 with gain=32 */
static double da_vfs1[2] = {1.964, 1.964};
static double ad_vfs1[2] = {1.757, 1.757};

   /* Pinnacle 18-Jun-97 with gain=180 */
static double da_vfs2[2] = {2.838, 2.838};
static double ad_vfs2[2] = {2.850, 2.850};

void
dsp_get_vfs(double *da_vfs, double *ad_vfs)
{
    int i;

    for (i = 0; i < 2; i++)
	if (dsp_select == 1) {           
	    da_vfs[i] = da_vfs1[i] / pow(10.0, cur_att_out / 20);
	    ad_vfs[i] = ad_vfs1[i] * pow(10.0, cur_att_in / 20);
	} else if (dsp_select == 2) {
	    da_vfs[i] = da_vfs2[i] / pow(10.0, cur_att_out / 20);
	    ad_vfs[i] = ad_vfs2[i] * pow(10.0, cur_att_in / 20);
	} else {
	    da_vfs[i] = 1;
	    ad_vfs[i] = 1;
	}
}

void
dsp_set_vfs(double *da_vfs, double *ad_vfs)
{
    int i;

    for (i = 0; i < 2; i++)
	if (dsp_select == 1) {           
	    da_vfs1[i] = da_vfs[i] * pow(10.0, cur_att_out / 20);
	    ad_vfs1[i] = ad_vfs[i] / pow(10.0, cur_att_in / 20);
	} else if (dsp_select == 2) {
	    da_vfs2[i] = da_vfs[i] * pow(10.0, cur_att_out / 20);
	    ad_vfs2[i] = ad_vfs[i] / pow(10.0, cur_att_in / 20);
	}
}

void
dsp_info(char *s)
{
    if (dsp_select == 1) {           /* Monterey */
        sprintf(s, "page=%X port=%X", page, port);
    } else if (dsp_select == 2) {   /* Pinnacle */
        sprintf(s, "page=%X port=%X cfgp=%X", page, port, cfgp);
    }
}

int
dsp_check_bank()
{
    int  w0, w1, w2, bank = 0;

    if (rxd_rdy()) {
        get_dsp_data(&w0, &w1, &w2);
        if (w0 == 1) {                  /* bank 1 completed */
            bank = 1;
        } else if (w0 == 3) {           /* bank 2 completed */
            bank = 2;
	}
    }
    return (bank);
}

/**************************************************************************/

static int ndc[2] = {2, 2};
static int sma_size = 0x8000;
static short *sma = NULL;
#ifdef linux
static int sma_fd = -1;
#endif /* linux */

int
dsp_open(int *nc, double *at, double *ms)
{
    nc[0] = ndc[0];	/* max number of input channels */
    nc[1] = ndc[1];	/* max number of output channels */
    at[0] = dsp_atten_input(0.0);
    at[1] = dsp_atten_output(0.0);
    ms[0] = MAX_SAMP;
    ms[1] = MAX_SAMP;

    /* open SMA */

#ifndef linux
#ifndef WIN32
    sma = (short *) (page * 16 + __djgpp_conventional_base);
#else  /* WIN32 */
    {
        WINRT_MEMORY_MAP map;          // description of memory to be mapped
        DWORD  iLength;                // return length from Ioctl call
        // map in desired memory
        map.address = (PVOID)(page * 16);
        map.length  = sma_size;
        if (!WinRTMapMemory(hWinRT, &map, &sma, &iLength))
        {
            printf("Unable to obtain pointer to memory. Error: %d\n", 
                GetLastError());
            printf("Is Memory Address & Count setup in Registry?\n");
            return (1);
        }
    }
#endif /* WIN32 */
#else  /* linux */
    {

	if ((sma_fd = open("/dev/mem", O_RDWR) ) < 0) {
	    perror("MEM");
	    return (2);
	}
	sma = (short *)mmap(
	    (caddr_t)0, 
	    sma_size,
	    PROT_READ|PROT_WRITE,
	    MAP_SHARED|MAP_FIXED,
	    sma_fd, 
	    page << 4
	);
	if ((long)sma < 0) {
	    perror("MMAP");
	    return (3);
	}
    }
#endif /* linux */

    return (0);
}

void
dsp_close()
{
#ifdef linux
    if (sma_fd >= 0)
	close(sma_fd);
#endif /* linux */
}

void
dsp_bank_ready(int bo)
{
    if (dsp_select == 1)
        dsp_bnkful1(bo + 1);
    else if (dsp_select == 2)
        dsp_bnkful2(bo + 1);
}

void
dsp_begin_io()
{
    if (dsp_select == 1)
        dsp_play1();
    else if (dsp_select == 2)
        dsp_play2();
}

void
dsp_stop_io()
{
    if (dsp_select == 1)
        dsp_stop1();
    else if (dsp_select == 2)
        dsp_stop2();
}

int
dsp_reset_io(int ns)
{
    if (ns > sma_size / 8)
	ns = sma_size / 8;
    dsp_bank(ns);
    dsp_stop_io();
    dsp_flush_data();
    return(ns);
}

/**************************************************************************/

void
dsp_bank_put(int bo, int is, int ns, int doit, int os)
{
    int c, i, j, d, n, nc;
    long *s, z = 0;
    short *sbuf = NULL;

    nc = ndc[1];    /* number of output channels */
    mem_blk_select(0);
    sbuf = &sma[bo * (sma_size / 4)] + is * nc;
    ptr_enable();
    n = ns * nc;
    for (c = 0; c < nc; c++) {
	if (bio_.stim_b[c] && doit) {
	    d = bio_.stim_i[c];
	    s = bio_.stim_b[c] + os * d;
	} else {
	    d = 0;
	    s = &z;
	}
	for (i = c, j = 0; i < n; i += nc, j += d) {
	    sbuf[i] = (short) s[j];
	}
    }
    ptr_disable();
}

void
dsp_bank_get(int bo, int is, int ns)
{
    int i, n, nc;
    long *b;
    short *sbuf = NULL;

    nc = ndc[0];    /* number of input channels */
    mem_blk_select(1);
    sbuf = &sma[bo * (sma_size / 4)] + is * nc;
    n = ns * nc;
    b = bio_.bank_b;
    ptr_enable();
    for (i = 0; i < n; i++)
	b[i] = sbuf[i];
    ptr_disable();
}
