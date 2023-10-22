@echo on
@..\..\..\..\tools\dagor3_cdk\util64\dsc2-metal-dev.exe ./shaders_metal.blk -q -shaderOn -codeDumpErr -nodisassembly -bones_start 70 -maxVSF 4096 -o _output/MTL %1 %2 %3 %4 %5
if %ERRORLEVEL% NEQ 0 exit /b 1
@echo off
