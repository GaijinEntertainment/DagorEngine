options gen2
options no_unused_function_arguments = false

//! Command-line utilities for daslang scripts and standalone executables.
//! Provides functions for locating the daslang interpreter, which is essential
//! when a script or standalone executable needs to spawn daslang subprocesses.

require strings

//! Returns the absolute path to the daslang interpreter executable.
//! When running interpreted (`daslang.exe script.das`), `argv[0]` is already the interpreter.
//! When running as a standalone executable (e.g. `mcp.exe`), `argv[0]` is that executable,
//! so the filename is replaced with `daslang.exe` (or `daslang` on non-Windows).
//! The directory is preserved, so the correct build configuration (Release, Debug, installed path) is used.
def get_das_exe() : string {
    let args <- get_command_line_arguments()
    if (empty(args)) {
        return ""
    }
    var exe = args[0]
    // Normalize to forward slashes
    exe = replace(exe, "\\", "/")
    // If relative path, make absolute using das_root
    if (find(exe, ":") < 0 && first_character(exe) != '/') {
        exe = "{get_das_root()}/{exe}"
    }
    // Strip the filename, keep the directory
    let last_slash = rfind(exe, "/")
    var dir = last_slash >= 0 ? slice(exe, 0, last_slash + 1) : ""
    // Append platform-appropriate interpreter name
    let interpreter = get_platform_name() == "windows" ? "daslang.exe" : "daslang"
    var result = "{dir}{interpreter}"
    // On Windows, replace forward slashes with backslashes for cmd.exe compatibility
    if (get_platform_name() == "windows") {
        result = replace(result, "/", "\\")
    }
    return result
}
