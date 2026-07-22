/* print.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int gr_getpix(int, int);
int gr_white();

extern int xpix, ypix;

static char *pgm_nam = "BTNRH SysRes";
static int sysflg = 0, lptflg = 0;
#ifndef WIN32
static char  line[256];
static char  temp_file[256] = "srXXXXXX";
#endif

int
check_sys_call(char *printer_name)
{
    int     ret = 0;
#ifndef WIN32
    int     i;
#ifdef MSDOS
    static char *null_dev = "NUL";
#else /* MSDOS */
    static char *null_dev = "/dev/null";
#endif /* MSDOS */
    
    for (i = 0; i < (int) strlen(printer_name); i++) {
	if (printer_name[i] == '!') {
	    ret = 1;
	    break;
	}
    }
    if (ret) {
        mkstemp(temp_file);
	sprintf(line, "%s %s > %s", printer_name + i + 1, temp_file, null_dev);
    }
#endif	/* WIN32 */
    return (ret);
}

void
do_sys_call()
{
#ifndef WIN32
    (void) system(line);
    (void) remove(temp_file);
#endif	/* WIN32 */
}

int
check_lpt(char *printer_name)
{
#ifdef MSDOS
  while (*printer_name == ' ')
    printer_name++;
  if (strnicmp(printer_name, "lpt", 3) == 0)
    return (1);
#endif /* MSDOS */
    return (0);
}

static int
colorps_pr(char *printer_name, char * title, int orient)
{
    char *bb;
    int     i, j, xx, yy, c;
    time_t  tt;
    FILE   *fpt;
    static char *co[16] = {
	"000", "00A", "0A0", "0AA", "A00", "A0A", "A50", "AAA",
	"555", "55F", "5F5", "5FF", "F55", "F5F", "FF5", "FFF"
    };

    fpt = fopen(printer_name, "wt");
    if (fpt == NULL)
        return (1);
    xx = xpix;
    yy = ypix;
    tt = time(&tt);
    bb = orient ? "0 324 612 792" : "0 0 612 792";
    fprintf(fpt, "%%!PS-Adobe-2.0 EPSF-2.0\n");
    fprintf(fpt, "%%%%Creator: %s\n", pgm_nam);
    fprintf(fpt, "%%%%CreationDate: %s", ctime(&tt));
    fprintf(fpt, "%%%%Title: %s\n", title ? title : "");
    fprintf(fpt, "%%%%BoundingBox: %s\n", bb);
    fprintf(fpt, "%%%%Pages 1\n");
    fprintf(fpt, "%%%%EndComments\n");
    fprintf(fpt, "%%%%Page: 1 1\n");
    fprintf(fpt, "/picstr %d string def\n", ypix);
    if (orient) {
	fprintf(fpt, "606 324 translate 90 rotate 0.75 0.75 scale\n");
    }
    fprintf(fpt, "36 36 moveto currentpoint translate 540 720 scale\n");
    fprintf(fpt, "%d %d 4 ", yy, xx);		    // dimensions of source image
    fprintf(fpt, "[%d 0 0 %d 0 %d] ", yy, -xx, xx); // map unit square to source
    fprintf(fpt, "{currentfile picstr readhexstring pop}\n");
    fprintf(fpt, "false 3 colorimage\n");
    for (i = 0; i < xx; i++) {
	for (j = yy - 1; j >= 0; j--) {
	    c = gr_getpix(i, j);
	    fputs(co[c & 15], fpt);
	}
	fputs("\n", fpt);
    }
    fputs("showpage\n", fpt);
    fclose(fpt);

    return (0);
}

static int
postscript_pr(char *printer_name, char * title, int orient)
{
    char *bb;
    int     i, j, k, w, xx, yy;
    time_t  tt;
    unsigned int wd = 0;
    FILE   *fpt;

    fpt = fopen(printer_name, "wt");
    if (fpt == NULL)
        return (1);
    xx = xpix;
    yy = ypix;
    tt = time(&tt);
    bb = orient ? "0 324 612 792" : "0 0 612 792";
    fprintf(fpt, "%%!PS-Adobe-2.0 EPSF-2.0\n");
    fprintf(fpt, "%%%%Creator: %s\n", pgm_nam);
    fprintf(fpt, "%%%%CreationDate: %s", ctime(&tt));
    fprintf(fpt, "%%%%Title: %s\n", title ? title : "");
    fprintf(fpt, "%%%%BoundingBox: %s\n", bb);
    fprintf(fpt, "%%%%Pages 1\n");
    fprintf(fpt, "%%%%EndComments\n");
    fprintf(fpt, "%%%%Page: 1 1\n");
    if (orient) {
	fprintf(fpt, "606 324 translate 90 rotate 0.75 0.75 scale\n");
    }
    fprintf(fpt, "36 36 moveto currentpoint translate 540 720 scale\n");
    fprintf(fpt, "%d %d ", yy, xx);	// dimensions of source mask
    fprintf(fpt, "true ");		// paint ones
    fprintf(fpt, "[%d 0 0 %d 0 %d] ", yy, -xx, xx); // map unit square to mask
    fprintf(fpt, "{<\n");		// mask data
    w = gr_white();
    for (i = 0; i < xx; i++) {
        k = 0;
	for (j = yy - 1; j >= 0; j--) {
	    if (k-- == 0) {
		k += 4;
		wd = 0;
	    }
	    if (gr_getpix(i, j) != w)
		wd |= (unsigned) 1 << k;
	    if (k == 0) {
		fprintf(fpt, "%x", wd);
	    }
	}
	fprintf(fpt, "\n");
    }
    fprintf(fpt, ">} imagemask showpage\n");
    fclose(fpt);

    return (0);
}

