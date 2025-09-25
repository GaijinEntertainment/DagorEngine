@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\dsc2-spirv-dev.exe ./game.spirv.blk -shaderOn -q  -o ../../../../_output/shaders/physTest-game~spirv %2 %3
