@echo off
..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-hlsl11-dev.exe shaders_tools11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -maxVSF 4096 -o ..\..\..\_output\shaders\outerSpace-tools~dx11 %1 %2 %3 %4 %5 %6 %7 %8 %9
if %ERRORLEVEL% NEQ 0 exit /b 1
..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-hlsl11-dev.exe shaders_tools_exp.blk -q -shaderOn -relinkOnly  -o ..\..\..\_output\shaders\outerSpace-tools~dx11 %1 %2 %3 %4 %5 %6 %7 %8 %9
if %ERRORLEVEL% NEQ 0 exit /b 1
