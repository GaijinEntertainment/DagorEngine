@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\dsc2-metal-dev.exe ./game.metal.blk -shaderOn -q  -o ../../../../_output/shaders/physTest-game~metal %1 %2 %3
