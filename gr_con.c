/* gr_con.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#ifdef GRX
#include <grdriver.h>
#include <grx20.h>
#include <grfontdv.h>
#endif
#ifndef WIN32
#include <signal.h>
#endif
#ifdef linux
#include <sys/ioctl.h>
#else  /* linux */
#include <pc.h>
#include <dos.h>
#include <conio.h>
#endif /* linux */
#include "fk.h"

#ifdef linux
#define tk_put(s)	        fputs(s, stdout)
int  _kbhit();
int  _getch();
void kbd_init();
void kbd_restore();
void xw_clrb(int, int, int, int);
void xw_clrs();
void xw_flush();
void xw_init();
void xw_line(int, int, int, int, int);
void xw_close();
void xw_text(int, int, char *, int, int);
int xw_kbhit();
int xw_getch();
int xw_white();
int xw_pixel(int, int);
#endif /* linux */

extern int gr_mode;
extern int c0, c1, bx, by, bw, bh, cw, ch, xpix, ypix, ytop, ybot, ycmd;
extern int keyevent;

#ifdef linux
static int tek_cando_color = 0;
static int oldhiy = 0, oldloy = 0, oldhix = 0;

void
tk_draw(int x, int y, int do_gs, int c)
{
    char    buf[8], *bp;
    char    hiy, loy, hix, lox;
    static int tkc = -1;
    static int tek_color[] = {
	'0', 'D', 'C', '5', 'B', '6', 'A', '8', 
	'9', '2', '3', '4', 'E', 'F', '7', '1', 
    };

    if (c != tkc && tek_cando_color) {
	buf[0] = '\033';
	buf[1] = 'M';
	buf[2] = 'L';
	buf[3] = (c == c0) ? '0' : (c == c1) ? '1' : tek_color[c];
	buf[4] = '\0';
        tk_put(buf);		/* set line color */
	tkc = c;
    }
    hiy = ' ' + (y >> 5);
    loy = '`' + (y & 31);
    hix = ' ' + (x >> 5);
    lox = '@' + (x & 31);
    bp = buf;
    if (do_gs == 0) {
	*bp++ = '\035';		/* GS char. is used to enable graphics mode */
    }
    if (hiy != oldhiy)
	*bp++ = hiy;
    if (loy != oldloy || hiy != oldhiy || hix != oldhix)
	*bp++ = loy;
    if (hix != oldhix)
	*bp++ = hix;
    *bp++ = lox;
    *bp = 0;
    tk_put(buf);
    oldhiy = hiy;
    oldloy = loy;
    oldhix = hix;
}

void
tk_clrb(int x1, int y1, int x2, int y2)
{
    int y;

    if (!tek_cando_color)
	return;
    if (y1 < y2) {
	y = y1;
	y1 = ypix - y2;
	y2 = ypix - y;
    } else {
	y1 = ypix - y1;
	y2 = ypix - y2;
    }
    x2--;
    y2--;
    for (y = y1; y <= y2; y++) {
	tk_draw(x1, y, 0, c0);
	tk_draw(x2, y, 1, c0);
	if (y < y2) {
	    y++;
	    tk_draw(x2, y, 0, c0);
	    tk_draw(x1, y, 1, c0);
	}
    }
}

int
kbhit()
{
    int c = 0;

    if (gr_mode == 0) {
        c = _kbhit();
    } else if (gr_mode == 1) {
        fflush(stdout);
        ioctl(fileno(stdin), FIONREAD, &c);
    } else if (gr_mode == 2) {
        xw_flush();
        c = xw_kbhit();
    }

    return (c);
}

int
getch()
{
    int c = 0;

    if (gr_mode == 0) {
        c = _getch();
    } else if (gr_mode == 1) {
        fflush(stdout);
        (void) read(fileno(stdin), &c, 1);
    } else if (gr_mode == 2) {
        c = xw_getch();
    }
    return (c);
}
#endif /* linux */

int
chk_event()
{
    
    return (kbhit());
}

int
get_event()
{
    int c = 0;
    
    while (c == 0) {
        if (kbhit()) {
            c = getch();
            if (c == 0 && gr_mode != 2) {
                c = FN | getch();
	    } else if (c == 27 && kbhit()) {
		c = getch();
		if (c == ']') {
		    c = getch();
		    if (c == 'A') {
			c = (FN | 72);	/* Up Arrow */
		    } else if (c == 'B') {
			c = (FN | 80);	/* Down Arrow */
		    } else if (c == 'C') {
			c = (FN | 77);	/* Right Arrow */
		    } else if (c == 'D') {
			c = (FN | 75);	/* Left Arrow */
		    }
		}
            }
        }
    }
    return (c);
}

void
flush_eventq()
{
    while (chk_event())
        get_event();
}

int
chk_escape()
{
    if(chk_event())
    {
        if(get_event() == 27)
            return 1;
    }
    return (0);
}

