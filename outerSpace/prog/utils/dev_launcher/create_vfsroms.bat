@ECHO OFF

set /P a=...<nul
  ..\..\..\..\tools\dagor_cdk\windows-x86_64\vromfsPacker-dev.exe common.vromfs.blk -platform:PC >log_vrom_pc
  if ERRORLEVEL 1 goto err_pc

  set /P a=...<nul

goto EOF

:err_pc
echo ERROR
type log_vrom_pc | find "[E]"
if [%BUILD_URL%]==[] pause > nul
exit /b 1

:EOF
verify > nul
del log_vrom_pc