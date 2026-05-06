; =====================================================================
; Patient Vital Signs Monitor -- Inno Setup 6 Installer Script
; Standard : IEC 62304 Class B Medical Device Software
; Version  : 2.7.0
;
; Build with: ISCC.exe installer.iss
; Output   : dist\PatientMonitorSetup-2.7.0.exe
; =====================================================================

#define AppName      "Patient Vital Signs Monitor"
#define AppVersion   "2.7.0"
#define AppPublisher "Medical Device Project"
#define AppExeName   "patient_monitor_gui.exe"
[Setup]
AppId={{A3F2B5C1-8D4E-4F72-9B3A-0C1D5E6F789A}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppComments=IEC 62304 Class B - Patient Vital Signs Monitoring System
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
AllowNoIcons=yes
LicenseFile=
OutputDir=dist
OutputBaseFilename=PatientMonitorSetup-{#AppVersion}
; Also update create_installer.bat echo lines if version changes
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x86 x64compatible
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppExeName}
ChangesAssociations=no
DisableProgramGroupPage=no
ShowLanguageDialog=no
DisableWelcomePage=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
; Main application -- no external DLLs needed (pure Win32, statically linked)
Source: "build_release\patient_monitor_gui.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Start Menu
Name: "{group}\{#AppName}";            Filename: "{app}\{#AppExeName}"
Name: "{group}\Uninstall {#AppName}";  Filename: "{uninstallexe}"
; Desktop (optional task)
Name: "{commondesktop}\{#AppName}";    Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; \
    Description: "Launch {#AppName} now"; \
    Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: dirifempty; Name: "{app}"
