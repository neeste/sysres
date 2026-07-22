# SysRes

**Boys Town National Research Hospital**  
**Authors:** Stephen Neely, Rob Stevenson  
**Revision Date:** March 2002 (Based on version 1.83)

## Abstract
SysRes is a program designed to measure a system response by performing synchronous averaging of the response to a wideband stimulus.

## Introduction
SysRes is short for System Response and was modeled after the SysID program. In addition to being able to do real-time simultaneous response measurements, SysRes can also do DPOAE averaging in the style of CUBeDIS. Both SysID and CUBeDIS were originally written by Jont Allen. 

SysRes is a highly portable program having been implemented on Windows, DOS, and Linux operating systems and in 5 different environments. The current version of SysRes supports high-quality sound cards such as the 20-bit Pinnacle or Fiji from Turtle Beach/Voyetra and the CardDeluxe from Digital Audio Labs. In addition, SysRes is capable of supporting more than one sound card in the Win32 environment (but not at the same time).

SysRes is a program with many possible uses. With the correct configuration, SysRes can measure a response from almost any system. In particular, the program was written with the use of inner ear responses using an OAE probe microphone, such as the Etymotic ER-10C.

## Tutorial
SysRes is a command-driven program, meaning all of the user input and program commands are given through the keyboard. Start SysRes by typing `sysres` on a Linux or DOS command line, or clicking the SysRes icon in Windows. When the program starts, the configuration parameters are all set to their default values.

The most basic use of SysRes is to get a system response. For a simple loop-back configuration, the soundcard output is connected to the soundcard input. Once connected, issue the `di` command to display current soundcard information. Next, issue the `fn` command to compute a frequency response using the noise stimulus.

To change the view to a time and magnitude axis, issue the `tr` command. To zoom into a portion of the graph (e.g., from 0 to 3msec), set the zoom time value by issuing `zt2=0.003`. The `zo` value is automatically set to `1`. Redraw the graph by issuing `tr` again. To see the whole graph again, reset `zo=1` and issue `tr`.

## Commands
SysRes commands are abbreviated. When entering a command to set a value, an equals sign (`=`) is needed directly after the command with no spaces. If no equals sign is present, the current state of the value is shown.

### Configuration Commands
* **`ad`** - Set A/D mode (input) to a channel [0=No input, 1=L, 2=R, 3=L&R, 4=L/R, 5=R/L]. Default: 1.
* **`da`** - Set D/A mode (output) to a channel [0=No output, 1=L, 2=R, 3=L&R, 4=L/R, 5=R/L]. Default: 1.
* **`dc`** - Set the DC component removed from response [0=keep, 1=remove].
* **`ia`** - Input attenuation from the microphone or probe (in dB). Default: 0.0.
* **`na`** - Set the number of averages for the stimulus (1 to 32000). Default: 20.
* **`ns`** - Set the number of samples for the stimulus (64 to 2^20, must be power of 2). Default: 1024.
* **`oa`** - Output attenuation to the speaker or probe (in dB). Default: 0.
* **`sk`** - Number of repetitions to skip prior to averaging.
* **`sr`** - Set the sample rate to be used. Default: 44 kHz.

### Frequency Response Commands
* **`fc`** - Measure frequency response using the chirp stimulus.
* **`fi`** - Measure frequency response using the impulse stimulus.
* **`ft`** - Measure frequency response using the tone stimulus.
* **`fp`** - Measure frequency response using the tone-pair stimulus.
* **`fn`** - Measure frequency response using the pseudo-random noise stimulus.
* **`fu`** - Measure frequency response using a user-defined stimulus (external file).
* **`fr`** - Frequency response using the previously used stimulus. Default is chirp.
* **`nf`** - Measure noise floor (normalized to stimulus).
* **`no`** - Normalize to saved response (`=filename`).
* **`sa`** - Measure spectral average by repeatedly measuring the system response.

