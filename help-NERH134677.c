/* help.c */

#include <stdio.h>
#include <string.h>
#include "sio.h"
#include "cardinfo.h"
#include "version.h"
#include "fk.h"

int     get_event(void);
void    clr_all(void);
void    gr_text(int, int, char *);
CARDINFO sio_get_cardinfo(int);

extern int cw, ch, sio_present;

static char *hd[] = {
    "    SysRes Commands                                             page %d  ",
    "-------------------------------------------------------------------------",
    "[press Enter to continue]"
};
static char *hlp1[] = {
    "Configuration commands",
    "  ad = A/D mode [0=No input 1=L 2=R 3=L&R 259=L/R]",
    "  da = D/A mode [0=No output 1=L 2=R 3=L&R]",
    "  dc = DC component removed from response [0=keep 1=remove]",
    "  ia = input attenuation (dB)",
    "  na = number of averages [1 ... 32000]",
    "  ns = number of samples [64 ... 2^20]",
    "  oa = output attenuation (dB)",
    "  sk - number of initial sweeps to skip",
    "  sr = sample rate [11, 22, 44 kHz]",
    "       [or 8, 32, 48, 88, 96 kHz on CardDeluxe]",
    "Frequency response commands",
    "  fc - using chirp stimulus",
    "  fi - using implulse stimulus",
    "  ft - using tone stimulus",
    "  fp - using tone-pair stimulus",
    "  fn - using pseudo-random-noise stimulus",
    "  fu = using user-defined stimulus (= filename)",
    "  fr - using previous stimulus",
    "  nf = noise floor (= # blocks) [4]",
    "  no = normalize to saved response (= filename)",
    "  nt = number of tones in wideband stimuli [0]",
    "  sa = spectral average (= # blocks) [4]",
    ""
};
static char *hlp2[] = {
    "Plot commands",
    "  cs  - erase plot (clear screen)",
    "  crn = specify upper-right corner ($fn=filename)",
    "  dr  = dynamic range (dB)",
    "  dm  = delay axis maximum (ms)",
    "  dn  = delay axis range (ms)",
    "  do  = delay offset removed (ms)",
    "  eml = extra message lines [0]",
    "  pl  - toggle log/linear frequency scale",
    "  pm  = phase axis maximum (cyc)",
    "  pn  = phase axis range (cyc)",
    "  po  = phase offset removed (cyc)",
    "  pr  - plot response vs frequency",
    "  ps  - plot stimulus vs frequency",
    "  shd = show delay (0=no, 1=yes)",
    "  shp = show phase (0=no, 1=yes)",
    "  tr  - plot response vs time",
    "  ts  - plot stimulus vs time",
    "  td  - tick direction (0=in, 1=out)",
    "  te  - plot reverse-time-energy vs time",
    "  vr  = voltage reference (dBV)",
    "  SR  = SPL reference (dBSPL)",
    "  zf1,2 = zoom frequency 1,2",
    "  zt1,2 = zoom time 1,2",
    "  zo  - zoom on/off (toggle)",
    ""
};
static char *hlp3[] = {
    "File commands (Use \"=filename\" to specify a file name.)",
    "  ckfn = check file name",
    "  id = ID string for file name",
    "  lf = log messages (0=off, 1=on)",
    "  ra = read ASCII data file",
    "  rc = read configuration",
    "  rd = read data file (MATLAB)",
    "  rl = read list of commands from file",
    "  rs = read stimulus file (MATLAB)",
    "  sv = sample value for 1 volt (in stimulus files) [1]",
    "  wa = write ASCII data file",
    "  wc = write configuration",
    "  wd = write data file (MATLAB)",
    "  wh = write help file (help.txt)",
    "  ws = write stimulus file (MATLAB)",
    "Printer commands",
    "  PL = printer label [$vn]",
    "  PN = printer name",
    "  PO = print orientation (0=Landscape 1=Portrait)",
    "  PS = print screen (= printer_name)",
    "  PT = printer type (0=PostScript 1=PCL 2=ColorPS)",
    ""
};
static char *hlp4[] = {
    "Stimulus commands",
    "  a1,2,3 = attenutation of tone 1,2,3 (dB)",
    "  bd     = beginning duration (msec)",
    "  dbms1,2,3 = msec per dB (exponential ramp rate)",
    "  ed     = end duration (msec)",
    "  f1,2,3 = frequency of tone 1,2,3 (Hz)",
    "  ga     = gap between stimulus repetions (sec)",
    "  l1,2,3 = level of tone 1,2,3 (dB SPL)",
    "  od     = ramp onset duration (msec)",
    "  rt     = ramp type: 0=None 1=Linear 2=Cosine 3=Blackman",
    "           4=Exponential,5=Lissajous,6=proscan,7=Nuttall",
    "  st     = stimulus type: 0=chirp 1=impulse 2=zero 3=tone",
    "           4=tone-pair 5=noise 6=user-defined 7=masking,",
    "           8=dbl.noise",
    "  vo     = volts (stimulus amplitude)",
    "Masking commands",
    "  ma1 = masker attenuation (dB)",
    "  ma2 = probe attenuation (dB)",
    "  mf1 = masker frequency (Hz)",
    "  mf2 = probe frequency (Hz)",
    "  mmd = masker duration (msec)",
    "  mmr = masker ramp time (msec)",
    "  mms = masker start time (msec)",
    "  mpd = probe duration (msec)",
    "  mpg = probe gap time (msec)",
    "  ms  = initialize forward masking stimulus",
    "  mtd = timing pulse duration (msec)",
    "  mtv = timing pulse amplitude (volts)",
    ""
};
static char *hlp5[] = {
    "DPOAE commands",
    "  avm  = average weighting mode (0=uniform, 1=RMS, 2=MS)",
    "  ca   = calibrate (specify buffer size)",
    "  cach = DP calibration channel (0 or 1)",
    "  ct   = calibration tones (specify frequency in Hz)",
    "  cn   = specify calibrate buffer size (samples)",
    "  dp   - display distortion product (2*F1-F2) vs time",
    "       = display one short-term spectrum",
    "  dpbw = DP envelope bandwidth (Hz)",
    "  dpch = DP analysis channel (0 or 1)",
    "  dpdp = display 3*F1-2*F2 component also (0=NO, 1=YES)",
    "  dpef = DP exponential fit type (0, 1 or 2)",
    "  dpen = DP envelope size (power of 2, 0=STFT)",
    "  dpfc = DP fit complex (0=NO, 1=YES)",
    "  dpft = DP filter type (0=24dB, 1=Blackman,2=Gauss,3=Rect)",
    "  dpno = set DP noise display (0=off, 1=on)",
    "  dpon = set DP order number [-1=2*F1-F2]",
    "  dpsb = DP noise sidebands (% of DP frequency)",
    "  dpsf = filter band around DP frequency (0=NO, 1=YES)",
    "  dpst = DP stimulus (copy to response)",
    "  dpt1 = DP initial time-constant (sec) [0.1]",
    "  om   = offset for DP envelope analysis (msec)",
    "  os   = offset for DP envelope analysis (samples)",
    "  rjm  = reject mode (0=none, 1=DPN)",
    "  rjn  = maximum number of rejects",
    "  rjt  = reject threshold (dB SPL)",
    ""
};
static char *hlp6[] = {
    "Lissajous Path commands",
    "  amin1,2,3 = amplitude modulation index [0]",
    "  amnc1,2,3 = number of AM cycles [0]",
    "  lpma1,2,3 = level modulation amplitude  (dB) [0]",
    "  lpmb1,2,3 = 1st other LM amp.  (dB) [0]",
    "  lpmc1,2,3 = 2nd other LM amp.  (dB) [0]",
    "  lpnc1,2,3 = number of LM cycles [0]",
    "  lpph1,2,3 = phase (degrees) [0]",
    "  lpol = compute optimum level, set L1, and write inc file",
    "  lsol = compute optimum level, set L1, and write lst file",
    "  lsavt = opt. lev. averaging time in lst file (sec) [32]",
    "  lsbeg = opt. lev. begining L2 in lst file (dB SPL) [-15]",
    "  lsf3  = suppr. frequency f3 in lst file (dB) [0]",
    "  lsinc = opt. lev. increment of L2 in lst file (dB) [5]",
    "  lsl3a  = suppr. level constant L3a in lst file (dB) [0]",
    "  lsl3b  = suppr. level slope L3b re L2 in lst file (dB) [0]",
    "  lsnlv = opt. lev. noise level in lst file (dB SPL) [-25]",
    "  lsnum = opt. lev. number of L2 in lst file [20]",
    "  lsrep = number of repeats in lst file [2]",
    "  lssnr = opt. lev. SNR in lst file (dB) [60]",
    "  olmin = opt. lev. offset from minimum L2 (dB) [6]",
    "  olord = opt. lev. order of polynomial fit (1-3) [0]",
    "  olsnr = opt. lev. SNR threshold (dB) [9]",
    ""
};
static char *hlp7[] = {
    "Other commands",
    "  after N C - delay N sec, then do C",
    "  be     = beep",
    "  bp     = band-pass filter (from F1 to F2)",
    "  cd     - get or set current directory",
    "  co     = comment",
    "  dbg    = debug mode (0=off, 1=on)",
    "  di     = SIO device information",
    "  dl     = SIO device list",
    "  et     = elapsed time",
    "  go     - average and show time-domain response",
    "  late   - set ASIO device latency",
    "  ls     - list file names",
    "  msg    = display message",
    "  ow     = over write (data files) (0=no, 1=yes)",
    "  pa     = pause (with message)",
    "  pg     = pregap before I/O (sec)",
    "  q      - quit",
    "  rp     - reset parameters",
    "  sl     = sleep for N seconds",
    "  sm     = sensitivity of microphone (V/Pa)",
    "  to     = time-out pause after N seconds",
    "  wci    = write cardinfo to file",
    "  vn     - version number",
    ""
};
static char **hlp[]  = {
    hlp1, hlp2, hlp3, hlp4, hlp5, hlp6, hlp7,
};
static int nhp = sizeof(hlp) / sizeof(hlp[0]);

