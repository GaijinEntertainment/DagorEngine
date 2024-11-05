@echo on
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-metal-dev.exe .\shaders_guiMinimal.blk -out ../../../../tools/dagor_cdk/commonData/guiShadersMTL -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\tools-guiShaders~metal %1 %2 %3
@echo off
