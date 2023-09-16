@echo on
..\..\..\..\tools\dagor3_cdk\util64\dsc2-metal-dev.exe .\shaders_metal_ios.blk -q -metal-glslang -metalios -shaderOn -nodisassembly -commentPP -codeDumpErr -bones_start 70  -o ..\..\..\..\_output\skiesSample\shaders~metalios %1 %2 %3
@echo off

