@echo off
..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-stub-dev.exe shaders_tools_exp.blk -q -shaderOn  -no_sha1_cache -o ..\..\..\_output\shaders\outerSpace~exp %1 %2 %3 %4 %5 %6 %7 %8 %9
if %ERRORLEVEL% NEQ 0 exit /b 1
