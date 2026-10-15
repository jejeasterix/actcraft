; Installateur Windows d'ActCraft (décision 26, DA9 du cadrage livraison-complete). Compilé par scripts/paquet.ps1 :
;   ISCC.exe /DAppVersion=<x.y.z> /DSourceDir=<dossier préparé> /DOutputDir=<build/paquet> actcraft.iss
#ifndef AppVersion
  #error AppVersion manquant (/DAppVersion=x.y.z)
#endif
#ifndef SourceDir
  #error SourceDir manquant (/DSourceDir=<dossier préparé>)
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

[Setup]
AppId={{5F0C2E7A-3B4D-4C8E-9A61-ACC0C4AF7A10}
AppName=ActCraft
AppVersion={#AppVersion}
AppVerName=ActCraft {#AppVersion}
AppPublisher=ActCraft
AppPublisherURL=https://github.com/jejeasterix/actcraft
AppSupportURL=https://github.com/jejeasterix/actcraft/issues
DefaultDirName={autopf}\ActCraft
DefaultGroupName=ActCraft
DisableProgramGroupPage=yes
; Par utilisateur, sans droits d'administrateur ; « pour tous » proposé (boîte ou /ALLUSERS).
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=ActCraft-{#AppVersion}-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ChangesAssociations=yes
UninstallDisplayIcon={app}\actcraft.exe
LicenseFile={#SourceDir}\LICENSE.txt
SetupLogging=yes

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[Tasks]
Name: "assoc_actcraft"; Description: "Ouvrir les documents .actcraft avec ActCraft"; GroupDescription: "Associations de fichiers :"
Name: "assoc_flipchart"; Description: "Proposer ActCraft pour ouvrir les paperboards .flipchart"; GroupDescription: "Associations de fichiers :"; Flags: unchecked
Name: "desktopicon"; Description: "Créer un raccourci sur le bureau"; GroupDescription: "Raccourcis :"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\ActCraft"; Filename: "{app}\actcraft.exe"
Name: "{autodesktop}\ActCraft"; Filename: "{app}\actcraft.exe"; Tasks: desktopicon

[Registry]
Root: HKA; Subkey: "Software\Classes\.actcraft"; ValueType: string; ValueName: ""; ValueData: "ActCraft.Document"; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: assoc_actcraft
; Le ProgID sert aux deux associations : créé dès que l'une des deux est cochée.
Root: HKA; Subkey: "Software\Classes\ActCraft.Document"; ValueType: string; ValueName: ""; ValueData: "Document ActCraft"; Flags: uninsdeletekey; Tasks: assoc_actcraft or assoc_flipchart
Root: HKA; Subkey: "Software\Classes\ActCraft.Document\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\actcraft.exe,0"; Tasks: assoc_actcraft or assoc_flipchart
Root: HKA; Subkey: "Software\Classes\ActCraft.Document\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\actcraft.exe"" ""%1"""; Tasks: assoc_actcraft or assoc_flipchart
; .flipchart : seulement « Ouvrir avec » ; l'application par défaut (ActivInspire installé) reste inchangée.
Root: HKA; Subkey: "Software\Classes\.flipchart"; ValueType: none; Flags: uninsdeletekeyifempty; Tasks: assoc_flipchart
Root: HKA; Subkey: "Software\Classes\.flipchart\OpenWithProgids"; ValueType: string; ValueName: "ActCraft.Document"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty; Tasks: assoc_flipchart

[Run]
Filename: "{app}\actcraft.exe"; Description: "Lancer ActCraft"; Flags: nowait postinstall skipifsilent
