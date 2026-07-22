/* rwfile.c */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "display.h"

#define MIN_NSAMP   64
#define ND          MIN_NSAMP

#ifdef WIN32
#include <io.h>
#else /* WIN32 */
#define __USE_XOPEN
#include <unistd.h>
#define _open       open
#define _read       read
#define _write      write
#define _lseek      lseek
#define _close      close
#define _swab       swab
#define _S_IREAD    S_IREAD
#define _S_IWRITE   S_IWRITE
#define __USE_XOPEN	/* for Linux */
#endif /* WIN32 */
#ifdef linux
#define PMODE       0666
#else
#define PMODE       _S_IREAD | _S_IWRITE
#endif
#ifndef _O_BINARY
#define _O_BINARY   0
#endif
#ifndef _O_RDONLY
#define _O_RDONLY   O_RDONLY
#define _O_RDWR     O_RDWR
#define _O_CREAT    O_CREAT
#endif
#define OFLAG       ((int) (_O_RDWR | _O_BINARY | _O_CREAT))

int     alloc_mem(int);
void    prn_msg(char *);

extern float *st, *rt;
extern int nsch, nrch, nrcp;
extern FILE *mfp;

typedef struct {
    long nrow, ncol, next; 
    char mach, roco, dtyp, text, cmpx, nchr, bswp, vers;
} MATHDR;

static int bsiz[5] = {8, 4, 4, 2, 4};
static char *mat_key = "MATLAB 5.0 MAT-file,";
static long ldbg = 0;

/*............................ SWAP BYTE ................................*/

static void
sswab(short *a, int n)
{
    _swab((char *) a, (char *) a, n * 2);
}

static void
lswab(long *a, int n)
{
    unsigned char *ch, tmp;

    while (n-- > 0) {
	ch = (unsigned char *) a++;
	tmp = ch[3];
	ch[3] = ch[0];
	ch[0] = tmp;
	tmp = ch[2];
	ch[2] = ch[1];
	ch[1] = tmp;
    }
}

static void
dswab(double *a, int n)
{
    unsigned char *ch, tmp;

    while (n-- > 0) {
	ch = (unsigned char *) a++;
	tmp = ch[7];
	ch[7] = ch[0];
	ch[0] = tmp;
	tmp = ch[6];
	ch[6] = ch[1];
	ch[1] = tmp;
	tmp = ch[5];
	ch[5] = ch[2];
	ch[2] = tmp;
	tmp = ch[4];
	ch[4] = ch[3];
	ch[3] = tmp;
    }
}

/*............................ MAT WRITE ................................*/

/*
 *  matlab header -
 *	type = MOPT where:
 *	M=0 for pc; 1 for sun
 *	O=0 col or 1 row;
 *	P= [0,1,2,3,4]=>[r*8,r*4,I*4,I*2,I*4 unsigned integers]
 *	T= 0 for matrix or 1 for text (stored as r*4 numbers 0<i<255).
 */
static long
encode_mopt(int p, int t)
{
    int     a = 1;
    char   *b = (char *) (&a);

    return (b[1] * 1000 + p * 10 + t);    
}

static void
mat_wr(int fd, char *nam, void *p, int nr, int nc, int dtyp, int txt, int cx)
{
    int     sz;
    long    ns, hdr[5];

    ns = strlen(nam) + 1;
    hdr[0] = encode_mopt(dtyp, txt);
    hdr[1] = nr; 		/* rows */
    hdr[2] = nc;		/* cols */
    hdr[3] = cx;		/* 0 for real, 1 for complex */
    hdr[4] = ns;	        /* name length (including null byte) */
    sz = bsiz[dtyp];
    if (cx)
        sz *= 2;
    (void) _write(fd, hdr, 20);
    (void) _write(fd, nam, (int) ns);
    _write(fd, p, nr * nc * sz);
}

#define mat_wr_d(f, v, p, n, c)    mat_wr(f, v, p, c, n, 0, 0, 0)
#define mat_wr_f(f, v, p, n, c)    mat_wr(f, v, p, c, n, 1, 0, 0)
#define mat_wr_l(f, v, p, n, c)    mat_wr(f, v, p, c, n, 2, 0, 0)
#define mat_wr_s(f, v, p, n, c)    mat_wr(f, v, p, c, n, 3, 0, 0)

/*............................ MAT READ ................................*/

static int
valid_mat_hdr(long *hdr)
{
    char    typ[4];
    int     status = 0;

    if (hdr[0] < 0 || hdr[0] > 9999)
	return (0);
    typ[0] = (hdr[0] / 1000) % 10;  /* M */
    typ[1] = (hdr[0] / 100) % 10;   /* O */
    typ[2] = (hdr[0] / 10) % 10;    /* P */
    typ[3] = (hdr[0] / 1) % 10;     /* T */
    if (typ[3] == 0)
	if (typ[0] == 0 || typ[0] == 1)
	    if (typ[1] == 0 || typ[1] == 1)
		if (typ[2] >= 0 && typ[2] <= 4)
		    if (hdr[3] == 0)
			if (hdr[1] > 0 && hdr[2] > 0)
			    if (hdr[4] > 0 && hdr[4] < 120)
				status = 1;
    return (status);
}

