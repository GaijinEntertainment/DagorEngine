echo off
..\..\..\tools\dagor_cdk\windows-x86_64\csq-dev.exe ..\..\..\prog\utils\ecs_sq_utils\test_templates.nut ^
-templatesPath:"gameBase/gamedata/templates/viewer.entities.blk" ^
-templatesPath:"gamebase_package/content/east_district/templates/east_district.pkg.entities.blk" ^
-mountPoint:danetlibs=../../../prog/daNetGameLibs -checkExtraComps ^
-mountPoint:gameLibs=../../../prog/gameLibs -checkExtraComps ^
-pathToScenes:gameBase/content -pathToScenes:gameBase/gamedata ^
-add-basepath:gameBase -add-basepath:gamebase_package

if %ERRORLEVEL% NEQ 0 exit /b 1
