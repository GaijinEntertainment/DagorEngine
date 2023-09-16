# Microsoft Developer Studio Project File - Name="Dagor" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Dagor - Win32 Debug MAX 7
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Dagor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Dagor.mak" CFG="Dagor - Win32 Debug MAX 7"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Dagor - Win32 Release MAX 5" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Dagor - Win32 Debug MAX 5" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Dagor - Win32 Public Release MAX 5" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Dagor - Win32 Release MAX 6" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Dagor - Win32 Debug MAX 6" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Dagor - Win32 Public Release MAX 6" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Dagor - Win32 Release MAX 7" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Dagor - Win32 Debug MAX 7" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Dagor - Win32 Public Release MAX 7" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Dagor - Win32 Release MAX 5"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dagor___Win32_Release_MAX_5"
# PROP BASE Intermediate_Dir "Dagor___Win32_Release_MAX_5"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dagor___Win32_Release_MAX_5"
# PROP Intermediate_Dir "Dagor___Win32_Release_MAX_5"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "e:\3dsmax6\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /YX"max.h" /FD /c
# SUBTRACT BASE CPP /u /Fr
# ADD CPP /nologo /MD /W3 /GX /O2 /I "e:\3dsmax5\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /YX"max.h" /FD /c
# SUBTRACT CPP /u /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /machine:I386 /out:"D:\3dsmax6\plugins\dagor.dlu"
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /machine:I386 /out:"e:\3dsmax5\plugins\dagor.dlu"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Dagor - Win32 Debug MAX 5"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Dagor___Win32_Debug_MAX_5"
# PROP BASE Intermediate_Dir "Dagor___Win32_Debug_MAX_5"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Dagor___Win32_Debug_MAX_5"
# PROP Intermediate_Dir "Dagor___Win32_Debug_MAX_5"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "e:\3dsmax6\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /FR /YX"max.h" /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "e:\3dsmax5\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /FR /YX"max.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"D:\3dsmax6\plugins\dagor.dlu" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"e:\3dsmax5\plugins\dagor.dlu" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Dagor - Win32 Public Release MAX 5"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dagor___Win32_Public_Release_MAX_5"
# PROP BASE Intermediate_Dir "Dagor___Win32_Public_Release_MAX_5"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dagor___Win32_Public_Release_MAX_5"
# PROP Intermediate_Dir "Dagor___Win32_Public_Release_MAX_5"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "e:\3dsmax5\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /YX"max.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "d:\3dsmax5\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /D "PUBLIC_RELEASE" /YX"max.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /machine:I386 /out:"e:\3dsmax6\plugins\dagor.dlu"
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /machine:I386 /out:"..\..\..\plugins\dagor.dlu"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Dagor - Win32 Release MAX 6"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dagor___Win32_Release_MAX_6"
# PROP BASE Intermediate_Dir "Dagor___Win32_Release_MAX_6"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dagor___Win32_Release_MAX_6"
# PROP Intermediate_Dir "Dagor___Win32_Release_MAX_6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "e:\3dsmax6\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /YX"max.h" /FD /c
# SUBTRACT BASE CPP /u /Fr
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /YX"max.h" /FD /c
# SUBTRACT CPP /u /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /machine:I386 /out:"D:\3dsmax6\plugins\dagor.dlu"
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /machine:I386 /out:"../../../plugins/dagor.dlu"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Dagor - Win32 Debug MAX 6"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Dagor___Win32_Debug_MAX_6"
# PROP BASE Intermediate_Dir "Dagor___Win32_Debug_MAX_6"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Dagor___Win32_Debug_MAX_6"
# PROP Intermediate_Dir "Dagor___Win32_Debug_MAX_6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "e:\3dsmax6\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /FR /YX"max.h" /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "e:\3dsmax6\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /FR /YX"max.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"D:\3dsmax6\plugins\dagor.dlu" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"e:\3dsmax6\plugins\dagor.dlu" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Dagor - Win32 Public Release MAX 6"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dagor___Win32_Public_Release_MAX_6"
# PROP BASE Intermediate_Dir "Dagor___Win32_Public_Release_MAX_6"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dagor___Win32_Public_Release_MAX_6"
# PROP Intermediate_Dir "Dagor___Win32_Public_Release_MAX_6"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "e:\3dsmax5\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /YX"max.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /D "PUBLIC_RELEASE" /YX"max.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /machine:I386 /out:"e:\3dsmax6\plugins\dagor.dlu"
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /machine:I386 /out:"../../../plugins/dagor.dlu"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Dagor - Win32 Release MAX 7"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dagor___Win32_Release_MAX_7"
# PROP BASE Intermediate_Dir "Dagor___Win32_Release_MAX_7"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dagor___Win32_Release_MAX_7"
# PROP Intermediate_Dir "Dagor___Win32_Release_MAX_7"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "e:\3dsmax7\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /YX"max.h" /FD /c
# SUBTRACT BASE CPP /u /Fr
# ADD CPP /nologo /MD /W3 /GX /O2 /I "c:\3dsmax7\maxsdk\include" /I "." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /YX"max.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /machine:I386 /out:"e:\3dsmax7\plugins\dagor.dlu"
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 core.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib nvDXTlib.lib /nologo /dll /machine:I386 /nodefaultlib:"libc.lib" /out:"c:\dagor\apps\develop\max7plugin\dagor.dlu" /libpath:"c:\3dsmax7\maxsdk\lib"
# SUBTRACT LINK32 /incremental:yes

