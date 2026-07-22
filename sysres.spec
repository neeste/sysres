# sysres.spec - spec file to create the sysres RPM
#
Name: SysRes
Summary: Syncronous averaging program measures audio system response
Version: 2.34
Release: 1
Copyright: Boys Town National Research Hospital (1988-2007)
Group: Applications/Sound
Packager: Stephen Neely <neely@boystown.org>
BuildRoot: /tmp/%{name}-buildroot
Source0: sysressc.tgz

%description
performs system response measurements

%prep
rm -rf $RPM_BUILD_DIR/sysres
%setup -n sysres
chmod -x -f *.c *.h *.ico *.rc *.txt makefile.*

%build
make -f makefile.lnx ROOT=$RPM_BUILD_ROOT

%install
make -f makefile.lnx ROOT=$RPM_BUILD_ROOT install

%files
/usr/local/bin/sysres
