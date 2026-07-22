/* dsp2.c */

#define MSND    "Pinnacle/Fiji"
#define STDGN    180

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

#ifdef linux
#include <unistd.h>
#include <asm/io.h>
#define _inp        inb
#define _outp(p,v)  outb(v,p)
int ioperm(unsigned long, unsigned long, int);
#else  /* linux */
#ifdef WIN32
#include <windows.h>
#include <winioctl.h>
#include <WinRTctl.h>
#define ptr_enable()
#define ptr_disable()
#define _exit       exit
#else  /* WIN32 */
#include <conio.h>
#include <pc.h>
#define _inp    inp
#define _outp   outp
#endif /* WIN32 */
#endif /* linux */

#define limit(mn,aa,mx) (((aa)<(mn))?(mn):((aa)>(mx))?(mx):(aa))

/* the first 8 MSND I/O registers contain the DSP host interface (HI) */
#define MSND_ICR    0x0     /* HI interrupt control register (R/W) */
#define MSND_CVR    0x1     /* HI command vector (R/W) */
#define MSND_ISR    0x2     /* HI interrupt status register (RO) */
#define MSND_IVR    0x3     /* HI interrupt vector register (R/W) */
#define MSND_ZER    0x4     /* byte of zero (RO) */
#define MSND_DAH    0x5     /* HI Tx/Rx data high */
#define MSND_DAM    0x6     /* HI Tx/Rx data middle */
#define MSND_DAL    0x7     /* Tx/Rx data low */

/* the next 4 MSND I/O registers contain Bus Ctrl. and Board Info */
#define MSND_DSPR   0x4     /* DSP reset control register (WO) */

#define CMD_STOP	0x92
#define CMD_PLAY	0x93
#define CMD_RATE	0x94
#define CMD_NPTS	0x95
#define CMD_SCALE1      0x96
#define CMD_SCALE2      0x97
#define CMD_SETPOT	0x9A
#define CMD_BNK1EMP	0x9B
#define CMD_BNK1FUL	0x9C
#define CMD_BNK2EMP	0x9D
#define CMD_BNK2FUL	0x9E

extern unsigned int port;
extern unsigned int page;
extern unsigned int cfgp;

static char *init_errs[] = {
    "No errors",
    "DSP configuration failed",
    "DSP reset failed",
    "DSP program upload failed",
    "Shared-memory area reset failed",
    "Digital pot settings failed",
};
static unsigned char pgm[0x600] = {
#include "tstp.h"
};
static struct {int act, ad0, ad1, irq, ram;} res[5];

#ifdef WIN32
LONG   LoadDriver(LONG);
extern HANDLE hWinRT;                 // device handle
static DWORD  iWinRTlength;           // length of returned buffers
#endif /* WIN32 */

static void
wrcfg(int a, int v)
{
#ifndef WIN32
    _outp(cfgp, a);
    _outp(cfgp + 1, v);
#else  /* WIN32 */
    {
        WINRT_CONTROL_ITEM _WinRTpp01[] = {
            {OUTP_BA, 0, 0},
            {OUTP_BA, 0, 0},
        };
        _WinRTpp01[ 0].port = cfgp;
        _WinRTpp01[ 0].value = a;
        _WinRTpp01[ 1].port = cfgp + 1;
        _WinRTpp01[ 1].value = v;
        (void)WinRTProcessIoBuffer(hWinRT, _WinRTpp01,
            sizeof(_WinRTpp01), &iWinRTlength);
    }
#endif /* WIN32 */
}

static int
config_dsp()
{
    wrcfg(0x07, 0);
    wrcfg(0x60, res[0].ad0 >> 8);
    wrcfg(0x61, res[0].ad0 & 0xFF);

    wrcfg(0x07, 0);
    wrcfg(0x62, res[0].ad1 >> 8);
    wrcfg(0x63, res[0].ad1 & 0xFF);

    wrcfg(0x07, 0);
    wrcfg(0x70, res[0].irq >> 8);
    wrcfg(0x71, res[0].irq & 0xFF);

    wrcfg(0x07, 0);
    wrcfg(0x40, res[0].ram >> 8);
    wrcfg(0x41, res[0].ram & 0xFF);
    wrcfg(0x42, 3);

    wrcfg(0x07, 0);
    wrcfg(0x30, res[0].act);

    return (0);
}