!ELSEIF  "$(CFG)" == "Dagor - Win32 Debug MAX 7"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Dagor___Win32_Debug_MAX_7"
# PROP BASE Intermediate_Dir "Dagor___Win32_Debug_MAX_7"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Dagor___Win32_Debug_MAX_7"
# PROP Intermediate_Dir "Dagor___Win32_Debug_MAX_7"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "e:\3dsmax7\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /FR /YX"max.h" /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "e:\3dsmax7\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /FR /YX"max.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"e:\3dsmax7\plugins\dagor.dlu" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib nvDXTlib.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"d:\3dsmax7\plugins\dagor.dlu" /pdbtype:sept

!ELSEIF  "$(CFG)" == "Dagor - Win32 Public Release MAX 7"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Dagor___Win32_Public_Release_MAX_7"
# PROP BASE Intermediate_Dir "Dagor___Win32_Public_Release_MAX_7"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dagor___Win32_Public_Release_MAX_7"
# PROP Intermediate_Dir "Dagor___Win32_Public_Release_MAX_7"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "e:\3dsmax7\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /D "PUBLIC_RELEASE" /YX"max.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "DAGOR_EXPORTS" /D "PUBLIC_RELEASE" /YX"max.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /machine:I386 /out:"e:\3dsmax7\plugins\dagor.dlu"
# SUBTRACT BASE LINK32 /incremental:yes
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib winmm.lib /nologo /dll /machine:I386 /out:"..\..\..\plugins\dagor.dlu"
# SUBTRACT LINK32 /incremental:yes

!ENDIF 

# Begin Target

# Name "Dagor - Win32 Release MAX 5"
# Name "Dagor - Win32 Debug MAX 5"
# Name "Dagor - Win32 Public Release MAX 5"
# Name "Dagor - Win32 Release MAX 6"
# Name "Dagor - Win32 Debug MAX 6"
# Name "Dagor - Win32 Public Release MAX 6"
# Name "Dagor - Win32 Release MAX 7"
# Name "Dagor - Win32 Debug MAX 7"
# Name "Dagor - Win32 Public Release MAX 7"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cfg.cpp
# End Source File
# Begin Source File

SOURCE=.\DagImp.cpp
# End Source File
# Begin Source File

SOURCE=.\Dagor.def
# End Source File
# Begin Source File

SOURCE=.\Dagor.rc
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\Dllmain.cpp
# End Source File
# Begin Source File

SOURCE=.\Dummy.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpAnim.cpp
# End Source File
# Begin Source File

SOURCE=.\expanim2.cpp
# End Source File
# Begin Source File

SOURCE=.\ExpUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\Font.cpp
# End Source File
# Begin Source File

SOURCE=.\freecam.cpp
# End Source File
# Begin Source File

SOURCE=.\loadta.cpp
# End Source File
# Begin Source File

SOURCE=.\Ltmap.cpp
# End Source File
# Begin Source File

SOURCE=.\MatConvUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\Mater.cpp
# ADD CPP /Od
# End Source File
# Begin Source File

SOURCE=.\Mater2.cpp
# ADD CPP /Od
# End Source File
# Begin Source File

SOURCE=.\meshtrace.cpp
# End Source File
# Begin Source File

SOURCE=.\ObjectPropertiesEditor.cpp
# End Source File
# Begin Source File

SOURCE=.\objonsrf.cpp
# End Source File
# Begin Source File

SOURCE=.\PhysExport.cpp
# End Source File
# Begin Source File

SOURCE=.\PolyBumpUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\RBDummy.cpp
# End Source File
# Begin Source File

SOURCE=.\ta.cpp
# End Source File
# Begin Source File

SOURCE=.\TexLoad.cpp
# End Source File
# Begin Source File

SOURCE=.\Util.cpp
# End Source File
# Begin Source File

SOURCE=.\Util2.cpp
# End Source File
# Begin Source File

SOURCE=.\vpconv.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Bones.h
# End Source File
# Begin Source File

SOURCE=.\cfg.h
# End Source File
# Begin Source File

SOURCE=.\dagfmt.h
# End Source File
# Begin Source File

SOURCE=.\Dagor.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\enumnode.h
# End Source File
# Begin Source File

SOURCE=.\expanim.h
# End Source File
# Begin Source File

SOURCE=.\Mater.h
# End Source File
# Begin Source File

SOURCE=.\math3d.h
# End Source File
# Begin Source File

SOURCE=.\TexLoad.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Libraries"

# PROP Default_Filter "lib"
# Begin Source File

SOURCE=..\..\lib\maxutil.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\core.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\bmm.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\mesh.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\geom.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\flt.lib
# End Source File
# Begin Source File

SOURCE=..\..\lib\MNMath.lib
# End Source File
# Begin Source File

SOURCE=.\nvDXTlib.lib
# End Source File
# End Group
# End Target
# End Project
