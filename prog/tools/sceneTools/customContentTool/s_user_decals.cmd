@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@for %%f in (userDecals/*.blk) do %DAGOR_CDK_DIR%\customContentTool-dev cfg_user_decal.blk %%~nf "userDecals/%%~f"
