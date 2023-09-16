@echo off
echo Press any key to continue or Ctrl-Break to cancel
pause
rar a dplg -agY_MMDD_HHMM -r -xRelease\*.* -xDebug\*.* -x*.rar -x*.aps -x*.ncb -x*.plg -x*.pch* -x*.obj -x*.lib -x*.exp -x*.idb