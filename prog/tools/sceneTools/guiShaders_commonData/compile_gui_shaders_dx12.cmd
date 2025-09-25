@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
%DAGOR_CDK_DIR%\dsc2-dx12-dev.exe .\shaders_guiMinimal.blk -out ../../../../tools/dagor_cdk/commonData/guiShadersDX12 -q -shaderOn -nodisassembly -commentPP -codeDumpErr -maxVSF 4096  -o ..\..\..\..\_output\shaders\tools-guiShaders~dx12 %1 %2 %3
