@echo off
if "%3" == "" (
  echo mk_def_ordinal.cmd ^<dll^> ^<out_exp^> ^<vc_root^>
  exit 1
)
if not exist "%3/bin/dumpbin.exe" (
  echo missing "%3/bin/dumpbin.exe"
  exit 2
)
"%3/bin/dumpbin.exe" /exports %1 >%1.exp
if not exist %1.exp (
  echo dumpbin^(%1 -^> %1.exp^) failed
  exit 3
)
echo EXPORTS >%2
if not exist "%2" (
  echo failed to create %2
  exit 4
)
for /F "skip=19 tokens=1,4" %%i in (%1.exp) do if not "%%j" == "" echo %%j @%%i NONAME >>%2
