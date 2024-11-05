set DAGOR=..\..
set SCRIPTS=%DAGOR%\outerSpace\prog %DAGOR%\prog\commonFx\commonFxGame %DAGOR%\launcher\client\scilauncher2 %DAGOR%\active_matter\prog %DAGOR%\cuisine_royale\prog %DAGOR%\enlisted\prog %DAGOR%\to_the_sky\prog %DAGOR%\modern_conflict\prog %DAGOR%\skyquake\prog\scripts\wt %DAGOR%\skyquake\prog\scripts\wtm %DAGOR%\skyquake\prog\scripts\vrt

setlocal EnableDelayedExpansion

for %%i in (%SCRIPTS%) do (
  echo @@@ validateScripts %%i
  pushd %%i
  python3 validateScripts.py
  if errorlevel 1 echo Failed to validate %%i && exit /b 1
  popd
)
echo Success!
