@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\dsc2-dx12-dev.exe .\shaders_dx12.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\dargbox~dx12 %1 %2 %3