static int
mat_rd_hdr(int fd, MATHDR *mh, char *nam)
{
    int nc = 0;
    long hdr[12], ofst, ofst_n;
    short s[2];
    static int dtyp5[10] = {3, 3, 3, 3, 3, 2, 2, 2, 1, 0};
 /*	P= [0,1,2,3,4]=>[r*8,r*4,I*4,I*2,I*4 unsigned integers] */

    if (mh->vers != 5)
	if (_read(fd, hdr, 20) < 20)
	    return (0);
    if (mh->vers == 0) {
        if (strncmp((char *) hdr, mat_key, 20) == 0) {  /* MATLAB 5.0 ? */
            mh->vers = 5;
            mh->next = 128;
        } else {
            mh->vers = 3;
        }
    }
    if (mh->vers == 5) {
        mh->bswp = 0;
	(void) _lseek(fd, mh->next, 0);
        if (_read(fd, hdr, 40) < 40)
            return (0);
	mh->next += hdr[1] + 8;
        if (_read(fd, &s, 4) < 4)
            return (0);
        nc = s[1];
        if (s[1]) {
            nc = s[1];
        } else {
            if (_read(fd, &hdr[10], 4) < 4)
                return (0);
            nc = hdr[10];
        }
        ofst_n = _lseek(fd, 0L, 1);
        ofst = _lseek(fd, (long) nc, 1);
        if (ofst % 8)
            (void) _lseek(fd, 8 - (ofst % 8), 1);
        if (_read(fd, &s, 4) < 4)
            return (0);
        if (s[1] == 0) {
            if (_read(fd, &hdr[11], 4) < 4)
                return (0);
        }
        ofst = _lseek(fd, 0L, 1);
       (void) _lseek(fd, ofst_n, 0);
        mh->mach = 0;               /* M */
        mh->roco = 0;               /* O */
        mh->dtyp = dtyp5[s[0]];     /* P */
        mh->text = 0;               /* T */
        mh->nrow = hdr[8];
        mh->ncol = hdr[9];
        mh->cmpx = 0;
        mh->nchr = nc;
    } else {
        mh->bswp = 0;
        if (!valid_mat_hdr(hdr)) {
            lswab(hdr, 5);
            if (!valid_mat_hdr(hdr))
                return (0);
            mh->bswp = 1;
        }
        mh->mach = (hdr[0] / 1000) % 10;    /* M */
        mh->roco = (hdr[0] / 100) % 10;     /* O */
        mh->dtyp = (hdr[0] / 10) % 10;      /* P */
        mh->text = (hdr[0] / 1) % 10;       /* T */
        mh->nrow = hdr[1];
        mh->ncol = hdr[2];
        mh->cmpx = (char) hdr[3];
        mh->nchr = (char) (hdr[4] - 1);
        ofst = _lseek(fd, 0L, 1) + hdr[4];
    }
    nc = mh->nchr;
    if (nc <= 0 || nc > 120)
        return (0);
    nc = _read(fd, nam, nc);
    if (nc <= 0)
        return (0);
    nam[nc] = '\0';
    (void) _lseek(fd, ofst, 0);

    return (nc);
}

static int
mat_rd_dat(int fd, MATHDR *mh, void *p, int n)
{
    int    sz, ne, dt, ns;
    static int txt = 0, cx = 0;

    if (mh->text != txt || mh->cmpx != cx)
        return (0);
    ne = (int) (mh->nrow * mh->ncol);
    if (ne <= 0)
        return (0);
    dt = mh->dtyp;
    sz = bsiz[dt];
    if (n < ne) {
        _read(fd, p, n * sz);
        (void) _lseek(fd, (long) (ne - n) * sz, 1);
    } else {
        _read(fd, p, ne * sz);
        n = ne;
    }
    if (mh->bswp) {
    	ns = n;
        if (dt == 0) {
            dswab(p, ns);
        } else if (dt == 3) {
            sswab(p, ns);
        } else {
            lswab(p, ns);
        }
    }
    return (n);
}

