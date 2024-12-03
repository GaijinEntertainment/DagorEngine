set CWD=%~dp0
set ROOT=%CWD%..\..\

pushd ..\..\prog\1stPartyLibs\daScript && "../../../tools/util/das-dev.exe" "das-fmt/dasfmt.das" ^
-- --path %CWD%scripts --path %ROOT%prog\daNetGame\scripts\das ^
--path %ROOT%prog\daNetGame\dasModules --path %ROOT%prog\gameLibs\dasModules --path %ROOT%prog\gameLibs\das --path %ROOT%prog\daNetGameLibs && ^
popd
