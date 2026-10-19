@echo off

echo Copyright (C) 2023-present WD Studios Corp. All Rights Reserved.
echo Building Qt tools...

setlocal
pushd "%~dp0"

set "QT_PATH=%QT_DIR%"
if not defined QT_PATH set "QT_PATH=%QtDir%"
set "QT_DIR_ARGS="
if defined QT_PATH (
	set "QT_DIR_ARGS=-s QtDir="%QT_PATH%""
	echo Using Qt from %QT_PATH%
)

rem qtresourcebuilder is commented out, not included in this PR.
rem jam %QT_DIR_ARGS% -s Root=../.. -f qtresourcebuilder/Jamfile
if errorLevel 1 goto error

rem BlkEditor 
jam %QT_DIR_ARGS% -s Root=../.. -f BlkEditor/Jamfile
if errorLevel 1 goto error

rem ToolFramework is commented out, not included in this PR.
rem jam %QT_DIR_ARGS% -s Root=../.. -f ToolFramework/Jamfile
rem if errorLevel 1 goto error

goto end

:error
echo Build failed.
popd
endlocal
exit /b 1

:end
popd
endlocal