pushd prog\tools
call build_dagor_cdk_mini.cmd
if errorlevel 1 (
  echo build_dagor3_cdk_mini.cmd failed, trying once more...
  call build_dagor_cdk_mini.cmd
)
if errorlevel 1 (
  echo failed to build CDK, stop!
  exit /b 1
)
pause
popd
