pushd samples\testGI\prog
jam -sPlatformArch=arm64
cd shaders
call compile_shaders_dx12_WOA.bat
call compile_shaders_dx11_WOA.bat
popd
