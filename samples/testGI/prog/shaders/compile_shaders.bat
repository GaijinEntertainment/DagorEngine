@echo on
call compile_shaders_dx11.bat -quiet %1 %2 %3
if %ERRORLEVEL% NEQ 0 exit /b 1
call compile_shaders_tools.bat -quiet %1 %2 %3
if %ERRORLEVEL% NEQ 0 exit /b 1
@echo off
