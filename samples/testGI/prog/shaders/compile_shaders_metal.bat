@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\dsc2-metal-dev.exe .\shaders_metal.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\testGI-game~metal %1 %2 %3
@%DAGOR_CDK_DIR%\dsc2-metal-dev.exe .\shaders_metal.blk -q -shaderOn -enableBindless:on -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\testGI-game~metal~b %1 %2 %3
