options gen2
require dastest/testing_boost public

require strings
require daslib/fio

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

[test]
def test_lint010_goto(t : T?) {
    let root = get_das_root()
    let exe = das_exe()
    if (empty(exe)) {
        t |> failure("could not resolve the daslang binary from argv")
        return
    }
    t |> run("stores read through a goto are not dead; a jump-free dead store still is") @(t : T?) {
        let fixture = "{root}/tests/lint/_fixture_lint010_goto.das"
        var output = ""
        run_argv([exe, "-dasroot", root, "{root}/utils/lint/main.das", "--", fixture, "--lint-fixtures", "--lint-excluded-paths"], output)
        t |> success(find(output, "LINT010") >= 0, "the jump-free dead store is reported: {output}")
        t |> success(find(output, "dead store of 't'") >= 0, "the finding names the control's variable: {output}")
        t |> success(find(output, "dead store of 'res'") < 0, "stores before a forward or computed goto are not dead: {output}")
        t |> success(find(output, "dead store of 'acc'") < 0 && find(output, "dead store of 'i'") < 0,
            "an overwrite under a backward-jump label is not dead: {output}")
        t |> success(find(output, "dead store of 'late'") < 0, "a variable declared under a label starts skipped: {output}")
    }
}
