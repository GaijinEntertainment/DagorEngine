@echo off
REM Build HTML documentation without running Doxygen

cd /d %~dp0
set SKIPDOXYGEN=1
sphinx-build -c . -b html source _build/html
