; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "XML Mill"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "William Hallatt"
#define MyAppURL "http://www.goblincoding.com/xmlmill"
#define MyAppExeName "XMLMill.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{098896F3-F9CF-44F9-9B23-76B83F852321}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
LicenseFile=C:\Documents\Personal\XMLMill5\GNUGPL.txt
OutputDir=C:\Documents\Personal\XMLMill5\Setup
OutputBaseFilename=SetupXMLMill
SetupIconFile=C:\Documents\Personal\XMLMill5\goblinappicon.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1

[Files]
Source: "C:\Documents\Personal\XMLMill5\XMLMill.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\D3DCompiler_43.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\icudt49.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\icuin49.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\icuuc49.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\libEGL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\libgcc_s_sjlj-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\libGLESv2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\Qt5Sql.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\Qt5Xml.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Documents\Personal\XMLMill5\sqldrivers\qsqlite.dll"; DestDir: "{app}\sqldrivers"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "C:\Documents\Personal\XMLMill5\platforms\qwindows.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:ProgramOnTheWeb,{#MyAppName}}"; Filename: "{#MyAppURL}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

