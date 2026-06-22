options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/fio
require daslib/strings_boost
require strings

def dastest_root() : string {
    return "{get_das_root()}/dastest"
}

def fixture_dir() : string {
    return "{dastest_root()}/tests/_isolated_fixture"
}

def fixture_dir_pre_dashdash() : string {
    return "{dastest_root()}/tests/_isolated_fixture_pre_dashdash"
}

def fixture_dir_crash() : string {
    return "{dastest_root()}/tests/_isolated_fixture_crash"
}

def fixture_dir_panic() : string {
    return "{dastest_root()}/tests/_isolated_fixture_panic"
}

def run_dastest(cmd : string; var output : string&; var exit_code : int&) {
    output = ""
    unsafe {
        exit_code = popen(cmd) $(f) {
            if (f != null) {
                while (!feof(f)) {
                    output += fgets(f)
                }
            }
        }
    }
}

[test]
def test_isolated_mode(tt : T?) {
    tt |> run("isolated mode spawns 4 workers, fixture suite all passes, --worker-index round-trips") <| @(t : T?) {
        let exe = get_command_line_arguments()[0]
        let cmd = "{exe} {dastest_root()}/dastest.das -- --test {fixture_dir()} --isolated-mode --isolated-mode-threads 4"
        var output : string
        var exit_code : int
        run_dastest(cmd, output, exit_code)
        t |> equal(exit_code, 0)
        t |> success(output |> contains("SUCCESS"), "expected SUCCESS in dastest output, got:\n{output}")
        t |> success(!(output |> contains("subprocess stderr")),
            "a passing isolated run must not emit the crashed-stderr header")
    }

    tt |> run("isolated mode preserves pre-`--` daslang flags (-dasroot) in singleTest cmd") <| @(t : T?) {
        // Pass `-dasroot {existing-dasroot}` — a no-op pre-`--` flag — to the outer
        // dastest. Pre-fix, dastest's singleTest reconstruction used
        // `raw_args[0] raw_args[1]` which dropped everything between exe and
        // dastest.das. The fixture test under fixture_dir_pre_dashdash() asserts
        // -dasroot survives into the spawned subprocess's argv.
        let exe = get_command_line_arguments()[0]
        let cmd = "{exe} -dasroot {get_das_root()} {dastest_root()}/dastest.das -- --test {fixture_dir_pre_dashdash()} --isolated-mode --isolated-mode-threads 1"
        var output : string
        var exit_code : int
        run_dastest(cmd, output, exit_code)
        t |> equal(exit_code, 0)
        t |> success(output |> contains("SUCCESS"), "expected SUCCESS in dastest output, got:\n{output}")
    }

    tt |> run("isolated mode folds a crashed worker's stderr (crash-handler stack) into the failing test's log") <| @(t : T?) {
        // The fixture hard-crashes, so the spawned dastest reports a failure
        // (non-zero exit) — the point is that the crash diagnostic is surfaced
        // under the failing test rather than dropped with the worker's stderr.
        let exe = get_command_line_arguments()[0]
        let cmd = "{exe} {dastest_root()}/dastest.das -- --test {fixture_dir_crash()} --isolated-mode --isolated-mode-threads 1"
        var output : string
        var exit_code : int
        run_dastest(cmd, output, exit_code)
        t |> success(output |> contains("subprocess stderr"),
            "expected the captured-stderr header in output, got:\n{output}")
        t |> success(output |> contains("CRASH"),
            "expected the crash-handler 'CRASH' line folded into the log, got:\n{output}")
    }

    tt |> run("--stack-on-exception folds an at-throw call stack into a panicking worker's log") <| @(t : T?) {
        // WITH the flag: the worker sets the test context's alwaysStackWalkOnException,
        // so the panic walks the stack at throw (frames intact) to the worker's stderr,
        // which the isolated runner folds into the failing test's log. The recursive
        // fixture means the deep_panic call chain shows up in the walk.
        let exe = get_command_line_arguments()[0]
        let cmd = "{exe} {dastest_root()}/dastest.das -- --test {fixture_dir_panic()} --isolated-mode --isolated-mode-threads 1 --stack-on-exception"
        var output : string
        var exit_code : int
        run_dastest(cmd, output, exit_code)
        // Positive proof the fixture actually ran and panicked (so an absent CALL
        // STACK below is meaningful, not a subprocess that never ran).
        t |> success(exit_code != 0, "expected non-zero exit from the panicking fixture, got {exit_code}\n{output}")
        t |> success(output |> contains("intentional panic"),
            "expected the fixture's panic message in output, got:\n{output}")
        t |> success(output |> contains("CALL STACK"),
            "expected the at-throw CALL STACK in output, got:\n{output}")
        t |> success(output |> contains("deep_panic"),
            "expected the panicking call chain (deep_panic) in the walk, got:\n{output}")

        // WITHOUT the flag: the fixture still panics (non-zero exit + message), but
        // no stack walk is emitted.
        let cmd2 = "{exe} {dastest_root()}/dastest.das -- --test {fixture_dir_panic()} --isolated-mode --isolated-mode-threads 1"
        var output2 : string
        var exit_code2 : int
        run_dastest(cmd2, output2, exit_code2)
        t |> success(exit_code2 != 0, "expected non-zero exit from the panicking fixture, got {exit_code2}\n{output2}")
        t |> success(output2 |> contains("intentional panic"),
            "expected the fixture's panic message in output, got:\n{output2}")
        t |> success(!(output2 |> contains("CALL STACK")),
            "without --stack-on-exception there must be no stack walk, got:\n{output2}")
    }
}
