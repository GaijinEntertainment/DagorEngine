@ECHO OFF
rem sphinx-build -b html -d _build/doctrees sphinx-build source _build/html
python3 -m sphinx -c . -d _build/doctrees -b html source _build
if errorlevel 1 exit /b 1
