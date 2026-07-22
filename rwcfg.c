/* rwcfg.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "version.h"
#include "sysres.h"
#include "sio.h"

extern int nsch, nrch, nrcp, sio_present;

int
write_config(char *msg, char *fn, PGMCFG *cf)
{
    FILE *fp;
    
    if (!*fn) {
        strcpy(msg, "");
        return (0);
    }
    fp = fopen(fn, "wt");
    if (fp == NULL) {
        sprintf(msg, "Can't write configuration to file (%s).", fn);
        return (0);
    }
    fprintf(fp, "; %s\n", VER);
    fprintf(fp, "fmax=%10.2f\n", cf->rate / 2);
    fprintf(fp, "delay=%9.6f\n", cf->delay);
    fprintf(fp, "nsamp=%d\n", cf->nsamp);
    fprintf(fp, "prn_name=%s\n", cf->prn_name);
    fprintf(fp, "prn_type=%d\n", cf->prn_type);
    fprintf(fp, "prn_orient=%d\n", cf->prn_orient);
    fprintf(fp, "prn_label=%s\n", cf->prn_label);
    fprintf(fp, ";\n");

    fprintf(fp, "; ncal=%d\n", cf->ncal);
    fprintf(fp, "; navg=%d\n", cf->nav);
    fprintf(fp, "; ad_mode=%d\n", cf->ad_mode);
    fprintf(fp, "; da_mode=%d\n", cf->da_mode);
    fprintf(fp, "; stim=%d\n", cf->stm_typ);
    fprintf(fp, "; volts=%.4f\n", cf->volts);
    fprintf(fp, "; sens_mp=%.4f\n", cf->sens_mp);
    fprintf(fp, "; f1=%.0f\n", cf->f1);
    fprintf(fp, "; f2=%.0f\n", cf->f2);
    fprintf(fp, "; f3=%.0f\n", cf->f3);
    fprintf(fp, "; l1=%.0f\n", cf->l1);
    fprintf(fp, "; l2=%.0f\n", cf->l2);
    fprintf(fp, "; l3=%.0f\n", cf->l3);
    fprintf(fp, "; beg_dur=%.3g\n", cf->beg_dur);
    fprintf(fp, "; end_dur=%.3g\n", cf->end_dur);
    fprintf(fp, "; rmp_dur=%.3g\n", cf->end_dur);
    fprintf(fp, "; gap=%.3g\n", cf->gap);
    fprintf(fp, "; pregap=%.3g\n", cf->pregap);

    fclose(fp);

    sprintf(msg, "Wrote configuration to file (%s).", fn);
    return (1);
}

int
read_config(char *msg, char *fn, PGMCFG *cf)
{
    char s[80];
    int i, n;
    FILE *fp;

    if (!*fn) {
        strcpy(msg, "");
        return (0);
    }
    fp = fopen(fn, "rt");
    if (fp == NULL) {
        strcpy(msg, "");
        return (0);
    }
    while (fgets(s, 80, fp) != NULL) {
    	n = strlen(s);
    	for (i = n - 1; i > 0 && s[i] && s[i] <= ' '; i--)  /* trim */
            s[i] = 0;
        if (strncmp(s, "fmax=", 5) == 0) {
            cf->rate = atof(s + 5) * 2;
            if (sio_present)
                cf->rate = sio_set_rate(cf->rate);
        } else if (strncmp(s, "nsamp=", 6) == 0) {
            n = atoi(s + 6);
            cf->nsamp = limit(MIN_NSAMP, n, MAX_NSAMP);
        } else if (strncmp(s, "rate=", 5) == 0) {
            cf->rate = atof(s + 5);
            if (sio_present)
                cf->rate = sio_set_rate(cf->rate);
        } else if (strncmp(s, "delay=", 6) == 0) {
            cf->delay = atof(s + 6);
        } else if (strncmp(s, "prn_name=", 9) == 0) {
            strcpy(cf->prn_name, s + 9);
        } else if (strncmp(s, "prn_type=", 9) == 0) {
            cf->prn_type = atoi(s + 9);
        } else if (strncmp(s, "prn_orient=", 11) == 0) {
            cf->prn_orient = atoi(s + 11);
        } else if (strncmp(s, "prn_label=", 10) == 0) {
            strcpy(cf->prn_label, s + 10);
        } else if (strncmp(s, "ncal=", 5) == 0) {
            cf->ncal = atoi(s + 5);
        } else if (strncmp(s, "navg=", 5) == 0) {
            cf->nav = atoi(s + 5);
        } else if (strncmp(s, "ad_mode=", 8) == 0) {
            cf->ad_mode = atoi(s + 8);
        } else if (strncmp(s, "da_mode=", 8) == 0) {
            cf->da_mode = atoi(s + 8);
        } else if (strncmp(s, "stim=", 5) == 0) {
            cf->stm_typ = atoi(s + 5);
	} else if (strncmp(s, "volts=", 6) == 0) {
            cf->volts = atof(s + 6);
        } else if (strncmp(s, "sens_mp=", 8) == 0) {
            cf->sens_mp = atof(s + 8);
        } else if (strncmp(s, "f1=", 3) == 0) {
            cf->f1 = atof(s + 3);
        } else if (strncmp(s, "f2=", 3) == 0) {
            cf->f2 = atof(s + 3);
        } else if (strncmp(s, "f3=", 3) == 0) {
            cf->f3 = atof(s + 3);
        } else if (strncmp(s, "l1=", 3) == 0) {
            cf->l1 = atof(s + 3);
        } else if (strncmp(s, "l2=", 3) == 0) {
            cf->l2 = atof(s + 3);
        } else if (strncmp(s, "l3=", 3) == 0) {
            cf->l3 = atof(s + 3);
	} else if (strncmp(s, "beg_dur=", 8) == 0) {
            cf->beg_dur = atof(s + 8);
	} else if (strncmp(s, "end_dur=", 8) == 0) {
            cf->end_dur = atof(s + 8);
	} else if (strncmp(s, "rmp_dur=", 8) == 0) {
            cf->rmp_dur = atof(s + 8);
	} else if (strncmp(s, "gap=", 4) == 0) {
            cf->gap = atof(s + 4);
	} else if (strncmp(s, "pregap=", 7) == 0) {
            cf->pregap = atof(s + 7);
        }
    }
    fclose(fp);

    sprintf(msg, "Read configuration from file (%s).", fn);
    return (1);
}
