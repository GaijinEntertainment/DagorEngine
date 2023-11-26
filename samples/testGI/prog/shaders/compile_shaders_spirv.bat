@echo on
..\..\..\..\tools\dagor3_cdk\util64\dsc2-spirv-dev.exe .\shaders_spirv.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -bones_start 70  -o ..\..\..\..\_output\testGI\shaders~spirv %1 %2 %3
@echo off