static int
reset_dsp()
{
    int time_out = 20000, reg;
    
#ifndef WIN32
    _outp(port + MSND_DSPR, 0);  /* turn DSP reset on */
    _outp(port + MSND_DSPR, 2);  /* turn DSP reset off */
    while(time_out-- >= 0) {
        reg = _inp(port + MSND_CVR);
        if (reg == 0x12)
            return (1);
    }
#else  /* WIN32 */
    {
	WINRT_CONTROL_ITEM _WinRTpp02[] = {
            {OUTP_B, MSND_DSPR, 0},
            {OUTP_B, MSND_DSPR, 2},
	};
	(void)WinRTProcessIoBuffer(hWinRT, _WinRTpp02,
            sizeof(_WinRTpp02), &iWinRTlength);
    }
    while(time_out-- >= 0) {
	WINRT_CONTROL_ITEM _WinRTpp03[] = {
            {INP_B, MSND_CVR, 0},
	};
	(void)WinRTProcessIoBuffer(hWinRT, _WinRTpp03,
            sizeof(_WinRTpp03), &iWinRTlength);
	reg = _WinRTpp03[ 0].value;
        if (reg == 0x12)
            return (1);
    }
#endif /* WIN32 */
    return (0);
}

static int
txd_rdy()
{
    int     time_out = 20000, reg;
    
    while(time_out-- >= 0) {
#ifndef WIN32
        reg = _inp(port + MSND_ISR);
#else  /* WIN32 */
        WINRT_CONTROL_ITEM _WinRTpp06[] = {
            {INP_B, MSND_ISR, 0},
        };
        (void)WinRTProcessIoBuffer(hWinRT, _WinRTpp06,
            sizeof(_WinRTpp06), &iWinRTlength);
        reg = _WinRTpp06[ 0].value;
#endif /* WIN32 */
        if (reg & 0x2)
            return (1);
    }
/*    printf("txd_rdy timeout\n"); */
    return (0);
}

static int
load_dsp_data(int w0, int w1, int w2)
{
    if (txd_rdy()) {
#ifndef WIN32
        _outp(port + MSND_DAH, w0);
        _outp(port + MSND_DAM, w1);
        _outp(port + MSND_DAL, w2);
#else  /* WIN32 */
	WINRT_CONTROL_ITEM _WinRTpp07[] = {
	    {OUTP_B, MSND_DAH, 0},
	    {OUTP_B, MSND_DAM, 0},
	    {OUTP_B, MSND_DAL, 0},
	};
	_WinRTpp07[ 0].value = w0;
	_WinRTpp07[ 1].value = w1;
	_WinRTpp07[ 2].value = w2;
	(void)WinRTProcessIoBuffer(hWinRT, _WinRTpp07,
				sizeof(_WinRTpp07), &iWinRTlength);
#endif
        return (1);
    }
/*    printf("load_dsp_data failed\n"); */
    return (0);
}

static int
cvr_rdy()
{
    int time_out = 20000, reg;
    
    while(time_out-- >= 0) {
#ifndef WIN32
        reg = _inp(port + MSND_CVR);
#else  /* WIN32 */
	WINRT_CONTROL_ITEM _WinRTpp08[] = {
		{INP_B, MSND_CVR, 0},
	};
	(void)WinRTProcessIoBuffer(hWinRT, _WinRTpp08,
            sizeof(_WinRTpp08), &iWinRTlength);
	reg = _WinRTpp08[ 0].value;
#endif /* WIN32 */
        if ((reg & 0x80) == 0)
            return (1);
    }
/*    printf("cvr_rdy timeout\n"); */
    return (0);
}

static void
dsp_cmd(int c)
{
    if (cvr_rdy()) {
#ifndef WIN32
	_outp(port + MSND_CVR, c);
#else  /* WIN32 */
	WINRT_CONTROL_ITEM _WinRTpp09[] = {
		{OUTP_B, MSND_CVR, 0},
	};
	_WinRTpp09[ 0].value = c;
	(void)WinRTProcessIoBuffer(hWinRT, _WinRTpp09,
            sizeof(_WinRTpp09), &iWinRTlength);
#endif /* WIN32 */
    }
}

