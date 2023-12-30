@echo on
..\..\..\..\tools\dagor3_cdk\util64\dsc2-metal-dev.exe .\shaders_metal.blk -metal-glslang -q -shaderOn -nodisassembly -commentPP -codeDumpErr -bones_start 70  -o ..\..\..\..\_output\shaders\testGI-game~metal %1 %2 %3
@echo off
