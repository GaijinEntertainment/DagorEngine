options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost
require daslib/fio
require daslib/command_line
require strings

//! Unit tests for the ``popen_argv_pipe`` C++ builtin (bidirectional
//! subprocess pipe). Validates the round-trip semantics that the MCP
//! subprocess tests depend on, isolated from MCP itself.

// get_das_exe() assumes the interpreter binary is named `daslang`; when the host
// build names it differently (e.g. dagor's das-dev), fall back to argv[0].
def private resolved_das_exe() : string {
    let exe = get_das_exe()
    if (stat(exe).is_valid) {
        return exe
    }
    let args <- get_command_line_arguments()
    return empty(args) ? exe : args[0]
}

def private fixture_path(name : string) : string {
    return path_join(get_das_root(), "tests/mcp/{name}")
}

[test]
def test_echo_roundtrip(t : T?) {
    t |> run("write+read+write+read round-trips through pipe") @(t : T?) {
        let exe = resolved_das_exe()
        let echo = fixture_path("_fixture_echo.das")
        var responses : array<string>
        var rc : int
        unsafe {
            rc = popen_argv_pipe([exe, "-dasroot", get_das_root(), echo]) $(stdin_w, stdout_r) {
                return if (stdin_w == null || stdout_r == null)
                fprint(stdin_w, "hello\n")
                fflush(stdin_w)
                let l1 = fgets(stdout_r)
                responses |> push(clone_string(l1))
                fprint(stdin_w, "world\n")
                fflush(stdin_w)
                let l2 = fgets(stdout_r)
                responses |> push(clone_string(l2))
            }
        }
        t |> equal(rc, 0, "subprocess exit code")
        t |> equal(length(responses), 2)
        return if (length(responses) != 2)
        // fgets keeps the trailing newline; strip CR/LF for cross-platform.
        t |> equal(rtrim(responses[0], "\r\n"), "hello")
        t |> equal(rtrim(responses[1], "\r\n"), "world")
    }
}

[test]
def test_exit_code_propagated(t : T?) {
    t |> run("subprocess exit code returned to caller") @(t : T?) {
        let exe = resolved_das_exe()
        let exit_script = fixture_path("_fixture_exit.das")
        var rc : int
        unsafe {
            rc = popen_argv_pipe([exe, "-dasroot", get_das_root(), exit_script]) $(stdin_w, stdout_r) {
                // No I/O. Block returns immediately; parent closes pipes,
                // waits for the child (which returns 7), captures exit code.
                pass
            }
        }
        t |> equal(rc, 7)
    }
}

[test]
def test_stdin_eof_after_block(t : T?) {
    t |> run("closing stdin (block return) lets the echo fixture exit cleanly") @(t : T?) {
        let exe = resolved_das_exe()
        let echo = fixture_path("_fixture_echo.das")
        // Don't write anything — block returns, stdin closes, echo loop
        // sees feof, exits with 0. Verifies the parent-side close-stdin
        // path doesn't leave the child hanging.
        var rc : int
        unsafe {
            rc = popen_argv_pipe([exe, "-dasroot", get_das_root(), echo]) $(stdin_w, stdout_r) {
                pass
            }
        }
        t |> equal(rc, 0)
    }
}