static int 
dsp_pot_level(int left, int right, int select)
{
    if (load_dsp_data(left, right, select)) {
	dsp_cmd(CMD_SETPOT);
	return (0);
    }
    return (1);
}

static int
dsp_pots()
{
    static int inp = STDGN, aux = 0, mic = STDGN;

    if (dsp_pot_level(inp, inp, 0))
	return (1);
    if (dsp_pot_level(aux, aux, 1))
	return (2);
    if (dsp_pot_level(mic, mic, 2))
	return (3);
    return (0);
}

static int
upload_pgm()
{
    unsigned char *bb, w0, w1, w2;
    int     i;
    static int nw = 512;

    bb = pgm;
    for (i = 0; i < nw; i++) {
	w0 = *bb++;
	w1 = *bb++;
	w2 = *bb++;
	if (!load_dsp_data(w0, w1, w2))
	    return (1);
    }
    return (0);
}

/**************************************************************************/

void
read_cfg2()
{
    char line[80];
    int i, dev = 0;
    unsigned int v;
    FILE *cfp = NULL;
    static char *cfn[] = {
    	"pincfg.ini", 
#ifdef linux
    	"/usr/etc/pincfg.ini",
    	"/etc/pincfg.ini",
#else
    	"c:/tbspros/dosapps/pincfg.ini",
    	"c:/pinnacle/program/pincfg.ini",
#endif
    };
    static int ncf = sizeof(cfn) / sizeof(cfn[0]);
    
    res[0].ad0 = port;
    res[0].ram  = page >> 4;
    res[0].act = 1;
    for (i = 0; i < ncf; i++)
        if ((cfp = fopen(cfn[i], "r")) != NULL)
            break;
    if (i >= ncf)
        return;
    while(fgets(line, 80, cfp) != NULL) {
        if (strncmp(line, "[ConfigControl]", 15) == 0) {
            dev = -1;
        } else if (strncmp(line, "[LogicalDevice", 14) == 0) {
            dev = line[14] - '0';
        } else if (dev == -1 && strncmp(line, "IOAddress0", 10) == 0) {
            sscanf(line + 10, "=%x", &v);
            if (v >= 0x100 && v <= 0x3F0 && (v %0x10) == 0)
                cfgp = v;
        } else if (dev >= 0 && dev < 4) {
            if (strncmp(line, "Active", 6) == 0) {
                sscanf(line + 6, "=%x", &v);
                res[dev].act = v;
            } else if (strncmp(line, "IOAddress0", 10) == 0) {
                sscanf(line + 10, "=%x", &v);
                if (v >= 0x100 && v <= 0x3F0 && (v %0x10) == 0)
                    res[dev].ad0 = v;
            } else if (strncmp(line, "IOAddress1", 10) == 0) {
                sscanf(line + 10, "=%x", &v);
                if (v >= 0x100 && v <= 0x3F0 && (v %0x10) == 0)
                    res[dev].ad1 = v;
            } else if (strncmp(line, "IRQNumber", 9) == 0) {
                sscanf(line + 9, "=%x", &v);
                if (v >= 5 && v <= 12 && (v %0x10) == 0)
                    res[dev].ad0 = v;
            } else if (strncmp(line, "RAMAddress", 10) == 0) {
                sscanf(line + 10, "=%x", &v);
                if (v >= 0xB000 && v <= 0xE800 && (v %0x800) == 0)
                    res[dev].ram = v >> 4;
            }
        }
    }
    fclose(cfp);
    port = res[0].ad0;
    page = res[0].ram << 4;
}

int
rxd_rdy2()
{
    int reg;

#ifndef WIN32
    reg = _inp(port + MSND_ISR);
#else
    WINRT_CONTROL_ITEM _WinRTpp04[] = {
        {INP_B, MSND_ISR, 0},
    };
    (void)WinRTProcessIoBuffer(hWinRT, _WinRTpp04,
        sizeof(_WinRTpp04), &iWinRTlength);
    reg = _WinRTpp04[ 0].value;
#endif /* WIN32 */
    return (reg & 0x1);
}

