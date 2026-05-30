@echo on
..\..\..\..\tools\dagor_cdk\windows-arm64\dsc2-hlsl11-dev.exe .\shaders_guiMinimal.blk -out ../../../../tools/dagor_cdk/commonData/guiShaders -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\tools-guiShaders~dx11 %1 %2 %3
@echo off
