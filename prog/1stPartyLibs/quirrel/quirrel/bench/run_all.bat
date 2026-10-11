@echo off
@rem a quick unpinned pass over the docs suite; benchmarks.py is the measurement
for %%l in (quirrel squirrel lua luajit luau js) do call %~dp0%%l\run.bat
