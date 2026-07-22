                                                    November-97

        SYSRES - System Response Program

SYSRES performs sychronous averaging of the soundcard input while
repeatedly presenting a stimulus on the soundcard output.  When the
stimulus is a wideband chirp or click, the program can be used to obtain
the frequency (or impulse) response of a arbitrary "system" attached to
the soundcard.  

SYSRES currently works with the Turtle Beach Monterey/Tahiti or
Pinnacle/Fiji soundcards and runs under DOS or Linux.  SYSRES may be
used to display previously recorded data on systems with no soundcard
installed. 


        DOS INSTALLATION

Put SYSRES.EXE in one of your PATH directories.  SYSRES requires DPMI
services to access extended memory.  These services are provided by
Windows 95, if you run SYSRES in a "DOS box".  Otherwise, run CWSDPMI
before running SYSRES (see CWSDPMI.TXT). 

The Monterey/Tahiti soundcard is assumed to have the control port set to
290 (hex) and shared-memory address set to D000-D777 (hex).  Other
values can be specifed in the MSND.CFG file which may be in either the
current directory, c:/tahiti, or c:/monterey. 

The Pinnacle/Fiji soundcard is assumed to have its configuration control
port set to 260 (hex).  SYSRES will configure the soundcard to use
control port 290 (hex) and shared-memory address set to D000-D777 (hex). 
Other values can be specifed in the PINCFG.INI file which may be in the
current directory, c:/tbspros/dosapps, or c:/pinnacle/program. 


        LINUX INSTALLATION

Put "sysres" in /usr/local/bin (or other PATH directory).  Sysres
requires super-user privelege to access the soundcard.  To run sysres as
a normal user, the "sysres" file must be own by root and the "set user
id" bit must be set in the file protection mode (e.g.  "chmod 4755
/usr/local/bin/sysres"). 

The Monterey/Tahiti soundcard is assumed to have the control port set to
290 (hex) and shared-memory address set to D000-D777 (hex).  Other
values can be specifed in the "msnd.cfg" file which may be in the
current directory, /usr/etc, or /etc. 

The Pinnacle/Fiji soundcard is assumed to have its configuration control
port set to 260 (hex).  The program will configure the soundcard to use
control port 290 (hex) and shared-memory address set to D000-D777 (hex). 
Other values can be specifed in the "pincfg.ini" file which may be in
the current directory, /usr/etc, or /etc. 

                                                    Stephen T. Neely
