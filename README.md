# SYSRES Build Instructions

This document provides instructions for compiling and building SYSRES on Linux, MacOS, and Windows platforms.

## Linux
*SysRes uses the X window system to provide a graphical user interface under Linux and the ARSC library to access the soundcard. ARSC assumes the presence of an ALSA driver for the soundcard.*

**To build and install:**
1. Open a terminal and navigate to the `sysres` source directory.
2. Compile the source using the provided Linux makefile by running:
   ```bash
   make -f makefile.lnx
   ```
3. Install SYSRES by copying the executable file `sysres` to your system's binaries folder:
   ```bash
   sudo cp sysres /usr/local/bin/
   ```

*(Note: The precompiled Linux version of SYSRES is online at http://audres.org/sysres-linux.zip. SYSRES has been tested with several versions of Linux and Echo soundcards like Indigo IO, Gina3G, and Layla3G).*

---

## MacOS
*SysRes can be built on MacOS using the provided Makefile. It relies on X11 (XQuartz) for its graphical interface and the Apple CoreAudio/AudioToolbox frameworks.*

**To build:**
1. Ensure you have Xcode Command Line Tools installed (`xcode-select --install`).
2. Ensure you have **XQuartz** installed, as the build requires X11 headers and libraries (located in `/opt/X11`).
3. Open a terminal, navigate to the `sysres` source directory, and run:
   ```bash
   make -f makefile.mac
   ```

**To install:**
Run the following command to install the executable to `/usr/local/bin`:
```bash
sudo make -f makefile.mac install
```

---

## Windows
*SysRes can be built on Windows using Visual Studio 2019.*

**To build:**
1. Ensure you have Visual Studio 2019 (v16.0) installed along with the "Desktop development with C++" workload.
2. Navigate to the `VS16` subfolder and open the solution file `sysres.sln` in Visual Studio.
3. Select your desired build configuration and platform (e.g., Release / x86 or x64) from the toolbar.
4. Build the solution from the **Build** menu (or press `Ctrl+Shift+B`).
5. The compiled executable will be generated in the respective output folder within the `VS16` directory. You can then run the setup or the executable directly.
