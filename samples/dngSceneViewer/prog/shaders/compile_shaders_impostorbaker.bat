@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\dsc2-dx12-dev.exe shaders_impostorbaker.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -localTimestamp -maxVSF 4096 -o ..\..\..\..\_output\shaders\dngSceneViewer-impostorbaker~dx12 %*
@if %ERRORLEVEL% NEQ 0 exit /b 1
