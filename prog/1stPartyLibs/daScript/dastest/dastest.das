options gen2
options indenting = 4
options rtti

require daslib/fio
require strings
require uriparser
require daslib/debugger
require daslib/ast
require daslib/ast_boost
require daslib/json_boost
require daslib/jobque_boost
require daslib/rtti

require fs
require suite
require log
require math
require dastest_clargs

options multiple_contexts

let private jsonResultPrefix = "##dastest##\n"

struct FileTiming {
    uri : string
    dt : int     // microseconds
}

struct JsonTestReport {
    file : string
    total : int
    passed : int
    failed : int
    errors : int
    skipped : int
    success : bool
    time_usec : int
    tests : array<SuiteTestResult>
}

struct FailedFile {
    uri : string
    reason : string
}

var private fileTimings : array<FileTiming>
var private failedFiles : array<FailedFile>
var private timingOutliers : int = 0
var private jsonFilePath : string
var private jsonTestFile : string


def private collect_files_filtered(path, das_test_path : string; var cache : table<string; void?>; var files : array<string>) : bool {
    var success = false
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file(das_test_path, access, unsafe(addr(mg)), cop) $(ok, program, output) {
                if (!ok) {
                    log::error("Failed to compile {das_test_path}\n{output}")
                    return
                }
                simulate(program) $(sok, context, serrors) {
                    if (!sok) {
                        log::error("Failed to simulate {das_test_path}\n{serrors}")
                        return
                    }
                    success = fs::scan_dir(path, cache, files, ".das") $(n) {
                        var result = true
                        unsafe {
                            *context |> invoke_in_context("can_visit_folder", n, addr(result))
                        }
                        return result
                    }
                    if (!success) {
                        log::error("Unable to scan given path '{path}'")
                    }
                }
            }
        }
    }
    return success
}


def private collect_files(input_paths : array<string>; var files : array<string>) : bool {
    var inscope cache : table<string; void?>
    for (path in input_paths) {
        if (path |> ends_with(".das")) {
            if (!cache |> key_exists(path)) {
                cache |> insert(path, null)
                files |> push <| path
            }
        } else {
            let das_test_path = path |> fs::join_path(".das_test")
            let das_test_stat = stat(das_test_path)
            if (das_test_stat.is_reg) {
                if (!collect_files_filtered(path, das_test_path, cache, files)) {
                    return false
                }
            } else {
                if (!fs::scan_dir(path, cache, files, ".das")) {
                    log::error("Unable to scan given path '{path}'")
                    return false
                }
            }
        }
    }
    return true
}


def private get_float_arg(args : array<string>; name : string; def_val : float) : float {
    let idx = find_index(args, name)
    return idx >= 0 && idx + 1 < length(args) ? float(args[idx + 1]) : def_val
}

def private get_int_arg(args : array<string>; name : string; def_val : int) : int {
    let idx = find_index(args, name)
    return idx >= 0 && idx + 1 < length(args) ? int(args[idx + 1]) : def_val
}

struct IsoInput {
    uri : string
    cmd : string
}

struct IsolatedResult {
    uri : string
    cmd : string
    exitCode : int
    parsedResult : bool
    status : SuiteResult
    tests : array<SuiteTestResult>
    fileDt : int
    err : array<string>
    log : array<string>
}

