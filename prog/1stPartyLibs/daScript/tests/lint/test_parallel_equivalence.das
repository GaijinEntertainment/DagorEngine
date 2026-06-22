options gen2
options no_unused_function_arguments = false

require dastest/testing_boost public

require strings
require math
require daslib/fio


// argv[0] is the running interpreter; dastest is invoked as
// `daslang(.exe) dastest/dastest.das ...`, so argv[0] points at the
// daslang binary we want to spawn for the lint integration test.
def das_exe() : string {
    let args <- get_command_line_arguments()
    return empty(args) ? "" : args[0]
}


def run_argv(args : array<string>; var output : string&) : int {
    return unsafe(popen_argv(args, 0.0, $(f) {
        if (f != null) {
            output := unsafe(fread_to_eof(f))
        }
    }))
}


// Collect tests/lint/parallel_fixtures/_fixture_*.das. fio::dir filters
// to keep ordering reproducible across filesystems (sorted alphabetical).
// Paths are absolute (via get_das_root()) so they resolve regardless of
// CWD — CI runs dastest from the build directory, not the project root.
def collect_fixtures() : array<string> {
    var out : array<string>
    let dir = "{get_das_root()}/tests/lint/parallel_fixtures"
    fio::dir(dir) $(name) {
        return if (name == "." || name == "..")
        if (name |> ends_with(".das") && name |> starts_with("_fixture_")) {
            out |> push("{dir}/{name}")
        }
    }
    sort(out)
    return <- out
}


def lint_main_path() : string {
    return "{get_das_root()}/utils/lint/main.das"
}


def build_argv(workers : int; fixtures : array<string>; quiet : bool = true) : array<string> {
    var argv <- [das_exe(), "-dasroot", get_das_root(), lint_main_path(), "--", "--workers", "{workers}"]
    if (quiet) {
        argv |> push("-q")
    }
    argv |> reserve(length(argv) + length(fixtures))
    for (f in fixtures) {
        argv |> push(f)
    }
    return <- argv
}


[test]
def test_parallel_matches_serial(t : T?) {
    t |> run("--workers 1 and --workers 4 produce byte-identical output") @(t : T?) {
        let fixtures <- collect_fixtures()
        t |> success(length(fixtures) >= 16, "expected >= 16 fixtures to trigger parallel threshold, got {length(fixtures)}")

        let argv_serial <- build_argv(1, fixtures)
        let argv_parallel <- build_argv(4, fixtures)

        var out_serial : string
        var out_parallel : string
        let rc_serial = run_argv(argv_serial, out_serial)
        let rc_parallel = run_argv(argv_parallel, out_parallel)

        t |> equal(rc_serial, rc_parallel, "exit codes must match")
        t |> equal(out_serial, out_parallel, "stdout must be byte-identical")
    }
}


[test]
def test_below_threshold_stays_serial(t : T?) {
    t |> run("--workers 8 on <16 files runs serially (no temp files leaked)") @(t : T?) {
        let all <- collect_fixtures()
        // Drop down to 4 files — well under PARALLEL_THRESHOLD = 16.
        var small : array<string>
        for (i in range(min(4, length(all)))) {
            small |> push(all[i])
        }
        // quiet=false so the banner is emitted; that's what we assert on.
        let argv <- build_argv(8, small, false)
        var output : string
        let rc = run_argv(argv, output)
        t |> equal(rc, 0, "expected clean PASS for 4 fixtures")
        // Serial banner = "Linting N file(s)...", parallel = "Linting N file(s) across M worker(s)...".
        t |> success(find(output, "Linting") >= 0,
            "expected serial banner, got:\n{output}")
        t |> success(find(output, "across") < 0,
            "expected NO parallel banner (below threshold), got:\n{output}")
    }
}


[test]
def test_workers_one_explicit_is_serial(t : T?) {
    t |> run("--workers 1 explicit always serial regardless of file count") @(t : T?) {
        let fixtures <- collect_fixtures()
        let argv <- build_argv(1, fixtures, false)
        var output : string
        let rc = run_argv(argv, output)
        t |> equal(rc, 0, "expected clean PASS")
        t |> success(find(output, "Linting") >= 0,
            "expected serial banner, got:\n{output}")
        t |> success(find(output, "across") < 0,
            "expected NO parallel banner with --workers 1")
    }
}
