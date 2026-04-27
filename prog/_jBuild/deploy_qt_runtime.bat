
@echo off
echo Copyright (C) 2023-present WD Studios Corp. All Rights Reserved.
setlocal EnableDelayedExpansion

set "SCRIPT_NAME=%~nx0"
set "ROOT_HINT=%~dp0..\.."
for %%I in ("%ROOT_HINT%") do set "ROOT_DIR=%%~fI"

set "QT_DIR=%~1"
if not defined QT_DIR if defined QtDir set "QT_DIR=%QtDir%"
if not defined QT_DIR (
  echo Usage: %SCRIPT_NAME% [QtDir] [TargetExe] [--debug^|--release]
  echo.
  echo   QtDir      - path to the Qt installation containing the bin\ folder.
  echo   TargetExe  - optional path to the built executable. Defaults to the 
  echo                QtWidgetsSample.
  echo   --debug    - force deployment of debug Qt libraries.
  echo   --release  - force deployment of release Qt libraries.
  exit /b 1
)

set "TARGET_EXE=%~2"
if not defined TARGET_EXE (
  for /f "delims=" %%I in ('dir /b /s /a:-d /od "!ROOT_DIR!\_output\QtWidgetsSample*.exe" 2^>nul') do set "TARGET_EXE=%%I"
)
if not defined TARGET_EXE (
  echo %SCRIPT_NAME%: could not locate a QtWidgetsSample executable. Build the target first or pass the path explicitly.
  exit /b 1
)
for %%I in ("%TARGET_EXE%") do (
  set "TARGET_EXE=%%~fI"
  set "TARGET_DIR=%%~dpI"
  set "TARGET_NAME=%%~nI"
)
if not exist "%TARGET_EXE%" (
  echo %SCRIPT_NAME%: target executable not found: %TARGET_EXE%
  exit /b 1
)
if not defined TARGET_DIR (
  echo %SCRIPT_NAME%: failed to resolve target directory for %TARGET_EXE%
  exit /b 1
)

set "QT_BIN=!QT_DIR!\bin"
if not exist "!QT_BIN!\Qt5Core.dll" if not exist "!QT_BIN!\Qt5Cored.dll" (
  echo %SCRIPT_NAME%: Qt bin directory does not look valid: !QT_BIN!
  exit /b 1
)

set "DEPLOY_MODE=--release"
if /I "%~3"=="--debug" set "DEPLOY_MODE=--debug"
if /I "%~3"=="--release" set "DEPLOY_MODE=--release"
if /I not "!DEPLOY_MODE!"=="--debug" (
  echo !TARGET_NAME! | find /I "dbg" >nul
  if not errorlevel 1 set "DEPLOY_MODE=--debug"
)

echo Copying Qt runtime for !TARGET_EXE!
echo   Qt root : !QT_DIR!
echo   Output  : !TARGET_DIR!
echo   Mode    : !DEPLOY_MODE!

rem TODO: ADD LINUX & MAC SUPPORT bash files!

set "DLL_BASE_LIST=Qt5Core Qt5Gui Qt5Widgets"
set "AUX_DLL_LIST=libEGL.dll libGLESV2.dll opengl32sw.dll d3dcompiler_47.dll"
set "PLUGIN_PLATFORM=qwindows"
set "PLUGIN_STYLES=qwindowsvistastyle"

if /I "!DEPLOY_MODE!"=="--debug" (
  set "MODE_SUFFIX=d"
) else (
  set "MODE_SUFFIX="
)

for %%M in (!DLL_BASE_LIST!) do (
  call :CopyFrom "!QT_BIN!" "%%M!MODE_SUFFIX!.dll" "" required
)

for %%F in (!AUX_DLL_LIST!) do (
  call :CopyFrom "!QT_BIN!" "%%F" "" optional
)

call :CopyFrom "!QT_DIR!\plugins\platforms" "!PLUGIN_PLATFORM!!MODE_SUFFIX!.dll" "!TARGET_DIR!platforms" required
call :CopyFrom "!QT_DIR!\plugins\styles" "!PLUGIN_STYLES!!MODE_SUFFIX!.dll" "!TARGET_DIR!styles" optional

echo %SCRIPT_NAME%: deployment completed.
exit /b 0

:CopyFrom
set "SRC_DIR=%~1"
set "FILE_NAME=%~2"
set "DEST_DIR=%~3"
set "COPY_KIND=%~4"

if not defined DEST_DIR set "DEST_DIR=!TARGET_DIR!"
if not exist "!SRC_DIR!\!FILE_NAME!" (
  if /I "!COPY_KIND!"=="required" (
    echo %SCRIPT_NAME%: required file not found: !SRC_DIR!\!FILE_NAME!
    exit /b 2
  ) else (
    echo %SCRIPT_NAME%: warning - optional file not found: !SRC_DIR!\!FILE_NAME!
    goto :eof
  )
)

if not exist "!DEST_DIR!" (
  mkdir "!DEST_DIR!" >nul 2>nul
)
if not "!DEST_DIR:~-1!"=="\" set "DEST_DIR=!DEST_DIR!\"

copy /Y "!SRC_DIR!\!FILE_NAME!" "!DEST_DIR!" >nul
if errorlevel 1 (
  echo %SCRIPT_NAME%: failed to copy !FILE_NAME! to !DEST_DIR!
  exit /b 3
) else (
  echo   copied !FILE_NAME!
)
goto :eof