def serialize_path(ctx : SuiteCtx, files : array<string>, out_file : string) {
    var res : SuiteResult
    fopen(out_file, "wb") $(f) {
        if (f == null) {
            log::error("Failed to open {out_file} for writing")
            res.errors++
            return
        }
        var ser = create_ast_serializer()
        var inscope access <- make_file_access(ctx.projectPath)
        access |> add_file_access_root("dastest", ctx.dastestRoot)
        for (file in files) {
            using() $(var mg : ModuleGroup) {
                using() $(var cop : CodeOfPolicies) {
                    cop.aot_module = true
                    cop.aot = ctx.use_aot
                    cop.fail_on_no_aot = ctx.use_aot
                    cop.threadlock_context = true
                    cop.jit_enabled = jit_enabled()
                    compile_file(file, access, unsafe(addr(mg)), cop) $(ok, program, output) {
                        if (!ok) {
                            log::error("Failed to compile {file}\n{output}")
                            res.errors++
                            return
                        }
                        if (serialize_program(ser, program)) {
                            res.total++
                        } else {
                            log::error("Failed to serialize {file}")
                            res.errors++
                        }
                    }
                }
            }
        }
        fwrite(f, res.total)
        ast_serializer_get_data(ser) $(data) {
            var sz = uint64(length(data))
            fwrite(f, sz)
            fwrite(f, data)
        }
        delete_ast_serializer(ser)
    }
    log::info("Serialized {res.total:d} programs to {out_file}")
    if (res.errors > 0) {
        log::error("{res.errors} error(s)")
    }
    return res
}

def deserialize_path(var ctx : SuiteCtx, files : array<string>, in_file : string) : SuiteResult {
    var res : SuiteResult
    fopen(in_file, "rb") $(f) {
        if (f == null) {
            log::error("Failed to open {in_file}")
            res.errors++
            res.total++
            return
        }
        var count = 0
        unsafe {
            _builtin_read(f, addr(count), typeinfo sizeof(count))
        }
        var sz = 0 |> uint64()
        unsafe {
            _builtin_read(f, addr(sz), typeinfo sizeof(sz))
        }
        var data : array<uint8>
        data |> resize(int(sz))
        fread(f, data)
        log::info("Deserializing {count:d} programs from {in_file}")
        var ser = create_ast_deserializer(data)
        using() $(var mg : ModuleGroup) {
            for (i in range(int(count))) {
                deserialize_program(ser) <| $(ok, program, error) {
                    if (!ok) {
                        log::error("Deserialization failed at program {i}: {error}")
                        res.errors++
                        res.total++
                        return
                    }
                    if (program._options |> find_arg("rtti") ?as tBool ?? false) {
                        return
                    }
                    simulate(program) $(sok, context, serrors) {
                        if (!sok) {
                            log::error("Failed to simulate program {i}\n{serrors}")
                            res.errors++
                            res.total++
                            return
                        }
                        var mod = program |> get_this_module()
                        if (mod != null && context != null) {
                            res += suite::test_module(*mod, *context, ctx)
                        }
                    }
                }
            }
        }
        delete_ast_serializer(ser)
    }
    return res
}

