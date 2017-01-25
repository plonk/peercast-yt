; waplugin.nsi
;
; This script will generate an installer that installs a Winamp plug-in.
; It also puts a license page on, for shits and giggles.
;
; This installer will automatically alert the user that installation was
; successful, and ask them whether or not they would like to make the 
; plug-in the default and run Winamp.
;

; The name of the installer
Name "PeerCast Plug-in v0.1218"


; The file to write
OutFile "peercast-winamp.exe"

; License page
; LicenseText "This installer will install the Nullsoft Tiny Visualization 2000 Plug-in for Winamp. Please read the license below."
; use the default makensis license :)
; LicenseData license.txt

; The default installation directory
InstallDir $PROGRAMFILES\Winamp
; detect winamp path from uninstall string if available
InstallDirRegKey HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"

; The text to prompt the user to enter a directory
DirText "Please select your Winamp path below (you will be able to proceed when Winamp is detected):"
;DirShow hide

; automatically close the installer when done.
AutoCloseWindow true
; hide the "show details" box
ShowInstDetails nevershow

Function .onVerifyInstDir
!ifndef WINAMP_AUTOINSTALL
  IfFileExists $INSTDIR\Winamp.exe Good
    Abort
  Good:
!endif ; WINAMP_AUTOINSTALL
FunctionEnd


Function KillPeercast
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

Function KillWinamp
  FindWindow $0 "Winamp v1.x"
  IsWindow $0 Kill Skip
  Kill:
    MessageBox MB_OKCANCEL "Winamp is already running, press OK to close it and continue installing."  IDCANCEL Die
    SendMessage $0 16 0 0
    Sleep 3000
    Goto Skip
  Die:
    Abort
  Skip:
FunctionEnd


!ifdef WINAMP_AUTOINSTALL
Function GetWinampInstPath
  Push $0
  Push $1
  Push $2
  ReadRegStr $0 HKLM \
     "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \ 
     "UninstallString"
  StrCmp $0 "" fin

    StrCpy $1 $0 1 0 ; get firstchar
    StrCmp $1 '"' "" getparent 
      ; if first char is ", let's remove "'s first.
      StrCpy $0 $0 "" 1
      StrCpy $1 0
      rqloop:
        StrCpy $2 $0 1 $1
        StrCmp $2 '"' rqdone
        StrCmp $2 "" rqdone
        IntOp $1 $1 + 1
        Goto rqloop
      rqdone:
      StrCpy $0 $0 $1
    getparent:
    ; the uninstall string goes to an EXE, let's get the directory.
    StrCpy $1 -1
    gploop:
      StrCpy $2 $0 1 $1
      StrCmp $2 "" gpexit
      StrCmp $2 "\" gpexit
      IntOp $1 $1 - 1
      Goto gploop
    gpexit:
    StrCpy $0 $0 $1

    StrCmp $0 "" fin
    IfFileExists $0\winamp.exe fin
      StrCpy $0 ""
  fin:
  Pop $2
  Pop $1
  Exch $0
FunctionEnd



Function MakeSureIGotWinamp
  Call GetWinampInstPath
  Pop $0
  StrCmp $0 "" getwinamp
    Return
  getwinamp:
  StrCpy $1 $TEMP\porearre1.dll 
  StrCpy $2 "$TEMP\Winamp Installer.exe"
  File /oname=$1 nsisdl.dll
  Push http://download.nullsoft.com/winamp/client/winamp281_lite.exe
  Push $2
  CallInstDLL $1 download
  Delete $1
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint "download failed: $0"
    Abort
  success:
    ExecWait '"$2" /S'
    Delete $2
    Call GetWinampInstPath
    Pop $0
    StrCmp $0 "" skip
    StrCpy $INSTDIR $0
  skip:
FunctionEnd

!endif ; WINAMP_AUTOINSTALL
; The stuff to install
Section "ThisNameIsIgnoredSoWhyBother?"

  call killWinamp
  call killPeercast


  StrCpy $1 $INSTDIR\Plugins
  SetOutPath $1

  ; File to extract
  File "gen_peercast.dll"

  StrCpy $1 $INSTDIR\Plugins\peercast\html

  SetOutPath $1\en
  File "..\..\..\html\en\*.*"
  SetOutPath $1\en\images
  File "..\..\..\html\en\images\*.*"

  SetOutPath $1\de
  File "..\..\..\html\de\*.*"
  SetOutPath $1\de\images
  File "..\..\..\html\de\images\*.*"


  SetOutPath $1\ja
  File "..\..\..\html\ja\*.*"
  SetOutPath $1\ja\images
  File "..\..\..\html\ja\images\*.*"

  SetOutPath $1\fr
  File "..\..\..\html\fr\*.*"
  SetOutPath $1\fr\images
  File "..\..\..\html\fr\images\*.*"


  StrCpy $1 $INSTDIR\Plugins



  ; prompt user, and if they select no, skip the following 3 instructions.
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "The plug-in was installed. Would you like to run Winamp now?" \
             IDNO NoWinamp
    ;WriteINIStr "$INSTDIR\Winamp.ini" "Winamp" "visplugin_name" "vis_nsfs.dll"
    ;WriteINIStr "$INSTDIR\Winamp.ini" "Winamp" "visplugin_num" "0"
    Exec '"$INSTDIR\Winamp.exe"'
  NoWinamp:

  WriteRegStr HKEY_CLASSES_ROOT "peercast" "" "URL:PeerCast Protocol"
  WriteRegStr HKEY_CLASSES_ROOT "peercast" "URL Protocol" ""
  ;WriteRegStr HKEY_CLASSES_ROOT "peercast\Default Icon" "" "$1\gen_peercast.dll"
  WriteRegStr HKEY_CLASSES_ROOT "peercast\shell\open\command" "" 'rundll32 "$1\gen_peercast.dll",callURL %1'


SectionEnd

; eof
