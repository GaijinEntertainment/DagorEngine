@echo off
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-metal-dev.exe .\shaders_metal.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\testGI-game~metal %1 %2 %3
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-metal-dev.exe .\shaders_metal.blk -q -shaderOn -enableBindless:on -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\testGI-game~metal~b %1 %2 %3
