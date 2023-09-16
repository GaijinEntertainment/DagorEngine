@echo off

rem DaEditor3x
jam -s Root=../.. -s Platform=win64 -f sceneTools/daEditor3/jamfile depjam
  if errorlevel 1 goto error

rem dabuild
jam -s Root=../.. -s Platform=win64 -f sceneTools/assetExp/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f converters/ddsxCvt2/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f ../3rdPartyLibs/convert/PVRTexLib/astc-runner/jamfile depjam
  if errorlevel 1 goto error

rem AssetViewer
jam -s Root=../.. -s Platform=win64 -f AssetViewer/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/findUnusedTex/jamfile depjam
  if errorlevel 1 goto error

rem daImpostorBaker
jam -s Root=../.. -f sceneTools/impostorBaker/jamfile depjam
  if errorlevel 1 goto error

rem DDSx plugins
jam -s Root=../.. -s Platform=win64 -f libTools/pcDdsxConv/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -s iOS_exp=yes -f libTools/pcDdsxConv/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -s Tegra_exp=yes -f libTools/pcDdsxConv/jamfile depjam
  if errorlevel 1 goto error

rem shader compilers
jam -s Root=../.. -f ../3rdPartyLibs/legacy_parser/dolphin/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f ../3rdPartyLibs/legacy_parser/whale/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/jamfile-hlsl11 depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/jamfile-hlsl2spirv depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/jamfile-hlsl2metal depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/jamfile-dx12 depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderCompiler2/nodeBased/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f shaderInfo/jamfile depjam
  if errorlevel 1 goto error

rem utils
jam -s Root=../.. -s Platform=win64 -f sceneTools/vromfsPacker/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/csvUtil/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f converters/ddsxCvt/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f converters/ddsx2dds/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f converters/ddsConverter/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f FontGenerator/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f converters/GuiTex/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/utils/jamfile-blk depjam
  if errorlevel 1 goto error

jam -s Root=../.. -f sceneTools/dumpGrp/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/utils/jamfile-blk depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -f sceneTools/dbldUtil/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f sceneTools/resDiff/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f sceneTools/resUpdate/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -f sceneTools/resClean/jamfile depjam
  if errorlevel 1 goto error

jam -s Root=../.. -sConfig=dev -sPlatform=win64 -f consoleSq/jamfile depjam
  if errorlevel 1 goto error

jam -s Root=../.. -f miscUtils/fastdep-0.16/jamfile depjam
  if errorlevel 1 goto error

rem GUI tools
jam -s Root=../.. -f dargbox/jamfile depjam
  if errorlevel 1 goto error

rem 3ds Max plugins
jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2024 -f maxplug/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2024 -f maxplug/jamfile-imp depjam
  if errorlevel 1 goto error

jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2023 -f maxplug/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2023 -f maxplug/jamfile-imp depjam
  if errorlevel 1 goto error

jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2022 -f maxplug/jamfile depjam
  if errorlevel 1 goto error
jam -s Root=../.. -s Platform=win64 -s MaxVer=Max2022 -f maxplug/jamfile-imp depjam
  if errorlevel 1 goto error

goto EOF

:error

echo.
echo An error occured
exit /b 1

:EOF
