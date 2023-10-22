del *.h

@for /f "usebackq" %%i in (`dir *.psh /b`) do "%DXSDK_DIR%"\Utilities\bin\x86\fxc.exe /T ps_4_0 /O3 %%i /Qstrip_debug /Qstrip_reflect /Fh %%~ni.h
@for /f "usebackq" %%i in (`dir *.vsh /b`) do "%DXSDK_DIR%"\Utilities\bin\x86\fxc.exe /T vs_4_0 /O3 %%i /Qstrip_debug /Qstrip_reflect /Fh %%~ni.h
