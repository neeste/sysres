/* dsp1.c */

#define MSND    "Monterey/Tahiti"
#define STDGN   32

#include<stdio.h>
#include<stdlib.h>
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
#define MSND_MEMM   0x8     /* shared memory map (WO) */
#define MSND_IRQM   0x9     /* IRQ mapping register (WO) */
#define MSND_DSPR   0xA     /* DSP reset control register (WO) */
#define MSND_PROR   0xB     /* PROTEUS reset control register (WO) */
#define MSND_BLKS   0xC     /* memory block select register (WO):
                               0 = X/P:4000-7FFF Y:8000-BFFF
                               1 = X/P:8000-BFFF Y:4000-7FFF */
#define MSND_WAIT   0xD     /* extra wait state register */
#define MSND_BMMR   0xE     /* 8/16-bit memmory mode register */
#define MSND_RSRV   0xF     /* (not used) */

#define CMD_STOP	0x92
#define CMD_PLAY	0x93
#define CMD_RATE	0x94
#define CMD_NPTS	0x95
#define CMD_SETPOT	0x9A
#define CMD_BNK1EMP	0x9B
#define CMD_BNK1FUL	0x9C
#define CMD_BNK2EMP	0x9D
#define CMD_BNK2FUL	0x9E

extern unsigned int port;
extern unsigned int page;

static char *init_errs[] = {
    "No errors",
    "DSP configuration failed",
    "DSP reset failed",
    "DSP program upload failed",
    "Digital pot settings failed",
};
static unsigned char pgm[0x600] = {
#include "tstm.h"
};

#ifdef WIN32
LONG   LoadDriver(LONG);
extern HANDLE hWinRT;                 // device handle
static DWORD  iWinRTlength;           // length of returned buffers
#endif /* WIN32 */

static int
config_dsp()
{
    int pc;

    if (page == 0)
    	pc = 0;
    else if (page == 0xB000)
        pc = 1;
    else if (page == 0xC800)
        pc = 2;
    else if (page == 0xD000)
        pc = 3;
    else if (page == 0xD800)
        pc = 5;
    else if (page == 0xE000)
        pc = 6;
    else if (page == 0xE800)
        pc = 7;
    else
        return (1);
#ifndef WIN32
    _outp(port + MSND_MEMM, pc);
#else  /* WIN32 */
    {
        WINRT_CONTROL_ITEM _WinRTpp01[] = {
            {OUTP_B, MSND_MEMM, 0},
        };
        _WinRTpp01[ 0].value = pc;
        (void)WinRTProcessIoBuffer(hWinRT, _WinRTpp01,
            sizeof(_WinRTpp01), &iWinRTlength);
    }
#endif /* WIN32 */
    return (0);	
}

static int
reset_dsp()
{
    int time_out = 20000, reg;
    
#ifndef WIN32
    _outp(port + MSND_DSPR, 1);  /* turn DSP reset on */
    _outp(port + MSND_DSPR, 0);  /* turn DSP reset off */
    while(time_out-- >= 0) {
        reg = _inp(port + MSND_CVR);
        if (reg == 0x12)
            return (1);
    }
#else  /* WIN32 */
    {
	WINRT_CONTROL_ITEM _WinRTpp02[] = {
	    {OUTP_B, MSND_DSPR, 1},
	    {OUTP_B, MSND_DSPR, 0},
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
    static int inp = STDGN, aux = 0;

    if (dsp_pot_level(inp, inp, 0))
	return (1);
    if (dsp_pot_level(aux, aux, 1))
	return (2);
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
read_cfg1()
{
    int i;
    unsigned int v;
    FILE *cfp = NULL;
    static char *cfn[] = {
    	"msnd.cfg",
#ifdef linux
    	"/usr/etc/msnd.cfg",
    	"/etc/msnd.cfg",
#else
    	"c:/tahiti/msnd.cfg",
    	"c:/monterey/msnd.cfg",
#endif
    };
    static int ncf = sizeof(cfn) / sizeof(cfn[0]);

    for (i = 0; i < ncf; i++)
        if ((cfp = fopen(cfn[i], "r")) != NULL)
            break;
    if (i >= ncf)
        return;
    fscanf(cfp, "%d", &v);
    if (v >= 0xB000 && v <= 0xE800 && (v %0x800) == 0)
        page = v;
    fscanf(cfp, "%d", &v);
    if (v >= 0x210 && v <= 0x3E0 && (v %0x10) == 0)
        port = v;
    fclose(cfp);
}

int
rxd_rdy1()
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
get_dsp_data1(int *w0, int *w1, int *w2)
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
flush_data1()
{
    int w;

    while (rxd_rdy1())
        get_dsp_data1(&w, &w, &w);
}

int
init_dsp1()
{
#ifdef linux
    if (ioperm(port, 16, 3))
	return (-1);
#endif
    if (config_dsp())
    	return (1);
    if (!reset_dsp())
    	return (2);
    if (upload_pgm())
    	return (3);
    flush_data1();
    if (dsp_pots())
	return (4);
    return (0);
}

int
dsp_rate1(int rc)
{
    if (load_dsp_data(0, 0, rc)) {
    	dsp_cmd(CMD_RATE);
        return (0);
    }
    return (1);
}

int
dsp_bank1(int np)
{
    if (load_dsp_data(0, ((np >> 8) & 0xFF), (np & 0xFF))) {
    	dsp_cmd(CMD_NPTS);
        return (0);
    }
    return (1);
}

void
mem_blk_select1(int p)
{
#ifndef WIN32
    _outp(port + MSND_BLKS, p);             /* set SMA */
#else  /* WIN32 */
    WINRT_CONTROL_ITEM _WinRTpp10[] = {
	{OUTP_B, MSND_BLKS, 0},
    };
    _WinRTpp10[ 0].value = p;
    (void)WinRTProcessIoBuffer(hWinRT, _WinRTpp10,
        sizeof(_WinRTpp10), &iWinRTlength);
#endif /* WIN32 */
}

void
dsp_bnkful1(int bnk)
{
    dsp_cmd((bnk == 1) ? CMD_BNK1FUL : CMD_BNK2FUL);
}

void
dsp_play1()
{
    dsp_cmd(CMD_PLAY);
}

void
dsp_stop1()
{
    dsp_cmd(CMD_STOP);
}

char *
dsp_dev1()
{
    return (MSND);
}

char *
dsp_err1(int errno)
{
    return (init_errs[errno]);
}

int
dsp_atten1(double *atl, double *atr)
{
    int gnl, gnr;

    gnl = (int) (STDGN * pow(10.0, *atl / -20) + 0.5);
    gnr = (int) (STDGN * pow(10.0, *atr / -20) + 0.5);
    gnl = limit(1, gnl, 255);
    gnr = limit(1, gnr, 255);
    if (dsp_pot_level(gnl, gnr, 0)) {     /* INP */
	*atl = -20 * log10(gnl / STDGN);
	*atr = -20 * log10(gnr / STDGN);
	return (1);
    }
    return (0);
}

double
dsp_scale1(double sf, int chan)
{
    return(1);
}
