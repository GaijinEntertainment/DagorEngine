set GDK_VER=250401
set LLVM_VER=18.1.8

call :NORMALIZEPATH "..\..\..\..\..\"
set DAGOR_ROOT=%RETVAL%
set DAGOR_PROG=%DAGOR_ROOT%\prog\

set BUILD_PC_DXC=%DAGOR_ROOT%\tools\dagor_cdk\windows-x86_64\dxc-dx12\pc\dxc.exe /nologo /O3 /Qstrip_debug /Qstrip_reflect
set BUILD_SCARLETT=%GDEVTOOL%\xbox.gdk.%GDK_VER%\%GDK_VER%\GXDK\bin\Scarlett\dxc.exe /nologo /O3 /Qstrip_debug
set BUILD_XBOX_ONE=%GDEVTOOL%\xbox.gdk.%GDK_VER%\%GDK_VER%\GXDK\bin\XboxOne\dxc.exe /nologo /O3 /Qstrip_debug
set FIX_IOG=%DAGOR_PROG%\fix_iog.exe %DAGOR_PROG%\fix_private_hdr.rls .

del *.h

@for /f "usebackq" %%i in (`dir *.ps.hlsl /b`) do %BUILD_PC_DXC% /T ps_6_0 %%i /Fh %%~ni.h
@for /f "usebackq" %%i in (`dir *.vs.hlsl /b`) do %BUILD_PC_DXC% /T vs_6_0 %%i /Fh %%~ni.h
@for /f "usebackq" %%i in (`dir *.cs.hlsl /b`) do %BUILD_PC_DXC% /T cs_6_0 %%i /Fh %%~ni.h

@for /f "usebackq" %%i in (`dir *.ps.hlsl /b`) do %BUILD_XBOX_ONE% /T ps_6_0 %%i /Fh %%~ni.x.h
@for /f "usebackq" %%i in (`dir *.vs.hlsl /b`) do %BUILD_XBOX_ONE% /T vs_6_0 %%i /Fh %%~ni.x.h
@for /f "usebackq" %%i in (`dir *.cs.hlsl /b`) do %BUILD_XBOX_ONE% /T cs_6_0 %%i /Fh %%~ni.x.h

@for /f "usebackq" %%i in (`dir *.ps.hlsl /b`) do %BUILD_SCARLETT% /T ps_6_0 %%i /Fh %%~ni.xs.h
@for /f "usebackq" %%i in (`dir *.vs.hlsl /b`) do %BUILD_SCARLETT% /T vs_6_0 %%i /Fh %%~ni.xs.h
@for /f "usebackq" %%i in (`dir *.cs.hlsl /b`) do %BUILD_SCARLETT% /T cs_6_0 %%i /Fh %%~ni.xs.h

:: generate copyright notice
@for /f "usebackq" %%i in (`dir *.h /b`) do %FIX_IOG% %%i

@for /F "usebackq" %%i in (`dir *.h /b`) do %GDEVTOOL%\LLVM-%LLVM_VER%\bin\clang-format -i -style=file %%i
:: remove trailing whitespaces from generated headers
@for /F "usebackq" %%i in (`dir *.h /b`) do %GDEVTOOL%\util\msysutil\sed.exe -i "s/\s*$//" %%i


:: ========== FUNCTIONS ==========
EXIT /B

:NORMALIZEPATH
  SET RETVAL=%~f1
  EXIT /B