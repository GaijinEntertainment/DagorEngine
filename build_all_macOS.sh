pushd prog/tools
./build_dagor3_cdk_mini_macOS.sh
popd

pushd prog/tools/dargbox
../../../tools/dagor3_cdk/util-macosx/vromfsPacker-dev darg.vromfs.blk -platform:PC
cd shaders
./compile_shaders_metal.sh
popd

pushd prog/samples/physTest
jam
jam -f jamfile-test-jolt
cd shaders
./compile_shaders_metal.sh
popd

pushd samples/skiesSample/prog
jam
cd shaders
./compile_shaders_metal.sh
./compile_tool_shaders_metal.sh
popd

pushd samples/testGI/prog
jam
cd shaders
./compile_shaders_metal.sh
./compile_tool_shaders_metal.sh
popd
