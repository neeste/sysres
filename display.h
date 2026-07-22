/* display.h */

#define NDP 5

typedef struct{
    char    dp_dsp[NDP];
    double  dbvref, dbrange, dbvpu, zf1, zf2, zt1, zt2, lv[4];
    double  delay_max, delay_range, delay_off;
    double  phase_max, phase_range, phase_off;
    double  df_khz, dpbw, dpsb, dpt1, dpt2, efc[10];
    int     nfft, dpen, dpft, ofst, nnsb, npfit;
    short   log_axis, show_phase, show_delay, clear_screen, time_axis;
    short   i1, i2, nc, dp, dpef, dpfc, dpfw, zoom, tic_dir, eml, dpch;
    short   dpon, dprm;
} PLTFMT;
typedef struct{
    char    prn_name[80], prn_label[80];
    double  volts, rate, delay, sens_mp, f1, f2, f3, l1, l2, l3;
    double  beg_dur, end_dur, rmp_dur, gap, pregap, olsnr;
    double  dbms[3], lpnc[3], lpph[3], lpma[3], lpmb[3], lpmc[3];
    double  amin[3], amnc[3];
    double  lsavt, lsbeg, lsinc, lsnlv, lssnr, lsf3, lsl3a, lsl3b;
    double  pefr, pesl;
    int     nsamp, ncal;
    short   prn_type, prn_orient, ovrwrt, ad_mode, da_mode;
    short   nav, stm_typ, ntone, rmp_typ, cal_chn, dpsf, acsf;
    short   lsnum, lsrep, olmin, olord, choi, choo;
} PGMCFG;
typedef struct{
    double  dbref, vref, xo;
    double  fmn, fmx, tmn, tmx, dbvmn, dbvmx, phmn, phmx, vmn, vmx;
    float  *dpdb, *dpph;
    int     nfft, time_axis, log_axis, unit_type;
    int     ax, ay, bx, by, bh, bw, cx, cy, np, nn;
    int     mg, wo, wi, wj[NDP], wave;
    short   nframe, sh_b, sh_d, sh_p, phdp, td, ss;
} FRAME;

#ifdef WIN32
int _isnan();
#else
#define _isnan isnan
#endif
