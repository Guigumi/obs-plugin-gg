#define MyAppName "Input Overlay GG"
#define MyAppVersion "1.1.0"
#define MyAppPublisher "GUI"
#define MyAppURL "https://github.com/Guigumi/obs-plugin-gg"
#define MyAppId "{D54E1B8B-8AAE-4B3A-9B9A-6B8C2E2D7C10}"

[Setup]
AppId={{#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonappdata}\obs-studio\plugins\mouse-overlay-gg
DisableProgramGroupPage=yes
PrivilegesRequired=admin
OutputDir=release
OutputBaseFilename=Input_overlay_gg_v1.1.0_Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
Uninstallable=yes
UninstallDisplayName={#MyAppName}
VersionInfoVersion={#MyAppVersion}
VersionInfoDescription={#MyAppName} installer
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible

[Files]
Source: "build_x64\Release\mouse-overlay-gg.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Result := '';
  if MsgBox('OBS Studio precisa ser fechado antes da instalação. Deseja continuar?', mbConfirmation,
    MB_OKCANCEL) = IDCANCEL then begin
    Result := 'A instalação foi cancelada.';
    exit;
  end;

  Exec(ExpandConstant('{sys}\taskkill.exe'), '/IM obs64.exe /T /F', '', SW_HIDE,
    ewWaitUntilTerminated, ResultCode);
  Sleep(1000);
end;
