; Instalador de Circuito de Escape (Inno Setup 6)
; Antes de compilarlo:
;   cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
;   cmake --build build --target map_generator escape_circuit
;   .\build\map_generator.exe --maps 10 --workers 4 --seed 2026 --save assets/maps
; Luego:  ISCC installer\setup.iss   (crea dist\CircuitoDeEscape_Instalador.exe)

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
UninstallDisplayIcon={app}\escape_circuit.exe

[Files]
Source: "..\build\escape_circuit.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\map_generator.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\assets\maps\mapa_*.txt"; DestDir: "{app}\assets\maps"; Flags: ignoreversion
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Circuito de Escape - Facil"; Filename: "{app}\escape_circuit.exe"; Parameters: "--level easy --ascii"; WorkingDir: "{app}"
Name: "{group}\Circuito de Escape - Medio"; Filename: "{app}\escape_circuit.exe"; Parameters: "--level medium --ascii"; WorkingDir: "{app}"
Name: "{group}\Circuito de Escape - Dificil"; Filename: "{app}\escape_circuit.exe"; Parameters: "--level hard --ascii"; WorkingDir: "{app}"
Name: "{group}\Generador de mapas"; Filename: "{cmd}"; Parameters: "/k ""{app}\map_generator.exe"" --maps 10 --workers 4 --seed 2026 --rows 15 --cols 21 --save assets/maps"; WorkingDir: "{app}"
Name: "{group}\Desinstalar Circuito de Escape"; Filename: "{uninstallexe}"
