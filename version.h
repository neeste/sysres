/* version.h */

#define VER     "SysRes version 2.34, 18-Jun-21"
#define NOTICE	"Copyright 1988-2021 Boys Town National Research Hospital"
#define RIGHTS	"Non-profit redistribution permitted."

/**************************************************************************
2.33 - 18-Jun-21
> Recompiled with VS16 for latest version of ARSC
> Cleaned up for MinGW
2.33 - 14-Jan-19
> Cleaned up for MinGW
2.32 - 18-Nov-14
> Fixed device list to show current device
2.31 - 15-Nov-14
> Added ARSC version to bottom of device list
2.30 - 2-Mar-11
> Added chirp pre-emphasis parameters pef & pes
2.29 - 18-Feb-11
> Don't set default rate to 50 kHz when reading stimulus files
> Corrected zf message units (Hz)
2.28 - 5-Aug-10
> Eliminate call to sio_get_cardtype
2.27 - 27-Apr-10
> Recompile with sio_arsc.c to check LynxTwo-B ASIO crash
> ARSC has new cardinfo for Layla3G & LynxTwo
2.26 - 20-Dec-09
> eliminate gd from resp_message when ntone > 0
> added rms to spec_avg message
2.25 - 17-Nov-09
> Changed play_zero(1) to play_zero(8) to avoid hanging under Linux
> Added support for channel offset
2.24 - 28-Apr-09
> Defined M_PI
2.23 - 8-Apr-09
> Changed wreg to wci
> Re-implement wci for Linux
2.22 - 22-Feb-09
> Added backpage to help
> Added Nuttall window
2.21 - 5-Feb-09
> Cleaned up for Mac
2.20 - 12-Nov-08
> Added card info for current device
> Help page escape
2.19 - 7-Feb-08
> Added ifdef GRX for Linux
2.18 - 14-Dec-07
> Removed unused variables and compute_sweep function from sysres.c
> Updated sio function definitions
2.17 - 16-Nov-07
> Fix max_volt when "no device"
> Added proscan ramp
2.16 - 19-Jul-07
> allocate temp space in rwfile.c to avoid dereferencing warning
2.15 - 9-Jul-07
> Re-open i/o device when reseting parameters with 'rp'
> Put sanity check on volts & gap in prn_top()
2.14 - 5-Jul-07
> Display version number and device info on start
> Limit "volts" to soundcard maximum
> Limit l1,l2,l3 to soundcard maximum
> Fixed bugs in ARSC
2.13 - 14-Jun-07
> Added vpp display mode
> Marked slection on device list
2.12 - 16-Nov-06
> Added ASIO latency function "late"
> Fixed bug in "dl" command
2.11 - 16-Nov-06
> Made sio.dll compatible with arsc
> Added "device list" (dl) commanddl

2.10 - 31-Jul-06
> Added EMAV lst file parameters lsf3, lsl3a, lsl3b, lsrep
2.09 - 9-Sep-05
> Added dbl.noise stimulus type.
2.08 - 27-May-05
> Added "id" parameter.
> Added "$id" substitution in datfn.
> Added "ckfn" command.
2.07 - 21-Apr-05
> Minor changes to work under Linux/ALSA.
2.06 - 15-Feb-05
> Added M-Audio device.
> Expanded cardinfo table
2.05 - 12-Feb-05
> Fixed memory allocation bug.
> Fixed bug in reading multichannel ascii data
2.04 - 4-Oct-04
> Added "after" function to make self-testing easier.
2.03 - 13-Sep-04
> Modified dpcp function to always divide ns by 4.
> Added dpmp function to select "middle pair".
> Allow arithmetic expressions and user variables
2.02 - 1-Sep-04
> Added amplitude modulation parameters: amin1,2,3 & amnc1,2,3
2.01 - 25-Jun-04
> Add higher sampling rates to 200000
2.00 - 28-May-04
> Changed def. of lpmb1,2,3 & lpmc1,2,3 to "other sine mod. amp."
> Fixed Delete for Win32
1.99 - 18-May-04
> Added L2-range message to lpol & lsol commands
> Added partial skip feature
1.98 - 9-May-04
> Fixed ntone bug for zero stimulus
> Added -r commandline argument to set window size
> Cleaned-up setting stmtyp
1.97 - 6-May-04
> Select 24-bit soundcard at start
> Added msg command
> Added "Color PostScript" printer type
1.96 - 5-May-04
> Added wreg command
> Increased window size on high-resolution screens
1.95 - 14-Apr-04
> Added l3eq command
> Modified dpsf to zero only +/- 3*dpbw
1.94 - 18-Mar-04
> Added DP "closest pair" function
1.93 - 13-Jan-04
1.92 - 15-Dec-03
> Open soundcard each time i/o is initiated (for Indigo)
> Moved check_registry function call into main.
> Shorten directory name if too long to print
> Improved log frequency axis
> Added i/o parameters to system response file.
> Tweaked display.
> Added cursor position message on mouse click
1.91 - 1-Dec-03
> Fixed bug in opt.lev. computation
1.90 - 25-Nov-03
> Added calibration to stimulus files
> Allow f1,f2,f3 to be specified in stimulus file
> Third channel in stimulus file folded into second
> Fixed bug in mat_rd1_s function
> Added lpmb & lpmc to allow sin^2 and sin^3 dependence
> Added l1eq to specify L1 dependence on L2
> Added lpol command to generate l1eq inc file
> Added lsol command to generate EMAV lst file
> Added olmin,olord,olsnr "lpol" parameters
> Added lsavt,lsbeg,lsinc,lsnlv,lssnr "lsol" parameters
> Modified rwfile.c to write dbms & lp?? as arrays
> Implement response folding for opt.lev.
  1.89 - 18-Oct-03
> Modified wa.c for Indigo with WinXP
1.88 - 21-Apr-03
> Added Lissajous variables to data file
> Strip comment from corner label
> Added "edit" command.
> Default to "rl=" when valid list file is specified.
> Cleaned up cmd pushback and history
1.87 - 6-Mar-03
> Added Lissajous path stimulus
1.86 - 7-Feb-03
> Added commands to plot the data file on the command line.
> Applied correction to reverse time-energy plot
1.85 - 23-Apr-02
> Fixed read_stim_mat() to allow use of stmgen files.
1.84 - 8-Apr-02
> Added preliminary support for Echo Gina soundcard
1.83 - 20-Mar-02
> Set ad=1 & da=1 when executing dpst command
> Warn if dpen > nsamp/10.
1.82 - 17-Mar-02
> Added waveOutPause and waveOutRestart calls to wa.c
1.81 - 26-Feb-02
> Added phase fit when writing dpenv line to log file
> Erase message line at left & right ends
> Added "dpst" to copy DP stimulus to reponse buffer
> Added "exponential" ramp type (rt=4)
> Added parameters "dbms1,2,3" to specify dB/msec for rt=4
> Added parameter "dpsf" to control DP stimulus filter
> Added parameter "dpfd" to specify DP fit duration
> Added parameter "ow" to specify overwriting data files
> Added parameter "dbg" to enable debug mode
> Fixed reading of "left" from registry
1.80 - 6-Jan-02
> Added negative ramp types that are applied to F3 only
> Adjusted t=0 value in heterodyne analysis
> Fixed small bug in gr_gets()
> Fixed format error on DP Help page
> Switch alternate DP component to F3 for hetr. anal. display
1.79 - 13-Nov-01
> Removed '\n' from "pause" messages.
> Improved gr_gets()
> X window improvements
1.78 - 13-Nov-01
> Added parameter dpch to specify DP analysis channel
> Added parameter cach to specify DP calibration channel
1.77 - 8-Nov-01
> Fixed print screen function under X windows
> Applied ramp to noise stimuli
> changed "st" command to always compute stimulus.
1.76 - 5-Nov-01
> Effectively put cal_init into cal_create
> Added parameter "ntwb" to specify number of tones in wideband stimuli
1.75 - 2-Nov-01
> Added cal_tones function
1.74 - 18-Oct-01
> improved X window support
> new command parsing function
1.73 - 12-Sep-01
> use new lookup() function to parse command line
1.72 - 24-Aug-01
> Fixed bug in reading rate from MAT files
1.71 - 17-Jun-01
> Remove F1,F2,L1,L2,etc from data file when ntone=0.
> Make list_devices static in wa.c
1.70 - 23-May-01
> Fixed definition of F2-F1 when dpon=0.
1.69 - 27-Apr-01
> Fixed bug in read_data_mat().
1.68 - 14-Apr-01
> Improved phase average for complex exp. fit.
> Fixed three-byte shift bug in wa.c
> Eliminated click on CardDeluxe at start of stimulus buffer
> Added parameter "pregap" to insert delay prior to stimulus
> Put "pregap" into config file
> Minor changes to help.c.
> set pregap default to zero
1.67 - 2-Apr-01
> Switched CardDeluxe setting to +4db out and -10dB in.
> Moved check_registry() from wa.c into chkreg.c
1.66 - 29-Mar-01
> Fixed rate setting in wa.c.
> Open "In" before "Out" to avoid "allocated" error
> Reject 2-exp fit when its variance is not smallest
> Add decimal places to phase axis when possible
> Put "gap" on top in place of "delay".
> Interpret PL=$vn as "VER".
> Added gr_uncur() to gr_disp.c to pnly erase cursor
1.65 - 25-Mar-01
> Added new command "lf" to turn logging on & off.
> Added new paramter "eml" to specify extra message line
> Changed format of exp. fit message, put phase on 2nd line
> Elimated display when reading data.
> Added new parameter "crn" to specify first corner line
> If crn=$fn, then put data file name on first corner line
> Added all data file variables to configuration file
> Elimated variable "ascfn" in gui.c, use "datfn" instead.
> Added new parameter "om" to specify fit offset in msec.
> Added parameters beg_dur, end_dur, & rmp_dur to data and config files
> Added parameter "pn" to specify range of phase axis
> Added parameter "dn" to specify range and offset of delay axis
> Added parameter "do" to specify an offset for dealy values
1.64 - 24-Mar-01
> Cleaned up wa.c functions
> Redefined windows registry entries
> Fixed (?) first I/O bug
1.63 - 21-Mar-01
> Fixed rms message
> specify L&R vfs separately in wa.c
1.62 - 19-Mar-01
> retrieve card-type info from registry
> fixed R-channel attenuation with Pinnacle
> display dBV axes correctly for tones, display dB otherwise
> added Pa & units to waveform display when appropriate
1.61 - 16-Feb-01
> Modified DP exp. fit to avoid negative VAC
1.60 - 18-Dec-00
> Wave audio functions device selection (STN)
1.59 - 26-Oct-00
> Wave audio functions implemented (Denis)
> Changed WM_Destroy message code in WinProc
1.58 - 8-Oct-00
> Changed the way attenuation scaling is handled 
1.57 - 10-Sep-00
> Added option of Gaussian filter to DP env. (dpft=2).
> Working on Win32 wave audio support
1.56 - 3-Sep-00
> Added complex mode to DP exp fit.
> Fixed STFT plot horiz. alignment
1.55 - 20-Jun-00
> Start implementing wave-audio support using Win32 API.
1.54 - 15-May-00
> Start implementing wave-audio support using Win32 API.
> Tested on Thinkpad, Pinnacle, & CardDeluxe.
1.53 - 13-Mar-00
> SIO API nearly complete.
> Implemented SIO support for multiple buffers per channel.
1.52 - 6-Mar-00
> SIO API under development.
> added dpenv message line to facilite MATLAB processing.
> ordered DP exp. fit. prms. with shortest time first.
1.51 - 16-Feb-00
> Compiles without warnings (or errors) under Visual C++ 6.0.
1.50 - 26-Jan-00
> Added log file option
1.49 - 22-Nov-99
> Displays steady-state value for DP exp. fit
1.48 - 14-Oct-99
> Modified "pr" command to not set "pd" and "pp"
> Remove "pd" and "pp" commands
> Added "zt1" and "zt2" to help
1.47 - 12-Oct-99
> Added parameter "dpdp" to display 3*F1-2*F2
1.46 - 5-Oct-99
> Added parameter "dpon" to specify DP order number
1.45 - 26-Aug-99
> adjust voltage scale for internal attenuation
> set same INP and MIC attenation on Pinnacle/Fiji
> add masking commands to help page
> move config file routines to rwcfg.c
> add version info to config file
1.44 - 19-Aug-99
> Fixed outward tick marks
> Fixed I/O error under Linux (compiler bug ???)
1.43 - 18-Aug-99
> Modified exponential fit method again, fixed minor bug
> Added tick direction parameter "td"
1.42 - 4-Aug-99
> Modified exponential fit method again
> Added DP initial time constant parameter "dpt1"
1.41 - 4-Aug-99
> Modified exponential fit method
1.40 - 2-Aug-99
> Added variables to data file: l1, l2, l3, gap
> Removed "pf.ofst=0" when reading data, running "ca", or setting "cn"
1.39 - 29-Jun-99
> Added command (wh) to write help file
> Added more parameters to data file
1.38 - 24-Jun-99
> Time constant in DP exponential fit can't exceed duration of response
1.37 - 22-Jun-99
> Fixed bug in weighting (reciprocal of noise)
1.36 - 22-Jun-99
> Cleaned up code for Linux
> Revised method of averaging to allow gap processing
> Added code for artifact rejection
> Added code for weighted averaging
> Added new parameters avm, dpsb, rjm, rjs, rjt
> Changed display DP Ln to use dpsb
> fixed bug in prn_top re range of stm_typ
1.35 - 7-Jun-99
> Revised print function
1.34 - 4-Jun-99
> Increased pf.ofst from short to long
1.33 - 19-May-99
> Added "be" command to "beep" console
> Added "sl" command to sleep for N seconds
> Added "to" command to "timout" pause commands
1.32 - 13-May-99
> Added "dd" command to print DSP device info
1.31 - 20-Apr-99
> Fixed onset/offset ramps
> Added "Blackman" ramp type
1.30 - 19-Apr-99
> Added "show delay" and "show phase" parameters
1.29 - 16-Apr-99
> Fix STFT DP envelope display
> Revise DP hetr envelope computation
1.28 - 6-Apr-99
> Display instantaneous freq. instead of delay for DP envelope
> Implemented time zoom
> Added "beg duration" and "end duration" parameters (bd & ed)
> Added "DP filter type" parameter (dpft: 0=lowpass, 1=Blackman)
1.27 - 11-Mar-99
> Corrected STFT plot so that it agrees more closely with HETR plot
1.26 - 9-Mar-99
> Fixed wr_ph() not to change phase by less than 1 cycle
1.25 - 8-Mar-99
> Added size of effect (dB) to exp. fit report
> Fixed missing "else" that caused "dp" computation for "dpbw"
1.24 - 8-Mar-99
> Added "DP exponential fit" command (dpef)
> Added "DP envelope size" command (dpen)
> Added "zoom" commands (zo, zf1, zf2)
1.23 - 1-Feb-99
> Add SR command to set SPL reference
> Fixed "next-line-null" bug in command history
1.22 - 21-Oct-98
> allow vr and dr to be set to nearest dB, instead of nearest 10 dB
1.21 - 1-Oct-98
> fixed dbSPL (1dB off) plotting error
> allow MAT or LST files to be specified on command line
> added "ex" command to exit from command file
> pressing escape during a "pause" evokes an "ex" command
1.20 - 24-Sep-98
> added "os" command to set offest of short-term spectra
> now prints values of spectral components when only FFT is shown
1.19 - 23-Sep-98
> put check in "dp" command to keep it from crashing
> fixed bug in get_line() which close the wrong file pointer
1.18 - 2-Sep-98
> added "et" (elapsed time) function
1.17 - 27-Aug-98
> fixed read_cfg2() to allow port=0x140 with Pinnacle soundcard
1.16 - 31-Jun-98
> added "dp" command
> added parameters (ncal, sens_mp, f1, f2, f3) to config & data files
1.15 - 28-Jun-98
> added bandpass filter command "bp"
1.14 - 17-Jun-98
> added "pa" (pause) function for command lists
> added suppressor (f3) to tone pair
1.13 - 17-Jun-98
> increased number of possible sample rates
1.12 - 16-Jun-98
> made I/O buffer larger when gap size is larger
1.11 - 12-Jun-98
> saves history of typed command lines
1.10 - 11-Jun-98
> added "read list" command
1.9 - 10-Jun-98
> added ramps to tones
> added dB SPL display mode
> added calibration file and tone level
1.8 - 13-Feb-98
> re-organized code for wio.dll
1.7 - 12-Jan-98
> tweaked masking stimulus
1.6 - 31-Dec-97
> put control-char edit into gr_line()
> put gr_line in separate file
> redefined ybot
1.5 - 22-Dec-97
> first win32 version
> first X window version
1.4 - 5-Nov-97
> fixed "no skip" for "sk=0"
1.3 - 30-Oct-97
> fixed (?) reading MAT5 files (29-Oct)
> redid spectral average
1.2 - 27-Oct-97
> added print screen function
> fixed spectral average
1.1 - 17-Oct-97
> removed digital "feedthru" from idle loop in DSP code
1.0 - 15-Oct-97
> split sysres.c and display.c into smaller files
> added "gap" feature for Jeff McGregor & Duck Kim
0.9 - 3-Oct-97
> added "forward masking" stimulus
> added "go" command to display time-response
0.8 - 2-Oct-97
> fixed bugs in read/write ascii data
0.7 - 24-Sep-97
> fixed MAT files for MATLAB 5.0 and byte-swap
0.6 - 16-Sep-97
> fixed typo on help page
0.5 - 7-Jul-97
> added input attenuation
> read and write 2-channel files
0.4 - 6-Jul-97
> added all a/d & d/a modes
> added RTE display
0.3 - 1-Jul-97
> add response samples as "long" integer values
0.2 - 30-Jun-97
> added phase and delay plots
> reads and writes MATLAB files
0.1 - 24-Jun-97
> made GRX mode similar to TEK mode
0.0 - 23-Jun-97
> first version runs on Monterey & Pinnacle
**************************************************************************/