static int
mat_rd_f(int fd, MATHDR *mh, float *p, int n)
{
    int     sz, ns, ch, ne, tr, i, j, k, nd, dt;
    double *dd;
    float *df;
    static int txt = 0, cx = 0;

    if (mh->text != txt || mh->cmpx != cx)
        return (0);
    dt = mh->dtyp;
    if (mh->nrow == 1 || mh->nrow == 2) {
	ch = mh->nrow;
	ns = mh->ncol;
    } else if (mh->ncol == 1 || mh->ncol == 2) {
	ch = mh->ncol;
	ns = mh->nrow;
    } else {
	ch = 1;
	ns = mh->nrow * mh->ncol;
    }
    dd = (double *) calloc(ND, sizeof(double)); // avoids dreferencing warning
    df = (float *) dd;
    ne = ns * ch;
    tr = (mh->ncol == 2);
    sz = bsiz[dt];
    if (dt == 0) {
	for (i = 0; i < ne; i += ND) {
	    nd = (ND <= (ne - i)) ? ND : ne - i;
	    _read(fd, dd, nd * sz);
            if (mh->bswp)
                dswab(dd, nd);
	    for (j = 0; j < nd; j++) {
		k = i + j;
		if (tr)
		    k = ch * (k % ns) + (k / ns);
		p[k] = (float) dd[j];
	    }
	}
    } else if (dt == 1) {
	if (tr == 0) {
	    _read(fd, p, ne * sz);
            if (mh->bswp)
                lswab((long *) p, ne);
	} else {
	    for (i = 0; i < ne; i += ND) {
		nd = (ND <= (ne - i)) ? ND : ne - i;
		_read(fd, df, nd * sz);
                if (mh->bswp)
                    lswab((long *) df, nd);
		for (j = 0; j < nd; j++) {
		    k = i + j;
		    k = ch * (k % ns) + (k / ns);
		    p[k] = df[j];
		}
	    }
	}
    } else {
	ne = 0;
    }
    free(dd);
    return (ns);
}

/*............................ CHECK DATA ..............................*/

int
is_mat(char *fn)
{
    char    nam[120];
    int     fd, mode;
    MATHDR  mh;

    mode = (int) (_O_RDONLY | _O_BINARY);
    fd = _open(fn, mode, PMODE);    /* open the file */
    if (fd <= 0)
	return (0);
    if (mat_rd_hdr(fd, &mh, nam) <= 0)
	return (0);
    _close(fd);
    return (1);
}

int
is_ascii(char *fn)
{
    char    s[256];
    int     fd, mode, nbr, i;

    mode = (int) (_O_RDONLY | _O_BINARY);
    fd = _open(fn, mode, PMODE);    /* open the file */
    nbr = _read(fd, s, 256);
    for (i = 0; i < nbr; i++)
        if (s[i] < '\t' || s[i] > '~')
            return (0);
    _close(fd);
    return (1);
}

/*............................ WRITE DATA ..............................*/

int
write_data_mat(char *msg, char *fn, PGMCFG *cf)
{
    double fm;
    int mfd, ns;
    
    mfd = _open(fn, OFLAG, PMODE);    /* open the file */
    if (mfd < 0) {
        sprintf(msg, "Can't open data file (%s).", fn);
        return (0);
    }
    fm = cf->rate / 2;
    ns = cf->nsamp;
    mat_wr_d(mfd, "fmax", &fm, 1, 1);
    mat_wr_f(mfd, "resp", rt, ns, nrcp);
    mat_wr_s(mfd, "ntone", &cf->ntone, 1, 1);
    mat_wr_s(mfd, "navg", &cf->nav, 1, 1);
    mat_wr_s(mfd, "admd", &cf->ad_mode, 1, 1);
    mat_wr_s(mfd, "damd", &cf->da_mode, 1, 1);
    mat_wr_s(mfd, "stim", &cf->stm_typ, 1, 1);
    mat_wr_d(mfd, "volts", &cf->volts, 1, 1);
    mat_wr_d(mfd, "gap", &cf->gap, 1, 1);
    if (cf->sens_mp > 0) {
        mat_wr_d(mfd, "sens_mp", &cf->sens_mp, 1, 1);
    }
    if (cf->ncal > 0) {
        mat_wr_l(mfd, "ncal", &cf->ncal, 1, 1);
	if (cf->ntone > 0) {
            mat_wr_d(mfd, "f1", &cf->f1, 1, 1);
            mat_wr_d(mfd, "f2", &cf->f2, 1, 1);
            mat_wr_d(mfd, "f3", &cf->f3, 1, 1);
            mat_wr_d(mfd, "l1", &cf->l1, 1, 1);
            mat_wr_d(mfd, "l2", &cf->l2, 1, 1);
	    if (cf->f3 > 0)
                mat_wr_d(mfd, "l3", &cf->l3, 1, 1);
            mat_wr_d(mfd, "beg_dur", &cf->beg_dur, 1, 1);
            mat_wr_d(mfd, "end_dur", &cf->end_dur, 1, 1);
            mat_wr_d(mfd, "rmp_dur", &cf->rmp_dur, 1, 1);
            mat_wr_s(mfd, "rmp_typ", &cf->rmp_typ, 1, 1);
            mat_wr_s(mfd, "cal_chn", &cf->cal_chn, 1, 1);
            mat_wr_s(mfd, "dpsf", &cf->dpsf, 1, 1);
	    if (cf->rmp_typ == 4) {
		mat_wr_d(mfd, "dbms", cf->dbms, 3, 1);
	    }
	    if (cf->rmp_typ == 5) {
		mat_wr_d(mfd, "lpma", cf->lpma, 3, 1);
		mat_wr_d(mfd, "lpmb", cf->lpmb, 3, 1);
		mat_wr_d(mfd, "lpmc", cf->lpmc, 3, 1);
		mat_wr_d(mfd, "lpnc", cf->lpnc, 3, 1);
		mat_wr_d(mfd, "lpph", cf->lpph, 3, 1);
		mat_wr_d(mfd, "amin", cf->amin, 3, 1);
		mat_wr_d(mfd, "amnc", cf->amnc, 3, 1);
	    }
        }
    }
    _close(mfd);

    sprintf(msg, "Wrote data to file (%s ns=%d, nc=%d nt=%d).", fn, ns, nrcp, 
        cf->ntone);
    return (1);
}

