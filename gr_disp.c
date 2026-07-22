/* gr_disp.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fk.h"

#ifndef WIN32
#define _strdup strdup
#endif

#define NHIST       32
#define MAXNPB      256

int  chk_event();
int  get_event();
void gr_cinit();
void gr_clrb(int, int, int, int);
void gr_line(int, int, int, int, int);
void gr_set_clip(int, int, int, int);
void gr_text(int, int, char *);

extern int gr_mode, inclev, debug;
extern FILE *lfp[8];

int c0, c1, bx, by, bw, bh, cw, ch, xpix, ypix, ytop, ybot, ycmd;

static char *hist[NHIST] = {NULL};
static char pbbuf[MAXNPB];
static int head = 0, tail = 0, curl = 0, npb = 0;

void
gr_init()
{
    gr_cinit();
    gr_set_clip(0, 0, xpix - 1, ypix - 1);
}

void
gr_erase(int x, int y, int n)
{
    if (n > 0)
	gr_clrb(x, y, x + cw * n, y + ch + 1);
}

void
gr_cur(int x, int y)
{
    gr_line(x, y + ch, x + cw - 1, y + ch, c1);
}

void
gr_uncur(int x, int y)
{
    gr_clrb(x, y + ch, x + cw, y + ch + 1);
}

static int
gr_gets(int x, int y, char *s, int len)
{
    int     i, j, k, c, n, p, m;
    int     esflg = 0, vtflg = 0;

    if (len <= 0 || s == NULL)
	return (0);
    n = strlen(s);     	    /* number of characters in string */
    i = n;      	    /* index of cursor within string */
    p = -1;		    /* previous cursor position */
    k = 0;      	    /* number of characters printed */
    for (;;) {
	/* selective print */
	if (k != n) {
	    m = (i < p) ? i : (p < 0) ? 0 : p;
	    gr_erase(x + m * cw, y, k - m);
	    gr_text(x + m * cw, y, s + m);
	} else if (p != i) {
	    gr_uncur(x + p * cw, y);
	    p = -1;
	}
	if (i < len) {
	    gr_cur(x + i * cw, y);
	    p = i;
	}
	k = strlen(s);
	c = get_event();
	if (c == CTRL_C) {
	    s[0] = CTRL_C;
	    s[1] = 0;
	    return (0);
	} else if (esflg && c == ESC) {
	    s[0] = ESC;
	    s[1] = 0;
	} else if (c == LEFT_CLICK) {
	    s[0] = CTRL_O;
	    s[1] = 0;
	    return (0);
	} else if (esflg) {
	    c |= ES;
	    esflg = 0;
	} else if (vtflg) {
	    c |= VT;
	    vtflg = 0;
	}
        gr_uncur(x + i * cw, y);
	switch (c) {
	case CTRL_A:	    /* CNTL-A */
	case (FN | 71):	    /* HOME */
	    i = 0;
	    break;
	case CTRL_B:	    /* CNTL-B */
	case (FN | 75):	    /* Left-Arrow */
	case (VT | 'D'):    /* Escape,[,D */
	    if (i > 0)
		i--;
	    break;
	case CTRL_F:	    /* CNTL-F */
	case (FN | 77):	    /* Right-Arrow */
	case (VT | 'C'):    /* Escape,[,C */
	    if (i < n)
		i++;
	    break;
	case 11:	    /* CNTL-K */
	    n = i;
	    s[n] = '\0';
	    break;
	case CTRL_N:	    /* CNTL-N */
	case (FN | 80):	    /* Down-Arrow */
	case (VT | 'B'):    /* Escape,[,B */
	    s[n] = CTRL_N;
	    s[n + 1] = '\0';
	    return (-1);
	case CTRL_P:	    /* CNTL-P */
	case (FN | 72):	    /* Up-Arrow */
	case (VT | 'A'):    /* Escape,[,A */
	    s[n] = CTRL_P;
	    s[n + 1] = '\0';
	    return (-1);
	case 21:	    /* CNTL-U */
	    n -= i;
	    for (j = 0; j < n; j++)
		s[j] = s[j + i];
	    i = 0;
	    s[n] = '\0';
	    break;
	case CTRL_E:	    /* CNTL-E */
	case (FN | 79):	    /* END */
	    i = n;
	    break;
	case CTRL_D:	    /* CNTL-D */
	case (FN | 83):	    /* DEL */
	case 127:	    /* DEL */
	    if (i < n)
		i++;	/* fall through to next case */
	    else
		break;
	case 8:		    /* CNTL-H */
            if (i > 0 && i <= n) {
		for (j = i; j < n; j++)
		    s[j - 1] = s[j];
		n--;
		s[n] = 0;
                i--;
            }
	    break;
	case '\r':
	case '\n':
	    s[n++] = '\n';
	    s[n] = 0;
	    return (n);
	case (ES | '['):     /* Escape,[ */
	    vtflg++;
	    break;
	case ESC:            /* Escape */
	    esflg++;
	    break;
	default:
            if ((c & 0x7F) < ' ')
                break;
	    if (n < len && c != (FN | 77) && c != 6) {
		for (j = n; j > i; j--)
		    s[j] = s[j - 1];
		s[i] = c;
		n++;
		s[n] = 0;
	    }
	    if (i < n) {
		i++;
	    }
	}
    }
}

