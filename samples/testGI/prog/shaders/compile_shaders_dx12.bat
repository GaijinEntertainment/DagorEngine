@echo off
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-dx12-dev.exe .\shaders_dx12.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -maxVSF 4096 -o ..\..\..\..\_output\shaders\testGI-game~dx12 -wx %1 %2 %3 %4 %5
if %ERRORLEVEL% NEQ 0 exit /b 1
