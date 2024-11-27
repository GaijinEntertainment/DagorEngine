@echo on
..\..\..\..\tools\dagor_cdk\windows-arm64\dsc2-spirv-dev.exe .\shaders_spirv.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\testGI-game~spirv %1 %2 %3
..\..\..\..\tools\dagor_cdk\windows-arm64\dsc2-spirv-dev.exe .\shaders_spirv.blk -q -shaderOn -enableBindless:on -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\testGI-game~spirv~b %1 %2 %3
@echo off
