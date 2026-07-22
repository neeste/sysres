/* sysres.h */

#define MAX_NSAMP   (1024*1024*64)
#define MIN_NSAMP   (64)
#define MAXNDC	    2	    /* maximum number of I/O channels */

#define limit(mn,aa,mx) (((aa)<(mn))?(mn):((aa)>(mx))?(mx):(aa))

#ifdef WIN32
#define DEFNFO "sysres.reg"
#else
#define DEFNFO "arscrc"
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

typedef struct {double mr, ms, md, pg, pd, tv, td;} MSKSTM;
extern MSKSTM tm;

/* function prototypes */

double  cdb(float *);
double  gr_etime();
int     gcd(int, int);
int     ilog2(int);
int     gr_getkey(char *, double);
int     nint(double);
void    defpar();
void    defext(char *, char *);
void    dp_lpol(char *, char *);
void    dp_lsol(char *, char *);
void    l1eq(char *, int);
void    play_stim(int, int, int, int *);
void    play_zero(int);
void    resp_compute();
