
[Setup]
AppId={{6A3B7E42-92F1-4F62-A80E-35C9D7B14209}
AppName=Circuito de Escape
AppVersion=2.0
AppPublisher=Grupo 4 - UTEC
DefaultDirName={localappdata}\Programs\CircuitoDeEscape
DefaultGroupName=Circuito de Escape
PrivilegesRequired=lowest
OutputDir=..\dist
OutputBaseFilename=CircuitoDeEscape_Instalador
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\Proyecto1PG3.exe

[Files]
Source: "..\cmake-build-debug\Proyecto1PG3.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\cmake-build-debug\map_generator.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\assets\maps\mapa_*.txt"; DestDir: "{app}\assets\maps"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Circuito de Escape"; Filename: "{app}\Proyecto1PG3.exe"; Parameters: "--level easy --ascii"; WorkingDir: "{app}"
Name: "{group}\Generador de mapas"; Filename: "{app}\map_generator.exe"; Parameters: "--maps 10 --workers 4 --seed 2026 --rows 15 --cols 21 --save assets/maps"; WorkingDir: "{app}"
Name: "{group}\Desinstalar Circuito de Escape"; Filename: "{uninstallexe}"
