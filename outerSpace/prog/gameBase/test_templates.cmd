echo off
..\..\..\tools\dagor_cdk\windows-x86_64\csq-dev.exe ..\..\..\prog\utils\ecs_sq_utils\test_templates.nut -templatesPath:"content/outer_space/gamedata/templates/outer_space.entities.blk" -mountPoint:danetlibs=../../../prog/daNetGameLibs -checkExtraComps -pathToScenes:content
if %ERRORLEVEL% NEQ 0 exit /b 1
