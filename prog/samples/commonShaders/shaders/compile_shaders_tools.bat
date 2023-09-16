@echo on
..\..\..\..\tools\dagor3_cdk\util64\dsc2-hlsl11-dev.exe .\shaders_tools11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -bones_start 70  -o ..\..\..\..\_output\sampleShaders\shaders~tools11 %1 %2 %3
..\..\..\..\tools\dagor3_cdk\util64\dsc2-hlsl11-dev.exe .\shaders_tools_exp.blk -q -shaderOn -o  ..\..\..\..\_output\sampleShaders\shaders~tools~exp %1 %2

rem copy ..\..\..\..\samples\sampleShaders\tools\common\compiledShaders\tools.ps40.shdump.bin ..\..\..\..\samples\shipSample\tools\common\compiledShaders\tools.ps40.shdump.bin
rem copy ..\..\..\..\samples\sampleShaders\tools\common\compiledShaders\tools.exp.ps40.shdump.bin ..\..\..\..\samples\shipSample\tools\common\compiledShaders\tools.exp.ps40.shdump.bin

rem copy ..\..\..\..\samples\sampleShaders\tools\common\compiledShaders\tools.ps40.shdump.bin ..\..\..\..\samples\planeSample\tools\common\compiledShaders\tools.ps40.shdump.bin
rem copy ..\..\..\..\samples\sampleShaders\tools\common\compiledShaders\tools.exp.ps40.shdump.bin ..\..\..\..\samples\planeSample\tools\common\compiledShaders\tools.exp.ps40.shdump.bin

copy ..\..\..\..\samples\sampleShaders\tools\common\compiledShaders\tools.ps40.shdump.bin ..\..\..\..\samples\humanSample\tools\common\compiledShaders\tools.ps40.shdump.bin
copy ..\..\..\..\samples\sampleShaders\tools\common\compiledShaders\tools.exp.ps40.shdump.bin ..\..\..\..\samples\humanSample\tools\common\compiledShaders\tools.exp.ps40.shdump.bin
@echo off

