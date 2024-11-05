JOBS=$(getconf _NPROCESSORS_ONLN)
pushd samples/skiesSample/develop
../../../tools/dagor_cdk/macOS-x86_64/daBuild-dev ../application.blk -jobs:$JOBS -q
popd

pushd samples/testGI/develop
../../../tools/dagor_cdk/macOS-x86_64/daBuild-dev ../application.blk -jobs:$JOBS -q
popd
