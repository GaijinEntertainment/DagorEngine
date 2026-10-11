options gen2
options indenting = 4

module bench_runner public

require bench_suite
require benchstat
require daslib/fio
require daslib/json_boost
require daslib/strings_boost
require strings

enum FileStatus {
    ok
    skipped
    spawn_failed
    compile_error
    failed
    timeout
    exit_nonzero
}

// JSON: run record
struct Sample {
    id : string
    ns, ns_median, spread : double
    runs : int
    bytes, allocs, string_bytes, string_allocs : int64
}

// JSON: run record
struct FileResult {
    path, id, group, lane : string
    status : FileStatus
    exit_code : int
    seconds : double
    message, log_tail : string
    samples : array<Sample>
}

struct RunLimits {
    timeout_seconds : int = 900
    repeat : int = 3
}

// JSON: dastest --json-file report
struct ReportTest {
    name : string
    passed : bool
}

// JSON: dastest --json-file report
struct DastestReport {
    success : bool
    tests : array<ReportTest>
}

let TAIL_LINES = 40

let AOT_HOST = "test_aot_bench"
let JIT_HOST = "test_llvm_aot"

struct private OutputScan {
    by_arm : table<string; array<BenchmarkRunStats>>
    arm_order, tail : array<string>
    compile_error, failed_to_compile, fatal : string
    failed_fns : table<string>
}

def private note(var slot : string&; line : string; hit : bool) {
    if (hit && empty(slot)) {
        slot = strip(line)
    }
}

def private scan_line(var scan : OutputScan; line : string) {
    var st = BenchmarkRunStats()
    if (starts_with(line, "\{") && sscan_json(line, st) && st.n > 0) {
        let key = "{st.name}/{st.sub_name}"
        scan.arm_order |> push(key) if (!key_exists(scan.by_arm, key))
        scan.by_arm[key] |> push(st)
        return
    }
    scan.tail |> erase(0) if (length(scan.tail) >= TAIL_LINES)
    scan.tail |> push(length(line) > 400 ? slice(line, 0, 400) : line)
    note(scan.compile_error, line, find(line, "error[") >= 0 && find(line, "]: ") >= 0)
    note(scan.failed_to_compile, line, find(line, "Failed to compile") >= 0)
    note(scan.fatal, line, starts_with(line, "FATAL"))
    let at = find(line, "--- FAIL '")
    return if (at < 0)
    let name_end = find(line, "'", at + 10)
    scan.failed_fns |> insert(slice(line, at + 10, name_end)) if (name_end > at + 10)
}

def private aggregate_arm(key : string; stats : array<BenchmarkRunStats>) : Sample {
    var ns <- [for (st in stats); double(st.time_ns) / double(st.n)]
    var bytes <- [for (st in stats); int64(st.heap_bytes) / int64(st.n)]
    var allocs <- [for (st in stats); int64(st.allocs) / int64(st.n)]
    var sbytes <- [for (st in stats); int64(st.string_heap_bytes) / int64(st.n)]
    var sallocs <- [for (st in stats); int64(st.string_allocs) / int64(st.n)]
    sort(ns)
    let lo = ns[0]
    let hi = ns[length(ns) - 1]
    return Sample(id = key, ns = lo, ns_median = median(ns), spread = lo > 0.0lf ? (hi - lo) / lo : 0.0lf, runs = length(ns),
        bytes = median_i64(bytes), allocs = median_i64(allocs), string_bytes = median_i64(sbytes), string_allocs = median_i64(sallocs))
}

def is_failure(st : FileStatus) : bool {
    return st != FileStatus.ok && st != FileStatus.skipped
}

def classify_run_output(lines : array<string>; report_text : string; exit_code : int; var res : FileResult&) {
    var scan <- OutputScan()
    for (line in lines) {
        scan_line(scan, line)
    }
    var report : DastestReport
    let report_ok = !empty(report_text) && sscan_json(report_text, report)
    for (test in report.tests) {
        scan.failed_fns |> insert(test.name) if (!test.passed)
    }
    res.log_tail = join(scan.tail, "\n")
    res.exit_code = exit_code
    for (key in scan.arm_order) {
        res.samples |> emplace(aggregate_arm(key, scan.by_arm[key])) if (!key_exists(scan.failed_fns, slice(key, 0, find(key, "/"))))
    }
    return if (res.status == FileStatus.timeout)
    if (!empty(scan.failed_to_compile)) {
        res.status = FileStatus.compile_error
        res.message = !empty(scan.compile_error) ? scan.compile_error : scan.failed_to_compile
    } elif (!empty(scan.failed_fns)) {
        var names <- [for (n in keys(scan.failed_fns)); n]
        sort(names)
        res.status = FileStatus.failed
        res.message = "benchmark function(s) failed: {join(names, ", ")}"
    } elif (exit_code != 0 && report_ok && !empty(res.samples)) {
        res.status = FileStatus.ok
        res.message = "measured, then exited with code {exit_code} at shutdown{empty(scan.fatal) ? "" : ": " + scan.fatal}"
    } elif (exit_code != 0 || !report_ok) {
        res.status = FileStatus.exit_nonzero
        res.message = !empty(scan.fatal) ? scan.fatal : exit_code != 0 ? "process exited with code {exit_code} without a finished dastest report" : "dastest wrote no report file - the process ended before the suite finished"
    } elif (empty(res.samples)) {
        res.status = FileStatus.failed
        res.message = "the file ran but produced no benchmark samples"
    } else {
        res.status = FileStatus.ok
    }
}