void
gr_cinit()
{
#ifndef WIN32
    signal(SIGINT, SIG_IGN);
#endif
    if (gr_mode == 0) {
#ifdef linux
	kbd_init();
#endif /* linux */
#ifdef GRX
        GrSetMode(GR_width_height_color_graphics, 640, 480, 16);
        xpix = GrSizeX();
        ypix = GrSizeY();
        c0 = GrWhite();
        c1 = GrBlack();
        cw = 8;
        ch = 14;
#else /* GRX */
	fprintf(stderr, "No console graphics\n");
	exit(1);
#endif /* GRX */
#ifdef linux
    } else if (gr_mode == 1) {
        tk_put("\037");		/* switch to alpha mode */
        c0 = 0;
        c1 = 15;
        xpix = 1024;
        ypix = 768;
        cw = 14;
        ch = 24;
	tek_cando_color =  (strcmp(getenv("TERM"), "vt100") == 0);
    } else if (gr_mode == 2) {
	kbd_init();
	xw_init();
#endif /* linux */
    }
    bx = 8 * cw;
    by = ypix - 4 * ch;
    bw = xpix - bx - 2 * cw;
    bh = by - 4 * ch;
    ytop = 3 * ch + ch / 2;
    ybot = by + ch + ch / 2;
    ycmd = ybot + ch;
}

void
gr_close()
{
    if (gr_mode == 0) {
#ifdef GRX
        GrSetMode(GR_default_text);
#endif /* GRX */
#ifdef linux
	kbd_restore();
    } else if (gr_mode == 1) {
	tk_draw(0, 0, 0, c1);	    /* select forground color */
//        tk_put("\033\f\0332\033\"0g");
        tk_put("\033\f\0332\033\"0");
        tk_put("\033[?38l");	    /* put kermit in text mode */
        fflush(stdout);
    } else if (gr_mode == 2) {
	xw_close();
	kbd_restore();
#endif /* linux */
    }
}

void
gr_clrb(int x1, int y1, int x2, int y2)
{
    if (gr_mode == 0) {
#ifdef GRX
        GrFilledBox(x1, y1, x2,  y2, c0);
#endif /* GRX */
#ifdef linux
    } else if (gr_mode == 1) {
        tk_clrb(x1, y1, x2,  y2);
    } else if (gr_mode == 2) {
	xw_clrb(x1, y1, x2,  y2);
#endif /* linux */
    }
}
	
void
gr_text(int x, int y, char *s)
{
    if (gr_mode == 0) {
#ifdef GRX
        GrTextXY(x, y, s, c1, c0);
#endif /* GRX */
#ifdef linux
    } else if (gr_mode == 1) {
        tk_draw(x, ypix - ch - y, 0, c0);	/* move to (x,y) */
        tk_put("\037");				/* switch to alpha mode */
        tk_put(s);
    } else if (gr_mode == 2) {
        xw_text(x, y, s, c1, c0);
#endif /* linux */
    }
}

int
gr_getpix(int x, int y)
{
    if (gr_mode == 0) {
#ifdef GRX
	return (GrPixel(x, y));
#endif /* GRX */
#ifdef linux
    } else if (gr_mode == 2) {
	return (xw_pixel(x, y));
#endif /* linux */
    }

    return (0);
}

int
gr_white()
{
    if (gr_mode == 0) {
#ifdef GRX
	return (GrWhite());
#endif /* GRX */
#ifdef linux
    } else if (gr_mode == 2) {
	return (xw_white());
#endif /* linux */
    }

    return (0);
}

void
gr_line(int x1, int y1, int x2, int y2, int c)
{
    if (gr_mode == 0) {
#ifdef GRX
        GrLine(x1, y1, x2, y2, c);
#endif /* GRX */
#ifdef linux
    } else if (gr_mode == 1) {
        tk_draw(x1, ypix - y1, 0, c);
        tk_draw(x2, ypix - y2, 1, c);
    } else if (gr_mode == 2) {
        xw_line(x1, y1, x2, y2, c);
#endif /* linux */
    }
}

void
gr_clrs()
{
    if (gr_mode == 0) {
#ifdef GRX
        GrFilledBox(0, 0, xpix,  ypix, c0);
#endif /* GRX */
#ifdef linux
    } else if (gr_mode == 1) {
        tk_put("\037");		/* switch to alpha mode */
        tk_put("\033\f");	/* erase */
    } else if (gr_mode == 2) {
        xw_clrs();
#endif /* linux */
    }
}

void
gr_beep()
{
#ifdef linux
    tk_put("\007");	/* bell */
#else /* linux */
    sound(1000);
    sleep(1);
    sound(0);
#endif /* linux */
}

void
gr_sleep(double s)
{
    if (s < 1000)
        usleep((int) (s * 1e6));
    else
        sleep((int) s);
}

void
gr_get_cwd(char *s, int n)
{
    getcwd(s, n);
}

void
gr_set_cwd(char *s)
{
    chdir(s);
}

int
gr_find(char *s, char *fn, int ix)
{
#ifdef MSDOS
    int done;
    static struct _find_t f;

    if (ix == 0)
        done = _dos_findfirst(s, _A_NORMAL, &f);
     else
        done = _dos_findnext(&f);
    if (!done)
        strcpy(fn, f.name);
    return (!done);
#else /* MSDOS */
    return (0);
#endif /* MSDOS */
}

int
gr_edit(char *fn)
{
    return (0);
}

void
gr_update()
{
}