void
ascii_wr3_d(FILE *fp, char *vn, double *vv)
{
    fprintf(fp, "; %s=%.4g %.4g %.4g\n", vn, vv[0], vv[1], vv[2]);
}

int
write_data_ascii(char *msg, char *fn, PGMCFG *cf)
{
    int i, j, n;
    FILE *fp;
    
    fp = fopen(fn, "wt");
    if (fp == NULL) {
        sprintf(msg, "Can't write data to file (%s).", fn);
        return (0);
    }
    fprintf(fp, "; %s\n", fn);
    fprintf(fp, "; fmax=%.0f\n", cf->rate / 2);
    fprintf(fp, "; ntone=%d\n", cf->ntone);
    fprintf(fp, "; navg=%d\n", cf->nav);
    fprintf(fp, "; admd=%d\n", cf->ad_mode);
    fprintf(fp, "; damd=%d\n", cf->da_mode);
    fprintf(fp, "; stim=%d\n", cf->stm_typ);
    fprintf(fp, "; volts=%.4f\n", cf->volts);
    fprintf(fp, "; gap=%.3g\n", cf->gap);
    if (cf->sens_mp > 0) {
        fprintf(fp, "; sens_mp=%.4f\n", cf->sens_mp);
    }
    if (cf->ncal > 0) {
        fprintf(fp, "; ncal=%d\n", cf->ncal);
	if (cf->ntone > 0) {
            fprintf(fp, "; f1=%.1f\n", cf->f1);
            fprintf(fp, "; f2=%.1f\n", cf->f2);
            fprintf(fp, "; f3=%.1f\n", cf->f3);
            fprintf(fp, "; l1=%.1f\n", cf->l1);
            fprintf(fp, "; l2=%.1f\n", cf->l2);
	    if (cf->f3 > 0)
		fprintf(fp, "; l3=%.1f\n", cf->l3);
            fprintf(fp, "; beg_dur=%.3g\n", cf->beg_dur);
            fprintf(fp, "; end_dur=%.3g\n", cf->end_dur);
            fprintf(fp, "; rmp_dur=%.3g\n", cf->rmp_dur);
            fprintf(fp, "; rmp_typ=%d  \n", cf->rmp_typ);
            fprintf(fp, "; cal_chn=%d  \n", cf->cal_chn);
            fprintf(fp, "; dpsf=%d  \n", cf->dpsf);
	    if (cf->rmp_typ == 4) {
		ascii_wr3_d(fp, "dbms", cf->dbms);
	    }
	    if (cf->rmp_typ == 5) {
		ascii_wr3_d(fp, "lpma", cf->lpma);
		ascii_wr3_d(fp, "lpmb", cf->lpmb);
		ascii_wr3_d(fp, "lpmc", cf->lpmc);
		ascii_wr3_d(fp, "lpnc", cf->lpnc);
		ascii_wr3_d(fp, "lpph", cf->lpph);
		ascii_wr3_d(fp, "amin", cf->amin);
		ascii_wr3_d(fp, "amnc", cf->amnc);
	    }
	}
    }
    n = cf->nsamp;
    for(i = 0; i < n; i++) {
	for(j = 0; j < nrcp; j++)
	    fprintf(fp, "%15.6e", rt[i * nrcp + j]);
        fprintf(fp, "\n");
    }
    fclose(fp);

    sprintf(msg, "Wrote data to file (%s ns=%d, nc=%d nt=%d).", fn, n, nrcp, 
        cf->ntone);
    return (1);
}