def run_bench_file(bin, repo_root : string; f : BenchFile; lane : string; limits : RunLimits) : FileResult {
    var res <- FileResult(path = f.path, id = f.id, group = f.group, lane = lane)
    res.message = skip_reason(f, lane)
    if (!empty(res.message)) {
        res.status = FileStatus.skipped
        return <- res
    }
    var err = ""
    let report_path = create_temp_file("bench_stand_", ".json", err)
    if (empty(report_path)) {
        res.status = FileStatus.spawn_failed
        res.message = "cannot create the dastest report file: {err}"
        return <- res
    }
    remove(report_path)
    let host_name = lane == "aot" ? AOT_HOST : (lane == "jit" ? JIT_HOST : "")
    let host = empty(host_name) ? "" : to_generic_path(path_join(path_join(repo_root, "bin"), host_name))
    let compiled = !empty(host) && stat(host).is_reg
    var argv <- [compiled ? host : bin]
    argv |> push("-jit") if (!compiled && lane == "jit")
    argv |> push(to_generic_path(path_join(repo_root, "dastest/dastest.das")))
    argv |> push("--")
    argv |> push("--use-aot") if (compiled || lane == "aot")
    argv |> push_from(["--bench", "--bench-format", "json", "--json-file", report_path,
        "--count", "{limits.repeat}", "--test", to_generic_path(path_join(path_join(repo_root, "benchmarks"), f.path))])
    var lines : array<string>
    var exit_code = process_running
    let t0 = ref_time_ticks()
    let noenv : array<string>
    with_process(argv, repo_root, noenv) $(var p) {
        unsafe {
            if (process_pid(p) <= 0) {
                res.status = FileStatus.spawn_failed
                res.message = "cannot spawn {bin}"
                return
            }
            while (exit_code == process_running) {
                process_drain(p) $(line) {
                    lines |> push(line)
                }
                exit_code = process_poll(p)
                if (exit_code == process_running && double(get_time_nsec(t0)) / 1000000000.0lf > double(limits.timeout_seconds)) {
                    res.status = FileStatus.timeout
                    res.message = "killed after {limits.timeout_seconds} s (timeout_seconds)"
                    process_kill(p)
                    exit_code = process_wait(p, 10.0)
                }
                sleep(100u) if (exit_code == process_running)
            }
            process_drain(p) $(line) {
                lines |> push(line)
            }
        }
    }
    res.seconds = double(get_time_nsec(t0)) / 1000000000.0lf
    return <- res if (res.status == FileStatus.spawn_failed)
    let report_text = fread(report_path)
    remove(report_path)
    classify_run_output(lines, report_text, exit_code, res)
    return <- res
}

def probe_jit(daslang_bin : string) : string {
    var err = ""
    let probe = create_temp_file("bench_stand_jit_", ".das", err)
    return "cannot create the probe file: {err}" if (empty(probe))
    let source = fread(path_join(get_das_root(), "utils/benchctl/_jit_probe.das"))
    return "cannot read utils/benchctl/_jit_probe.das" if (empty(source))
    fwrite(probe, source)
    var out = ""
    let code = run_and_capture([daslang_bin, "-jit", probe], out, 120.0)
    remove(probe)
    return "" if (find(out, "jit-probe-ok") >= 0)
    var said = ""
    for (raw in split(out, "\n")) {
        let line = strip(raw)
        continue if (empty(line))
        said = line
        break if (starts_with(line, "FATAL") || starts_with(line, "error") || find(line, "[E]") >= 0)
    }
    return "daslang -jit produced no jit-compiled run (exit {code}) and printed nothing - is {daslang_bin} runnable?" if (empty(said))
    return "daslang -jit produced no jit-compiled run (exit {code}): {said}"
}
