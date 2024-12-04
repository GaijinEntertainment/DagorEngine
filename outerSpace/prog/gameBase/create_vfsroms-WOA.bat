@ECHO OFF
set /P a=...<nul

set /P a=...<nul
@ECHO ON
..\..\..\tools\dagor_cdk\windows-arm64\vromfsPacker-dev.exe mk.vromfs.blk %VROMOPT% %1 %2 %3 %4 %5 -quiet -addpath:.
@ECHO OFF
if ERRORLEVEL 1 goto on_error
goto EOF

:on_error
echo [1;31m ERROR [0m
if [%BUILD_URL%]==[] pause > nul
exit /b 1

:EOF
verify > nul
