@echo off
setlocal EnableExtensions
rem ---------------------------------------------------------------------------
rem daslang MCP server launcher for Windows + MSVC.
rem
rem Loads the x64 Visual Studio developer environment (INCLUDE / LIB / PATH)
rem BEFORE starting the MCP server, so the cpp_compile_check tool can spawn
rem cl.exe and have it find the system headers. The compile DB
rem (compile_commands.json) omits system include paths on MSVC by design -- the
rem compiler reads them from the environment -- so without this the syntax check
rem fails on <vcruntime.h> etc.
rem
rem Point .mcp.json at this script instead of the bare daslang.exe. The first
rem argument selects the server script (defaults to main.das, the full server):
rem   "daslang":     { "command": "cmd", "args": ["/c", "utils\\mcp\\daslang-mcp-msvc.cmd"] }
rem   "daslang-cpp": { "command": "cmd", "args": ["/c", "utils\\mcp\\daslang-mcp-msvc.cmd", "cpp_main.das"] }
rem
rem Stdio MCP discipline (this whole script's stdin/stdout IS the JSON-RPC pipe):
rem   * NO `for /f` here. A `for /f` that shells out runs a nested cmd that
rem     shares -- and drains -- this script's stdin (the JSON-RPC request bytes)
rem     before daslang reads them. vswhere's output is captured via a temp file
rem     + `set /p` instead, and vswhere is given `<nul` so it can't touch the pipe.
rem   * vcvars64.bat reads stdin too -> the `<nul` on its `call` line (top-level,
rem     NOT inside a parenthesized block, where cmd mis-parses the redirect and a
rem     stray al.exe fires "ALINK error A1017") keeps it off the pipe.
rem   * vcvars64.bat's banner is silenced (>nul 2>&1) -- stray stdout corrupts frames.
rem
rem Linux / macOS do NOT need this: clang/gcc find their system headers
rem automatically, so there point .mcp.json straight at the daslang binary.
rem ---------------------------------------------------------------------------

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto run
set "VSINSTALL="
set "VSTMP=%TEMP%\das_mcp_vsinstall_%RANDOM%.txt"
"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath <nul > "%VSTMP%" 2>nul
set /p VSINSTALL=<"%VSTMP%"
del "%VSTMP%" >nul 2>&1
if not defined VSINSTALL goto run
if not exist "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" goto run
call "%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat" <nul >nul 2>&1

:run
rem Capture the script dir BEFORE any `shift` (plain `shift` also shifts %0, so
rem %~dp0 would otherwise stop pointing at this launcher's folder).
set "MCPDIR=%~dp0"

rem The supervisor resolves the binary (newest of the single-config bin\ and the
rem MSVC bin\Release\ layouts) and passes it in DASLANG_MCP_BIN. Standalone runs
rem fall back to the MSVC Release layout first -- when both layouts exist,
rem first-wins on bin\daslang.exe silently ran a STALE binary whose DAS_BUILD_ID
rem no longer matched the tree's dynamic modules (every native `require` failed).
set "DASLANG=%MCPDIR%..\..\bin\Release\daslang.exe"
if not exist "%DASLANG%" set "DASLANG=%MCPDIR%..\..\bin\daslang.exe"
if defined DASLANG_MCP_BIN set "DASLANG=%DASLANG_MCP_BIN%"

rem First arg selects the server. A *.exe selects a prebuilt server binary in
rem bin/ (the AOT cpp-mcp) run directly; otherwise it's an interpreted .das
rem script handed to daslang.exe. Defaults to main.das (the full server).
rem Any further args are forwarded to the chosen server.
set "MCPSCRIPT=%~1"
if not defined MCPSCRIPT (
    "%DASLANG%" "%MCPDIR%main.das"
    exit /b %ERRORLEVEL%
)
rem Goto (not a parenthesized block) so MCPBIN's set + use are sequential
rem statements -- this script has no EnableDelayedExpansion, so %MCPBIN%
rem read inside an `if ( ... )` block would expand stale (pre-set).
if /i "%MCPSCRIPT:~-4%"==".exe" goto runexe
shift
"%DASLANG%" "%MCPDIR%%MCPSCRIPT%" %1 %2 %3 %4 %5 %6 %7 %8
exit /b %ERRORLEVEL%

:runexe
rem Use the basename only (%~nx1) so a path-y arg can't corrupt the bin\ prefix
rem (e.g. a full path would otherwise yield ...\bin\C:\foo\cpp-mcp.exe).
set "MCPEXE=%~nx1"
set "MCPBIN=%MCPDIR%..\..\bin\%MCPEXE%"
if not exist "%MCPBIN%" set "MCPBIN=%MCPDIR%..\..\bin\Release\%MCPEXE%"
if not exist "%MCPBIN%" (
    echo cpp-mcp launcher: binary "%MCPEXE%" not found under "%MCPDIR%..\..\bin\" 1>&2
    exit /b 1
)
shift
"%MCPBIN%" %1 %2 %3 %4 %5 %6 %7 %8
exit /b %ERRORLEVEL%
