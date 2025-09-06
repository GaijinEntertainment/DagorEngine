@echo on
..\..\..\..\tools\dagor_cdk\windows-arm64\dsc2-dx12-dev.exe .\shaders_dx12.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -maxVSF 4096 -o ..\..\..\..\_output\shaders\skiesSample-game~dx12 %1 %2 %3 %4 %5
if %ERRORLEVEL% NEQ 0 exit /b 1
@echo off
