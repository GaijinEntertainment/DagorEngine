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

[test]
def test_timeout(tt : T?) {
    tt |> run("a suite that runs past --timeout ends with the timeout message and exit code 124, not a crash") <| @(t : T?) {
        let exe = get_command_line_arguments()[0]
        let cmd <- [exe, "-dasroot", get_das_root(), "{dastest_root()}/dastest.das", "--",
            "--test", "{dastest_root()}/tests/_timeout_fixture", "--timeout", "1"]
        var output : string
        let exit_code = run_and_capture(cmd, output, 60.0)
        t |> equal(exit_code, 124, "expected the timeout exit code, got {exit_code}:\n{output}")
        t |> success(output |> contains("ran past its --timeout of 1s"),
            "expected the timeout message, got:\n{output}")
        t |> success(!(output |> contains("should have ended the process")),
            "the fixture's test ran to its end - the timeout did not end the process:\n{output}")
        t |> success(!(output |> contains("CALL STACK")) && !(output |> contains("[daslang atexit]")),
            "the timeout ended the process as a crash or an unbalanced shutdown, not a plain exit:\n{output}")
    }
}
