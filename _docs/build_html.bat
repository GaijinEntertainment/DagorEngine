@echo off
REM Build full HTML documentation

cd /d %~dp0
python build_all_docs.py
sphinx-build -c . -b html source _build/html
