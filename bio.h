/* bio.h - bank I/O header file */

#define MAXNDC 8	/* maximum number of I/O channels */
#define TB_DEV 1	/* Turtle Beach device type */
#define WA_DEV 2	/* wave-audio device type */
#define limit(mn,aa,mx) (((aa)<(mn))?(mn):((aa)>(mx))?(mx):(aa))

char   *dsp_devnam();
double  dsp_atten_input(double);
double	dsp_atten_output(double);
double  dsp_set_rate(double);
int     dsp_check_bank();
int     dsp_open(int *, double *, double *);
int     dsp_reset_io(int);
int     dsp_test();
void    dsp_bank_get(int, int, int);
void    dsp_bank_put(int, int, int, int, int);
void    dsp_bank_ready(int);
void    dsp_begin_io();
void    dsp_close();
void    dsp_get_vfs(double *, double *);
void    dsp_info(char *);
void    dsp_set_vfs(double *, double *);
void    dsp_stop_io();

#ifdef WIN32
char   *wa_devnam();
double  wa_adjust_rate(double);
double  wa_atten_input(double);
double  wa_atten_output(double);
double  wa_set_rate(double);
int     wa_check_bank();
int     wa_devsel(int);
int     wa_devsel_byname(char *);
int     wa_open(int *, double *, double *);
int     wa_reset_io();
int     wa_test();
void    wa_bank_get(int, int, int);
void    wa_bank_put(int, int, int, int, int);
void    wa_bank_ready(int);
void    wa_begin_io();
void    wa_close();
void    wa_get_vfs(double *, double *);
void    wa_info(char *);
void    wa_set_ndc(int *);
void    wa_set_vfs(double *, double *);
void    wa_stop_io();
#endif /* WIN32 */

typedef struct {
    long accept;
    long avmode[MAXNDC];
    long resp_i[MAXNDC];
    long stim_i[MAXNDC];
    long *bank_b;
    long *resp_a[MAXNDC];
    long *resp_b[MAXNDC];
    long *stim_b[MAXNDC];
} BIO;

extern BIO bio_;
