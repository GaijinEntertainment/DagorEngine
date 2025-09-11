@echo off

rem DaEditorX
jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/daEditorX/jamfile-editor

jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/daEditorX/jamfile

  

rem dabuild
jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/assetExp/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f converters/ddsxCvt2/jamfile


rem AssetViewer
jam -sPlatformArch=arm64  -s Root=../.. -f AssetViewer/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/findUnusedTex/jamfile


rem daImpostorBaker
jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/impostorBaker/jamfile


rem DDSx plugins
jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/assetExp/ddsxConv/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -s iOS_exp=yes -f sceneTools/assetExp/ddsxConv/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -s Tegra_exp=yes -f sceneTools/assetExp/ddsxConv/jamfile


rem shader compilers
jam -sPlatformArch=arm64 -s Root=../.. -f ../3rdPartyLibs/legacy_parser/dolphin/jamfile

jam -sPlatformArch=arm64 -s Root=../.. -f ../3rdPartyLibs/legacy_parser/dolphin/jamfile

jam -sPlatformArch=arm64 -s Root=../.. -f shaderCompiler2/jamfile-hlsl11 -dx  

jam -sPlatformArch=arm64 -s Root=../.. -f shaderCompiler2/jamfile-hlsl2spirv

jam -sPlatformArch=arm64 -s Root=../.. -f shaderCompiler2/jamfile-hlsl2metal

jam -sPlatformArch=arm64 -s Root=../.. -f shaderCompiler2/jamfile-dx12

jam -sPlatformArch=arm64 -s Root=../.. -f shaderCompiler2/jamfile-stub

jam -sPlatformArch=arm64 -s Root=../.. -f shaderCompiler2/hlslCompiler/jamfile

jam -sPlatformArch=arm64 -s Root=../.. -f shaderCompiler2/nodeBased/jamfile

jam -sPlatformArch=arm64 -s Root=../.. -f shaderInfo/jamfile


rem common minimal gui shaders for tools
pushd sceneTools\guiShaders_commonData
call compile_gui_shaders_dx11_WOA.cmd
popd

rem utils
jam -sPlatformArch=arm64 -sPlatformSpec=vc17 -s Root=../.. -f sceneTools/vromfsPacker/jamfile

jam -sPlatformArch=arm64 -sPlatformSpec=vc17 -s Root=../.. -f sceneTools/csvUtil/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f converters/ddsxCvt/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f converters/ddsx2dds/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f converters/ddsConverter/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f FontGenerator/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f converters/GuiTex/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/utils/jamfile-binBlk


jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/dumpGrp/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/dbldUtil/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/resDiff/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/resUpdate/jamfile

jam -sPlatformArch=arm64  -s Root=../.. -f sceneTools/resClean/jamfile


jam -sPlatformArch=arm64  -s Root=../.. -sConfig=dev -sPlatformArch=arm64 -f consoleSq/jamfile


rem jam -sPlatformArch=arm64  -s Root=../.. -f miscUtils/fastdep-0.16/jamfile
rem 

rem GUI tools
jam -sPlatformArch=arm64  -s Root=../.. -f dargbox/jamfile


rem Blender plugin
pushd dag4blend
__build_pack.py FINAL
popd

rem 3ds Max plugins, we don't care if these plugins fail to compile (this could happen due to missing SDK or compiler)
jam -sPlatformArch=arm64  -s Root=../.. -s MaxVer=Max2025 -f maxplug/jamfile
jam -sPlatformArch=arm64  -s Root=../.. -s MaxVer=Max2025 -f maxplug/jamfile-imp

jam -sPlatformArch=arm64  -s Root=../.. -s MaxVer=Max2024 -f maxplug/jamfile
jam -sPlatformArch=arm64  -s Root=../.. -s MaxVer=Max2024 -f maxplug/jamfile-imp

jam -sPlatformArch=arm64  -s Root=../.. -s MaxVer=Max2023 -f maxplug/jamfile
jam -sPlatformArch=arm64  -s Root=../.. -s MaxVer=Max2023 -f maxplug/jamfile-imp

jam -sPlatformArch=arm64  -s Root=../.. -s MaxVer=Max2022 -f maxplug/jamfile
jam -sPlatformArch=arm64  -s Root=../.. -s MaxVer=Max2022 -f maxplug/jamfile-imp
if errorlevel 1 goto EOF

goto EOF

:error

echo.
echo An error occured
pause
exit /b 1

:EOF
exit /b 0
