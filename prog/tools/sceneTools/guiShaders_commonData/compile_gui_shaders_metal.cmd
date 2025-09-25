@call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd
%DAGOR_CDK_DIR%\dsc2-metal-dev.exe .\shaders_guiMinimal.blk -out ../../../../tools/dagor_cdk/commonData/guiShadersMTL -q -shaderOn -nodisassembly -commentPP -codeDumpErr  -o ..\..\..\..\_output\shaders\tools-guiShaders~metal %1 %2 %3