double
gr_etime()
{
    return (((double) clock()) / CLOCKS_PER_SEC);    
}

void
gr_prcl(char *s)
{
    gr_erase(cw, ycmd, (xpix - cw) / cw);
    gr_text(cw, ycmd, s);
}

int
gr_getkey(char *p, double to)
{
    double et;

    gr_prcl(p);
    if (to <= 0) {
        return (get_event());
    } else {
        et = gr_etime() + to;  
        while (gr_etime() < et) {
            if (chk_event()) {
                return (get_event());
            }
        }
    }
    return (0);
}

void
gr_exit_incl()
{
    if (inclev > 0)
        fclose(lfp[--inclev]);
}

void
gr_pbstr(char *s)
{
    char *e;

    for (e = s; *e; e++)
        continue;
    pbbuf[npb++] = '\0';
    while (e > s && npb < MAXNPB)
        pbbuf[npb++] = *--e;
}

static void
gr_gtstr(char *s, int n)
{
    while (npb > 0 && n-- > 0) {
        *s = pbbuf[--npb];
	if (!*s++)
	    break;
    }
}

void
gr_getprompt(char *p, char *s, int n)
{
    int c, xx, sl, l, m;

    gr_clrb(0, ycmd, xpix - 1,  ypix - 1);
    gr_text(cw, ycmd, p);
    if (gr_mode == 1) {
        fgets(s, n, stdin);
    } else {
        curl = tail;
	xx = (strlen(p) + 1) * cw;
	*s = '\0';
	c = (xpix - xx) / cw;
	m = (c < n) ? c : n;
	while(gr_gets(xx, ycmd, s, m) < 0) {
	    sl = strlen(s);
	    gr_erase(xx, ycmd, sl);
	    c = s[sl - 1];
	    if (c == CTRL_N) {		/* Ctrl-N */
	        l = (curl + 1) % NHIST;
	        if (l == tail) {
		    curl = l;
		    *s = '\0';
		} else if (l != tail && hist[l]) {
		    curl = l;
		    strcpy(s, hist[l]);
		}
	    } else if (c == CTRL_P) {	/* Ctrl-P */
		l = (curl - 1 + NHIST) % NHIST;
		if (curl != head && hist[l]) {
		    curl = l;
		    strcpy(s, hist[l]);
		}
	    }
	    sl = strlen(s);
	    while (sl > 0 && s[sl - 1] <= ' ')
	        s[--sl] = '\0';
        }
        if (*s == CTRL_C || *s == CTRL_O)
	    return;
        sl = strlen(s);
        while ((sl > 0) && (s[sl - 1] <= ' ')) {
            s[--sl] = '\0';
        }
	if (sl > 0) {
	    if (hist[tail]) {
	        free(hist[tail]);
	    }
	    hist[tail++] = _strdup(s);
	    tail %= NHIST;
	    if (head == tail) {
		head++;
		head %= NHIST;
	    }
	}
    }
}

