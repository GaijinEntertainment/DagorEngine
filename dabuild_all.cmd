pushd samples\skiesSample\develop
call dabuild.cmd
popd

pushd samples\testGI\develop
call dabuild.cmd
cd gui
call create_fonts.bat
popd
