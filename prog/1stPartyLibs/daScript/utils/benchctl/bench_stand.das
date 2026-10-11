options gen2
options indenting = 4

module bench_stand public

require bench_suite
require bench_runner
require bench_history
require daslib/clargs
require daslib/fio
require daslib/json_boost
require daslib/strings_boost
require strings

[CommandLineArgs]
struct Config {
    @clarg_positional
    @clarg_doc = "run | report"
    verb : string
    @clarg_doc = "Repo root (default: .)"
    root : string
    @clarg_doc = "suite.json to use (default: the one beside this tool)"
    suite : string
    @clarg_doc = "run: daslang binary to measure with (default: bin/daslang under --root)"
    bin : string
    @clarg_doc = "run: JSON with run_id, started, commit, machine, build - written by stand.cmake"
    meta : string
    @clarg_doc = "run: where to write the run record"
    out : string
    @clarg_doc = "run: only files whose path contains this substring"
    filter : string
    @clarg_doc = "run: comma-separated lanes to run (default: suite.json lanes)"
    lanes : string
    @clarg_doc = "run: benchmark nothing - write the record of a night that could not run the suite, with this reason, and exit 1"
    failed : string
    @clarg_doc = "run: repeats per file (default: suite.json repeat)"
    repeat : int
    @clarg_doc = "report: directory of run records"
    runs : string
    @clarg_doc = "report: where to write data.json"
    out_data : string
    @clarg_doc = "report: repository URL for commit links"
    repo_url : string
    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}

def private fail(msg : string) : StandExit {
    to_log(LOG_ERROR, "bench-stand: {msg}\n")
    return StandExit.failed
}

def private write_record(path : string; value) : bool {
    return true if (fwrite(path, sprint_json(value, true)))
    fail("cannot write {path}")
    return false
}

def private measure(var rec : RunRecord&; files : array<BenchFile>; lanes : array<string>; bin, root, filter : string; repeat : int) : int {
    var limits = RunLimits(repeat = repeat)
    var failed = 0
    for (f in files) {
        continue if (!empty(filter) && find(f.path, filter) < 0)
        for (lane in lanes) {
            continue if (rec.lanes[lane] != "ok")
            limits.timeout_seconds = f.timeout_seconds
            var res <- run_bench_file(bin, root, f, lane, limits)
            failed++ if (is_failure(res.status))
            to_log(is_failure(res.status) ? LOG_ERROR : LOG_INFO, "{f.path} {lane}: {res.status} {res.seconds:.1f}s {length(res.samples)} arms{empty(res.message) ? "" : " - " + res.message}\n")
            rec.files |> emplace(res)
        }
    }
    return failed
}

def private verb_run(cfg : Config; suite : SuiteConfig) : StandExit {
    return fail("run: --meta and --out are required") if (empty(cfg.meta) || empty(cfg.out))
    let root = empty(cfg.root) ? "." : cfg.root
    let bin = empty(cfg.bin) ? path_join(root, "bin/daslang") : cfg.bin
    var rec = RunRecord()
    return fail("run: --meta {cfg.meta} does not parse or has no run_id") if (!sscan_json(fread(cfg.meta), rec) || empty(rec.run_id))
    if (!empty(cfg.failed)) {
        mark_failed_run(rec, "{iso8601_now()}", cfg.failed)
        write_record(cfg.out, rec)
        return fail("run: {rec.status} - {cfg.failed}; record {cfg.out}")
    }
    var error = ""
    let files <- discover_files(root, suite, error)
    return fail("run: {error}") if (!empty(error))
    let lanes <- empty(cfg.lanes) ? clone_to_move(suite.lanes) : split(cfg.lanes, ",")
    for (lane in lanes) {
        let why = lane == "jit" ? probe_jit(bin) : ""
        rec.lanes[lane] = empty(why) ? "ok" : why
        to_log(LOG_INFO, "lane {lane}: {rec.lanes[lane]}\n")
    }
    let t0 = ref_time_ticks()
    let failed = measure(rec, files, lanes, bin, root, cfg.filter, cfg.repeat > 0 ? cfg.repeat : suite.repeat)
    rec.seconds = double(get_time_nsec(t0)) / 1000000000.0lf
    rec.finished = "{iso8601_now()}"
    rec.status = derive_status(rec)
    return StandExit.failed if (!write_record(cfg.out, rec))
    to_log(rec.status == RunStatus.ok ? LOG_INFO : LOG_ERROR, "bench-stand run: {rec.status} - {length(rec.files)} file runs, {failed} failed, {rec.seconds:.0f} s; record {cfg.out}\n")
    return rec.status == RunStatus.ok ? StandExit.ok : StandExit.failed
}

def private verb_report(cfg : Config) : StandExit {
    return fail("report: --runs and --out-data are required") if (empty(cfg.runs) || empty(cfg.out_data))
    var errors : array<string>
    let runs <- load_runs(cfg.runs, errors)
    for (e in errors) {
        to_log(LOG_WARNING, "bench-stand report: {e}\n")
    }
    let ds <- build_dataset(runs, "{iso8601_now()}", cfg.repo_url)
    return fail("cannot write {cfg.out_data}") if (!fwrite(cfg.out_data, sprint_json(ds, false)))
    to_log(LOG_INFO, "bench-stand report: {length(runs)} runs, {length(ds.series)} series -> {cfg.out_data}\n")
    return report_verdict(ds)
}

def stand_main() : int {
    var r <- parse_args(type<Config>)
    if (r |> is_err) {
        to_log(LOG_ERROR, "error: {r |> unwrap_err}\n")
        print_help(get_command_info(type<Config>), "bench-stand")
        return 1
    }
    let cfg <- r |> move_unwrap
    if (cfg.help || (cfg.verb != "run" && cfg.verb != "report")) {
        print_help(get_command_info(type<Config>), "bench-stand")
        return int(cfg.help ? StandExit.ok : StandExit.failed)
    }
    var error = ""
    let suite <- load_suite_config(empty(cfg.suite) ? path_join(get_das_root(), "utils/benchctl/suite.json") : cfg.suite, error)
    return int(fail(error)) if (!empty(error))
    return int(cfg.verb == "run" ? verb_run(cfg, suite) : verb_report(cfg))
}
