@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\dsc2-spirv-dev.exe .\shaders_spirv.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\testGI-game~spirv %1 %2 %3
@%DAGOR_CDK_DIR%\dsc2-spirv-dev.exe .\shaders_spirv.blk -q -shaderOn -enableBindless:on -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\testGI-game~spirv~b %1 %2 %3
