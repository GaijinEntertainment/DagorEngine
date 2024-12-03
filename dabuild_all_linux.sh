JOBS=$(getconf _NPROCESSORS_ONLN)
ARCH=$(uname -m)
pushd samples/skiesSample/develop
../../../tools/dagor_cdk/linux-$ARCH/daBuild-dev ../application.blk -jobs:$JOBS -q
popd

pushd samples/testGI/develop
../../../tools/dagor_cdk/linux-$ARCH/daBuild-dev ../application.blk -jobs:$JOBS -q
popd

pushd outerSpace/develop
../../tools/dagor_cdk/linux-$ARCH/daBuild-dev ../application.blk -jobs:$JOBS -q
cd gui
../../../tools/dagor_cdk/linux-$ARCH/vromfsPacker-dev input.vromfs.blk -quiet
../../../tools/dagor_cdk/linux-$ARCH/vromfsPacker-dev skin.vromfs.blk -quiet
rm -rf pc.out.ui
mkdir pc.out.ui
pushd fonts
../../../../tools/dagor_cdk/linux-$ARCH/fontgen2-dev fontgenPC.blk -quiet -fullDynamic
popd
../../../tools/dagor_cdk/linux-$ARCH/vromfsPacker-dev fonts.vromfs.blk -quiet
popd