### Plot Commands
* **`cs`** - Clear screen (clears plotted data).
* **`crn`** - Set the upper right-hand corner string. Default: "SysRes".
* **`dr`** - Dynamic range (sets dB range). Default: 100.
* **`dm`** - Delay axis maximum (ms). Default: 4.
* **`dn`** - Delay axis range (ms). Default: 5.
* **`do`** - Delay offset removed (ms). Default: 0.
* **`eml`** - Extra message lines under the graph. Default: 0.
* **`pl`** - Toggle frequency domain scale from log to linear.
* **`pm`** - Set upper bound of the phase axis (cycles). Default: 2.
* **`pn`** - Set range and lower bound of the phase axis. Default: 4.
* **`po`** - Phase offset removed (cycles). Default: 0.
* **`pr`** - Plot response versus frequency (Default setting).
* **`ps`** - Plot stimulus versus frequency.
* **`shd`** - Show delay mode [0=off, 1=on].
* **`shp`** - Show phase mode [0=off, 1=on].
* **`tr`** - Plot response versus time.
* **`ts`** - Plot stimulus versus time.
* **`td`** - Tick direction on graph [0=inward, 1=outward]. Default: 0.
* **`te`** - Plot reverse-time-energy vs time.
* **`vr`** - Voltage reference (dBV). Default: 10.
* **`SR`** - SPL reference (dBSPL).
* **`zf1` / `zf2`** - Zoom frequency bounds (Hz).
* **`zt1` / `zt2`** - Zoom time bounds (seconds).
* **`zo`** - Zoom on/off (toggle).

### File Commands
* **`lf`** - Log messages to `sysres.log` [0=off, 1=on].
* **`ra`** - Read ASCII data file (`sysres.txt` if none given).
* **`rc`** - Read configuration file (`sysres.cfg`).
* **`rd`** - Read data file (MATLAB format, `sysres.mat`).
* **`rl`** - Read list of commands from a `.lst` file.
* **`rs`** - Read stimulus file (MATLAB format).
* **`wa`** - Write ASCII data file (`sysres.txt`).
* **`wc`** - Write configuration to `sysres.cfg`.
* **`wd`** - Write data file in MATLAB format (`sysres.mat`).
* **`wh`** - Write help file (`help.txt`).
* **`ws`** - Write stimulus file (MATLAB format).

### Printer Commands
* **`PL`** - Printer label. Default: `"$vn"` (version number).
* **`PN`** - Printer name/destination (e.g., `sysres.prn` or `!lpr`).
* **`PO`** - Print orientation [0=Landscape, 1=Portrait].
* **`PS`** - Print screen to PostScript/PCL file or printer.
* **`PT`** - Printer type (PostScript or PCL).

### Other Commands
*(Note: SysRes also supports extensive DPOAE, Stimulus, and Masking commands. Please refer to the program help for full details.)*
* **`be`** - Beep, plays a system sound.
* **`bp`** - Band-pass filter (from F1 to F2).
* **`cd`** - Get or set the current directory.
* **`co`** - Comment. Inserts one line of text on screen.
* **`dbg`** - Debug mode [0=off, 1=on].
* **`di`** - SIO device information (view or change soundcards).
* **`et`** - Elapsed time in seconds.
* **`ga`** - Gap between stimulus repetitions (sec).
* **`go`** - Average and show time-domain response.
* **`ls`** - List file names of the current working directory.
* **`sm`** - Sensitivity of microphone (V/Pa).
* **`ow`** - Overwrite existing data files without prompting [0=no, 1=yes].
* **`pa`** - Pause (with optional message).
* **`pg`** - Pregap before I/O (sec).
* **`q`** - Quit.
* **`sl`** - Sleep for N seconds.
* **`to`** - Time-out pause after N seconds.
* **`vn`** - Displays the version number of SysRes.

## Command Lists
SysRes has a scripting capability to run list files (`.lst` extension) that contain commands instead of typing them every time. 
* Execute on startup: `sysres command.lst`
* Execute from within SysRes: `rl=command.lst`
* Add comments in a list file by starting a line with a semi-colon (`;`).
* Use the pause (`pa`) command within a list to pause execution (e.g., `pa=record data` will display "record data" and pause).