[export]
def main() : int {
    // ansi-colors detection scans full argv for --color / --no-color so the
    // user can place either flag before or after the "--" separator.
    log::init_log()

    // Captured for isolated-mode subprocess invocation; raw_args[0] is the
    // daslang interpreter path and raw_args[1] is dastest.das.  Everything
    // else for this main() comes from the parsed test_args below.
    var raw_args <- get_command_line_arguments()

    var test_args = DastestArgs()
    let parse_err = parse_args(test_args)
    if (parse_err != "") {
        log::red("command-line args: {parse_err}")
        return 1
    }

    if (!empty(test_args.run)) {
        // Isolated-mode single-file: --run=<file> covers both `--run X` and
        // `--run=X` forms because parse_args has already populated the field.
        let res = suite::test_file(test_args.run, SuiteCtx(test_args))
        // Send full report with per-test details for isolated mode JSON collection
        var report = JsonTestReport(
            file = test_args.run,
            total = res.total,
            passed = res.passed,
            failed = res.failed,
            errors = res.errors,
            skipped = res.skipped,
            success = res.failed + res.errors == 0,
            time_usec = 0,
            tests <- suite::test_results
        )
        log::info_raw("\n{jsonResultPrefix}{sprint_json(report, false)}\n{jsonResultPrefix}")
        return res.failed + res.errors
    }

    log::failuresOnly = test_args.failures_only

    var res : SuiteResult
    let startTime = ref_time_ticks()

    let timeout = test_args.timeout
    if (timeout > 0.) {
        unsafe {
            var mainCtx & = this_context()
            new_thread <| @capture(& res, & mainCtx) {
                // Polled sleep: chunk into 100ms slices and watch for jobque
                // shutdown so a clean main-return does not have to wait for
                // the full timeout. ~Module_JobQue busy-waits on the thread
                // count, so without this the shutdown path hangs.
                let total_ms = uint(timeout * 1000.)
                var slept = 0u
                while (slept < total_ms && !is_job_que_shutting_down()) {
                    let chunk = min(100u, total_ms - slept)
                    fio::sleep(chunk)
                    slept += chunk
                }
                if (slept >= total_ms) {
                    unsafe {
                        mainCtx |> invoke_in_context("timeout_tests", res, startTime, timeout)
                    }
                }
            }
        }
    }

    var inputPaths := test_args.test_files
    var files : array<string>
    if (!collect_files(inputPaths, files)) {
        return 1
    }

    timingOutliers = test_args.timing_outliers
    jsonFilePath = test_args.json_file
    if (!empty(jsonFilePath)) {
        suite::clear_test_results()
        if (length(files) == 1) {
            jsonTestFile = files[0]
        } else {
            jsonTestFile = build_string() $(var w) {
                for (p, pi in inputPaths, range(length(inputPaths))) {
                    if (pi > 0) {
                        write(w, ",")
                    }
                    write(w, p)
                }
            }
        }
    }

    let isolatedMode = test_args.isolated_mode
    var ctx <- SuiteCtx(test_args)
    if (!ctx.cov_path |> empty()) {
        remove(ctx.cov_path)
    }

    let ser_file = test_args.ser
    let deser_file = test_args.deser
    if (!empty(ser_file)) {
        res = serialize_path(ctx, files, ser_file)
    } elif (!empty(deser_file)) {
        res = deserialize_path(ctx, files, deser_file)
    } elif (!isolatedMode) {
        for (file in files) {
            let uri = ctx.uriPaths ? file_name_to_uri(file) : file
            let fileTime = ref_time_ticks()
            let status = suite::test_file(file, ctx)
            let fileDt = get_time_usec(fileTime)
            if (status.errors + status.failed == 0) {
                if (status.total > 0) {
                    log::green("PASS {uri} {time_dt_hr(fileDt)}")
                }
            } else {
                log::red("FAIL {uri} {time_dt_hr(fileDt)}")
                var reason = "{status.failed} failed, {status.errors} errors"
                failedFiles |> push(FailedFile(uri = clone_string(uri), reason = clone_string(reason)))
            }
            if (timingOutliers > 0) {
                fileTimings |> push(FileTiming(uri = clone_string(uri), dt = fileDt))
            }
            res += status
        }
    } else {
        let totalFiles = length(files)
        var totalThreads = test_args.isolated_mode_threads
        if (totalThreads == 0) {
            totalThreads = get_total_hw_threads() * 2
        }
        let testTimeout = test_args.test_timeout
        log::info("Running {totalFiles} tests in isolated mode with {totalThreads} threads\n")
        with_channel(totalFiles) $(inputChannel) {
            with_channel(totalFiles) $(outputChannel) {
                with_job_status(totalThreads) $(completion) {

                    for (t in range(totalThreads)) {
                        new_thread <| @ {
                            for_each_clone(inputChannel) $(input : IsoInput#) {
                                var isoRes : IsolatedResult
                                isoRes.uri = clone_string(input.uri)
                                isoRes.cmd = clone_string(input.cmd)
                                let fileTime = ref_time_ticks()
                                unsafe {
                                    isoRes.exitCode = popen_timeout(input.cmd, testTimeout) $(f) {
                                        if (f == null) {
                                            // log::error("Internal error: Failed to execute cmd > {input.cmd}")
                                            isoRes.err |> push("Internal error: Failed to execute cmd > {input.cmd}")
                                            return
                                        }
                                        var readJson = false
                                        var jsonResult = ""
                                        while (!feof(f)) {
                                            let st = fgets(f)
                                            if (st == jsonResultPrefix) {
                                                readJson = !readJson
                                                continue
                                            }
                                            if (readJson) {
                                                jsonResult += st
                                            } else {
                                                // log::info_raw(st)
                                                isoRes.log |> push(st)
                                            }
                                        }
                                        var jsError : string
                                        var js = jsonResult |> read_json(jsError)
                                        if (!empty(jsError)) {
                                            // log::error("Internal error: failed to parse json result {jsError}")
                                            isoRes.err |> push("Internal error: failed to parse json result {jsError}")
                                        } elif (js != null) {
                                            isoRes.parsedResult = true
                                            isoRes.status = SuiteResult(js)
                                            parse_test_results(js, isoRes.tests)
                                        }
                                    }
                                }
                                isoRes.fileDt = get_time_usec(fileTime)

                                outputChannel |> push_clone(isoRes)
                                outputChannel |> notify()
                            }

                            inputChannel |> release()
                            outputChannel |> release()
                            completion |> notify_and_release()
                        }
                    }

                    for (file in files) {
                        let uri = ctx.uriPaths ? file_name_to_uri(file) : file
                        let jitstr = jit_enabled() ? "-jit" : ""
                        let aotstr = is_in_aot() ? "-use-aot" : ""
                        let benchstr = ctx.bench_enabled ? "--bench" : ""
                        let singleTest = "{raw_args[0]} {raw_args[1]} {jitstr} {aotstr} -- --run {file} {benchstr} {log::useTtyColors ? " --color" : ""}"
                        inputChannel |> push_clone(IsoInput(uri = clone_string(uri), cmd = clone_string(singleTest)))
                        inputChannel |> notify()
                    }

                    for (isoRes in each_clone(outputChannel, type<IsolatedResult>)) {
                        res += isoRes.status
                        for (tr in isoRes.tests) {
                            var msgs : array<string>
                            for (m in tr.messages) {
                                msgs |> push(clone_string(m))
                            }
                            suite::test_results |> emplace(SuiteTestResult(
                                name = clone_string(tr.name),
                                passed = tr.passed,
                                skipped = tr.skipped,
                                time_usec = tr.time_usec,
                                messages <- msgs,
                                location = clone_string(tr.location)
                            ))
                        }
                        for (err in isoRes.err) {
                            log::error(err)
                        }
                        if (timingOutliers > 0) {
                            fileTimings |> push(FileTiming(uri = clone_string(isoRes.uri), dt = isoRes.fileDt))
                        }
                        let isoPassed = isoRes.exitCode == 0 && isoRes.parsedResult && isoRes.status.failed + isoRes.status.errors == 0
                        if (!log::failuresOnly || !isoPassed) {
                            for (l in isoRes.log) {
                                log::info_raw(l)
                            }
                        }
                        if (isoPassed) {
                            log::green("PASS {isoRes.uri} {time_dt_hr(isoRes.fileDt)}")
                        } else {
                            log::red("FAIL {isoRes.uri} {time_dt_hr(isoRes.fileDt)}")
                            var reason = ""
                            if (isoRes.exitCode != 0 || !isoRes.parsedResult) {
                                res.total += 1
                                res.errors += 1
                            }
                            if (isoRes.exitCode == popen_timed_out) {
                                reason = "timed out after {testTimeout}s"
                                log::red("`{isoRes.uri}` {reason}")
                                log::info("to get more information, run the test manually\n> {isoRes.cmd}")
                            } elif (!isoRes.parsedResult) {
                                reason = "crashed (exit status {isoRes.exitCode})"
                                log::red("`{isoRes.uri}` execution was interrupted, crashed for example")
                                log::info("to get more information, run the test manually\n> {isoRes.cmd}")
                            } else {
                                reason = "{isoRes.status.failed} failed, {isoRes.status.errors} errors"
                            }
                            if (isoRes.exitCode != 0 && isoRes.exitCode != popen_timed_out) {
                                log::red("exit status {isoRes.exitCode}")
                            }
                            failedFiles |> push(FailedFile(uri = clone_string(isoRes.uri), reason = clone_string(reason)))
                        }
                    }

                    completion |> join()
                }
            }
        }
    }
    return finish_tests(res, startTime)
}


