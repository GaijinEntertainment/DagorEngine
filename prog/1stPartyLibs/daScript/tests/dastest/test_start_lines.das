options gen2
options no_aot

require dastest/testing_boost
require daslib/defer
require daslib/fio
require daslib/strings_boost

// the start lines are what a hung sweep leaves behind, so they must be printed under
// --failures-only, in both modes, and not depend on the run finishing

def run_dastest_capturing(t : T?; mode_args : array<string>) : string {
    set_env_variable("DAS_LOG_LEVEL", "info")
    defer() {
        set_env_variable("DAS_LOG_LEVEL", "")
    }
    let das_root = get_das_root()
    var args <- [
        get_command_line_arguments()[0],
        "-dasroot", das_root,
        "{das_root}/dastest/dastest.das",
        "--",
        "--failures-only",
        "--test", "{das_root}/tests/dastest/_fixture_pass.das"
    ]
    args |> push_from(mode_args)
    var output : string
    let exit_code = run_and_capture(args, output)
    t |> equal(exit_code, 0, "the child dastest run must succeed; output:\n{output}")
    return output
}

[test]
def test_sequential_sweep_names_each_program(t : T?) {
    let output = run_dastest_capturing(t, [])
    let at = find(output, "run 1/1: ")
    t |> success(at >= 0, "the sequential sweep prints `run N/M: <file>` before the program")
    if (at >= 0) {
        let line = (split(slice(output, at), "\n"))[0]
        t |> success(find(line, "_fixture_pass.das") >= 0, "the start line names the program: {line}")
    }
}

[test]
def test_isolated_sweep_names_each_batch(t : T?) {
    let output = run_dastest_capturing(t, ["--isolated-mode", "--isolated-mode-threads", "1"])
    let at = find(output, "worker 1 starts: ")
    t |> success(at >= 0, "the isolated dispatcher prints `worker K starts: <files>` before the batch")
    if (at >= 0) {
        let line = (split(slice(output, at), "\n"))[0]
        t |> success(find(line, "_fixture_pass.das") >= 0, "the batch line names its files: {line}")
    }
}
