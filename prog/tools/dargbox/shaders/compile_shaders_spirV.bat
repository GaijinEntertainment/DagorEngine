@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\dsc2-spirv-dev.exe ./shaders_spirV.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ../../../../_output/shaders/dargbox~spirv %1
