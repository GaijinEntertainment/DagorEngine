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
jam -sPlatformArch=arm64 
jam -sPlatformArch=arm64  -f jamfile-test-jolt
cd shaders
call compile_game_shaders-dx11_WOA.bat
popd

pushd samples\skiesSample\prog
jam -sPlatformArch=arm64 
cd shaders
call compile_shaders_dx12_WOA.bat
call compile_shaders_dx11_WOA.bat
popd

pushd samples\testGI\prog
jam -sPlatformArch=arm64
cd shaders
call compile_shaders_dx12_WOA.bat
call compile_shaders_dx11_WOA.bat
popd

pushd outerSpace\prog
call build_aot_compiler_arm64.cmd
jam -sNeedDasAotCompile=yes
jam -sNeedDasAotCompile=yes -sDedicated=yes
jam -f jamfile-decrypt
call compile_all_prog_vromfs_arm64.cmd
cd shaders
call compile_shaders_dx11_WOA.bat
call compile_shaders_tools_WOA.bat
cd ..\..\develop\gui
call build_ui.cmd
popd
pushd outerSpace\prog\utils\dev_launcher
call create_vfsroms.bat
popd