void
get_dsp_data2(int *w0, int *w1, int *w2)
{
#ifndef WIN32
    *w0 = _inp(port + MSND_DAH);
    *w1 = _inp(port + MSND_DAM);
    *w2 = _inp(port + MSND_DAL);
#else  /* WIN32 */
    WINRT_CONTROL_ITEM _WinRTpp05[] = {
        {INP_B, MSND_DAH, 0},
        {INP_B, MSND_DAM, 0},
        {INP_B, MSND_DAL, 0},
    };
    (void)WinRTProcessIoBuffer(hWinRT, _WinRTpp05,
        sizeof(_WinRTpp05), &iWinRTlength);
    *w0 = _WinRTpp05[ 0].value;
    *w1 = _WinRTpp05[ 1].value;
    *w2 = _WinRTpp05[ 2].value;
#endif /* WIN32 */
}

void
flush_data2()
{
    int w;

    while (rxd_rdy2())
        get_dsp_data2(&w, &w, &w);
}

int
init_dsp2()
{
#ifdef linux
    if (ioperm(port, 16, 3))
	return (-1);
    (void) ioperm(cfgp, 2, 3);
#endif
    if (config_dsp())
    	return (1);
    if (!reset_dsp())
    	return (2);
    if (upload_pgm())
    	return (3);
    flush_data2();
    if (dsp_pots())
	return (4);
    return (0);
}

int
dsp_rate2(int rc)
{
    if (load_dsp_data(0, 0, rc)) {
    	dsp_cmd(CMD_RATE);
        return (0);
    }
    return (1);
}

int
dsp_bank2(int np)
{
    if (load_dsp_data(0, ((np >> 8) & 0xFF), (np & 0xFF))) {
    	dsp_cmd(CMD_NPTS);
        return (0);
    }
    return (1);
}

void
mem_blk_select2(int p)
{
#ifndef WIN32
    _outp(port + MSND_DSPR, p | 2);
#else  /* WIN32 */
    WINRT_CONTROL_ITEM _WinRTpp10[] = {
	{OUTP_B, MSND_DSPR, 0},
    };
    _WinRTpp10[ 0].value = p | 2;
    (void)WinRTProcessIoBuffer(hWinRT, _WinRTpp10,
        sizeof(_WinRTpp10), &iWinRTlength);
#endif /* WIN32 */
}

void
dsp_bnkful2(int bnk)
{
    dsp_cmd((bnk == 1) ? CMD_BNK1FUL : CMD_BNK2FUL);
}

void
dsp_play2()
{
    dsp_cmd(CMD_PLAY);
}

void
dsp_stop2()
{
    dsp_cmd(CMD_STOP);
}

char *
dsp_dev2()
{
    return (MSND);
}

char *
dsp_err2(int errno)
{
    return (init_errs[errno]);
}

int
dsp_atten2(double *atl, double *atr)
{
    int gnl, gnr;

    gnl = (int) (STDGN - *atl * 2 + 0.5);
    gnr = (int) (STDGN - *atr * 2 + 0.5);
    gnl = limit(0, gnl, 255);
    gnr = limit(0, gnr, 255);
    if (dsp_pot_level(gnl, gnr, 0)) {   	/* INP */
	if (dsp_pot_level(gnl, gnr, 2)) {   	/* MIC */
	    *atl = (STDGN - gnl) / 2.0;
	    *atr = (STDGN - gnr) / 2.0;
	    return (1);
	}
    }
    return (0);
}

double
dsp_scale2(double sf, int chan)
{
    long isf;
    int w0, w1, w2;

    isf = (long) (0x800000 * limit(0, sf, 1) + 0.5);
    w0 = (isf >> 16) & 0xFF;
    w1 = (isf >>  8) & 0xFF;
    w2 = isf & 0xFF;
    if (load_dsp_data(w0, w1, w2)) {
	dsp_cmd(chan == 0 ? CMD_SCALE1 : CMD_SCALE2);
    }
    return (1);
}
