/* gr_w32.c */

#include <stdio.h>
#include <io.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>
#include "fk.h"

void sysres(int, char **);

extern int c0, c1, bx, by, bw, bh, cw, ch, xpix, ypix, ytop, ybot, ycmd;

int pgm_terminate = 0;

static int  keyval = 0;
static int  xwin = 640, ywin = 480;
static HBITMAP hOBM, hNBM;
static HBRUSH  hbr0, hbr1;
static HDC hDCMem;
static HFONT hfnt, hOldFont;      
static HWND hwnd;
static HPEN hpen;
static LOGPEN lpen;
static COLORREF palette_color[16];
static POINT pnpt = {1, 1};
static POINTS mpos = {0, 0};
static RECT lpSize;
static struct {
    short int r, g, b;
} ct[16] = {
    {0, 0, 0},	      /* black      */
    {0, 0, 170},      /* blue       */
    {0, 170, 0},      /* green      */
    {0, 170, 170},    /* cyan       */
    {170, 0, 0},      /* red        */
    {170, 0, 170},    /* magenta    */
    {170, 85, 0},     /* brown      */
    {170, 170, 170},  /* lt grey    */
    {85, 85, 85},     /* dk grey    */
    {85, 85, 255},    /* lt blue    */
    {85, 255, 85},    /* lt green   */
    {85, 255, 255},   /* lt cyan    */
    {255, 85, 85},    /* lt red     */
    {255, 85, 255},   /* lt magenta */
    {255, 255, 85},   /* yellow     */
    {255, 255, 255},  /* white      */
};

static long
set_color(int c)
{

    return (RGB(ct[c].r, ct[c].g, ct[c].b));
}

static void
set_pen(int c)
{
    static int last_color = 0;

    if (c != last_color) {
        DeleteObject(hpen);
        lpen.lopnStyle = PS_SOLID;
        lpen.lopnWidth = pnpt;
        lpen.lopnColor = set_color(c);
        hpen = CreatePenIndirect(&lpen);
        last_color = c;
    }
}

void
gr_cinit()
{
    int     i;
    HDC     hdc;
    TEXTMETRIC tm;

    hdc = GetDC(hwnd);
    GetClientRect(hwnd, (LPRECT) &lpSize);
    xpix = lpSize.right - lpSize.left;
    ypix = lpSize.bottom - lpSize.top;

    hDCMem = CreateCompatibleDC(hDCMem);
    hNBM = CreateCompatibleBitmap(hdc,xpix,ypix); 
    hOBM = SelectObject(hDCMem, hNBM);

    lpen.lopnStyle = PS_SOLID;
    lpen.lopnWidth = pnpt;
    lpen.lopnColor = set_color(c1);
    hpen = CreatePenIndirect(&lpen);
    SelectPen(hdc, hpen);
    SelectPen(hDCMem, hpen);
    hfnt = GetStockObject(ANSI_FIXED_FONT); 
    hOldFont = SelectObject(hdc, hfnt);
    if (hOldFont)
        (void) SelectObject(hDCMem, hfnt);     
    ReleaseDC(hwnd, hdc);

    c0 = 15;
    c1 = 0;
    hbr0 = CreateSolidBrush(set_color(c0));
    hbr1 = CreateSolidBrush(set_color(c1));

    for (i = 0; i < 16; i++) {
        SetPixel(hDCMem, 0, 0, set_color(i));
	palette_color[i] = GetPixel(hDCMem, 0, 0);
    }

    GetTextMetrics(hDCMem, &tm);
    cw = tm.tmAveCharWidth;
    ch = tm.tmHeight + 1;
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
    DeleteObject(hbr0);
    DeleteObject(hbr1);
    DeleteObject(hpen);
    DeleteObject(hNBM);
    DeleteDC(hDCMem);
}

void
gr_update(int mstatus)
{
    UpdateWindow(hwnd);
}

