echo off
..\..\..\..\tools\dagor_cdk\windows-x86_64\csq-dev.exe ..\..\..\..\prog\utils\ecs_sq_utils\test_templates.nut -templatesPath:"content/dng_scene_viewer/gamedata/templates/viewer.entities.blk" -templatesPath:"content/east_district/templates/east_district.pkg.entities.blk" -mountPoint:danetlibs=../../../../prog/daNetGameLibs -checkExtraComps -pathToScenes:content
if %ERRORLEVEL% NEQ 0 exit /b 1
