@ECHO OFF
%GDEVTOOL%\python3\python3.exe test.py

if %ERRORLEVEL% NEQ 0 echo "FAILED TEST"
if %ERRORLEVEL% NEQ 0 exit /b 1