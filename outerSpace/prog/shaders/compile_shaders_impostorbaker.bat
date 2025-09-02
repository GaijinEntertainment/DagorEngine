@echo off
..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-dx12-dev.exe shaders_impostorbaker.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -localTimestamp -maxVSF 4096 -o ..\..\..\_output\shaders\outerSpace-impostorbaker~dx12 %1 %2 %3 %4 %5 %6 %7 %8 %9
if %ERRORLEVEL% NEQ 0 exit /b 1
