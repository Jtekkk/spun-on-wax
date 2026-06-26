; Inno Setup script for Spun on Wax.
;
; Produces a Windows installer (Setup.exe) that installs the VST3 plugin into
; the system VST3 folder and, optionally, the standalone app.
;
; Build (from the repository root, after a Release CMake build):
;   iscc packaging\windows\SpunOnWax.iss
;
; The CMake build must have produced the artefacts under build\, e.g.
;   build\SpunOnWax_artefacts\Release\VST3\Spun on Wax.vst3
; Override the build directory with:  iscc /DBuildDir=somedir packaging\windows\SpunOnWax.iss

#define MyAppName "Spun on Wax"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "SpunOnWax"
#define MyAppURL "https://github.com/Jtekkk/spun-on-wax"

#ifndef BuildDir
  #define BuildDir "build"
#endif

#define ArtefactDir BuildDir + "\SpunOnWax_artefacts\Release"

[Setup]
AppId={{8E2D3A41-5C6B-4F2E-9A77-5357756E5761}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=LICENSE
OutputDir=build\installer
OutputBaseFilename=SpunOnWax-{#MyAppVersion}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; The VST3 install location requires administrator rights.
PrivilegesRequired=admin
SourceDir=..\..

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plugin (64-bit)"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "Standalone application"; Types: full custom

[Files]
; VST3 is a bundle (a folder) — copy it recursively into the system VST3 dir.
Source: "{#ArtefactDir}\VST3\{#MyAppName}.vst3\*"; \
    DestDir: "{commoncf64}\VST3\{#MyAppName}.vst3"; \
    Components: vst3; \
    Flags: recursesubdirs createallsubdirs ignoreversion

; Standalone application.
Source: "{#ArtefactDir}\Standalone\{#MyAppName}.exe"; \
    DestDir: "{app}"; \
    Components: standalone; \
    Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"; Components: standalone
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppName}.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Components: standalone; Flags: unchecked

[Run]
Filename: "{app}\{#MyAppName}.exe"; Description: "Launch {#MyAppName}"; Components: standalone; Flags: nowait postinstall skipifsilent
