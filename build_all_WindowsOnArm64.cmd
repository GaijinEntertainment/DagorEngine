pushd prog\tools
call build_dagor_cdk_mini_WindowsOnArm64.cmd
if errorlevel 1 (
  echo build_dagor3_cdk_mini.cmd failed, trying once more...
  call build_dagor_cdk_mini_WindowsOnArm64.cmd
)
if errorlevel 1 (
  echo failed to build CDK, stop!
 pause
  exit /b 1
)
pause
popd

pushd prog\tools\dargbox
call create_vfsroms_WoA64.bat
cd shaders
call compile_shaders_pc11.bat
call compile_shaders_metal.bat
call compile_shaders_spirV.bat
popd

pushd prog\samples\physTest
jam -sPlatformArch=arm64 -sPlatformSpec=vc17
jam -sPlatformArch=arm64 -sPlatformSpec=vc17 -f jamfile-test-jolt
cd shaders
call compile_game_shaders-dx11_WOA.bat
popd

pushd samples\skiesSample\prog
jam -sPlatformArch=arm64 -sPlatformSpec=vc17
cd shaders
call compile_shaders_dx12_WOA.bat
call compile_shaders_dx11_WOA.bat
popd

pushd samples\testGI\prog
jam -sPlatformArch=arm64 -sPlatformSpec=vc17
cd shaders
call compile_shaders_dx12_WOA.bat
call compile_shaders_dx11_WOA.bat
popd