[export, pinvoke]
def timeout_tests(var res : SuiteResult; start_time : int64; timeout_sec : float) {
    res.errors += 1
    res.total += 1
    log::error("Test timed out after {timeout_sec}s")
    // timeout fires from a worker thread; fio::exit is the cross-thread
    // cancellation mechanism (bypasses shutdown dump — acceptable for this path).
    unsafe {
        fio::exit(finish_tests(res, start_time))
    }
}


def private print_timing_outliers() {
    let n = length(fileTimings)
    // compute mean
    var sum = 0.0lf
    for (ft in fileTimings) {
        sum += double(ft.dt)
    }
    let mean = sum / double(n)
    // compute stddev
    var variance = 0.0lf
    for (ft in fileTimings) {
        let diff = double(ft.dt) - mean
        variance += diff * diff
    }
    let stddev = sqrt(variance / double(n))
    // sort descending by time
    sort(fileTimings) $(a, b) {
        return a.dt > b.dt
    }
    let count = min(timingOutliers, n)
    let threshold = mean + 2.0lf * stddev
    // only print if there are actual outliers (top entry above threshold) or if few files
    var meanStr = ""
    meanStr = fmt(":.3f", mean / 1000000.0lf)
    var stddevStr = ""
    stddevStr = fmt(":.3f", stddev / 1000000.0lf)
    var thresholdStr = ""
    thresholdStr = fmt(":.3f", threshold / 1000000.0lf)
    log::info("Timing: mean {meanStr}s, stddev {stddevStr}s, threshold {thresholdStr}s")
    log::info("Top {count} slowest files:")
    for (i in range(count)) {
        let dtSec = double(fileTimings[i].dt) / 1000000.0lf
        let marker = double(fileTimings[i].dt) > threshold ? " [outlier]" : ""
        var dtStr = ""
        dtStr = fmt(":.3f", dtSec)
        var line = "  {dtStr}s  {fileTimings[i].uri}{marker}"
        log::info(line)
    }
}


def finish_tests(res : SuiteResult; start_time : int64) : int {
    let resCode = res.failed + res.errors
    let totalDt = get_time_usec(start_time)
    log::info("\n{to_string(res)}")
    if (resCode == 0 && timingOutliers > 0 && length(fileTimings) > 1) {
        print_timing_outliers()
    }
    if (resCode == 0) {
        log::green("SUCCESS! {time_dt_hr(totalDt)}")
    } else {
        if (length(failedFiles) > 0) {
            log::red("\nFAILURES:")
            for (ff in failedFiles) {
                log::red("  {ff.uri} — {ff.reason}")
            }
            log::info("")
        }
        log::red("FAILED! {time_dt_hr(totalDt)}")
    }
    if (!empty(jsonFilePath)) {
        var report = JsonTestReport(
            file = jsonTestFile,
            total = res.total,
            passed = res.passed,
            failed = res.failed,
            errors = res.errors,
            skipped = res.skipped,
            success = resCode == 0,
            time_usec = totalDt,
            tests <- suite::test_results
        )
        fopen(jsonFilePath, "wb") $(f) {
            if (f != null) {
                fwrite(f, sprint_json(report, false))
            }
        }
    }
    return resCode
}


// options debugger
// require daslib/debug
