# Build PDFs using rinohtype (no LaTeX required)
# Usage: .\make_pdf.ps1

$python = "D:\Work\daScript\.venv\Scripts\python.exe"
$sphinx = "D:\Work\daScript\.venv\Scripts\sphinx-build.exe"
$buildDir = "build"
$pdfDir = "$buildDir\rinoh_pdf"
$siteDocDir = "..\site\doc"

$buildScript = @"
import sys
import threading

def build_main():
    from sphinx.cmd.build import main
    raise SystemExit(main(sys.argv[1:]))

sys.setrecursionlimit(10000)
threading.stack_size(64 * 1024 * 1024)  # 64MB stack
t = threading.Thread(target=build_main)
t.start()
t.join()
"@
$buildScriptFile = "$buildDir\sphinx_build_wrapper.py"
Set-Content -Path $buildScriptFile -Value $buildScript

Write-Host "Building Reference Manual PDF..." -ForegroundColor Cyan
$env:RINOH_BUILD_MODE = 'reference'
& $python $buildScriptFile -b rinoh -d "$buildDir/doctrees_ref" `
    -D "master_doc=reference/index" source "$pdfDir/reference"
$env:RINOH_BUILD_MODE = ''
if ($LASTEXITCODE -ne 0) {
    Write-Error "Reference Manual build failed (exit $LASTEXITCODE)"
    exit 1
}

Write-Host ""
Write-Host "Building Standard Library PDF..." -ForegroundColor Cyan
$env:RINOH_BUILD_MODE = 'stdlib'
& $python $buildScriptFile -b rinoh -d "$buildDir/doctrees_stdlib" `
    -D "master_doc=stdlib/index" source "$pdfDir/stdlib"
$env:RINOH_BUILD_MODE = ''
if ($LASTEXITCODE -ne 0) {
    Write-Error "Standard Library build failed (exit $LASTEXITCODE)"
    exit 1
}

Write-Host ""
Write-Host "Copying PDFs to $siteDocDir ..." -ForegroundColor Cyan
if (-not (Test-Path $siteDocDir)) { New-Item -ItemType Directory -Path $siteDocDir | Out-Null }
Get-ChildItem "$pdfDir\reference", "$pdfDir\stdlib" -Filter "*.pdf" | Copy-Item -Destination $siteDocDir

Write-Host ""
Write-Host "Done!" -ForegroundColor Green
Write-Host "PDFs written to: $(Resolve-Path $pdfDir)"
Write-Host "Copied to site:  $(Resolve-Path $siteDocDir)"
