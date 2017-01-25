; example2.nsi
;
; This script is based on example1.nsi, but adds uninstall support
; and (optionally) start menu shortcuts.
;
; It will install notepad.exe into a directory that the user selects,
;

; The name of the installer
Name "PeerCast v0.1214 Licensed"


Function Kill
  FindWindow $0 PeerCast
  IsWindow $0 Kill Skip
  Kill:
    MessageBox MB_OKCANCEL "PeerCast is already running, press OK to close it and continue installing."  IDCANCEL Die
    SendMessage $0 16 0 0
    Sleep 3000
    Goto Skip
  Die:
    Abort
  Skip:
FunctionEnd


Function un.Kill
  FindWindow $0 PeerCast
  IsWindow $0 Kill Skip
  Kill:
    MessageBox MB_OKCANCEL "PeerCast is already running, press OK to close it and continue uninstalling."  IDCANCEL Die
    SendMessage $0 16 0 0
    Sleep 3000
    Goto Skip
  Die:
    Abort
  Skip:
FunctionEnd

Function .onInit
  SetOutPath $TEMP
  File /oname=spltmp.bmp "peercast.bmp"

; optional
; File /oname=spltmp.wav "my_splashshit.wav"

;  File /oname=spltmp.exe "C:\program files\nsis\splash.exe"
;  ExecWait '"$TEMP\spltmp.exe" 1000 $HWNDPARENT $TEMP\spltmp'
;  Delete $TEMP\spltmp.exe
;  Delete $TEMP\spltmp.bmp
;  Delete $TEMP\spltmp.wav

  Call Kill
FunctionEnd

Function un.onInit
 Call un.Kill
FunctionEnd


; The file to write
OutFile "peercast-lic-win32.exe"

; The default installation directory
InstallDir $PROGRAMFILES\PeerCast
; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM SOFTWARE\PeerCast "Install_Dir"

; The text to prompt the user to enter a directory
ComponentText "This will install PeerCast on your computer. Select which optional things you want installed."
; The text to prompt the user to enter a directory
DirText "Choose a directory to install in to:"

;InstType "Normal"
;InstType "NT Service"

; The stuff to install
Section "PeerCast (required)" 


  StrCpy $1 "$INSTDIR\html"

  SetOutPath "$1\en"
  File "..\..\html\en\*.*"
  SetOutPath "$1\en\images"
  File "..\..\html\en\images\*.*"

  SetOutPath "$1\ja"
  File "..\..\html\ja\*.*"
  SetOutPath "$1\ja\images"
  File "..\..\html\ja\images\*.*"

  SetOutPath "$1\de"
  File "..\..\html\de\*.*"
  SetOutPath "$1\de\images"
  File "..\..\html\de\images\*.*"

  SetOutPath "$1\fr"
  File "..\..\html\fr\*.*"
  SetOutPath "$1\fr\images"
  File "..\..\html\fr\images\*.*"


  SetOutPath "$INSTDIR"
  File "peercast.exe"
  File "oggcap.exe"


  ; add files / whatever that need to be installed here.
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\PeerCast" "" "$INSTDIR"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\PeerCast" "DisplayName" "PeerCast (remove only)"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\PeerCast" "UninstallString" '"$INSTDIR\uninst.exe"'

  ; register peercast:// URI 
  WriteRegStr HKEY_CLASSES_ROOT "peercast" "" "URL:PeerCast Protocol"
  WriteRegStr HKEY_CLASSES_ROOT "peercast" "URL Protocol" ""
  WriteRegStr HKEY_CLASSES_ROOT "peercast\Default Icon" "" "$INSTDIR\peercast.exe"
  WriteRegStr HKEY_CLASSES_ROOT "peercast\shell\open\command" "" '"$INSTDIR\peercast.exe" -inifile "$INSTDIR\peercast.ini" -url "%1"'

  ; write out uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd 

