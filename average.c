/* average.c */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sysres.h"
#include "display.h"
#include "sio.h"
#include "fk.h"

double  gr_etime(void);
void    gr_prcl(char *);
void    prn_msg(char *);
void    resp_ft(int);

extern double reject_thresh, dpn_sb, splref;
extern int nrch, nrcp, rchk;
extern int nrej, reject_mode, wtav_mode;
extern float *rt, *rf, *rx;
extern PGMCFG cf;

static double wtav_iwt = 0, wtav_twt = 0;
static int av = 0;

void
init_chkrsp()
{
    av = 0;
    wtav_twt = 0;
}

int
chk_accept(int i, char *s)
{
    double vref, lv[4], rfr, rfi, sum;
    int j, n, acc = 0;
    static double dbref = 0;
    static int ii[4] = {0};
    static int i1 = 0, i2 = 0, nn = 0;

    if (reject_mode == 0) {
        acc = 1;
        *s = '\0';
	wtav_iwt = 1;
    } else if (reject_mode == 1) {
        if (i) {
            vref = 2.0 / cf.nsamp;
            dbref = 20 * log10(cf.sens_mp * splref / vref);
            ii[1] = (int) (cf.nsamp * cf.f1 / cf.rate + 0.5);
            ii[2] = (int) (cf.nsamp * cf.f2 / cf.rate + 0.5);
            ii[0] = 2 * ii[1] - ii[2];
            ii[3] = (ii[0] + ii[1]) / 2;
            nn = nint(ii[0] * dpn_sb / 100);
            if (nn > 0) {
                i1 = limit(ii[2] - ii[1] + 1, ii[0] - nn, ii[1] - 1);
                i2 = limit(ii[2] - ii[1] + 1, ii[0] + nn, ii[1] - 1);
            }
        }
        resp_ft(nrch);
        for (j = 0; j < 4; j++)
            lv[j] = cdb(&rf[ii[j] * 2]) - dbref;
        sum = 0;
        if (nn > 0) {
            n = 0;
            for (j = i1; j <= i2; j++) {
                if (j != ii[0]) {
                    rfr = rf[j * 2];
                    rfi = rf[j * 2 + 1];
                    sum += rfr * rfr + rfi * rfi;
                    n++;
                }
            }
            if (n > 0 && sum > 0)
                lv[3] = 10 * log10(sum / n) - dbref;
        }
        acc = (lv[3] < reject_thresh);
        if (acc) {
            if (wtav_mode == 1 && sum > 0)
                wtav_iwt = 1 / sqrt(sum);
            else if (wtav_mode == 2 && sum > 0)
                wtav_iwt = 1 / sum;
            else
                wtav_iwt = 1;
        }
        sprintf(s, "l1=%.1f l2=%.1f ld=%.1f ln=%.1f nn=%d :", 
            lv[1], lv[2], lv[0], lv[3], nn);
    }
    return(acc);
}

void
chk_resp(int *escflg)
{
    char s[120], am[40], *ms;
    double gt, ct;
    int i, k, init, rsc = 0;
    static double sc = 0;
    static int ns = 0, rj = 0;

    ct = gr_etime();
    gr_prcl("---");
    *s = '\0';
    av++;
    if (!rchk) {
        rsc = (av >= cf.nav);	/* reached stopping criterion ? */
        sprintf(s, "avg=%d", av);
    } else {
	init = (av == 1);
        if (init) {
	    rj = 0;
	    ns = cf.nsamp * nrch;
	    memset(rx, 0, ns * sizeof(float));	/* zero accum. buff. */
        }
        if (chk_accept(init, am)) {
            for (i = 0; i < ns; i++)
                rx[i] += (float) (rt[i] * wtav_iwt);
	    wtav_twt += wtav_iwt;
        } else {
            rj = rj + 1;
            av = av - 1;
        }
        rsc = (av >= cf.nav || (nrej > 0 && rj >= nrej));   /* reached stopping criteria ? */
        if (rsc) {
            sc = (wtav_twt > 0) ? 1 / wtav_twt : 0;
            for (i = 0; i < ns; i++)
                rt[i] = (float) (rx[i] * sc);
        }
        sprintf(s, "%s avg=%d rej=%d", am, av, rj);
    }
    prn_msg(s);
    if (!rsc && !*escflg) {
        gt = cf.gap - (gr_etime() - ct);
        if (gt > 0) {
           ms = "____";
        } else {
           ms = "GAP TOO LONG!";
           gt = 2;
        }
        k = gr_getkey(ms, gt);
        if (k == CTRL_C || k == ESC)
            rsc = 1;
	else
	    gr_prcl("~~~");
    }
    if (rsc)
	*escflg = 1;
}

