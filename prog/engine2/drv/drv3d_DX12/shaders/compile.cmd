set GDK_VER=230303
set LLVM_VER=15.0.7

set BUILD_PC_DXC=%GDEVTOOL%\win.sdk.100\bin\10.0.18362.0\x64\dxc.exe /O3 /Qstrip_debug /Qstrip_reflect
set BUILD_PC_FXC=%GDEVTOOL%\win.sdk.100\bin\10.0.18362.0\x64\fxc.exe /O3 /Qstrip_debug /Qstrip_reflect
set BUILD_SCARLETT=%GDEVTOOL%\xbox.gdk.%GDK_VER%\%GDK_VER%\GXDK\bin\Scarlett\dxc.exe /O3 /Qstrip_debug
set BUILD_XBOX_ONE=%GDEVTOOL%\xbox.gdk.%GDK_VER%\%GDK_VER%\GXDK\bin\XboxOne\dxc.exe /O3 /Qstrip_debug

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

@for /f "usebackq" %%i in (`dir *.ps.hlsl /b`) do %BUILD_PC_FXC% /T ps_5_1 %%i /Fh %%~ni.dxbc.h
@for /f "usebackq" %%i in (`dir *.vs.hlsl /b`) do %BUILD_PC_FXC% /T vs_5_1 %%i /Fh %%~ni.dxbc.h
@for /f "usebackq" %%i in (`dir *.cs.hlsl /b`) do %BUILD_PC_FXC% /T cs_5_1 %%i /Fh %%~ni.dxbc.h

@for /F "usebackq" %%i in (`dir *.h /b`) do %GDEVTOOL%\LLVM-%LLVM_VER%\bin\clang-format -i -style=file %%i
:: remove trailing whitespaces from generated headers
@for /F "usebackq" %%i in (`dir *.h /b`) do %GDEVTOOL%\util\msysutil\sed.exe -i "s/\s*$//" %%i