Section /o "Run on Startup" 
  CreateShortCut "$SMSTARTUP\PeerCast.lnk" "$INSTDIR\peercast.exe" ""
SectionEnd 

;Section "PeerCast NT Service" 
; SectionIn 2
;  File "instsrv.exe"
;  File "srvany.exe"
;  ExecWait '"$INSTDIR\instsrv.exe" PeerCast "$INSTDIR\srvany.exe"' 
;  ExecWait '"$INSTDIR\instsrv.exe" PeerCast c:\srvany.exe' 
;  WriteRegStr HKEY_LOCAL_MACHINE "SYSTEM\CurrentControlSet\Services\PeerCast\Parameters" "AppDirectory" "$INSTDIR"
;  WriteRegStr HKEY_LOCAL_MACHINE "SYSTEM\CurrentControlSet\Services\PeerCast\Parameters" "Application" "$INSTDIR\peercast.exe"
;SectionEnd 

; optional section
Section "Add Start Menu shortcuts"
;  File "peercast.hlp"
  CreateDirectory "$SMPROGRAMS\PeerCast"
  CreateShortCut "$SMPROGRAMS\PeerCast\Start PeerCast.lnk" "$INSTDIR\peercast.exe" 
  CreateShortCut "$SMPROGRAMS\PeerCast\Stop PeerCast.lnk" "$INSTDIR\peercast.exe" "-kill" 
  CreateShortCut "$SMPROGRAMS\PeerCast\OggCap Video Broadcaster.lnk" "$INSTDIR\oggcap.exe" 
  CreateShortCut "$SMPROGRAMS\PeerCast\Yellow Pages.lnk" "http://yp.peercast.org" 
  CreateShortCut "$SMPROGRAMS\PeerCast\Online Help.lnk" "http://www.peercast.org/help.php" 
  CreateShortCut "$SMPROGRAMS\PeerCast\Latest News.lnk" "http://www.peercast.org/forum/viewforum.php?f=9" 
  CreateShortCut "$SMPROGRAMS\PeerCast\Uninstall.lnk" "$INSTDIR\uninstall.exe" 
  CreateDirectory "$SMPROGRAMS\PeerCast\Links"
  CreateShortCut "$SMPROGRAMS\PeerCast\Links\PeerCast Home.lnk" "http://www.peercast.org" 
  CreateShortCut "$SMPROGRAMS\PeerCast\Links\PeerCast Forums.lnk" "http://www.peercast.org/forum" 
  CreateShortCut "$SMPROGRAMS\PeerCast\Links\PeerCast Download.lnk" "http://www.peercast.org/download.php" 
  CreateShortCut "$SMPROGRAMS\PeerCast\Links\Report a bug.lnk" "http://www.peercast.org/bugs.php" 
SectionEnd


 Function .onInstSuccess
    MessageBox MB_YESNO "Installation has finished. Would you like to run PeerCast now?"  IDNO NoExe
      Exec '"$INSTDIR\peercast.exe"'
    NoExe:
  FunctionEnd


; begin uninstall settings/section
UninstallText "This will uninstall PeerCast from your system"

Section "Uninstall"
  ; add delete commands to delete whatever files/registry keys/etc you installed here.
  ; Delete "$INSTDIR\uninst.exe"

  ;ExecWait '"$INSTDIR\instsrv.exe" PeerCast REMOVE' 

  RMDir /r "$SMPROGRAMS\PeerCast"
  Delete "$SMSTARTUP\PeerCast.lnk"

  
  DeleteRegKey HKEY_CLASSES_ROOT "peercast"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\PeerCast"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\PeerCast"

  Delete "$INSTDIR\peercast.exe"
  Delete "$INSTDIR\peercast.ini"
  Delete "$INSTDIR\play.pls"
  Delete "$INSTDIR\uninstall.exe"
  RMDir /r "$INSTDIR\html"

SectionEnd ; end of uninstall section

; eof