static int
prn_page(char **tl, int pn)
{
    char    s[80];
    int     x, y, i;

    clr_all();
    x = cw / 2;
    y = cw / 4;
    if (pn>0) {
	sprintf(s, hd[0], pn);
        gr_text(x, y, s);
	y += ch;
        gr_text(x, y, hd[1]);
	y += ch;
    }
    for (i = 0; *tl[i]; i++) {
	gr_text(x, y, tl[i]);
	y += ch;
    }
    gr_text(x, y, hd[1]);
    y += ch;
    gr_text(x, y, hd[2]);
    return (get_event());
}

void
prn_help()
{
    int i, c;

    for (i = 0; i < nhp; i++) {
	c = prn_page(hlp[i], i + 1);
	if (c == ESC || c == CTRL_C) {
	    break;
	} else if (c == CTRL_H) {
	    i = (i > 1) ? i - 2 : -1;
	}
    }
}

static void
wh_page(FILE *fp, char **tl)
{
    int     i;

    for (i = 0; *tl[i]; i++)
	fprintf(fp, "     %s\n", tl[i]);
}

void
write_help(char *fn, char *msg)
{
    int i;
    FILE *fp;
    
    fp = fopen(fn, "wt");
    if (fp == NULL) {
        sprintf(msg, "Can't open help file: %s.", fn);
        return;
    }
    fprintf(fp, "     %s\n     %s\n", VER, hd[1]);
    for (i = 0; i < nhp; i++)
	wh_page(fp, hlp[i]);
    fclose(fp);
    sprintf(msg, "Wrote help file to %s.", fn);
}

