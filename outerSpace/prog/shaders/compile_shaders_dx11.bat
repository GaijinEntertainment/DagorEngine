@echo off
del ..\..\game\compiledShaders\game.ps50.shdump.bin
call ..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
%DAGOR_CDK_DIR%\dsc2-hlsl11-dev.exe shaders_dx11.blk -q -shaderOn -nodisassembly -commentPP -codeDumpErr -maxVSF 4096 -o ..\..\..\_output\shaders\%BRANCH%\outerSpace-game~dx11 %*
if %ERRORLEVEL% NEQ 0 exit /b 1
