@REM This batch file is for validation scripts debugging. It is for use by programmers only.
@REM It validates all WT localizaton files, and writes results into local "result.txt" file.

@REM For better usage exaples see: dagor\skyquake\prog\scripts\tools\localization_tools

@echo off
setlocal
set PARAMS_JSON="{\"root\":\"../../../../skyquake/prog/gameBase/lang_src\",\"scan\":[\"\"],\"exclude\":[\"lang\"],\"verbose\":true}"
echo require("langs_validation.nut").validateLangs(require("json").parse_json(%PARAMS_JSON%)) > _temp.nut
..\..\..\..\tools\dagor_cdk\windows-x86_64\csq-dev.exe _temp.nut | tee result.txt
del /Q _temp.nut
endlocal
