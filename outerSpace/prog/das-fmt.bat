set CWD=%~dp0
set ROOT=%CWD%..\..\
set OUT=%ROOT%_output
if defined GOUT_ROOT set OUT=%GOUT_ROOT%

pushd ..\..\prog\1stPartyLibs\daScript && "../../../tools/util/das-dev.exe" "das-fmt/dasfmt.das" ^
-module-cache "%OUT%\das-fmt-das-dev.dascache" ^
-- --path %CWD%scripts --path %ROOT%prog\daNetGame\scripts\das ^
--path %ROOT%prog\daNetGame\dasModules --path %ROOT%prog\gameLibs\dasModules --path %ROOT%prog\gameLibs\das --path %ROOT%prog\daNetGameLibs && ^
popd
