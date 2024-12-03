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

pushd prog\tools\dargbox
call create_vfsroms.bat
cd shaders
call compile_shaders_pc11.bat
call compile_shaders_metal.bat
call compile_shaders_spirV.bat
popd

pushd prog\samples\physTest
jam
jam -f jamfile-test-jolt
cd shaders
call compile_game_shaders-dx11.bat
call compile_game_shaders-metal.bat
call compile_game_shaders-spirv.bat
popd

pushd samples\skiesSample\prog
jam
cd shaders
call compile_shaders_dx12.bat
call compile_shaders_dx11.bat
call compile_shaders_metal.bat
call compile_shaders_spirv.bat
call compile_shaders_tools.bat
popd

pushd samples\testGI\prog
jam
cd shaders
call compile_shaders_dx12.bat
call compile_shaders_dx11.bat
call compile_shaders_metal.bat
call compile_shaders_spirv.bat
call compile_shaders_tools.bat
popd

pushd outerSpace\prog
call build_aot_compiler.cmd
jam -sNeedDasAotCompile=yes
jam -sNeedDasAotCompile=yes -sDedicated=yes
jam -f jamfile-decrypt
call compile_all_prog_vromfs.cmd
cd shaders
call compile_shaders_dx11.bat
call compile_shaders_tools.bat
cd ..\..\develop\gui
call build_ui.cmd
popd
pushd outerSpace\prog\utils\dev_launcher
call create_vfsroms.bat
popd
