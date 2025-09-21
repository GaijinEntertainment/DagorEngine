@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\dsc2-hlsl11-dev.exe .\shaders_tools11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\skiesSample-tools~dx11 %1 %2 %3
@%DAGOR_CDK_DIR%\dsc2-hlsl11-dev.exe shaders_tools_exp.blk -q -shaderOn -relinkOnly  -o ..\..\..\..\_output\shaders\skiesSample-tools~dx11 %1 %2
