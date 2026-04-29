pushd samples\dngSceneViewer\prog
jam -sPlatformArch=arm64
pause
call compile_all_prog_vromfs_WOA.cmd
cd shaders
call compile_shaders_dx11_WOA.bat
call compile_shaders_tools_WOA.bat
pause
popd