int
write_stim_mat(char *msg, char *fn, PGMCFG *cf)
{
    int mfd;
    
    mfd = _open(fn, OFLAG, PMODE);    /* open the file */
    if (mfd < 0) {
        sprintf(msg, "Can't open stimulus file (%s).", fn);
        return (0);
    }
    mat_wr_d(mfd, "rate", &cf->rate, 1, 1);
    mat_wr_f(mfd, "stim", st, cf->nsamp, nsch);
    mat_wr_s(mfd, "ntone", &cf->ntone, 1, 1);
    _close(mfd);

    sprintf(msg, "Wrote stimulus to file (%s).", fn);
    return (1);
}

/*............................ READ DATA ...............................*/

int
mat_rd1_d(int cfd, MATHDR *mh, double *v)
{
    if (!mat_rd_dat(cfd, mh, v, 1))
	return (0);
    if (mh->dtyp == 1)
	*v = *((float *) v);
    else if (mh->dtyp == 2)
	*v = *((long *) v);
    else if (mh->dtyp == 3)
	*v = *((short *) v) & 0xFFFF;
    else if (mh->dtyp == 4)
	*v = *((long *) v);
    return (1);
}

int
mat_rd1_s(int cfd, MATHDR *mh, short *v)
{
    double *w;

    w = (double *) calloc(1, sizeof(double)); // avoids dreferencing warning
    if (!mat_rd_dat(cfd, mh, w, 1))
	return (0);
    if (mh->dtyp == 0)
	*v = (short) *w;
    else if (mh->dtyp == 1)
	*v = (short) *((float *) w);
    else if (mh->dtyp == 2)
	*v = (short) *((long *) w);
    else if (mh->dtyp == 3)
	*v = (short) *((short *) w);
    else if (mh->dtyp == 4)
	*v = (short) *((long *) w);
    free(w);
    return (1);
}

int
mat_rd1_l(int cfd, MATHDR *mh, long *v)
{
    double *w;

    w = (double *) calloc(1, sizeof(double)); // avoids dreferencing warning
    if (!mat_rd_dat(cfd, mh, w, 1))
	return (0);
    if (mh->dtyp == 0)
	*v = (long) *w;
    else if (mh->dtyp == 1)
	*v = (long) *((float *) w);
    else if (mh->dtyp == 2)
	*v = *((long *) w);
    else if (mh->dtyp == 3)
	*v = (long) *((short *) w);
    else if (mh->dtyp == 4)
	*v = *((long *) w);
    free(w);
    return (1);
}

