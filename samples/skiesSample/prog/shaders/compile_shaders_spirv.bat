@echo off
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-spirv-dev.exe .\shaders_spirv.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\skiesSample-game~spirv %1 %2 %3
