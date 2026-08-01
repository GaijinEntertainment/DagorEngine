set DAGOR=..\..
set SCRIPTS=%DAGOR%\outerSpace\prog %DAGOR%\prog\commonFx\commonFxGame %DAGOR%\launcher\client\scilauncher2 %DAGOR%\active_matter\prog %DAGOR%\enlisted\prog %DAGOR%\skyquake\prog\scripts\wt %DAGOR%\skyquake\prog\scripts\wtm %DAGOR%\skyquake\prog\scripts\vrt

setlocal EnableDelayedExpansion

for %%i in (%SCRIPTS%) do (
  echo @@@ validateScripts %%i
  pushd %%i
  python validateScripts.py
  if errorlevel 1 echo Failed to validate %%i && exit /b 1
  popd
)
echo Success!
