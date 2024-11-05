ARCH=$(uname -m)
pushd prog/tools
./build_dagor_cdk_mini_linux.sh
popd

pushd prog/tools/dargbox
../../../tools/dagor_cdk/linux-$ARCH/vromfsPacker-dev darg.vromfs.blk -platform:PC
cd shaders
./compile_shaders_spirv.sh
popd

pushd prog/samples/physTest
jam
jam -f jamfile-test-jolt
cd shaders
./compile_shaders_spirv.sh
popd

pushd samples/skiesSample/prog
jam
cd shaders
./compile_shaders_spirv.sh
./compile_tool_shaders_spirv.sh
popd

pushd samples/testGI/prog
jam
cd shaders
./compile_shaders_spirv.sh
./compile_tool_shaders_spirv.sh
popd
