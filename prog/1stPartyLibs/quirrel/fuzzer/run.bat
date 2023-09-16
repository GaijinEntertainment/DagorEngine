#@echo off

call :loop tmp_files\opt_on.txt //
call :loop tmp_files\opt_off.txt #disable-optimizer
del tmp_files\test.nut

:loop
del %1
for /L %%a in (0,1,21010) do (
  echo %2 > tmp_files/test.nut
  csq sqfuzzer.nut -- seed=%%a >> tmp_files/test.nut

  echo SEED=%%a >> %1
  csq tmp_files/test.nut >> %1
  echo. >> %1
  echo. >> %1
)
exit /b