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
            output = unsafe(fread_to_eof(f))
        }
    }))
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
