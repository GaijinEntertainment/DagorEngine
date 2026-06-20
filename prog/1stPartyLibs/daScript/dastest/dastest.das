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
require daslib/strings_boost

require fs
require suite
require log
require math
require dastest_clargs

options multiple_contexts

// BBATKIN:
//  temporary: disables persistent heap, because this crashes /tests/dastest
//  it needs investigation but disabling for now to unblock other work
options persistent_heap = false

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
    for (raw in input_paths) {
        let path = replace(raw, "_dasroot_", get_das_root())
        if (path |> ends_with(".das")) {
            if (!cache |> key_exists(path)) {
                cache |> insert(path, null)
                files |> push <| path
            }
        } else {
            let das_test_path = path |> fs::join_path(".das_test")
            let das_test_stat = stat(das_test_path)
            if (das_test_stat.is_reg) {
                return false if (!collect_files_filtered(path, das_test_path, cache, files))
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

// One unit of work pulled by a worker thread: a batch of test files run by a
// single subprocess. uris/singleCmds are parallel arrays (one per file, in run
// order); batchCmd runs all files at once. The worker appends --worker-index.
struct IsoInput {
    uris : array<string>         // display names, one per file, in batch order
    batchCmd : string            // runs every file in the batch (one subprocess)
    singleCmds : array<string>   // singleCmds[i] runs only file i (crash re-dispatch)
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

// Result of running ONE subprocess (a batch or a single file). The subprocess
// streams one ##dastest## JSON report per file as it finishes, so results[]
// holds one IsolatedResult per REPORTED file in run order (a prefix of the
// batch). If the process dies mid-batch, results is short and pendingLog holds
// the log of the file that was running when it died.
struct BatchRun {
    results : array<IsolatedResult>
    exitCode : int
    pendingLog : array<string>
    err : array<string>
}

// prefix = "<launch> -jit -- " ; suffix = " --bench --color --headless".
// Worker-index is appended by the caller, not here.
def private build_iso_cmd(filesList : array<string>; prefix, suffix : string) : string {
    return build_string() $(var w) {
        w |> write(prefix)
        for (f in filesList) {
            // Always quote the path: popen runs this through cmd.exe / /bin/sh,
            // so a test path with a space (or other shell-significant char) would
            // otherwise split into multiple argv entries. The shell strips the
            // quotes, so the child sees the clean path. Mirrors the launchPrefix
            // quoting in the dispatch loop.
            w |> write(" --run \"{f}\"")
        }
        w |> write(suffix)
    }
}

// Run one subprocess and parse its streamed per-file JSON reports.
def private run_iso_subprocess(cmd : string; timeoutSec : float; var br : BatchRun) {
    var pending : array<string>
    // popen_timeout's stderr handling is timeout-dependent: with timeout<=0 it
    // falls back to plain _popen (stdout only — stderr DROPPED), which is the
    // path isolated mode hits since per-test test_timeout defaults to 0; with
    // timeout>0 it routes stderr into the captured pipe, interleaved with the
    // ##dastest## JSON markers. Either way a hard crash (AV / abort / __fastfail)
    // writes its diagnostic — the crash-handler C++ stack + the daslang call
    // stack — to stderr. Redirect stderr to a temp file (overrides both paths,
    // and avoids the timeout>0 interleave) and, on abnormal exit, fold it into
    // the crashed file's log so the stack is surfaced under the failing test.
    let tfRes = create_temp_file_result("dastest_stderr_", ".txt")
    let errPath = (tfRes is value) ? (tfRes as value) : ""
    if (empty(errPath)) {
        // The point of this is to NOT silently lose crash diagnostics, so make
        // the temp-file failure visible rather than quietly dropping stderr.
        br.err |> push("dastest: could not create a temp file for subprocess stderr capture ({tfRes as error}); stderr (crash stacks, etc.) will not be surfaced")
    }
    let runCmd = empty(errPath) ? cmd : "{cmd} 2>\"{errPath}\""
    unsafe {
        br.exitCode = popen_timeout(runCmd, timeoutSec) $(f) {
            if (f == null) {
                br.err |> push("Internal error: Failed to execute cmd > {cmd}")
                return
            }
            var readJson = false
            var jsonAccum = ""
            while (!feof(f)) {
                let st = fgets(f)
                if (st == jsonResultPrefix) {
                    if (readJson) {
                        // Closing marker: this file completed (the --run handler
                        // writes its report after running), so emit EXACTLY one
                        // result whether or not the JSON parses. Dropping it on a
                        // parse error would shorten the batch prefix and make the
                        // parent misattribute a later crash and re-dispatch
                        // already-run files; `pending` is moved out either way.
                        var jsErr : string
                        var js = jsonAccum |> read_json(jsErr)
                        jsonAccum = ""
                        if (empty(jsErr) && js != null) {
                            let obj & = unsafe(js as _object)
                            var r = IsolatedResult(
                                parsedResult = true,
                                status = SuiteResult(js),
                                fileDt = from_JV(obj?["time_usec"] ?? JV(0), type<int>), // nolint:STYLE020 — `?? defV` doesn't typecheck on _object indexing; suite_result.das pattern
                                log <- pending
                            )
                            parse_test_results(js, r.tests)
                            br.results |> emplace(r)
                        } else {
                            var perr : array<string>
                            perr |> push("Internal error: failed to parse json result {jsErr}")
                            var r = IsolatedResult(parsedResult = false, log <- pending, err <- perr)
                            br.results |> emplace(r)
                        }
                    }
                    readJson = !readJson
                    continue
                }
                if (readJson) {
                    jsonAccum += st
                } else {
                    pending |> push(st)
                }
            }
        }
    }
    if (!empty(errPath)) {
        // Folded in on ANY abnormal exit (crash, panic, compile error, timeout),
        // not just hard crashes — so the header stays neutral and carries the
        // exit code rather than asserting "crash". A clean run's stderr (e.g.
        // libhv logs) is noise we don't attribute to the test.
        if (br.exitCode != 0) {
            var folded : array<string>
            fopen(errPath, "rb") $(ef) {
                if (ef != null) {
                    while (!feof(ef)) {
                        let ln = fgets(ef)
                        continue if (empty(ln))
                        folded |> push(ln)
                    }
                }
            }
            if (!empty(folded)) {
                let header = "----- worker subprocess stderr (abnormal exit {br.exitCode}; batch>1 may include other files in the worker) -----\n"
                // A caught panic emits a JSON result, so `pending` was already moved into
                // that result's log above — fold the captured stderr into the last result's
                // log so the at-throw stack (--stack-on-exception) surfaces under the
                // failing test. A hard crash emits no result, so fall back to `pending`
                // (the trailing log attributed to the dead file).
                if (empty(br.results)) {
                    pending |> push(header)
                    pending |> push_from(folded)
                } else {
                    let li = length(br.results) - 1
                    br.results[li].log |> push(header)
                    br.results[li].log |> push_from(folded)
                }
            }
        }
        remove_result(errPath)
    }
    br.pendingLog <- pending
}

// Worker entry: run one batch, attribute results, and on a mid-batch crash
// re-run the remainder one-process-per-file. Pushes exactly one result per
// file in the batch to outputChannel.
def private run_iso_batch(input : IsoInput#; t : int; perTestTimeout : float; outputChannel : Channel?) {
    let nFiles = length(input.uris)
    // 1-based: clients deriving a port from the worker index (e.g. imgui_playwright
    // maps worker-index N to live-API port 9090+N) start at 9091, leaving 9090 free
    // for a dev/attached host.
    let widx = " --worker-index {t + 1}"
    // Batch timeout: per-test budget scaled by batch size (<=0 means none).
    let batchTimeout = perTestTimeout <= 0.0 ? perTestTimeout : perTestTimeout * float(nFiles)
    var br : BatchRun
    run_iso_subprocess("{input.batchCmd}{widx}", batchTimeout, br)
    let k = min(length(br.results), nFiles)
    // Reported files (a prefix of the batch). The per-file JSON is authoritative,
    // so exitCode is 0 even if the batch returned non-zero for other failures.
    for (j in range(k)) {
        br.results[j].uri = clone_string(input.uris[j])
        br.results[j].cmd = clone_string("{input.singleCmds[j]}{widx}")
        br.results[j].exitCode = 0
        outputChannel |> push_clone(br.results[j])
        outputChannel |> notify()
    }
    if (k < nFiles) {
        // The process died (crash or timeout) on file k: the batch's trailing
        // log belongs to it. Mark it failed, then re-run the rest as singles so
        // a second crash can only sink itself.
        // The shared accounting loop reports timeouts using the per-test budget;
        // in a batch the effective deadline is batchTimeout (perTestTimeout x
        // nFiles), so spell out the real number here to avoid a misleading
        // "timed out after {perTestTimeout}s".
        if (br.exitCode == popen_timed_out && nFiles > 1) {
            br.err |> push("batch of {nFiles} files timed out after {batchTimeout}s (per-test budget {perTestTimeout}s x {nFiles}); re-running the remainder individually")
        }
        let cr = IsolatedResult(
            uri = clone_string(input.uris[k]),
            cmd = clone_string("{input.singleCmds[k]}{widx}"),
            exitCode = br.exitCode,
            parsedResult = false,
            log <- br.pendingLog,
            err <- br.err
        )
        outputChannel |> push_clone(cr)
        outputChannel |> notify()
        for (i in range(k + 1, nFiles)) {
            var brs : BatchRun
            run_iso_subprocess("{input.singleCmds[i]}{widx}", perTestTimeout, brs)
            if (!empty(brs.results)) {
                brs.results[0].uri = clone_string(input.uris[i])
                brs.results[0].cmd = clone_string("{input.singleCmds[i]}{widx}")
                brs.results[0].exitCode = 0
                outputChannel |> push_clone(brs.results[0])
                outputChannel |> notify()
            } else {
                let r2 = IsolatedResult(
                    uri = clone_string(input.uris[i]),
                    cmd = clone_string("{input.singleCmds[i]}{widx}"),
                    exitCode = brs.exitCode,
                    parsedResult = false,
                    log <- brs.pendingLog,
                    err <- brs.err
                )
                outputChannel |> push_clone(r2)
                outputChannel |> notify()
            }
        }
    }
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
            // Tests whose basename starts with cant_, failed_, or invalid_
            // are expected-failure compile tests anywhere in the tests tree;
            // they have nothing to serialize.
            let base = base_name(file)
            continue if (base |> starts_with("cant_") || base |> starts_with("failed_") || base |> starts_with("invalid_"))
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
            let sz = uint64(length(data))
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

def deserialize_path(var ctx : SuiteCtx, _files : array<string>, in_file : string) : SuiteResult {
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
        using() $(var _mg : ModuleGroup) {
            for (i in range(count)) {
                deserialize_program(ser) $(ok, program, error) {
                    if (!ok) {
                        log::error("Deserialization failed at program {i}: {error}")
                        res.errors++
                        res.total++
                        return
                    }
                    return if (program._options |> find_arg("rtti") ?as tBool ?? false)
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
    let raw_args <- get_command_line_arguments()

    var parse_result <- parse_args(type<DastestArgs>)
    if (parse_result |> is_err) {
        log::red("command-line args: {parse_result |> unwrap_err}")
        return 1
    }
    var test_args <- parse_result |> move_unwrap

    if (!empty(test_args.run)) {
        // Isolated/semi-isolated worker subprocess: run each --run file in this
        // one process, emitting a ##dastest## JSON report PER FILE as it
        // finishes (streamed) so the parent can attribute results and detect
        // exactly which file was running if the process dies mid-batch.
        var rctx <- SuiteCtx(test_args)
        var rc = 0
        for (rf in test_args.run) {
            suite::clear_test_results()
            let t0 = ref_time_ticks()
            let res = suite::test_file(rf, rctx)
            let dt = get_time_usec(t0)
            let report = JsonTestReport(
                file = rf,
                total = res.total,
                passed = res.passed,
                failed = res.failed,
                errors = res.errors,
                skipped = res.skipped,
                success = res.failed + res.errors == 0,
                time_usec = dt,
                tests <- suite::test_results
            )
            log::info_raw("\n{jsonResultPrefix}{sprint_json(report, false)}\n{jsonResultPrefix}")
            rc += res.failed + res.errors
        }
        return rc
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
    return 1 if (!collect_files(inputPaths, files))
    if (!empty(test_args.exclude_files)) {
        files |> erase_if() $(f) {
            let base = base_name(f)
            return (find_index_if(test_args.exclude_files) $(pat) => base |> contains(pat)) >= 0
        }
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
                let reason = "{status.failed} failed, {status.errors} errors"
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
        // --headless is a pass-through flag (not parsed by DastestArgs); detect once
        // here so each spawned --run subprocess re-receives it via singleTest cmd.
        // Without this, the post-`--` slice in the child loses --headless and any
        // playwright/harness that depends on it reverts to windowed mode.
        let headlessFlag = find_index(raw_args, "--headless") >= 0 ? " --headless" : ""
        // Preserve EVERY pre-`--` arg (daslang flags + script path), not just
        // raw_args[0..1]. With raw_args = [daslang, -load_module, <path>,
        // dastest.das, --, ...], the old "raw_args[0] raw_args[1]" reconstruction
        // produced "daslang -load_module" — losing the flag value, breaking
        // local runs that point dastest at an external module via -load_module.
        // Find index of `--` ≥ 0 always (dastest was invoked with `--` to receive
        // its own args); fall back to all raw_args if not, matching the old
        // 2-arg implicit assumption. Quote any arg containing a space so paths
        // like `C:/Program Files/...` survive popen's shell-split on the other
        // side; both cmd.exe and /bin/sh honor double-quote-wrapped args.
        let dashdash = find_index(raw_args, "--")
        let launchPrefix = build_string() $(var w) {
            let n = dashdash > 0 ? dashdash : length(raw_args)
            for (i in range(n)) {
                if (i > 0) w |> write(" ")
                let needs_quote = raw_args[i] |> contains(" ")
                if (needs_quote) w |> write("\"")
                w |> write(raw_args[i])
                if (needs_quote) w |> write("\"")
            }
        }
        let jitstr = jit_enabled() ? "-jit" : ""
        let aotstr = is_in_aot() ? "-use-aot" : ""
        let benchstr = ctx.bench_enabled ? "--bench" : ""
        // Forward --stack-on-exception to each worker: the flag sets the test
        // context's at-throw stack walk, and the tests run in the worker, not here.
        let stackExcFlag = test_args.stack_on_exception ? " --stack-on-exception" : ""
        let cmdPrefix = "{launchPrefix} {jitstr} {aotstr} --"
        let cmdSuffix = " {benchstr}{log::useTtyColors ? " --color" : ""}{headlessFlag}{stackExcFlag}"
        let batchSize = max(1, test_args.batch)
        // Group files into batches of batchSize. Each batch is run by one worker
        // subprocess (batchCmd); per-file singleCmds are kept so a crash can be
        // re-dispatched one-process-per-file.
        var batches : array<IsoInput>
        var bidx = 0
        while (bidx < totalFiles) {
            let hi = min(bidx + batchSize, totalFiles)
            let bn = hi - bidx
            var bUris : array<string>
            var bFiles : array<string>
            var bSingles : array<string>
            bUris |> reserve(bn)
            bFiles |> reserve(bn)
            bSingles |> reserve(bn)
            for (j in range(bidx, hi)) {
                let file = files[j]
                let uri = ctx.uriPaths ? file_name_to_uri(file) : file
                bUris |> push(clone_string(uri))
                bFiles |> push(clone_string(file))
                bSingles |> push(build_iso_cmd([clone_string(file)], cmdPrefix, cmdSuffix))
            }
            let bCmd = build_iso_cmd(bFiles, cmdPrefix, cmdSuffix)
            unsafe { delete bFiles }
            batches |> emplace(IsoInput(uris <- bUris, batchCmd = clone_string(bCmd), singleCmds <- bSingles))
            bidx = hi
        }
        let numBatches = length(batches)
        log::info("Running {totalFiles} tests in isolated mode: {totalThreads} threads, batch {batchSize} ({numBatches} worker subprocess(es))\n")
        with_channel(numBatches) $(inputChannel) {
            with_channel(totalFiles) $(outputChannel) {
                with_job_status(totalThreads) $(completion) {

                    for (t in range(totalThreads)) {
                        new_thread <| @capture(= t) {
                            for_each_clone(inputChannel) $(input : IsoInput#) {
                                run_iso_batch(input, t, testTimeout, outputChannel)
                            }

                            inputChannel |> release()
                            outputChannel |> release()
                            completion |> notify_and_release()
                        }
                    }

                    for (b in batches) {
                        inputChannel |> push_clone(b)
                        inputChannel |> notify()
                    }

                    for (isoRes in each_clone(outputChannel, type<IsolatedResult>)) {
                        res += isoRes.status
                        for (tr in isoRes.tests) {
                            var msgs <- [for (m in tr.messages); clone_string(m)]
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
                                res.total++
                                res.errors++
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
    res.errors++
    res.total++
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
    let meanStr = fmt(":.3f", mean / 1000000.0lf)
    let stddevStr = fmt(":.3f", stddev / 1000000.0lf)
    let thresholdStr = fmt(":.3f", threshold / 1000000.0lf)
    log::info("Timing: mean {meanStr}s, stddev {stddevStr}s, threshold {thresholdStr}s")
    log::info("Top {count} slowest files:")
    for (i in range(count)) {
        let dtSec = double(fileTimings[i].dt) / 1000000.0lf
        let marker = double(fileTimings[i].dt) > threshold ? " [outlier]" : ""
        let dtStr = fmt(":.3f", dtSec)
        let line = "  {dtStr}s  {fileTimings[i].uri}{marker}"
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
        if (!empty(failedFiles)) {
            log::red("\nFAILURES:")
            for (ff in failedFiles) {
                log::red("  {ff.uri} — {ff.reason}")
            }
            log::info("")
        }
        log::red("FAILED! {time_dt_hr(totalDt)}")
    }
    if (!empty(jsonFilePath)) {
        let report = JsonTestReport(
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
