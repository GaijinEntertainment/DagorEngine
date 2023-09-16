@echo on
..\..\..\..\tools\dagor3_cdk\util64\dsc2-spirv-dev.exe .\shaders_spirv.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -bones_start 70  -o ..\..\..\..\_output\skiesSample\shaders~spirv %1 %2 %3
rem ..\..\..\..\tools\dagor3_cdk\util64\dsc2-hlsl11-dev.exe shaders_tools_exp.blk -q -shaderOn -o  ..\..\..\..\_output\testPBR\shaders~tools~exp %1 %2
@echo off

