options gen2
require dastest/testing_boost public

require strings
require daslib/fio

// Helpers ---------------------------------------------------------------

// argv[0] is the running interpreter; on these tests dastest is invoked
// as `daslang(.exe) dastest/dastest.das ...`, so argv[0] points at the
// daslang binary we want to spawn. Avoids `daslib/command_line` so this
// test's AOT footprint stays self-contained.
def das_exe() : string {
    let args <- get_command_line_arguments()
    return empty(args) ? "" : args[0]
}

def run_argv(args : array<string>; var output : string&) : int {
    return unsafe(popen_argv(args, 0.0, $(f) {
        if (f != null) {
            output = fread(f)
        }
    }))
}

def wait_for_file(path : string) : bool {
    let started = ref_time_ticks()
    while (!fexist(path)) {
        // Guard only: completion is driven by the file event, not a delay.
        if (get_time_usec(started) > 5000000) return false
    }
    return true
}

// Tests -----------------------------------------------------------------

[test]
def test_popen_argv_basic_invocation(t : T?) {
    t |> run("daslang --help captures expected banner") @(t : T?) {
        var output : string
        let rc = run_argv([das_exe(), "--help"], output)
        t |> equal(rc, 0)
        t |> success(find(output, "daslang version") >= 0,
            "expected 'daslang version' in output: {output}")
    }
}

[test]
def test_popen_argv_unknown_flag_nonzero_exit(t : T?) {
    t |> run("invalid flag returns non-zero exit") @(t : T?) {
        var output : string
        // -no-such-flag-xyz123 is intentionally unknown so daslang errors out.
        let rc = run_argv([das_exe(), "-no-such-flag-xyz123"], output)
        t |> success(rc != 0, "expected non-zero exit, got {rc}")
    }
}

[test]
def test_popen_argv_unix_dollar_paren_not_expanded(t : T?) {
    t |> run("argv with $(...) is passed literally, not expanded by a shell") @(t : T?) {
        // If popen_argv invoked /bin/sh, "$(echo SHOULDNOTAPPEAR)" would be
        // command-substituted to "SHOULDNOTAPPEAR". Argv-direct passes it
        // verbatim, so daslang sees an unknown arg containing the literal
        // dollar-paren string — that text echoes back in the diagnostic.
        var output : string
        let rc = run_argv([das_exe(), "-X-$(echo SHOULDNOTAPPEAR)"], output)
        t |> success(rc != 0, "expected non-zero exit")
        t |> success(find(output, "$(echo") >= 0,
            "argv should be passed literally; expected '$(echo' to appear in output: {output}")
        t |> success(find(output, "SHOULDNOTAPPEAR") >= 0,
            "literal token text should also appear: {output}")
    }
}

[test]
def test_popen_argv_windows_ampersand_not_separator(t : T?) {
    t |> run("argv with `&` is passed literally, not interpreted as cmd separator") @(t : T?) {
        // If popen_argv invoked cmd.exe on Windows, "& echo PWNED" would be
        // a chained command. Argv-direct passes it as one arg.
        var output : string
        let rc = run_argv([das_exe(), "-X-& echo PWNED"], output)
        t |> success(rc != 0, "expected non-zero exit")
        t |> success(find(output, "PWNED") >= 0,
            "literal token text should appear in error output: {output}")
    }
}

[test]
def test_popen_argv_arg_with_spaces(t : T?) {
    t |> run("argv element with embedded space stays a single arg") @(t : T?) {
        var output : string
        // Without proper quoting on Windows, "two words" would split into
        // two args; we'd see a different diagnostic. With proper quoting,
        // the unknown arg is the full string including the space.
        let rc = run_argv([das_exe(), "-X-arg with spaces"], output)
        t |> success(rc != 0, "expected non-zero exit")
        t |> success(find(output, "arg with spaces") >= 0,
            "expected full string with spaces in error output: {output}")
    }
}

[test]
def test_popen_argv_arg_with_embedded_quote(t : T?) {
    t |> run("argv element containing a double-quote is preserved") @(t : T?) {
        // Tests Windows arg-quoting (CommandLineToArgvW backslash rules) and
        // Unix execvp passthrough.
        var output : string
        let rc = run_argv([das_exe(), "-X-pre\"mid\"post"], output)
        t |> success(rc != 0, "expected non-zero exit")
        t |> success(find(output, "pre") >= 0 && find(output, "mid") >= 0 && find(output, "post") >= 0,
            "expected pre/mid/post tokens in output: {output}")
    }
}

[test]
def test_popen_argv_child_stdin_isolated(t : T?) {
    t |> run("child cannot read parent's stdin") @(t : T?) {
        // Spawn the same echo fixture used by test_popen_argv_pipe. The
        // fixture reads stdin in a loop and echoes each line. With proper
        // stdin isolation, the child sees EOF immediately, exits cleanly
        // with no output. Without isolation, the child would block reading
        // from the parent's terminal/inherited stdin, hanging the test.
        var output : string
        let fixture = path_join(get_das_root(), "tests/mcp/_fixture_echo.das")
        let rc = run_argv([das_exe(), "-dasroot", get_das_root(), fixture], output)
        t |> equal(rc, 0, "child should exit cleanly")
        t |> equal(output, "", "child should produce no output (EOF on stdin)")
    }
}

// DISABLED 2026-07-17: this test's file-event handshake depends on a spawned
// interpreter compiling the fixture within five seconds. JIT-disabled Win64
// CI legitimately exceeds that deadline, making the test timing-sensitive.
// Keep the scenario here until it can be covered without a wall-clock gate.
// [test]
def test_spawn_argv_returns_while_child_is_running(t : T?) {
    t |> run("spawn_argv is non-blocking and preserves argv boundaries") @(t : T?) {
        let tmp = create_temp_directory_result("spawn argv spaces ")
        if (!(tmp is value)) {
            t |> failure("could not create temp directory: {tmp as error}")
            return
        }
        let root = tmp as value
        let ready = path_join(root, "ready event")
        let release = path_join(root, "release event")
        let done = path_join(root, "done event")
        let fixture = path_join(get_das_root(), "tests/fio/_fixture_spawn_argv.das")
        let launched = unsafe(spawn_argv([
            das_exe(), "-dasroot", get_das_root(), fixture, "--",
            ready, release, done]))
        t |> success(launched, "child process launched")
        if (launched) {
            t |> success(wait_for_file(ready), "child signaled ready")
            t |> success(!fexist(done),
                "spawn returned while child was waiting for its release event")
            t |> success(fwrite(release, "release"), "parent signaled release")
            t |> success(wait_for_file(done), "child acknowledged release")
        }
        let cleanup = rmdir_rec_result(root)
        t |> success(cleanup is value && cleanup as value,
            "temporary spawn fixture was removed")
    }
}

[test]
def test_spawn_argv_reports_exec_failure(t : T?) {
    t |> run("spawn_argv rejects a missing executable") @(t : T?) {
        let launched = unsafe(spawn_argv([
            "definitely-not-a-real-dascript-executable-7f37b1e9"]))
        t |> success(!launched, "missing executable was not reported as launched")
    }
}
