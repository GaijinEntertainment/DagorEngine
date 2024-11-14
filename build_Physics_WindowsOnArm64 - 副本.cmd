pushd prog\samples\physTest
jam -sPlatformArch=arm64 -sPlatformSpec=vc17
jam -sPlatformArch=arm64 -sPlatformSpec=vc17 -f jamfile-test-jolt
cd shaders
call compile_game_shaders-dx11_WOA.bat
pause
popd
