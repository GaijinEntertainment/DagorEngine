@echo off
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-hlsl11-dev.exe .\shaders_tools11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\testGI-tools~dx11 %1 %2 %3
..\..\..\..\tools\dagor_cdk\windows-x86_64\dsc2-hlsl11-dev.exe shaders_tools_exp.blk -q -shaderOn -relinkOnly  -o ..\..\..\..\_output\shaders\testGI-tools~dx11 %1 %2
