@echo off

if "%1"=="" set APK_NAME=dargbox
if not "%1"=="" set APK_NAME=%1

if "%ADBPATH%" == "" set ADBPATH=%GDEVTOOL%\android-sdk/platform-tools/adb.exe

powershell "&'%ADBPATH%' pull \"$( "&'%ADBPATH%'" shell cat sdcard/android/data/com.gaijinent.%APK_NAME%/files/logs/last_debug)\" ./last_log"