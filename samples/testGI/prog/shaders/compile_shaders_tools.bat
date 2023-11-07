@echo on
..\..\..\..\tools\dagor3_cdk\util64\dsc2-hlsl11-dev.exe .\shaders_tools11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -bones_start 70  -o ..\..\..\..\_output\testGI\shaders~tools11 %1 %2 %3
..\..\..\..\tools\dagor3_cdk\util64\dsc2-hlsl11-dev.exe shaders_tools_exp.blk -q -shaderOn -relinkOnly  -o ..\..\..\..\_output\testGI\shaders~tools11 %1 %2
@echo off

