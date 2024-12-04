pushd outerSpace\prog
call build_aot_compiler_arm64.cmd
jam -sNeedDasAotCompile=yes -sPlatformArch=arm64 -sPlatformSpec=vc17
jam -sNeedDasAotCompile=yes -sDedicated=yes -sPlatformArch=arm64 -sPlatformSpec=vc17
jam -f jamfile-decrypt -sPlatformArch=arm64 -sPlatformSpec=vc17
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