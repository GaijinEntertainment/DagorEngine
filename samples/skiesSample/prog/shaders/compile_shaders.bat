@echo on
call compile_shaders_pc11.bat %1 %2 %3
if %ERRORLEVEL% NEQ 0 exit /b 1
call compile_shaders_dx12.bat %1 %2 %3
if %ERRORLEVEL% NEQ 0 exit /b 1
call compile_shaders_tools.bat %1 %2 %3
if %ERRORLEVEL% NEQ 0 exit /b 1
@echo off

