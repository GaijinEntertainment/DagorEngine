@ECHO OFF

set /P a=...<nul
  ..\..\..\tools\dagor_cdk\windows-arm64\vromfsPacker-dev.exe darg.vromfs.blk -platform:PC >log_vrom_darg
  if ERRORLEVEL 1 goto err_pc

  @rem ..\..\..\tools\dagor_cdk\windows-x86_64\vromfsPacker-dev.exe darg.vromfs.blk -platform:iOS >log_vrom_darg_ios
  @rem if ERRORLEVEL 1 goto err_ios

  ..\..\..\tools\dagor_cdk\windows-arm64\vromfsPacker-dev.exe android.vromfs.blk -platform:and >log_vrom_android
  if ERRORLEVEL 1 goto err_and

  set /P a=...<nul

goto EOF

:err_darg_pc
echo ERROR
type log_vrom_darg_pc | find "[E]"
if [%BUILD_URL%]==[] pause > nul
exit /b 1

:err_darg_and
echo ERROR
type log_vrom_darg_and | find "[E]"
if [%BUILD_URL%]==[] pause > nul
exit /b 1

:err_darg_ios
echo ERROR
type log_vrom_darg_ios | find "[E]"
if [%BUILD_URL%]==[] pause > nul
exit /b 1

:err_android
echo ERROR
type log_vrom_android | find "[E]"
if [%BUILD_URL%]==[] pause > nul
exit /b 1

:EOF
verify > nul
