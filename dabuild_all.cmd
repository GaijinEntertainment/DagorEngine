pushd samples\skiesSample\develop
call dabuild.cmd
popd

pushd samples\testGI\develop
call dabuild.cmd
popd

pushd outerSpace\develop
call dabuild.cmd
cd gui
call build_ui.cmd
popd
