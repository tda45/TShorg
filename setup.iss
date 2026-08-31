[Setup]
AppName=TShorg
AppVersion=1.0.0
AppPublisher=tda_45
DefaultDirName={autopf}\TShorg
DefaultGroupName=TShorg
OutputDir=Output
OutputBaseFilename=TShorg_Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ChangesEnvironment=yes

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "addtopath"; Description: "TShorgModCompiler'ı terminalden kullanabilmek için PATH sistem değişkenine ekle"; GroupDescription: "Sistem Ayarları:"

[Files]
; Release klasöründeki tüm dosyaları (TShorg.exe, TShorgModCompiler.exe ve DLL'leri) alır
Source: "C:\Users\hp\TShorg\build\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Sadece GUI uygulaması için kısayol oluşturulur
Name: "{group}\TShorg"; Filename: "{app}\TShorg.exe"
Name: "{group}\{cm:UninstallProgram,TShorg}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\TShorg"; Filename: "{app}\TShorg.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\TShorg.exe"; Description: "{cm:LaunchProgram,TShorg}"; Flags: nowait postinstall skipifsilent

[Code]
// PATH değişkenine güvenli ekleme ve kaldırma işlemleri

const
  EnvironmentKey = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

procedure AddToPath();
var
  OldPath, NewPath: string;
begin
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', OldPath) then
  begin
    // Eğer dizin zaten PATH içinde yoksa sonuna ekle
    if Pos(ExpandConstant('{app}'), OldPath) = 0 then
    begin
      if (OldPath <> '') and (OldPath[Length(OldPath)] <> ';') then
        OldPath := OldPath + ';';
      NewPath := OldPath + ExpandConstant('{app}');
      RegWriteStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', NewPath);
    end;
  end;
end;

procedure RemoveFromPath();
var
  OldPath: string;
  AppDir: string;
  P: Integer;
begin
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', OldPath) then
  begin
    AppDir := ExpandConstant('{app}');
    P := Pos(AppDir, OldPath);
    if P > 0 then
    begin
      // Dizini ve yanındaki noktalı virgülü PATH içinden temizle
      Delete(OldPath, P, Length(AppDir));
      if (P <= Length(OldPath)) and (OldPath[P] = ';') then
        Delete(OldPath, P, 1);
      RegWriteStringValue(HKEY_LOCAL_MACHINE, EnvironmentKey, 'Path', OldPath);
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('addtopath') then
  begin
    AddToPath();
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    RemoveFromPath();
  end;
end;