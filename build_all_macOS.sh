pushd prog/tools
./build_dagor_cdk_mini_macOS.sh
popd

pushd prog/tools/dargbox
../../../tools/dagor_cdk/macOS-x86_64/vromfsPacker-dev darg.vromfs.blk -platform:PC
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

pushd outerSpace/prog
jam -sRoot=../.. -sProjectLocation=outerSpace/prog -sTarget=outer_space-aot -sOutDir=../tools/das-aot -f../../prog/daNetGame-das-aot/jamfile
jam -sNeedDasAotCompile=yes
jam -sNeedDasAotCompile=yes -sDedicated=yes
jam -f jamfile-decrypt
pushd shaders
./compile_shaders_metal.sh
./compile_tool_shaders_metal.sh
popd
pushd utils/dev_launcher
../../../../tools/dagor_cdk/macOS-x86_64/vromfsPacker-dev common.vromfs.blk -platform:PC -quiet -addpath:.
popd
pushd gameBase
../../../tools/dagor_cdk/macOS-x86_64/vromfsPacker-dev mk.vromfs.blk -quiet -addpath:.
popd
popd
pushd outerSpace/develop/gui
../../../tools/dagor_cdk/macOS-x86_64/vromfsPacker-dev input.vromfs.blk -quiet
../../../tools/dagor_cdk/macOS-x86_64/vromfsPacker-dev skin.vromfs.blk -quiet
rm -rf pc.out.ui
mkdir pc.out.ui
pushd fonts
../../../../tools/dagor_cdk/macOS-x86_64/fontgen2-dev fontgenPC.blk -quiet -fullDynamic
popd
../../../tools/dagor_cdk/macOS-x86_64/vromfsPacker-dev fonts.vromfs.blk -quiet
popd