void
gr_fill_rect(int x1, int y1, int x2, int y2, int c)
{
    RECT    r;
    HBRUSH  b;
    HDC     hdc;

    r.left = (x1 < x2) ? x1 : x2;
    r.right = (x1 < x2) ? x2 : x1;
    r.top = (y1 < y2) ? y1 : y2;
    r.bottom = (y1 < y2) ? y2 : y1;
    b = (c == c0) ? hbr0 : hbr1;
    hdc = GetDC(hwnd);
    FillRect(hdc, &r, (HBRUSH) b);
    FillRect(hDCMem, &r, (HBRUSH) b);
    ReleaseDC(hwnd, hdc);
}

void
gr_clrb(int x1, int y1, int x2, int y2)
{
    gr_fill_rect(x1, y1, x2, y2, c0);
}
	
void
gr_text(int x, int y, char *s)
{
    HDC     hdc;

    y++;
    hdc = GetDC(hwnd);
    (void) SelectObject(hdc, hfnt);
    TextOut(hdc, x, y, s, strlen(s));
    TextOut(hDCMem, x, y, s, strlen(s));
    ReleaseDC(hwnd, hdc);
}

void
gr_line(int x1, int y1, int x2, int y2, int c)
{
    HDC hdc;

    hdc = GetDC(hwnd);
    set_pen(c);
    SelectPen(hdc, hpen);
    SelectPen(hDCMem, hpen);
    MoveToEx(hdc, x1, y1, NULL);
    MoveToEx(hDCMem, x1, y1, NULL);
    LineTo(hdc, x2, y2);
    LineTo(hDCMem, x2, y2);
    ReleaseDC(hwnd, hdc);
}

void
gr_clrs()
{
    gr_clrb(0, 0, xpix, ypix);
}

/*---------------------------------------------------------------------*/

void
mouse_position(int *x, int *y)
{
    *x = mpos.x;
    *y = mpos.y;
}

/*---------------------------------------------------------------------*/

void
paint_scr(HWND hWindow, PAINTSTRUCT ps)
{
    HDC  hdc;
    INT  x = 0, y = 0, w = xpix, h = ypix;

    x = ps.rcPaint.left;
    y = ps.rcPaint.top;
    w = ps.rcPaint.right - ps.rcPaint.left;
    h = ps.rcPaint.bottom - ps.rcPaint.top;
    hdc = GetDC(hwnd);
    BitBlt(hdc, x, y, w, h, hDCMem, x, y, SRCCOPY); 
    ReleaseDC(hwnd, hdc);
}

/*---------------------------------------------------------------------*/

LRESULT CALLBACK 
WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps ;

    switch (iMsg) {

    case WM_CREATE:
        return 0 ;

    case WM_SIZE:
        SetWindowPos(hwnd, HWND_TOP, 0, 0, xwin, ywin, SWP_NOMOVE);
        return 0 ;

    case WM_PAINT :
        BeginPaint(hwnd, (LPPAINTSTRUCT) &ps);
        paint_scr(hwnd, ps);
        EndPaint(hwnd, (LPPAINTSTRUCT) &ps);
        return 0 ;

    case WM_KEYDOWN:
        if (wParam == VK_HOME) {
            keyval = (FN | 71);
        } else if (wParam == VK_END) {
            keyval = (FN | 79);
        } else if (wParam == VK_LEFT) {
            keyval = (FN | 75);
        } else if (wParam == VK_RIGHT) {
            keyval = (FN | 77);
        } else if (wParam == VK_UP) {
            keyval = (FN | 72);
        } else if (wParam == VK_DOWN) {
            keyval = (FN | 80);
        } else if (wParam == VK_DELETE) {
            keyval = (FN | 83);
        }
        return 0 ;

    case WM_CHAR:
        keyval = wParam;
	return 0 ;

    case WM_LBUTTONDOWN:
        mpos = MAKEPOINTS(lParam);
        keyval = LEFT_CLICK;
	return 0 ;

    case WM_CLOSE :
    case WM_DESTROY :
        keyval = CTRL_C;
        pgm_terminate = 1;
	return 0;
    }
    return DefWindowProc (hwnd, iMsg, wParam, lParam) ;
}

