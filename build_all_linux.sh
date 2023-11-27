pushd prog/tools
./build_dagor3_cdk_mini_linux.sh
popd

pushd prog/tools/dargbox
../../../tools/dagor3_cdk/util-linux64/vromfsPacker-dev darg.vromfs.blk -platform:PC
popd

pushd prog/samples/physTest
jam
jam -f jamfile-test-jolt
popd

pushd samples/skiesSample/prog
jam
popd

pushd samples/testGI/prog
jam
popd
