@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
@%DAGOR_CDK_DIR%\dsc2-metal-dev.exe shaders_metal.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -maxVSF 4096 -o ../../../../_output/shaders/dngSceneViewer~metal %*
@if %ERRORLEVEL% NEQ 0 exit /b 1
