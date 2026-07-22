/* gr_xw.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
//#include <X11/Xos.h>
#include "gr_xw.h"
#include "fk.h"

#define maxstrlen 80
#define NHIST 32

int     c0, c1, bx, by, bw, bh, cw, ch, xpix, ypix, ytop, ybot, xmax, ymax;
int     keyevent = 0;
void    gr_clrb(int, int, int, int);
void    gr_text(int, int, char *); 

extern char *hist[NHIST];
extern int head, tail, curl;
extern int inclev, npb;
extern int ycmd, xpix, ypix, cw, ch; 
extern FILE *lfp[8]; 

static int count;
static int keyval = 0; 
static int xw_dpth;
static int xw_scrn;
static int xw_x = 8;		/* x top left corner of window */
static int xw_y;		/* y top left corner of window */
static unsigned int xw_height = 480;	/* height of the window */
static unsigned int xw_width = 640;	/* width of the window */
static unsigned long xw_bp;
static unsigned long xw_wmsk;
static unsigned long xw_wp;

static Atom xw_kill;
static Colormap xw_cmap;
static Display *xw_mdsp;
static GC      xw_mgc;
static Pixmap graphdata;
static Window  xw_mwin;
static XComposeStatus compose; 
static XFontStruct *xw_xfs;
static XGCValues xw_gcv;
static XImage *graphimage = NULL;
static XSetWindowAttributes xw_swa;
static XSizeHints xw_theSizeHints;
static XWindowAttributes xw_wa; 

static void
set_color(int c)
{
    XColor  hw;
    static struct {
        short int r, g, b;
    } ct[16] = {
        {0, 0, 0},		/* black      */
        {0, 0, 170},		/* blue       */
        {0, 170, 0},		/* green      */
        {0, 170, 170},		/* cyan       */
        {170, 0, 0},		/* red        */
        {170, 0, 170},		/* magenta    */
        {170, 85, 0},		/* brown      */
        {170, 170, 170},	/* lt grey    */
        {85, 85, 85},		/* dk grey    */
        {85, 85, 255},		/* lt blue    */
        {85, 255, 85},		/* lt green   */
        {85, 255, 255},		/* lt cyan    */
        {255, 85, 85},		/* lt red     */
        {255, 85, 255},		/* lt magenta */
        {255, 255, 85},		/* yellow     */
        {255, 255, 255},	/* white      */
    };
    static int xw_last_color = -1;

    if (c == xw_last_color)
	return;
    hw.red = ct[c].r << 8;
    hw.green = ct[c].g << 8;
    hw.blue = ct[c].b << 8;
    hw.pixel = 0x20;
    hw.flags = -1;
    XAllocColor(xw_mdsp, xw_cmap, &hw);
    XSetForeground(xw_mdsp, xw_mgc, hw.pixel);
    xw_last_color = c;
}

