JOBS=$(getconf _NPROCESSORS_ONLN)
ARCH=$(uname -m)
pushd samples/skiesSample/develop
../../../tools/dagor_cdk/linux-$ARCH/daBuild-dev ../application.blk -jobs:$JOBS -q
popd

pushd samples/testGI/develop
../../../tools/dagor_cdk/linux-$ARCH/daBuild-dev ../application.blk -jobs:$JOBS -q
popd
