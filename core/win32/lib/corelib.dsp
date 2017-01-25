# Microsoft Developer Studio Project File - Name="corelib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=corelib - Win32 Private Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "corelib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "corelib.mak" CFG="corelib - Win32 Private Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "corelib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "corelib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "corelib - Win32 Private Release" (based on "Win32 (x86) Static Library")
!MESSAGE "corelib - Win32 Private Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "corelib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../" /I "../../common" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /X /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../" /I "../../common" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "corelib___Win32_Private_Release"
# PROP BASE Intermediate_Dir "corelib___Win32_Private_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "corelib___Win32_Private_Release"
# PROP Intermediate_Dir "corelib___Win32_Private_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "../../" /I "../../common" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT BASE CPP /X /YX
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../" /I "../../common" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "PRIVATE_BROADCASTER" /FD /c
# SUBTRACT CPP /X /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "corelib___Win32_Private_Debug"
# PROP BASE Intermediate_Dir "corelib___Win32_Private_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "corelib___Win32_Private_Debug"
# PROP Intermediate_Dir "corelib___Win32_Private_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../" /I "../../common" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../" /I "../../common" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "PRIVATE_BROADCASTER" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "corelib - Win32 Release"
# Name "corelib - Win32 Debug"
# Name "corelib - Win32 Private Release"
# Name "corelib - Win32 Private Debug"
# Begin Group "Core Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\channel.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\gnutella.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\html.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\http.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\icy.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\inifile.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\jis.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\mms.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\mp3.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\nsv.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\ogg.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\pcp.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\peercast.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\rtsp.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\servent.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\servhs.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\servmgr.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\socket.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\stats.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\stream.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\sys.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\url.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\common\xml.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# SUBTRACT BASE CPP /YX
# SUBTRACT CPP /YX

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Core Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\asf.h
# End Source File
# Begin Source File

SOURCE=..\..\common\atom.h
# End Source File
# Begin Source File

SOURCE=..\..\common\channel.h
# End Source File
# Begin Source File

SOURCE=..\..\common\common.h
# End Source File
# Begin Source File

SOURCE=..\..\common\cstream.h
# End Source File
# Begin Source File

SOURCE=..\..\common\gnutella.h
# End Source File
# Begin Source File

SOURCE="..\..\common\html-xml.h"
# End Source File
# Begin Source File

SOURCE=..\..\common\html.h
# End Source File
# Begin Source File

SOURCE=..\..\common\http.h
# End Source File
# Begin Source File

SOURCE=..\..\common\icy.h
# End Source File
# Begin Source File

SOURCE=..\..\common\id.h
# End Source File
# Begin Source File

SOURCE=..\..\common\inifile.h
# End Source File
# Begin Source File

SOURCE=..\..\common\jis.h
# End Source File
# Begin Source File

SOURCE=..\..\common\mms.h
# End Source File
# Begin Source File

SOURCE=..\..\common\mp3.h
# End Source File
# Begin Source File

SOURCE=..\..\common\nsv.h
# End Source File
# Begin Source File

SOURCE=..\..\common\ogg.h
# End Source File
# Begin Source File

SOURCE=..\..\common\pcp.h
# End Source File
# Begin Source File

SOURCE=..\..\common\peercast.h
# End Source File
# Begin Source File

SOURCE=..\..\common\rtsp.h
# End Source File
# Begin Source File

SOURCE=..\..\common\servent.h
# End Source File
# Begin Source File

SOURCE=..\..\common\servmgr.h
# End Source File
# Begin Source File

SOURCE=..\..\common\socket.h
# End Source File
# Begin Source File

SOURCE=..\..\common\stats.h
# End Source File
# Begin Source File

SOURCE=..\..\common\stream.h
# End Source File
# Begin Source File

SOURCE=..\..\common\sys.h
# End Source File
# Begin Source File

SOURCE=..\..\common\url.h
# End Source File
# Begin Source File

SOURCE=..\..\common\version2.h
# End Source File
# Begin Source File

SOURCE=..\..\common\xml.h
# End Source File
# End Group
# Begin Group "Win32 Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\core\win32\wsocket.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\core\win32\wsys.cpp
# End Source File
# End Group
# Begin Group "Win32 Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\core\win32\wsocket.h
# End Source File
# Begin Source File

SOURCE=..\..\..\core\win32\wsys.h
# End Source File
# End Group
# Begin Group "Unix Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\unix\usocket.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\unix\usys.cpp

!IF  "$(CFG)" == "corelib - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "corelib - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Release"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "corelib - Win32 Private Debug"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Unix Includes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\unix\usocket.h
# End Source File
# Begin Source File

SOURCE=..\..\unix\usys.h
# End Source File
# End Group
# End Target
# End Project
