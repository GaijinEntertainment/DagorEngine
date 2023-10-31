@echo off

rem DaEditorX
jam -s Root=../.. -s Platform=win64 -f sceneTools/daEditorX/jamfile
  if errorlevel 1 goto error

rem dabuild
jam -s Root=../.. -s Platform=win64 -f sceneTools/assetExp/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f converters/ddsxCvt2/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -f ../3rdPartyLibs/convert/PVRTexLib/astc-runner/jamfile
  if errorlevel 1 goto error

rem AssetViewer
jam -s Root=../.. -s Platform=win64 -f AssetViewer/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/findUnusedTex/jamfile
  if errorlevel 1 goto error

rem daImpostorBaker
jam -s Root=../.. -f sceneTools/impostorBaker/jamfile
  if errorlevel 1 goto error

rem DDSx plugins
jam -s Root=../.. -s Platform=win64 -f libTools/pcDdsxConv/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -s iOS_exp=yes -f libTools/pcDdsxConv/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -s Tegra_exp=yes -f libTools/pcDdsxConv/jamfile
  if errorlevel 1 goto error

rem shader compilers
jam -s Root=../.. -f ../3rdPartyLibs/legacy_parser/dolphin/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -f ../3rdPartyLibs/legacy_parser/whale/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/jamfile-hlsl11
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/jamfile-hlsl2spirv
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/jamfile-hlsl2metal
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/jamfile-dx12
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/nodeBased/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderInfo/jamfile
  if errorlevel 1 goto error

rem utils
jam -s Root=../.. -s Platform=win64 -f sceneTools/vromfsPacker/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/csvUtil/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f converters/ddsxCvt/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f converters/ddsx2dds/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -f converters/ddsConverter/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f FontGenerator/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f converters/GuiTex/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/utils/jamfile-blk
  if errorlevel 1 goto error

jam -s Root=../.. -f sceneTools/dumpGrp/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/utils/jamfile-blk
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/dbldUtil/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -f sceneTools/resDiff/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -f sceneTools/resUpdate/jamfile
  if errorlevel 1 goto error
jam -s Root=../.. -f sceneTools/resClean/jamfile
  if errorlevel 1 goto error

jam -s Root=../.. -sConfig=dev -sPlatform=win64 -f consoleSq/jamfile
  if errorlevel 1 goto error

jam -s Root=../.. -f miscUtils/fastdep-0.16/jamfile
  if errorlevel 1 goto error

rem GUI tools
jam -s Root=../.. -f dargbox/jamfile
  if errorlevel 1 goto error

rem 3ds Max plugins, we don't care if these plugins fail to compile (this could happen due to missing SDK or compiler)
jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2024 -f maxplug/jamfile
jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2024 -f maxplug/jamfile-imp

jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2023 -f maxplug/jamfile
jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2023 -f maxplug/jamfile-imp

jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2022 -f maxplug/jamfile
jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2022 -f maxplug/jamfile-imp
if errorlevel 1 goto EOF

goto EOF

:error

echo.
echo An error occured
exit /b 1

:EOF
exit /b 0
