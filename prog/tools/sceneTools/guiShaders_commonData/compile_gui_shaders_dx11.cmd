@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
%DAGOR_CDK_DIR%\dsc2-hlsl11-dev.exe .\shaders_guiMinimal.blk -out ../../../../tools/dagor_cdk/commonData/guiShaders -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\tools-guiShaders~dx11 %1 %2 %3

