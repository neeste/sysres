struct cclip {
    float   cx1, cy1, cx2, cy2;
    float   tcx1, tcy1, tcx2, tcy2;
};

extern struct cclip cclip_;

#define LEFT	1
#define RIGHT	2
#define BELOW	4
#define ABOVE	8
#define XMN cclip_.tcx1
#define YMN cclip_.tcy1
#define XMX cclip_.tcx2
#define YMX cclip_.tcy2
#define outwindow(x,y)	(((x < XMN) ? LEFT : 0) | ((XMX < x) ? RIGHT : 0)\
		        | ((y < YMN) ? BELOW : 0) | ((YMX < y) ? ABOVE : 0))