/**********************************************************************/

void
prn_devlst()
{
    char    s[80], msg[64], *select;
    int     x, y, i, j, curdev;
    CARDINFO ci;

    if (sio_present){
        curdev = sio_set_device(0);  // get current device number
    } else {
	curdev = 0;
    }

    clr_all();
    x = cw / 2;
    y = cw / 4;
    gr_text(x, y, "Device List");
    y += ch;
    gr_text(x, y, hd[1]);
    y += ch;

    for (i = 1; i < 30; i++) {
	j = sio_set_device(i);
	if (j != i) 
	    break;
	select = (i == curdev) ? ">" : " ";
        sio_get_device(msg);
	sprintf(s, "%s %2d: %s", select, i, msg);
	gr_text(x, y, s);
	y += ch;
    }
    gr_text(x, y, hd[1]);
    y += ch;
    sio_set_device(curdev);
    if (curdev > 0) {
        ci = sio_get_cardinfo(-1);
	sprintf(s, "cardinfo: name = %s", ci.name);
	gr_text(x, y, s);
	y += ch;
	sprintf(s, "bits=%d left=%d nbps=%d ncad=%d ncda=%d ad_vfs=%.3f,%.3f da_vfs=%.3f,%.3f", 
	    ci.bits, ci.left, ci.nbps, ci.ncad, ci.ncda, 
	    ci.ad_vfs[0], ci.ad_vfs[1], ci.da_vfs[0], ci.da_vfs[1]);
	gr_text(x, y, s);
	y += ch;
	gr_text(x, y, hd[1]);
	y += ch;
    }
    gr_text(x, y, sio_version());
    y += ch;
    gr_text(x, y, hd[2]);
    y += ch;
    get_event();
}
