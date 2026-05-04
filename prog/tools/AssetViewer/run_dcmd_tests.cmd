set TESTFAILED=0

Pushd %~dp0
Pushd "..\..\..\tools\%1CDK"

set OUTPUTFILE=av_scrn_001.jpg
if exist %OUTPUTFILE% goto remove
goto run
:remove
del %OUTPUTFILE%

:run
call setup.cmd
set DAG_CDK_LOC=dagor_cdk\.local
if not exist ..\%DAG_CDK_LOC% mkdir ..\%DAG_CDK_LOC%
copy /y %DAG_CDK_LOC%\workspace.blk ..\%DAG_CDK_LOC%\workspace.blk

echo %date% %time%

start /b /wait ..\dagor_cdk\windows-x86_64\assetViewer2-dev.exe ^
application.blk ^
-quiet ^
-fatals_to_stderr ^
-logerr_to_stderr ^
-ws:%1CDK ^
-async_batch:%~dp0Tests\screenshot.dcmd

if ERRORLEVEL 1 set TESTFAILED=1

echo %date% %time%

if exist %OUTPUTFILE% goto done
echo No such file exist: %OUTPUTFILE%
set TESTFAILED=1
:done

popd
popd

if %TESTFAILED% neq 0 exit /b 1