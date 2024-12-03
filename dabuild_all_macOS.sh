JOBS=$(getconf _NPROCESSORS_ONLN)
pushd samples/skiesSample/develop
../../../tools/dagor_cdk/macOS-x86_64/daBuild-dev ../application.blk -jobs:$JOBS -q
popd

pushd samples/testGI/develop
../../../tools/dagor_cdk/macOS-x86_64/daBuild-dev ../application.blk -jobs:$JOBS -q
popd

pushd outerSpace/develop
../../tools/dagor_cdk/macOS-x86_64/daBuild-dev ../application.blk -jobs:$JOBS -q
cd gui
../../../tools/dagor_cdk/macOS-x86_64/vromfsPacker-dev input.vromfs.blk -quiet
../../../tools/dagor_cdk/macOS-x86_64/vromfsPacker-dev skin.vromfs.blk -quiet
rm -rf pc.out.ui
mkdir pc.out.ui
pushd fonts
../../../../tools/dagor_cdk/macOS-x86_64/fontgen2-dev fontgenPC.blk -quiet -fullDynamic
popd
../../../tools/dagor_cdk/macOS-x86_64/vromfsPacker-dev fonts.vromfs.blk -quiet
popd
