@echo off
REM Build PDF for Dagor Maxscript Toolbox only

cd /d %~dp0
set PDF_SUBDOC=maxscript_toolbox
sphinx-build -c . -b latex source\dagor-tools\addons\3ds-max\dagor-maxscript-toolbox _build\pdf_maxscript_toolbox
cd _build\pdf_maxscript_toolbox
latexmk -xelatex dagor_maxscript_toolbox.tex
