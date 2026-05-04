@ECHO OFF
REM Build PDFs using rinohtype (no LaTeX required)
REM Usage: make_pdf.bat

if "%SPHINXBUILD%" == "" (
    set SPHINXBUILD=sphinx-build
)
set BUILDDIR=build
set PDFDIR=%BUILDDIR%\rinoh_pdf

echo Building Reference Manual PDF...
"D:\Work\daScript\.venv\Scripts\python.exe" -c ^
    "import sys; sys.setrecursionlimit(10000); ^
     from sphinx.cmd.build import main; ^
     main(['-b','rinoh','-d','%BUILDDIR%/doctrees_ref','-D','master_doc=reference/index','source','%PDFDIR%/reference'])"
if errorlevel 1 (
    echo ERROR: Reference Manual build failed
    exit /b 1
)

echo.
echo Building Standard Library PDF...
"D:\Work\daScript\.venv\Scripts\python.exe" -c ^
    "import sys; sys.setrecursionlimit(10000); ^
     from sphinx.cmd.build import main; ^
     main(['-b','rinoh','-d','%BUILDDIR%/doctrees_stdlib','-D','master_doc=stdlib/index','source','%PDFDIR%/stdlib'])"
if errorlevel 1 (
    echo ERROR: Standard Library build failed
    exit /b 1
)

echo.
echo Copying PDFs to site/doc...
if not exist "..\site\doc" mkdir "..\site\doc"
copy /Y "%PDFDIR%\reference\*.pdf" "..\site\doc\"
copy /Y "%PDFDIR%\stdlib\*.pdf" "..\site\doc\"

echo.
echo Done. PDFs are in %PDFDIR% and ..\site\doc\
