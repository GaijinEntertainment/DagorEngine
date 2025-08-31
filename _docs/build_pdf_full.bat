@echo off
REM Build full PDF documentation from LaTeX sources

cd /d %~dp0
sphinx-build -c . -b latex source _build\pdf
cd _build\pdf
latexmk -xelatex dagor.tex
