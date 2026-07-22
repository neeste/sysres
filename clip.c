/*
 * clip.c
 */

#define LEFT	1
#define RIGHT	2
#define BELOW	4
#define ABOVE	8
#define XMN     cx1
#define YMN     cy1
#define XMX     cx2
#define YMX     cy2
#define outx(x) (((x < XMN) ? LEFT : 0) | ((XMX < x) ? RIGHT : 0))
#define outy(y) (((y < YMN) ? BELOW : 0) | ((YMX < y) ? ABOVE : 0))
#define outwindow(x,y)	(outx(x) | outy(y))

static int cx1 = 0, cy1 = 0, cx2 = 0, cy2 = 0;

int  nint(double);
void gr_line(int, int, int, int, int);

/*
 * clip - line clipped at window
 */
static int
clip(int *x1, int *y1, int *x2, int *y2)
{
    double  dx, dy;
    int     w1, w2;
    int     accept;

/* clip line, if necessary */

    w1 = outwindow(*x1, *y1);	/* is first point outside ? */
    w2 = outwindow(*x2, *y2);	/* is second point outside ? */
    while (!(accept = !w1 && !w2) && !(w1 & w2)) {
	dx = *x2 - *x1;
	dy = *y2 - *y1;
	if (w1) {		/* (x1,y1) out of the window */
	    if (w1 & LEFT) {
		*y1 += nint((XMN - *x1) * dy / dx);
		*x1 = XMN;
	    } else if (w1 & RIGHT) {
		*y1 += nint((XMX - *x1) * dy / dx);
		*x1 = XMX;
	    } else if (w1 & BELOW) {
		*x1 += nint((YMN - *y1) * dx / dy);
		*y1 = YMN;
	    } else if (w1 & ABOVE) {
		*x1 += nint((YMX - *y1) * dx / dy);
		*y1 = YMX;
	    }
	    w1 = outwindow(*x1, *y1);
	} else {		/* (x2,y2) out of the window */
	    if (w2 & LEFT) {
		*y2 += nint((XMN - *x2) * dy / dx);
		*x2 = XMN;
	    } else if (w2 & RIGHT) {
		*y2 += nint((XMX - *x2) * dy / dx);
		*x2 = XMX;
	    } else if (w2 & BELOW) {
		*x2 += nint((YMN - *y2) * dx / dy);
		*y2 = YMN;
	    } else if (w2 & ABOVE) {
		*x2 += nint((YMX - *y2) * dx / dy);
		*y2 = YMX;
	    }
	    w2 = outwindow(*x2, *y2);
	}
    }
    return (accept);
}

void
gr_tpcl(int x1, int y1, int x2, int y2, int c)
{
/* Clip the line and plot it if there's anything left after clipping */

    if (clip(&x1, &y1, &x2, &y2))
	gr_line(x1, y1, x2, y2, c);
}

void
gr_set_clip(int x1, int y1, int x2, int y2)
{
    cx1 = x1;
    cy1 = y1;
    cx2 = x2;
    cy2 = y2;
}
