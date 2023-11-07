@echo on
..\..\..\..\tools\dagor3_cdk\util64\dsc2-hlsl11-dev.exe .\shaders_pc11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -bones_start 70  -o ..\..\..\..\_output\sampleShaders\shaders~pc11 %1 %2 %3
rem copy ..\..\..\..\samples\sampleShaders\game\compiledShaders\game.ps40.shdump.bin ..\..\..\..\samples\planeSample\game\compiledShaders\game.ps40.shdump.bin 
copy ..\..\..\..\samples\sampleShaders\game\compiledShaders\game.ps40.shdump.bin ..\..\..\..\samples\humanSample\game\compiledShaders\game.ps40.shdump.bin 
rem copy ..\..\..\..\samples\sampleShaders\game\compiledShaders\game.ps40.shdump.bin ..\..\..\..\samples\shipSample\game\compiledShaders\game.ps40.shdump.bin 
@echo off
