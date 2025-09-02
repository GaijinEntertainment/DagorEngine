echo off
..\..\tools\dagor_cdk\windows-x86_64\csq-dev.exe ..\..\prog\utils\ecs_sq_utils\test_templates.nut ^
-templatesPath:"gameBase/gamedata/templates/outer_space.entities.blk" ^
-mountPoint:gameLibs=../../prog/gameLibs ^
-mountPoint:danetlibs=../../prog/daNetGameLibs -checkExtraComps -pathToScenes:gameBase ^
-add-basepath:gameBase

if %ERRORLEVEL% NEQ 0 exit /b 1
