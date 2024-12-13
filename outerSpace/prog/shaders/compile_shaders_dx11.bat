@echo off
del ..\..\game\compiledShaders\game.ps50.shdump.bin
..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-hlsl11-dev.exe shaders_dx11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -maxVSF 4096 -o ..\..\..\_output\shaders\outerSpace-game~dx11 %1 %2 %3 %4 %5 %6 %7 %8 %9
if %ERRORLEVEL% NEQ 0 exit /b 1
