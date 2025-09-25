@ECHO OFF
call ..\..\..\..\prog\_jBuild\make_dagor_tools_path.cmd


set /P a=...<nul
  %DAGOR_CDK_DIR%\vromfsPacker-dev.exe common.vromfs.blk -platform:PC >log_vrom_pc
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