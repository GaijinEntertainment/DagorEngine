@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@for %%f in (extractUtil/decals/*.blk) do %DAGOR_CDK_DIR%\customContentTool-dev cfg_game_decal.blk %%~nf extractUtil/decals/%%f
