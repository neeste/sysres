/* fft.c - computes an in-place complex-to-complex FFT
   x and y are the real and imaginary arrays of 2^m points.
*/

#include <stdio.h>
#include <math.h>

static void
cfft(float *x, int n, int d)
{
    double  c1, c2, z;
    float  *y, tx, ty, t1, t2, u1, u2;
    int     i, i1, j, k, i2, l, l1, l2, m;
    int     ii, jj;

    for(i = 2, m = 1; m <= 30; i *= 2, m++)
        if (n == i)
            break;

    /* Do the bit reversal */
    i2 = n >> 1;
    j = 0;
    y = x + 1;
    for (i = 0; i < n - 1; i++) {
	if (i < j) {
            ii = i * 2;
            jj = j * 2;
	    tx = x[ii];
	    ty = y[ii];
	    x[ii] = x[jj];
	    y[ii] = y[jj];
	    x[jj] = tx;
	    y[jj] = ty;
	}
	k = i2;
	while (k <= j) {
	    j -= k;
	    k >>= 1;
	}
	j += k;
    }

    /* Compute the FFT */
    c1 = -1.0;
    c2 = 0.0;
    l2 = 1;
    for (l = 0; l < m; l++) {
	l1 = l2;
	l2 <<= 1;
	u1 = 1.0;
	u2 = 0.0;
	for (j = 0; j < l1; j++) {
	    for (i = j; i < n; i += l2) {
		i1 = i + l1;
		ii = i * 2;
		jj = i1 * 2;
		t1 = u1 * x[jj] - u2 * y[jj];
		t2 = u1 * y[jj] + u2 * x[jj];
		x[jj] = x[ii] - t1;
		y[jj] = y[ii] - t2;
		x[ii] += t1;
		y[ii] += t2;
	    }
	    z = u1 * c1 - u2 * c2;
	    u2 = (float) (u1 * c2 + u2 * c1);
	    u1 = (float) z;
	}
	c2 = sqrt((1.0 - c1) / 2.0);
	c1 = sqrt((1.0 + c1) / 2.0);
	if (d)
	    c2 = -c2;
    }

    /* Scaling */
    if (d) {
        for (i = 0; i < n; i++) {
            ii = i * 2;
            x[ii] /= n;
            y[ii] /= n;
        }
    }
}

void
fft(float *x, int n)
{
    cfft(x, n, 1);
}

void
ifft(float *x, int n)
{
    cfft(x, n, 0);
}

#ifdef TEST


void
prnx(float *x, int n, char *s)
{
    int i;
    
    printf("\n%s\n", s);
    for (i = 0; i < 2 * n; i++) {
        if ((i % 8) == 0)
            printf("\n");
        printf("%9.3f", x[i]);
    }
    printf("\n");
}

int
main()
{
    float x[32], *y;
    int i, ii, n = 16;
    
    y = x + 1;
    for (i = 0; i < n; i++) {
        ii = i * 2;
        x[ii] = i * 2;
        y[ii] = i * i;
    }
    prnx(x, n, "orginal values:");
    fft(x, n);
    prnx(x, n, "transformed values:");
    ifft(x, n);
    prnx(x, n, "inverse-transformed values:");

    return (0);
}

#endif /* TEST */
