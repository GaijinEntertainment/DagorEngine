options gen2

module command_line shared public

//! Command-line utilities for daslang scripts and standalone executables.
//! Provides functions for locating the daslang interpreter, which is essential
//! when a script or standalone executable needs to spawn daslang subprocesses.

require strings
require fio_core

//! Returns the path to the daslang interpreter executable, derived from
//! ``argv[0]``. When running interpreted (``daslang.exe script.das``),
//! ``argv[0]`` is already the interpreter. When running as a different
//! executable (e.g. ``mcp.exe``, ``test_aot``), ``argv[0]``'s filename is
//! replaced with ``daslang.exe`` (or ``daslang`` on non-Windows) while
//! its directory is preserved — so the correct build configuration
//! (Release, Debug, installed bin/) is used.
//!
//! Returned path follows ``argv[0]``'s shape: absolute if ``argv[0]`` is
//! absolute, relative-to-cwd (e.g. ``./daslang``) if ``argv[0]`` is
//! relative. ``execvp`` / ``popen_argv`` resolve relative paths against
//! the child's cwd (inherited from parent), so a relative result still
//! launches the right binary as long as cwd hasn't changed.
def get_das_exe() : string {
    let args <- get_command_line_arguments()
    if (empty(args)) return ""
    var exe = args[0]
    exe = replace(exe, "\\", "/")
    let last_slash = rfind(exe, "/")
    let dir = last_slash >= 0 ? slice(exe, 0, last_slash + 1) : ""
    let windows = get_platform_name() == "windows"
    let result = windows ? replace("{dir}daslang.exe", "/", "\\") : "{dir}daslang"
    if (fexist(result)) return result
    // dagor's interpreter is das-dev, not daslang: fall back to argv[0] itself, normalized for
    // cmd.exe and with the .exe suffix Windows needs, or the subprocess tests cannot launch it
    let self = windows ? replace(exe, "/", "\\") : exe
    if (fexist(self)) return self
    if (windows && fexist("{self}.exe")) return "{self}.exe"
    return self
}