void
gr_getline(char *p, char *s, int n)
{
    int c, e, xx, sl, l, m;

    if (npb > 0) {
    	gr_gtstr(s, n);
	hist[tail++] = _strdup(s);
        tail %= NHIST;
        if (head == tail) {
	    head++;
	    head %= NHIST;
        }
    } else if (inclev > 0) {
        if (fgets(s, n, lfp[inclev - 1]) != NULL) {
            e = strlen(s);
	    while (s[e - 1] <= ' ')
		    e--;
            s[e] = '\0';
            if (debug) {
                gr_clrb(0, ycmd, xpix - 1,  ypix - 1);
                gr_text(cw, ycmd, p);
                xx = (strlen(p) + 1) * cw;
		c = (xpix - xx) / cw;
		m = (c < n) ? c : n;
	        (void) gr_gets(xx, ycmd, s, m);
		if (*s == CTRL_C)
		    return;
		if (*s == ESC) {
		    fclose(lfp[--inclev]);
		    s[0] = '\1';
		}
                s[e] = '\0';
	    }
        } else {
            fclose(lfp[--inclev]);
            s[0] = '\1';
        }
    } else {
        gr_clrb(0, ycmd, xpix - 1,  ypix - 1);
        gr_text(cw, ycmd, p);
        if (gr_mode == 1) {
            fgets(s, n, stdin);
        } else {
	    curl = tail;
	    xx = (strlen(p) + 1) * cw;
	    *s = '\0';
	    c = (xpix - xx) / cw;
	    m = (c < n) ? c : n;
	    while(gr_gets(xx, ycmd, s, m) < 0) {
		sl = strlen(s);
	        gr_erase(xx, ycmd, sl);
		c = s[sl - 1];
		if (c == CTRL_N) {		/* Ctrl-N */
		    l = (curl + 1) % NHIST;
		    if (l == tail) {
		        curl = l;
			*s = '\0';
		    } else if (l != tail && hist[l]) {
		        curl = l;
		        strcpy(s, hist[l]);
		    }
	        } else if (c == CTRL_P) {	/* Ctrl-P */
		    l = (curl - 1 + NHIST) % NHIST;
		    if (curl != head && hist[l]) {
		        curl = l;
		        strcpy(s, hist[l]);
		    }
	        }
		sl = strlen(s);
		while (sl > 0 && s[sl - 1] <= ' ')
		    s[--sl] = '\0';
	    }
	    if (*s == CTRL_C || *s == CTRL_O)
		return;
	    sl = strlen(s);
	    while (sl > 0 && s[sl - 1] <= ' ')
	        s[--sl] = '\0';
	    if (sl > 0) {
	        if (hist[tail]) {
		    free(hist[tail]);
		}
	        hist[tail++] = _strdup(s);
	        tail %= NHIST;
	        if (head == tail) {
		    head++;
		    head %= NHIST;
	        }
	    }
        }
    }
}

int
gr_can_print()
{
    if (gr_mode == 0)
	return (1);
#ifdef linux
    if (gr_mode == 2)
	return (1);
#endif /* linux */

    return (0);
}

void
clr_hist()
{
    int i;

    for (i = 0; i < NHIST; i++) {
        if (hist[i] != NULL) {
            free(hist[i]);
            hist[i] = NULL;
        }
    }
}

void
dump_hist(FILE *fp)
{
    int i;

    for (i = head; i != tail; i = (i + 1) % NHIST) {
        fprintf(fp, "%s\n", hist[i]);
    }
}
