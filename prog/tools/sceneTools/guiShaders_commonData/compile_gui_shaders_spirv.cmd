@echo on
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-spirv-dev.exe .\shaders_guiMinimal.blk -out ../../../../tools/dagor_cdk/commonData/guiShadersSpirV -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\tools-guiShaders~spirv %1 %2 %3
@echo off
