options gen2
require dastest/testing_boost public

require strings
require daslib/fio

// LINT019 end-to-end: the lint CLI reports the stale directive and stays quiet
// about the consumed one. Mirrors test_parallel_equivalence's spawn pattern.

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
def test_stale_nolint(t : T?) {
    let root = get_das_root()
    let exe = das_exe()
    if (empty(exe)) {
        t |> failure("could not resolve the daslang binary from argv")
        return
    }
    t |> run("stale directive reported, live one consumed") @(t : T?) {
        let fixture = "{root}/tests/lint/stale_fixtures/_fixture_stale_nolint.das"
        var output = ""
        run_argv([exe, "-dasroot", root, "{root}/utils/lint/main.das", "--", fixture, "--lint-fixtures"], output)
        t |> success(find(output, "LINT019") >= 0, "LINT019 fires on the fixture: {output}")
        t |> success(find(output, "PERF006 suppresses nothing") >= 0,
            "the stale PERF006 directive is named: {output}")
        t |> success(find(output, "PERF017 suppresses nothing") < 0,
            "the consumed PERF017 directive is NOT reported: {output}")
    }
    t |> run("disabling LINT019 silences the scan") @(t : T?) {
        let fixture = "{root}/tests/lint/stale_fixtures/_fixture_stale_nolint.das"
        var output = ""
        run_argv([exe, "-dasroot", root, "{root}/utils/lint/main.das", "--", fixture, "--lint-fixtures", "--disable", "LINT019"], output)
        t |> success(find(output, "LINT019") < 0, "no LINT019 under --disable: {output}")
    }
}