int
chk_event()
{
    MSG         msg;

    if (keyval == 0) {
        if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
	    TranslateMessage (&msg);
	    DispatchMessage (&msg);
	}
    }

    return (keyval);
}

int
get_event()
{
    int c;
    UpdateWindow(hwnd);
    while (keyval == 0) {
	(void) chk_event();
    }
    c = keyval;
    keyval = 0;

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
    (void) chk_event();
    if (keyval == 27) {
	keyval = 0;
	return (1);
    } else if (keyval== 3) {
	return (1);
    }
    return (0);
}

int
gr_getpix(int x, int y)
{
    int i, b = 0;
    COLORREF r;

    r = GetPixel(hDCMem, x, y);
    for (i = 0; i < 16; i++) {
        if (r == palette_color[i]) {
            b = i;
	    break;
        }
    }
    return (b);
}

int 
gr_white()
{
    return (15);
}

void
gr_beep()
{
    (void) Beep(1000, 1000);
}

void
gr_sleep(double s)
{
    Sleep((int) (s * 1000));
}

void
gr_get_cwd(char *s, int n)
{
    GetCurrentDirectory(n, s);
}

void
gr_set_cwd(char *s)
{
    SetCurrentDirectory(s);
}

int
gr_find(char *s, char *fn, int ix)
{
    int done;
    static HANDLE  hnd;
    static WIN32_FIND_DATA f;

    if (ix == 0) {
	hnd = FindFirstFile(s, &f);
        done = (hnd == INVALID_HANDLE_VALUE);
    } else {
        done = !FindNextFile(hnd, &f);
    }
    if (!done)
        strcpy(fn, (char *) f.cFileName);
    return (!done);
}

int
gr_edit(char *fn)
{
    char cmd[256];

    if (_access(fn, 0) == 0) {
        sprintf(cmd, "Notepad %s", fn);
	WinExec(cmd, 1);
	return (1);
    }
    return (0);
}

/*---------------------------------------------------------------------*/

static void
check_args(int ac, char **av)
{
    int r;

    while (ac > 1) {
	if (av[1][0] == '-') {
	    if (av[1][1] == 'r') {
		r = atoi(av[1] + 2);
		if (r > 640) {
		    xwin = r;
		    ywin = (3 * r) / 4;
		    ywin -= (ywin % 4);
		}
	    }
	}
	ac--;
	av++;
    }
}

int WINAPI 
WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, 
    char *C, int S)
{
    WNDCLASS	 wndclass ;
    RECT r;
    static char *pgm = "SysRes" ;

    GetClientRect(GetDesktopWindow(), &r);
    if (r.right > 800) {
	xwin = 800;
	ywin = 600;
    }
    check_args(__argc, __argv);

    wndclass.style         = 0 ;
    wndclass.lpfnWndProc   = WndProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = hInstance ;
    wndclass.hIcon         = LoadIcon (hInstance, pgm) ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = NULL ;
    wndclass.lpszClassName = pgm ;
    RegisterClass (&wndclass) ;

    hwnd = CreateWindow (pgm, /* window class name */
        pgm,                  /* window caption */
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, /* window style */
        CW_USEDEFAULT,              /* initial x position */
        CW_USEDEFAULT,              /* initial y position */
        xwin,                       /* initial x size */
        ywin,                       /* initial y size */
        NULL,                       /* parent window handle */
        NULL,                       /* window menu handle */
        hInstance,                  /* program instance handle */
        NULL) ;	                    /* creation parameters */

    GetClientRect(hwnd, (LPRECT) &lpSize);
    xpix = lpSize.right - lpSize.left;
    ypix = lpSize.bottom - lpSize.top;
    xwin += xwin - xpix;
    ywin += ywin - ypix;
    SetWindowPos(hwnd, HWND_TOP, 0, 0, xwin, ywin, SWP_NOMOVE);

    ShowWindow (hwnd, S) ;
    UpdateWindow (hwnd) ;
    
    sysres(__argc, __argv);

    DestroyWindow(hwnd);

    return (0);
}

