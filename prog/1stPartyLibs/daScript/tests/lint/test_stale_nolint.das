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
        // --lint-excluded-paths keeps this test pinned to the SCANNER regardless of
        // whatever the repo [paths] policy excludes at the time
        run_argv([exe, "-dasroot", root, "{root}/utils/lint/main.das", "--", fixture, "--lint-fixtures", "--lint-excluded-paths"], output)
        t |> success(find(output, "LINT019") >= 0, "LINT019 fires on the fixture: {output}")
        t |> success(find(output, "PERF006 suppresses nothing") >= 0,
            "the stale PERF006 directive is named: {output}")
        t |> success(find(output, "PERF017 suppresses nothing") < 0,
            "the consumed PERF017 directive is NOT reported: {output}")
        t |> success(find(output, "PERF008") < 0,
            "string-literal / URL / mid-comment spellings are not directives: {output}")
        t |> success(find(output, "PERF010 suppresses nothing") >= 0,
            "a `//!` doc-comment directive is still recognized (and stale here): {output}")
    }
    t |> run("widened fixture naming is skipped without --lint-fixtures") @(t : T?) {
        let fixture = "{root}/tests/lint/stale_fixtures/_widened_fixture.das"
        var output = ""
        run_argv([exe, "-dasroot", root, "{root}/utils/lint/main.das", "--", fixture], output)
        t |> success(find(output, "(lint rule fixture)") >= 0, "skip reason printed: {output}")
        t |> success(find(output, "LINT003") < 0, "the deliberate finding is not reported: {output}")
    }
    t |> run("disabling LINT019 silences the scan") @(t : T?) {
        let fixture = "{root}/tests/lint/stale_fixtures/_fixture_stale_nolint.das"
        var output = ""
        run_argv([exe, "-dasroot", root, "{root}/utils/lint/main.das", "--", fixture, "--lint-fixtures", "--lint-excluded-paths", "--disable", "LINT019"], output)
        t |> success(find(output, "LINT019") < 0, "no LINT019 under --disable: {output}")
    }
    t |> run("generic-body directive consumed by a later-sorted consumer (end-scan)") @(t : T?) {
        // serial mode scans staleness AFTER all compiles: the consumer sorts after the
        // generic, so a per-file scan would falsely report both directives stale
        let gen = "{root}/tests/lint/stale_fixtures/_fixture_stale_gen.das"
        let consumer = "{root}/tests/lint/stale_fixtures/_fixture_stale_gen_consumer.das"
        var output = ""
        run_argv([exe, "-dasroot", root, "{root}/utils/lint/main.das", "--", gen, consumer, "--lint-fixtures", "--lint-excluded-paths"], output)
        t |> success(find(output, "LINT019") < 0, "consumed directives are not stale: {output}")
        t |> success(find(output, "PERF006") < 0 && find(output, "PERF022") < 0,
            "the directives suppress their rules: {output}")
    }
    t |> run("lint-skip-file directive skips the whole file") @(t : T?) {
        let fixture = "{root}/tests/lint/stale_fixtures/_fixture_lint_skip.das"
        var output = ""
        run_argv([exe, "-dasroot", root, "{root}/utils/lint/main.das", "--", fixture, "--lint-fixtures", "--lint-excluded-paths"], output)
        t |> success(find(output, "lint-skip-file") >= 0, "skip reason printed: {output}")
        t |> success(find(output, "LINT010") < 0, "the deliberate finding is not reported: {output}")
    }
    t |> run("lint-skip-file outside the header is prose, not a directive") @(t : T?) {
        let fixture = "{root}/tests/lint/stale_fixtures/_fixture_lint_skip_deep.das"
        var output = ""
        run_argv([exe, "-dasroot", root, "{root}/utils/lint/main.das", "--", fixture, "--lint-fixtures", "--lint-excluded-paths"], output)
        t |> success(find(output, "lint-skip-file") < 0, "the deep occurrence is not honored: {output}")
        t |> success(find(output, "PASS") >= 0, "the file lints normally: {output}")
    }
}
