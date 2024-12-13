@echo off
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-hlsl11-dev.exe .\shaders_dx11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\skiesSample-game~dx11 %1 %2 %3
