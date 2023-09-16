@echo off
setlocal enabledelayedexpansion
set error=0
for /f %%i in ('call dir /b /s *.txt') do (
  echo %%i
  ..\..\..\..\..\..\..\tools\test\importParserTest\importParser-dev.exe %%i %%i.expected
  if !errorlevel! neq 0 set error=1
)

exit /b %error%
