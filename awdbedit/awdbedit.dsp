# Microsoft Developer Studio Project File - Name="awdbedit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=awdbedit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "awdbedit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "awdbedit.mak" CFG="awdbedit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "awdbedit - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "awdbedit - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "awdbedit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "awdbedit - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "awdbedit - Win32 Release"
# Name "awdbedit - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "LZH engine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\lzhEngine\decode.cpp
# End Source File
# Begin Source File

SOURCE=.\lzhEngine\encode.cpp
# End Source File
# Begin Source File

SOURCE=.\lzhEngine\huf.cpp
# End Source File
# Begin Source File

SOURCE=.\lzhEngine\io.cpp
# End Source File
# Begin Source File

SOURCE=.\lzhEngine\lzhEngine.h
# End Source File
# Begin Source File

SOURCE=.\lzhEngine\maketbl.cpp
# End Source File
# Begin Source File

SOURCE=.\lzhEngine\maketree.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\bios.cpp
# End Source File
# Begin Source File

SOURCE=.\config.cpp
# End Source File
# Begin Source File

SOURCE=.\lzh.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\plugin.cpp
# End Source File
# Begin Source File

SOURCE=.\popupDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\popupMenu.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\awdbe_exports.h
# End Source File
# Begin Source File

SOURCE=.\awdbedit_ids.h
# End Source File
# Begin Source File

SOURCE=.\bios.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\lzh.h
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\plugin.h
# End Source File
# Begin Source File

SOURCE=.\popupDialog.h
# End Source File
# Begin Source File

SOURCE=.\popupMenu.h
# End Source File
# Begin Source File

SOURCE=.\types.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\action_extract.ico
# End Source File
# Begin Source File

SOURCE=.\action_extract_all.ico
# End Source File
# Begin Source File

SOURCE=.\action_hexedit.ico
# End Source File
# Begin Source File

SOURCE=.\action_insert.ico
# End Source File
# Begin Source File

SOURCE=.\action_remove.ico
# End Source File
# Begin Source File

SOURCE=.\action_replace.ico
# End Source File
# Begin Source File

SOURCE=.\awdbedit.rc
# End Source File
# Begin Source File

SOURCE=.\background.bmp
# End Source File
# Begin Source File

SOURCE=.\block_blue.ico
# End Source File
# Begin Source File

SOURCE=.\block_cyan.ico
# End Source File
# Begin Source File

SOURCE=.\block_dkblue.ico
# End Source File
# Begin Source File

SOURCE=.\block_dkcyan.ico
# End Source File
# Begin Source File

SOURCE=.\block_dkgreen.ico
# End Source File
# Begin Source File

SOURCE=.\block_dkgrey.ico
# End Source File
# Begin Source File

SOURCE=.\block_dkmagenta.ico
# End Source File
# Begin Source File

SOURCE=.\block_dkred.ico
# End Source File
# Begin Source File

SOURCE=.\block_dkyellow.ico
# End Source File
# Begin Source File

SOURCE=.\block_green.ico
# End Source File
# Begin Source File

SOURCE=.\block_grey.ico
# End Source File
# Begin Source File

SOURCE=.\block_magenta.ico
# End Source File
# Begin Source File

SOURCE=.\block_red.ico
# End Source File
# Begin Source File

SOURCE=.\block_yellow.ico
# End Source File
# Begin Source File

SOURCE=.\chip.ico
# End Source File
# Begin Source File

SOURCE=.\file_exit.ico
# End Source File
# Begin Source File

SOURCE=.\file_open.ico
# End Source File
# Begin Source File

SOURCE=.\file_properties.ico
# End Source File
# Begin Source File

SOURCE=.\file_save.ico
# End Source File
# Begin Source File

SOURCE=.\help_about.ico
# End Source File
# Begin Source File

SOURCE=.\icon2.ico
# End Source File
# Begin Source File

SOURCE=.\icon7.ico
# End Source File
# Begin Source File

SOURCE=.\icon8.ico
# End Source File
# Begin Source File

SOURCE=.\option_config.ico
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Library Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\MSVC6\VC98\Lib\COMCTL32.LIB
# End Source File
# End Group
# End Target
# End Project
