@echo off
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-metal-dev.exe .\shaders_metal_ios.blk -q -metalios -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\skiesSample-game~metal-iOS %1 %2 %3
