[Setup]
AppName=SysRes
AppVerName=BTNRH SysRes 2.34
AppPublisher=Boys Town Nationial Research Hospital
AppPublisherURL=http://www.boystownhospital.org
AppSupportURL=http://audres.org/rc/sysres/
AppUpdatesURL=http://audres.org/rc/sysres/
DefaultDirName={pf}\BTNRH\SYSRES
DefaultGroupName=BTNRH

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"; MinVersion: 4,4

[Files]
Source: "VS16\Release\asysres.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "caltone.lst"; DestDir: "{app}"; Flags: ignoreversion
Source: "tstlat.lst"; DestDir: "{app}"; Flags: ignoreversion
Source: "fr2.lst"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\BTNRH\SysRes"; Filename: "{app}\asysres.exe"
Name: "{userdesktop}\SysRes"; Filename: "{app}\asysres.exe"; MinVersion: 4,4; Tasks: desktopicon

[Run]
Filename: "{app}\asysres.exe"; Description: "Launch SysRes?"; Flags: nowait postinstall skipifsilent