void
xw_init()
{
    Pixmap icon;
    char *xw_name = "SysRes";   // sets the window name
    char *xw_icon = "";     // if we want to use an icon
    
    xw_mdsp = XOpenDisplay("");
    if (xw_mdsp == NULL) {
	return;
    }
    xw_wmsk = CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWBackingStore | CWEventMask; 
    xw_scrn = DefaultScreen(xw_mdsp); 
    xw_dpth = DefaultDepth(xw_mdsp, xw_scrn); 
    xw_cmap = DefaultColormap(xw_mdsp, xw_scrn); 
    if (XDisplayWidth(xw_mdsp, xw_scrn) > 1000) {
	xw_width = 800;
    	xw_height = 600;
    };
    xw_x = (XDisplayWidth(xw_mdsp, xw_scrn) - xw_width) / 2;
    xw_y = 50;
    xw_swa.border_pixel = BlackPixel(xw_mdsp, xw_scrn); 
    xw_swa.background_pixel = WhitePixel(xw_mdsp, xw_scrn); 
    xw_swa.override_redirect = False;
    xw_swa.backing_store = Always;
    xw_mwin = XCreateWindow(xw_mdsp,DefaultRootWindow(xw_mdsp),
            xw_x, xw_y , xw_width, xw_height, 3, xw_dpth, InputOutput, 
            CopyFromParent,xw_wmsk , &xw_swa);
    
    xw_wp = WhitePixel(xw_mdsp, xw_scrn);
    xw_bp = BlackPixel(xw_mdsp, xw_scrn);

    icon = XCreateBitmapFromData(xw_mdsp, xw_mwin, sysresicon_bits, 
            sysresicon_height, sysresicon_height); 

    xw_theSizeHints.flags = PPosition | PSize | PMinSize | PMaxSize;	/* set mask for the hints */
    xw_theSizeHints.x = xw_x;		/* x position */
    xw_theSizeHints.y = xw_y;		/* y position */
    xw_theSizeHints.width = xw_width;	/* width of the window */
    xw_theSizeHints.height = xw_height;	/* height of the window */
    // these set it so the window can't be resized
    xw_theSizeHints.min_width = xw_width;
    xw_theSizeHints.min_height = xw_height;
    xw_theSizeHints.max_width = xw_width;
    xw_theSizeHints.max_height = xw_height;

    XSetStandardProperties(xw_mdsp, xw_mwin, xw_name, xw_icon,
           icon, 0, 0, &xw_theSizeHints);

    // tell what events that we'll be using, keyboard input
    XSelectInput(xw_mdsp, xw_mwin, ExposureMask | KeyPressMask | ClientMessage | 
            StructureNotifyMask);
    
    xw_mgc = XCreateGC(xw_mdsp, xw_mwin, (unsigned long) 0, &xw_gcv);

    /* error... cannot create gc */
    if (xw_mgc == 0) 
    {
        XDestroyWindow(xw_mdsp, xw_scrn);
        exit(0);
    }
    /* set forground and background defaults */
    else 
    {
        XSetForeground(xw_mdsp, xw_mgc, xw_bp);
        XSetBackground(xw_mdsp, xw_mgc, xw_wp);
    }

    XMapRaised(xw_mdsp, xw_mwin);
    XGetWindowAttributes(xw_mdsp, xw_mwin, &xw_wa);
    // this will set up our pixmap as the backup window to redraw it
    graphdata = XCreatePixmap(xw_mdsp, RootWindow(xw_mdsp, xw_scrn), 
            xw_width, xw_height, DefaultDepth(xw_mdsp, xw_scrn));

    xw_kill = XInternAtom(xw_mdsp, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(xw_mdsp, xw_mwin, &xw_kill, 1);
    
    xw_xfs = XQueryFont(xw_mdsp, XGContextFromGC(xw_mgc));
    cw = xw_xfs->max_bounds.width;
    ch = xw_xfs->ascent + xw_xfs->descent;

    c0 = 15;	/* background color */
    c1 = 0;	/* foreground color */
    xpix = xw_wa.width;
    ypix = xw_wa.height;
    xmax = xpix - 1;
    ymax = ypix - 1;

    // set the graphdata pixmap to white
    set_color(c0);
    XFillRectangle(xw_mdsp, graphdata, xw_mgc, 0, 0, xw_width, xw_height);
    XFlush(xw_mdsp);
}

void
xw_close()
{
    XDestroyWindow(xw_mdsp, xw_mwin);
    XCloseDisplay(xw_mdsp);
}

void
xw_clrb(int x1, int y1, int x2, int y2)
{
    int     x, y, w, h;

    x = (x1 < x2) ? x1 : x2;
    y = (y1 < y2) ? y1 : y2;
    w = (x1 < x2) ? x2 - x1 : x1 - x2;
    h = (y1 < y2) ? y2 - y1 : y1 - y2;
    set_color(c0);
    XFillRectangle(xw_mdsp, xw_mwin, xw_mgc, x, y, w, h);
    XFillRectangle(xw_mdsp, graphdata, xw_mgc, x, y, w, h);
    XFlush(xw_mdsp);
}

void
xw_clrs()
{
    xw_clrb(0, 0, xpix, ypix);
}

int
xw_pixel(int x, int y)
{
    // returns color index  0-15
    // needs to be tested
    return (XGetPixel(graphimage, x, y));
}

int
xw_white()
{
    // this re-initializes the graphimage pointer
    // at the start of every new image
    graphimage = XGetImage(xw_mdsp, graphdata ,0, 0,
        xw_width, xw_height, xw_wp, XYPixmap);
    return ((int)xw_wp);
}


void
xw_line(int x1, int y1, int x2, int y2, int c)
{
    set_color(c);
    XDrawLine(xw_mdsp, xw_mwin, xw_mgc, x1, y1, x2, y2);
    XDrawLine(xw_mdsp, graphdata, xw_mgc, x1, y1, x2, y2);
    XFlush(xw_mdsp);
}

void
xw_text(int x, int y, char *s, int fgc, int bgc)
{
    set_color(fgc);
    y += ch - 2;
    XDrawString(xw_mdsp, xw_mwin, xw_mgc, x, y, s, strlen(s));
    XDrawString(xw_mdsp, graphdata, xw_mgc, x, y, s, strlen(s));
    XFlush(xw_mdsp);
}

void
xw_flush()
{
    XFlush(xw_mdsp);
}

int
xw_kbhit(void)
{
    char xw_buf[10];
    int buflen = 1;
    int keyer;
    KeySym keysym;
    XEvent report;   // holds type of event

    // only check KeyPress events when keyval is empty
    if(!keyval) {
        if(XCheckTypedEvent(xw_mdsp, KeyPress, &report)) {
            count = XLookupString(&report.xkey, xw_buf, buflen, &keysym,
                    &compose);
            keyer = xw_buf[0];
            if (keyer > 0)
                keyval = keyer;
            else if(keysym == XK_End)
                keyval = (FN | 79);
            else if(keysym == XK_Home)
                keyval = (FN | 71);
            else if(keysym == XK_Left)
                keyval = (FN | 75);
            else if(keysym == XK_Right)
                keyval = (FN | 77);
            else if(keysym == XK_Up)
                keyval = (FN | 72);
            else if(keysym == XK_Down)
                keyval = (FN | 80);
            else if(keysym == XK_Delete)
                keyval = (FN | 83);
        }
    } 
    // always check ClientMessage & Expose events
    if (XCheckTypedEvent(xw_mdsp, ClientMessage, &report)) {
	keyval = '\3';	// return Ctrl-C when window is closed
    }
    if (XCheckTypedEvent(xw_mdsp, Expose, &report)) {
        XCopyArea(xw_mdsp, graphdata, xw_mwin, xw_mgc, 0,0, xw_width,
                xw_height, 0,0);
    }

    return (keyval);
}

int
xw_getch(void)
{
    int c;

    while (!xw_kbhit())	// loop until keyval is not empty
	usleep(50000);	// wait 50 msec
    c = keyval;		// save keyval in c
    keyval = 0;		// make keyval empty

    return (c);
}

