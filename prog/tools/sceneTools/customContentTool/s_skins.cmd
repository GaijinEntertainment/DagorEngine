@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@for %%f in (extractUtil/skins/*.blk) do %DAGOR_CDK_DIR%\customContentTool-dev cfg_game_skin.blk %%~nf extractUtil/skins/%%f
