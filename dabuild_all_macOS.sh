JOBS=$(getconf _NPROCESSORS_ONLN)
pushd samples/skiesSample/develop
../../../tools/dagor3_cdk/bin-macosx/daBuild-dev ../application.blk -jobs:$JOBS -q
popd

pushd samples/testGI/develop
../../../tools/dagor3_cdk/bin-macosx/daBuild-dev ../application.blk -jobs:$JOBS -q
popd
