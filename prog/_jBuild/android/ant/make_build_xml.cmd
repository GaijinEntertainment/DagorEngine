@echo off
if "%1" == "" goto err
if "%2" == "" goto err
if "%3" == "" goto err

echo ^<?xml version="1.0" encoding="UTF-8"?^> > %1
echo ^<project name="%3" default="help"^> >> %1
echo   ^<property name="sdk.dir" value="%2"/^> >> %1
echo   ^<property name="target" value="android-%4"/^> >> %1
if NOT "%5" == "" if NOT "%6" == "" if NOT "%7" == "" (
echo   ^<property name="key.store" value="%5"/^> >> %1
echo   ^<property name="key.alias" value="%6"/^> >> %1
echo   ^<property name="key.store.password" value="%7"/^> >> %1
echo   ^<property name="key.alias.password" value="%7"/^> >> %1
)
echo   ^<!-- quick check on sdk.dir --^> >> %1
echo   ^<fail >> %1
echo     message="sdk.dir is missing" >> %1
echo     unless="sdk.dir" >> %1
echo   /^> >> %1
echo   ^<import file="${sdk.dir}/tools/ant/build.xml" /^> >> %1
echo ^</project^> >> %1

exit

:err
echo usage: make_build_xml.cmd ^<build.xml^> ^<android_win_sdk_dir^> ^<libname^>
