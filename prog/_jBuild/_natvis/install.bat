set documents=%USERPROFILE%\Documents

set "setup_dir=%documents%\Visual Studio 2012"
if exist "%setup_dir%" (
  mkdir "%setup_dir%\Visualizers"
  copy dagor_types.natvis "%setup_dir%\Visualizers\"
  copy eastl.natvis "%setup_dir%\Visualizers\"
)

set "setup_dir=%documents%\Visual Studio 2013"
if exist "%setup_dir%" (
  mkdir "%setup_dir%\Visualizers"
  copy dagor_types.natvis "%setup_dir%\Visualizers\"
  copy eastl.natvis "%setup_dir%\Visualizers\"
)

set "setup_dir=%documents%\Visual Studio 2015"
if exist "%setup_dir%" (
  mkdir "%setup_dir%\Visualizers"
  copy dagor_types.natvis "%setup_dir%\Visualizers\"
  copy eastl.natvis "%setup_dir%\Visualizers\"
)

set "setup_dir=%documents%\Visual Studio 2017"
if exist "%setup_dir%" (
  mkdir "%setup_dir%\Visualizers"
  copy dagor_types.natvis "%setup_dir%\Visualizers\"
  copy eastl.natvis "%setup_dir%\Visualizers\"
)

set "setup_dir=%documents%\Visual Studio 2019"
if exist "%setup_dir%" (
  mkdir "%setup_dir%\Visualizers"
  copy dagor_types.natvis "%setup_dir%\Visualizers\"
  copy eastl.natvis "%setup_dir%\Visualizers\"
)

set "setup_dir=%documents%\Visual Studio 2022"
if exist "%setup_dir%" (
  mkdir "%setup_dir%\Visualizers"
  copy dagor_types.natvis "%setup_dir%\Visualizers\"
  copy eastl.natvis "%setup_dir%\Visualizers\"
)
