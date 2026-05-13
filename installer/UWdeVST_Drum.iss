#define AppName "UWdeVST_Drum"
#ifndef AppVersion
#define AppVersion "1.0.1"
#endif
#define AppPublisher "Musique"
#define AppExeName "UWdeVST_drum.exe"
#define Vst3BundleName "UWdeVST_drum.vst3"

[Setup]
AppId={{9A1C70C0-58EF-4D1A-9FE3-2D7C43D4D701}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\Musique\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=output
OutputBaseFilename=UWdeVST_Drum_{#AppVersion}_Windows_x64_Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
SetupLogging=yes
UninstallDisplayIcon={app}\{#AppExeName}

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Components]
Name: "standalone"; Description: "Application standalone"; Flags: fixed
Name: "vst3"; Description: "Plugin VST3 64-bit"; Flags: fixed

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; Components: standalone

[InstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#Vst3BundleName}"

[Files]
Source: "staging\Standalone\{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone
Source: "staging\VST3\{#Vst3BundleName}\*"; DestDir: "{commoncf64}\VST3\{#Vst3BundleName}"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Components: standalone
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon; Components: standalone

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#Vst3BundleName}"