static int
pcl_resolution(double xres, double yres)
{
    int r;

    if (xres > yres)
	r = (int) xres;
    else
	r = (int) yres;
    if (r < 75)
	r = 75;
    else if (r < 150)
	r = 150;
    else
	r = 300;

    return (r);
}

static int
pcl_pr(char *printer_name, char * title, int orient)
{
    FILE   *fpt;
    int     i, j, k, w, r, m1, m2;
    int     nl;
    char   *scan_line;
    static unsigned char m[8] = {
    	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
    };

    fpt = fopen(printer_name, "wb");
    if (fpt == NULL)
        return (1);
    w = gr_white();
    fprintf(fpt, "\033E");	/* reset */
    fprintf(fpt, "\033&l0E");	/* top margin = 0 */
    if (orient == 0) {
	r = pcl_resolution(xpix / 10.5, ypix / 8.0);
	m1 = (int) (0.5 * (11. - xpix / (double) r) * 720); // vert. margin (dp)
	m2 = (int) (0.5 * (8.5 - ypix / (double) r) * 720); // horz. margin (dp)
        fprintf(fpt, "\033&a%dV", m1);	/* move cursor vert. */
	fprintf(fpt, "\033&a%dH", m2);	/* move cursor horz. */
	fprintf(fpt, "\033*t%dR", r);	/* resolution (dpi) */
	fprintf(fpt, "\033*r1A");	/* start graphics at cursor  */
	nl = (ypix + 7) / 8;
	scan_line = (char *) malloc(nl);
	for (i = 0; i < xpix; i++) {
	    for (k = 0; k < nl; k++)
		scan_line[k] = 0;
	    for (j = 0; j < ypix; j++) {
		if (gr_getpix(i, ypix - 1 - j) != w)
		    scan_line[j / 8] |= m[j % 8];
	    }
	    fprintf(fpt, "\033*b%dW", nl);
	    for (k = 0; k < nl; k++)
		putc(scan_line[k], fpt);
	}
    } else {
	r = pcl_resolution(xpix / 8.0, ypix / 10.5);
	m1 = 720;                                  // = 1-inch vert. margin (dp)
	m2 = (int) (0.5 * (8.5 - xpix / (double) r) * 720); // horz. margin (dp)
        fprintf(fpt, "\033&a%dV", m1);	/* move cursor vert. */
	fprintf(fpt, "\033&a%dH", m2);	/* move cursor horz. */
	fprintf(fpt, "\033*t%dR", r);	/* resolution (dpi) */
	fprintf(fpt, "\033*t150R");	/* resolution = 150 dpi */
	fprintf(fpt, "\033*r1A");	/* start graphics at cursor  */
	nl = (xpix + 7) / 8;
	scan_line = (char *) malloc(nl);
	for (i = 0; i < ypix; i++) {
	    for (k = 0; k < nl; k++)
		scan_line[k] = 0;
	    for (j = 0; j < xpix; j++) {
		if (gr_getpix(j, i) != w)
		    scan_line[j / 8] |= m[j % 8];
	    }
	    fprintf(fpt, "\033*b%dW", nl);
	    for (k = 0; k < nl; k++)
		putc(scan_line[k], fpt);
	}
    }
    fprintf(fpt, "\033E");	/* reset */
    free(scan_line);
    fclose(fpt);

    return (0);
}

int
print_screen(char *printer_name, char * title, int orient, int type)
{
    int ec = 0;

    if (printer_name) {
        sysflg = check_sys_call(printer_name);
        lptflg = check_lpt(printer_name);
        if (type == 0)
            ec = postscript_pr(printer_name, title, orient);
        else if (type == 1)
            ec = pcl_pr(printer_name, title, orient);
        else if (type == 2)
            ec = colorps_pr(printer_name, title, orient);
    }
    return (ec);
}
