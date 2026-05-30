@echo on
..\..\..\tools\dagor_cdk\windows-arm64\dsc2-hlsl11-dev.exe shaders_dx11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -maxVSF 4096 -o ..\..\..\_output\shaders\outerSpace-game~dx11 %1 %2 %3 %4 %5 %6 %7 %8 %9
pause
@echo off
