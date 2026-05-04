@set DST_GAME=%1
@if "%DST_GAME%" == "" (
  @echo usage: copy_dabuild_dlls.bat PROJECT_NAME
  @exit 1
)

set SRC_TOOLS=../../../tools/dagor_cdk/windows-x86_64
set DST=../../../%DST_GAME%/game/win64
set copy_pdb=true

@echo off

robocopy %SRC_TOOLS%/plugins/ddsx %DST%/plugins pcDdsxConv.dll
if %copy_pdb%==true (
  robocopy %SRC_TOOLS%/plugins/ddsx %DST%/plugins pcDdsxConv.pdb
)