int
read_data_mat(char *msg, char *fn, PGMCFG *cf)
{
    char    nam[120];
    double  sr = 0, fm = 0, v[3];
    int     cfd, mode, nn, ne, ns = 0, nr;
    long    li;
    short   si;
    MATHDR  mh;

    mode = (int) (_O_RDONLY | _O_BINARY);
    cfd = _open(fn, mode, PMODE);    /* open the file */
    if (cfd <= 0) {
        sprintf(msg, "Can't open data file (%s).", fn);
	return (0);
    }
    cf->sens_mp = 0;	// default value
    mh.vers = 0;
    nam[0] = '\0';
    for (;;) {
        nn = mat_rd_hdr(cfd, &mh, nam);
        ne = (int) (mh.nrow * mh.ncol);
        nr = (mh.nrow < mh.ncol) ? mh.nrow : mh.ncol;
        if (nn <= 0) {
            break;
        } else if (strncmp(nam, "rate", nn) == 0) {
             if (!mat_rd1_d(cfd, &mh, &sr))
		break;
        } else if (strncmp(nam, "fmax", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, &fm))
		break;
            sr = 2 * fm;
        } else if (strncmp(nam, "ntone", nn) == 0) {
            if (!mat_rd1_s(cfd, &mh, &si))
                break;
            cf->ntone = si;
        } else if (strncmp(nam, "ncal", nn) == 0) {
            if (!mat_rd1_l(cfd, &mh, &li))
                break;
            cf->ncal = li;
        } else if (strncmp(nam, "navg", nn) == 0) {
            if (!mat_rd1_s(cfd, &mh, &si))
                break;
            cf->nav = si;
        } else if (strncmp(nam, "admd", nn) == 0) {
            if (!mat_rd1_s(cfd, &mh, &si))
                break;
            cf->ad_mode = si;
        } else if (strncmp(nam, "damd", nn) == 0) {
            if (!mat_rd1_s(cfd, &mh, &si))
                break;
            cf->da_mode = si;
        } else if (strncmp(nam, "stim", nn) == 0) {
            if (!mat_rd1_s(cfd, &mh, &si))
                break;
            cf->stm_typ = si;
        } else if (strncmp(nam, "volts", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->volts = v[0];
        } else if (strncmp(nam, "sens_mp", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->sens_mp = v[0];
        } else if (strncmp(nam, "f1", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->f1 = v[0];
        } else if (strncmp(nam, "f2", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->f2 = v[0];
        } else if (strncmp(nam, "f3", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->f3 = v[0];
        } else if (strncmp(nam, "l1", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->l1 = v[0];
        } else if (strncmp(nam, "l2", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->l2 = v[0];
        } else if (strncmp(nam, "l3", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->l3 = v[0];
        } else if (strncmp(nam, "gap", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->gap = v[0];
        } else if (strncmp(nam, "beg_dur", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->beg_dur = v[0];
        } else if (strncmp(nam, "end_dur", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->end_dur = v[0];
        } else if (strncmp(nam, "rmp_dur", nn) == 0) {
            if (!mat_rd1_d(cfd, &mh, v))
		break;
            cf->rmp_dur = v[0];
        } else if (strncmp(nam, "rmp_typ", nn) == 0) {
            if (!mat_rd1_s(cfd, &mh, &si))
                break;
            cf->rmp_typ = si;
        } else if (strncmp(nam, "cal_chn", nn) == 0) {
            if (!mat_rd1_s(cfd, &mh, &si))
                break;
            cf->cal_chn = si;
        } else if (strncmp(nam, "dpsf", nn) == 0) {
            if (!mat_rd1_s(cfd, &mh, &si))
                break;
            cf->dpsf = si;
        } else if (strncmp(nam, "dbms", nn) == 0) {
            if (!mat_rd_dat(cfd, &mh, cf->dbms, 3))
		break;
        } else if (strncmp(nam, "lpma", nn) == 0) {
            if (!mat_rd_dat(cfd, &mh, cf->lpma, 3))
		break;
        } else if (strncmp(nam, "lpmb", nn) == 0) {
            if (!mat_rd_dat(cfd, &mh, cf->lpmb, 3))
		break;
        } else if (strncmp(nam, "lpmc", nn) == 0) {
            if (!mat_rd_dat(cfd, &mh, cf->lpmc, 3))
		break;
        } else if (strncmp(nam, "lpnc", nn) == 0) {
            if (!mat_rd_dat(cfd, &mh, cf->lpnc, 3))
		break;
        } else if (strncmp(nam, "lpph", nn) == 0) {
            if (!mat_rd_dat(cfd, &mh, cf->lpph, 3))
		break;
        } else if (strncmp(nam, "amin", nn) == 0) {
            if (!mat_rd_dat(cfd, &mh, cf->amin, 3))
		break;
        } else if (strncmp(nam, "amnc", nn) == 0) {
            if (!mat_rd_dat(cfd, &mh, cf->amnc, 3))
		break;
        } else if (ne >= MIN_NSAMP && nr <= 2) {
	    nrch = nrcp = nr;
            ns = ne / nr;
            if (!alloc_mem(ns)) {
                sprintf(msg, "Can't allocate memory for data (ne=%d).", ne);
                _close(cfd);
                return (0);
            }
	    if (!mat_rd_f(cfd, &mh, rt, ne)) {
                break;
            }
            if (sr <= 0)
                sr = 50000;
	    if (sr > 0)
		cf->rate = sr;
	    if (ns > 0)
		cf->nsamp = ns;
        } else {
            (void) _lseek(cfd, (long) bsiz[(int) mh.dtyp] * ne, 1);
        }
    }
    _close(cfd);
    if (sr <= 0 || ns <= 0) {
        sprintf(msg, "Can't read data from %s (nam=%s ne=%d nr=%d nn=%d).", fn, nam, ne, nr, nn);
	return (0);
    }
    sprintf(msg, "Read data from file (%s ns=%d nc=%d nt=%d).", fn, ns, nsch, 
        cf->ntone);

    return (1);
}

static int
valid_num(char *s, float *v)
{
    char *b, *n;
    int   d = 0;

    b = s;			    // start of string
    while (*s == ' ' || *s == '\t') // skip white space
	s++;
    n = s;			    // start of number
    if (*s == '+' || *s == '-')	    // skip sign
	s++;
    while (*s >= '0' && *s <= '9') { // skip digits
	s++;
	d++;
    }
    if (*s == '.') {		    // skip decimal
	s++;
        while (*s >= '0' && *s <= '9') {    // skip digits
	    s++;
	    d++;
	}
    }
    if (*s == 'e' || *s == 'E') {   // skip exponential
	s++;
        if (*s == '+' || *s == '-')	    // skip sign
	    s++;
        while (*s >= '0' && *s <= '9')	    // skip digits
	    s++;
    }
    if (d == 0)
	return (0);
    *v = (float) atof(n);

    return (s - b);
}

static int
valid_data(char *s, float *v, int nc)
{
    int i, n = 0;

    for (i = 0; i < nc; i++) {
	n = valid_num(s, v + i);
	if (n == 0)
	    break;
	s = s + n;
    }
    return (i);
}

static void
ascii_rd3_d(char *s, double *v)
{
    sscanf(s, "%lf %lf %lf", v, v + 1, v + 2);
}

int
read_data_ascii(char *msg, char *fn, PGMCFG *cf)
{
    char s[256];
    float   v[8], sr = 0, fm = 0;
    int     ns, na, nr, nn, nc;
    FILE   *fp;

    fp = fopen(fn, "rt");
    if (fp == NULL) {
        sprintf(msg, "Can't read data from file (%s).", fn);
        return (0);
    }
    ns = 0;
    nr = 8;
    while (fgets(s, 256, fp)) {
	nc = valid_data(s, v, nr);
	if (nc > 0) {
	    if (nr > nc)
		nr = nc;
            ns++;
	}
    }
    nrch = nrcp = nr;
    if (!alloc_mem(ns)) {
        sprintf(msg, "Can't allocate memory (nsamp=%d).", ns);
        fclose(fp);
        return (0);
    }
    cf->sens_mp = 0;	// default value
    na = ns;
    rewind(fp);
    ns = 0;
    while (fgets(s, 256, fp)) {
	if (ns < na && valid_data(s, v, nr)) {
	    for (nc = 0; nc < nr; nc++) {
		rt[ns + na * nc] = v[nc];
	    }
	    ns++;
	} else if (strncmp("; rate=", s, 7) == 0) {
            sscanf(s + 7, "%f", &sr);
	} else if (strncmp("; fmax=", s, 7) == 0) {
            sscanf(s + 7, "%f", &fm);
            sr = 2 * fm;
	} else if (strncmp("; f1=", s, 5) == 0) {
            sscanf(s + 5, "%f", v);
	    cf->f1 = v[0];
	} else if (strncmp("; f2=", s, 5) == 0) {
            sscanf(s + 5, "%f", v);
	    cf->f2 = v[0];
	} else if (strncmp("; f3=", s, 5) == 0) {
            sscanf(s + 5, "%f", v);
	    cf->f3 = v[0];
	} else if (strncmp("; l1=", s, 5) == 0) {
            sscanf(s + 5, "%f", v);
	    cf->l1 = v[0];
	} else if (strncmp("; l2=", s, 5) == 0) {
            sscanf(s + 5, "%f", v);
	    cf->l2 = v[0];
	} else if (strncmp("; l3=", s, 5) == 0) {
            sscanf(s + 5, "%f", v);
	    cf->l3 = v[0];
	} else if (strncmp("; gap=", s, 6) == 0) {
            sscanf(s + 6, "%f", v);
	    cf->gap = v[0];
	} else if (strncmp("; beg_dur=", s, 10) == 0) {
            sscanf(s + 10, "%f", v);
	    cf->beg_dur = v[0];
	} else if (strncmp("; end_dur=", s, 10) == 0) {
            sscanf(s + 10, "%f", v);
	    cf->end_dur = v[0];
	} else if (strncmp("; rmp_dur=", s, 10) == 0) {
            sscanf(s + 10, "%f", v);
	    cf->rmp_dur = v[0];
	} else if (strncmp("; rmp_typ=", s, 10) == 0) {
            sscanf(s + 10, "%d", &nn);
            cf->rmp_typ = (short) nn;
	} else if (strncmp("; cal_chn=", s, 10) == 0) {
            sscanf(s + 10, "%d", &nn);
            cf->cal_chn = (short) nn;
	} else if (strncmp("; dpsf=", s, 7) == 0) {
            sscanf(s + 7, "%d", &nn);
            cf->dpsf = (short) nn;
	} else if (strncmp("; dbms=", s, 7) == 0) {
            ascii_rd3_d(s + 7, cf->dbms);
	} else if (strncmp("; lpma=", s, 7) == 0) {
            ascii_rd3_d(s + 7, cf->lpma);
	} else if (strncmp("; lpmb=", s, 7) == 0) {
            ascii_rd3_d(s + 7, cf->lpmb);
	} else if (strncmp("; lpmc=", s, 7) == 0) {
            ascii_rd3_d(s + 7, cf->lpmc);
	} else if (strncmp("; lpnc=", s, 7) == 0) {
            ascii_rd3_d(s + 7, cf->lpnc);
	} else if (strncmp("; lpph=", s, 7) == 0) {
            ascii_rd3_d(s + 7, cf->lpph);
	} else if (strncmp("; amin=", s, 7) == 0) {
            ascii_rd3_d(s + 7, cf->amin);
	} else if (strncmp("; amnc=", s, 7) == 0) {
            ascii_rd3_d(s + 7, cf->amnc);
	} else if (strncmp("; ntone=", s, 8) == 0) {
            sscanf(s + 8, "%d", &nn);
            cf->ntone = (short) nn;
	} else if (strncmp("; ncal=", s, 7) == 0) {
            sscanf(s + 7, "%d", &cf->ncal);
	} else if (strncmp("; navg=", s, 7) == 0) {
            sscanf(s + 8, "%d", &nn);
            cf->nav = (short) nn;
	} else if (strncmp("; admd=", s, 7) == 0) {
            sscanf(s + 8, "%d", &nn);
            cf->ad_mode = (short) nn;
	} else if (strncmp("; damd=", s, 7) == 0) {
            sscanf(s + 8, "%d", &nn);
            cf->da_mode = (short) nn;
	} else if (strncmp("; stim=", s, 7) == 0) {
            sscanf(s + 8, "%d", &nn);
            cf->stm_typ = (short) nn;
	} else if (strncmp("; volts=", s, 10) == 0) {
            sscanf(s + 8, "%lf", &cf->volts);
	} else if (strncmp("; sens_mp=", s, 10) == 0) {
            sscanf(s + 10, "%lf", &cf->sens_mp);
	}
    }
    fclose(fp);
    if (sr > 0)
	cf->rate = sr;
    if (ns > 0)
	cf->nsamp = ns;
    sprintf(msg, "Read data from file (%s).", fn);

    return (1);
}

int
read_stim_mat(char *msg, char *fn, PGMCFG *cf)
{
    char    nam[256];
    double  sr = 0, f;
    int     cfd, mode, nc, ne, nr, ns = 0;
    short   nt = 0;
    MATHDR  mh;

    mode = (int) (_O_RDONLY | _O_BINARY);
    cfd = _open(fn, mode, PMODE);    /* open the file */
    if (cfd <= 0) {
        sprintf(msg, "Can't open stimulus file (%.40s).", fn);
	return (0);
    }
    mh.vers = 0;
    nam[0] = '\0';
    for (;;) {
        nc = mat_rd_hdr(cfd, &mh, nam);
        ne = (int) (mh.nrow * mh.ncol);
        nr = (mh.nrow < mh.ncol) ? mh.nrow : mh.ncol;
        if (nc <= 0) {
            break;
        } else if (strncmp(nam, "rate", nc) == 0) {
            if (!mat_rd1_d(cfd, &mh, &sr))
		break;
        } else if (strncmp(nam, "f1", nc) == 0) {
            if (!mat_rd1_d(cfd, &mh, &f))
		break;
	    cf->f1 = f;
        } else if (strncmp(nam, "f2", nc) == 0) {
            if (!mat_rd1_d(cfd, &mh, &f))
		break;
	    cf->f2 = f;
        } else if (strncmp(nam, "f3", nc) == 0) {
            if (!mat_rd1_d(cfd, &mh, &f))
		break;
	    cf->f3 = f;
        } else if (strncmp(nam, "fmax", nc) == 0) {
            if (!mat_rd1_d(cfd, &mh, &f))
		break;
            sr = 2 * f;
        } else if (strncmp(nam, "ntone", nc) == 0) {
            if (!mat_rd1_s(cfd, &mh, &nt))
                break;
        } else if (ne > MIN_NSAMP) {
	    nsch = (nr > MIN_NSAMP) ? 1 : nr;
            if (!alloc_mem(ne / nr)) {
                sprintf(msg, "Can't allocate memory for data (ne=%d).", ne);
                _close(cfd);
                return (0);
            }
	    if (!mat_rd_f(cfd, &mh, st, ne)) {
                break;
            }
            ns = ne / nr;
//            if (sr <= 0)
//                sr = 50000;
        } else {
            (void) _lseek(cfd, (long) bsiz[(int) mh.dtyp] * ne, 1);
        }
    }
    _close(cfd);
    if (sr <= 0 || ns <= 0) {
        sprintf(msg, "Can't read stimulus from %s (nam=%s ne=%d nr=%d nsch=%d nt=%d ldbg=%lX).", fn, nam, ne, nr, nsch, nt, ldbg);
	return (0);
    }

    if (sr > 0)
        cf->rate = sr;
    cf->nsamp = ns;
    cf->ntone = (nt > 0 && nt < ns) ? nt : 0;

    sprintf(msg, "Read stimulus from file (%s ns=%d nc=%d nt=%d).", fn, ns, nsch, nt);
    return (1);
}

void
log_file(int desired)
{
    char s[80];
    static char *logname = "sysres.log";

    if (desired && mfp == NULL) {
	sprintf(s, "Logging messages to %s.", logname);
	prn_msg(s);
	if (desired == 2)
            mfp = fopen(logname, "w");
	else
            mfp = fopen(logname, "a");
    } else if (!desired && mfp != NULL) {
        fclose(mfp);
	mfp = NULL;
	sprintf(s, "Messages were written to %s.", logname);
	prn_msg(s);
    }
}
