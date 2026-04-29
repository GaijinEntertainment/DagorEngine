options gen2
options indenting = 4
options relaxed_pointer_const

module suite shared

require daslib/ast
require strings
require daslib/debugger
require daslib/fio
require daslib/strings_boost
require daslib/json_boost
require daslib/defer
require math

require log
require testing
require suite_result public

var test_results : array<SuiteTestResult>
var bench_messages : array<string>

def clear_test_results() {
    delete test_results
}

struct SuiteCtx {
    // Shared options.
    dastestRoot : string
    projectPath : string
    uriPaths : bool = false
    verbose : bool = false
    compile_only : bool = false
    use_aot : bool = false
    cov_path : string = ""

    // Testing-related options.
    testNames : array<string>

    // Benchmarking-related options.
    bench_count : int = 1
    bench_format : BenchmarkOutputFormat = BenchmarkOutputFormat.native
    bench_enabled : bool = false // whether to run benchmarks or not
    bench_names : array<string> // filters the top-level benchmarks
    queued_benchmarks : array<Benchmark> // filled with file-local benchmarks
}

enum BenchmarkOutputFormat {
    native,
    go_export, // https://golang.org/design/14313-benchmark-format
    json,
}

// Benchmark holds everything we need to execute the
// user benchmark later, after all the tests are passed.
struct Benchmark {
    name : string
    fn : function<(b : B?) : void>
}


def createSuiteCtx() : SuiteCtx {
    return <- SuiteCtx(get_command_line_arguments())
}


def private collect_target_names(key : string, args : array<string>; var names : array<string>) {
    for (i in range(length(args) - 1)) {
        if (args[i] == key) {
            names |> push <| args[i + 1]
        }
    }
}


def private collect_tests_names(args : array<string>; var names : array<string>) {
    collect_target_names("--test-names", args, names)
}


def private collect_bench_names(args : array<string>; var names : array<string>) {
    collect_target_names("--bench-names", args, names)
}


def get_str_arg(args : array<string>; name : string; def_val : string) : string {
    let idx = find_index(args, name)
    return idx >= 0 && idx + 1 < length(args) ? args[idx + 1] : def_val
}


def SuiteCtx(args : array<string>) : SuiteCtx {
    var ctx <- SuiteCtx()
    let at = get_line_info()
    ctx.dastestRoot = dir_name(at.fileInfo != null ? string(at.fileInfo.name) : args[1])
    ctx.verbose = args |> has_value("--verbose")
    ctx.uriPaths = args |> has_value("--uri-paths")
    ctx.projectPath = args |> get_str_arg("--test-project", "")
    ctx.compile_only = args |> has_value("--compile-only")
    ctx.use_aot = args |> has_value("--use-aot") || is_in_aot()
    ctx.cov_path = args |> get_str_arg("--cov-path", "")
    collect_tests_names(args, ctx.testNames)

    ctx.bench_enabled = args |> has_value("--bench")
    collect_bench_names(args, ctx.bench_names)
    let bench_format = args |> get_str_arg("--bench-format", "")
    if (bench_format == "go") {
        ctx.bench_format = BenchmarkOutputFormat.go_export
    } elif (bench_format == "json") {
        ctx.bench_format = BenchmarkOutputFormat.json
    }
    ctx.bench_count  = to_int(args.get_str_arg("--count", "1"))

    return <- ctx
}

struct FileCtx {
    @do_not_delete context : rtti::Context?
    uriPaths : bool = false
    indenting : string = ""
    verbose : bool = false
    stackOnRecover : bool = true
    expectFailure : bool = false
}


def FileCtx(ctx : SuiteCtx) : FileCtx {
    var res <- internalFileCtx(ctx)
    unsafe {
        res.context = addr(this_context())
    }
    return <- res
}


def private internalFileCtx(ctx : SuiteCtx) : FileCtx {
    return <- FileCtx(uriPaths = ctx.uriPaths, verbose = ctx.verbose)
}


// match_func_name may reject a test if explicit test list
// was specified via --test-names parameter.
// If it's empty, the test is always accepted and therefore executed.
def private match_func_name(name : string; allowed : array<string>&) {
    if (length(allowed) == 0) {
        return true
    }
    for (match in allowed) {
        if (name |> starts_with(match)) {
            return true
        }
    }
    return false
}


def test_file(file_name : string; var ctx : SuiteCtx) : SuiteResult {
    var inscope fileCtx <- internalFileCtx(ctx)
    var res = test_file(file_name, ctx, fileCtx)
    return res
}


def test_file(file_name : string; var ctx : SuiteCtx; var file_ctx : FileCtx) : SuiteResult {
    var res : SuiteResult
    var inscope access <- make_file_access(ctx.projectPath)
    access |> add_file_access_root("dastest", ctx.dastestRoot)
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            ctx.queued_benchmarks |> clear
            cop.aot_module = true
            cop.aot = ctx.use_aot
            cop.fail_on_no_aot = ctx.use_aot
            cop.threadlock_context = true
            cop.jit_enabled = jit_enabled()
            if (jit_enabled()) {
                access |> add_extra_module("just_in_time", "{get_das_root()}/daslib/just_in_time.das")
            }
            if (!ctx.cov_path |> empty()) {
                cop.export_all = true
                cop.rtti = true
                access |> add_extra_module("coverage", "{get_das_root()}/daslib/coverage.das")
            }
            compile_file(file_name, access, unsafe(addr(mg)), cop) $(ok, program, output) {
                var expectedErrors : table<CompilationError; int>
                if (program != null) {
                    program |> for_each_expected_error() $(err, count) {
                        expectedErrors |> insert(err, count)
                    }
                }
                var failed = !ok
                if (ok) {
                    if (!empty(output)) {
                        log::info("{output}")
                    }
                } elif (program != null) {
                    failed = false
                    for (err in program.errors) {
                        let count = --unsafe(expectedErrors[err.cerr])
                        if (count < 0) {
                            failed = true
                        }
                    }
                }

                if (!failed) {
                    for (errC, errN in keys(expectedErrors), values(expectedErrors)) {
                        if (errN > 0) {
                            failed = true
                            break
                        }
                    }
                }
                if (!ok || failed) {
                    if (failed) {
                        log::error("Failed to compile {file_name}\n{output}")
                    } else {
                        log::blue("Failed to compile {file_name}\n{output}")
                    }
                    if (program != null) {
                        for (err in program.errors) {
                            if (expectedErrors |> get_value(err.cerr) < 0) {
                                log::error("{describe(err.at)}: {int(err.cerr)}: {err.what}")
                                if (!empty(err.extra)) {
                                    log::info("{err.extra}")
                                }
                                if (!empty(err.fixme)) {
                                    log::info("{err.fixme}")
                                }
                            }
                        }
                    }
                    for (errC, errN in keys(expectedErrors), values(expectedErrors)) {
                        if (errN > 0) {
                            log::error("expect {int(errC)}:{errN} // {errC}")
                            log::info("Expect declaration count is greater than the actual errors reported")
                        }
                    }

                    res.total += 1
                    if (failed) {
                        res.errors += 1
                    } else {
                        res.passed += 1
                    }
                    return
                }
                simulate(program) $(sok; context; serrors) {
                    if (!sok) {
                        res.total += 1
                        res.errors += 1
                        log::error("Failed to simulate {file_name}\n{serrors}")
                        return
                    }
                    var mod = program |> get_this_module()
                    if (mod != null && context != null) {
                        let module_result = test_module(*mod, *context, ctx, file_ctx)
                        if (module_result.total > 0 && !ctx.cov_path |> empty()) {
                            var report = ""
                            unsafe(invoke_in_context(*context, "fill_coverage_report", unsafe(addr(report)), ""))
                            // Clone to current context.
                            report := report
                            fopen(ctx.cov_path, "ab") <| $(fw) {
                                if (fw != null) {
                                    to_log(LOG_INFO, "Coverage to {ctx.cov_path}\n")
                                    fwrite(fw, report)
                                } else {
                                    to_log(LOG_ERROR, "Couldn't create output file {ctx.cov_path}\n")
                                }
                            }

                        }
                        if (module_result.failed + module_result.errors == 0) {
                            unsafe {
                                file_ctx.context = addr(*context)
                            }
                            run_queued_benchmarks(ctx, file_ctx, res)
                        }
                        res += module_result
                    } else {
                        res.errors += 1
                        res.total += 1
                        var msg = "Failed to execute {file_name}"
                        if (mod == null) {
                            msg = "{msg}. Current module is null."
                        }
                        if (context == null) {
                            msg = "{msg}. Current context is null."
                        }
                        log::error(msg)
                    }
                }
            }
        }
    }
    return res
}


def test_module(var mod : rtti::Module; var context : rtti::Context; var suite_ctx : SuiteCtx) : SuiteResult {
    var inscope fileCtx <- internalFileCtx(suite_ctx)
    var res = test_module(mod, context, suite_ctx, fileCtx)
    return res
}


def test_module(var mod : rtti::Module; var context : rtti::Context; var suite_ctx : SuiteCtx; file_ctx : FileCtx) : SuiteResult {
    if (suite_ctx.compile_only) {
        return default<SuiteResult>
    }
    var res : SuiteResult
    var inscope fileCtx := file_ctx
    var modPtr : Module?
    unsafe {
        fileCtx.context = addr(context)
        modPtr = addr(mod)
    }
    modPtr |> module_for_each_function() $(f) {
        if (f.hash == 0ul) {
            return
        }
        let func = context |> get_function_by_mnh(f.hash)
        var inscope fn <- modPtr |> find_module_function_via_rtti(func)
        if (fn == null) {
            return
        }
        var test_name = ""
        var test_kind = ""
        for (ann in fn.annotations) {
            var tag = "{ann.annotation.name}"
            if (tag != "test" && tag != "benchmark") {
                continue
            }
            test_kind = tag
            var name = "{fn.name}"
            // Both [test] and [benchmark] allow a form like [test(name=foo)]
            // to override the default test name.
            for (arg in ann.arguments) {
                if (arg.name == "name") {
                    name = "{arg.sValue}"
                }
            }
            test_name = name
            break
        }
        if (test_kind == "") {
            // Not a test nor benchmark.
            return
        }
        let allowed_list & = test_kind == "benchmark" ? suite_ctx.bench_names : suite_ctx.testNames
        if (!match_func_name(test_name, allowed_list)) {
            return
        }
        if (test_kind == "test") {
            test_any(test_name, func, length(fn.arguments), fileCtx, res)
        } elif (test_kind == "benchmark" && suite_ctx.bench_enabled) {
            let bench_func = unsafe(reinterpret<function<(b : B?) : void>>(func))
            let queued_bench = Benchmark(name = test_name, fn = bench_func)
            suite_ctx.queued_benchmarks |> push(queued_bench)
        }
    }
    return res
}


def test_func(name : string; var context : FileCtx; func : function<(t : T?) : void>) : SuiteResult {
    var res : SuiteResult
    test_any(name, func, 1, context, res)
    return res
}


def test_func(name : string; var context : FileCtx; func : function<() : void>) : SuiteResult {
    var res : SuiteResult
    test_any(name, func, 0, context, res)
    return res
}


def test_func(name : string; var context : FileCtx; func : lambda<(t : T?) : void>) : SuiteResult {
    var res : SuiteResult
    test_any(name, func, 1, context, res)
    return res
}


def test_func(name : string; var context : FileCtx; func : lambda<() : void>) : SuiteResult {
    var res : SuiteResult
    test_any(name, func, 0, context, res)
    return res
}


[export]
def private test_any_func(name : string; func : function; args_num : int; var context : FileCtx; var res : SuiteResult&) {
    test_any(name, func, args_num, context, res)
}


[export]
def private test_any_lambda(name : string; func : lambda; args_num : int; var context : FileCtx; var res : SuiteResult&) {
    test_any(name, func, args_num, context, res)
}


[export]
def private sub_test_any_func(name : string; func : function; args_num : int; var context : FileCtx; var res : SuiteResult&) {
    var inscope subContext := context
    subContext.indenting = "\t" + subContext.indenting
    test_any(name, func, args_num, subContext, res)
}


[export]
def private sub_test_any_lambda(name : string; func : lambda; args_num : int; var context : FileCtx; var res : SuiteResult&) {
    var inscope subContext := context
    subContext.indenting = "\t" + subContext.indenting
    test_any(name, func, args_num, subContext, res)
}


def private test_any(name : string; func; args_num : int; var context : FileCtx; var res : SuiteResult&) {
    if (!log::failuresOnly) {
        log::info("{context.indenting}=== RUN '{name}'")
    }
    let beforeFailed = res.failed
    let beforeErrors = res.errors
    res.total += 1
    var dt = 0
    var deliberateRecover = false
    var failed = false
    var skipped = false
    var testMessages : array<string>
    var testLocation : string
    let t0 = ref_time_ticks()
    var testing : T?
    defer <| $ {
        unsafe {
            delete testing
        }
    }
    try {
        if (args_num == 0) {
            unsafe {
                *context.context |> invoke_in_context(func)
            }
            dt = get_time_usec(t0)
        } else {
            testing = new T(name, t0, context.verbose)
            unsafe {
                var selfCtx & = this_context()
                testing.onFail <- @ capture(& testing, & failed, & deliberateRecover)(now : bool) {
                    testing.failed = true
                    failed = true
                    deliberateRecover = now
                }

                testing.onSkipNow <- @ capture(& testing, & skipped, & deliberateRecover)() {
                    testing.skipped = true
                    skipped = true
                    deliberateRecover = true
                }

                testing.onRun <- @ capture(& selfCtx, & context, & res)(test_name : string; f : RunT) {
                    if (f is func1) {
                        unsafe {
                            selfCtx |> invoke_in_context("sub_test_any_func", test_name, f as func1, 1, context, res)
                        }
                    } elif (f is lmd1) {
                        unsafe {
                            selfCtx |> invoke_in_context("sub_test_any_lambda", test_name, f as lmd1, 1, context, res)
                        }
                    }
                }

                testing.onLog <- @ capture(& context, & testMessages, & testLocation)(msg : string; at : LineInfo) {
                    let loc = file_info_hr(at, context.uriPaths)
                    log::info("{context.indenting}{loc}: {msg}")
                    testMessages |> push("{loc}: {msg}")
                    if (empty(testLocation)) {
                        testLocation = "{at.fileInfo != null ? string(at.fileInfo.name) : ""}:{int(at.line)}"
                    }
                }

                *context.context |> invoke_in_context(func, testing)
            }
            dt = get_time_usec(t0)
        }
    } recover {
        if (!deliberateRecover) {
            dt = get_time_usec(t0)
            failed = true
            res.errors += 1
            errorOrBlue(context.expectFailure, "{context.indenting}--- FAIL '{name}' {time_dt_hr(dt)}")
            if (!empty(context.context.exception)) {
                let loc = file_info_hr(context.context.exceptionAt, context.uriPaths)
                errorOrBlue(context.expectFailure, "{loc}: {context.indenting}{context.context.exception}")
                testMessages |> push("{loc}: {context.context.exception}")
                if (empty(testLocation)) {
                    testLocation = "{loc}"
                }
            }
            if (!empty(context.context.last_exception)) {
                let loc = file_info_hr(context.context.exceptionAt, context.uriPaths)
                errorOrBlue(context.expectFailure, "{loc}: {context.indenting}{context.context.last_exception}")
                testMessages |> push("{loc}: {context.context.last_exception}")
                if (empty(testLocation)) {
                    testLocation = "{loc}"
                }
            }
            if (context.stackOnRecover) {
                *context.context |> stackwalk(context.context.exceptionAt)
            }
        }
    }

    // clone strings from test context into dastest context
    if (!empty(testMessages)) {
        var clonedMessages : array<string>
        for (m in testMessages) {
            clonedMessages |> push(clone_string(m))
        }
        delete testMessages
        testMessages <- clonedMessages
    }
    if (!empty(testLocation)) {
        testLocation = clone_string(testLocation)
    }

    var expectFailure = context.expectFailure
    if (expectFailure && !failed) {
        expectFailure = false
        failed = true
        log::error("{context.indenting}--- expect test failure, but it's success '{name}' {time_dt_hr(dt)}")
    }

    if (failed) {
        if (beforeErrors == res.errors) {
            res.failed += 1
        }
    } elif (skipped) {
        res.skipped += 1
        log::info("{context.indenting}--- SKIPPED '{name}' {time_dt_hr(dt)}")
    } else {
        res.passed += 1
    }

    if (beforeFailed < res.failed || beforeErrors < res.errors) {
        errorOrBlue(expectFailure, "{context.indenting}--- FAIL '{name}' {time_dt_hr(dt)}")
    } else {
        log::green("{context.indenting}--- PASS '{name}' {time_dt_hr(dt)}")
    }

    test_results |> emplace(SuiteTestResult(
        name = clone_string(name),
        passed = !failed && !skipped,
        skipped = skipped,
        time_usec = dt,
        messages <- testMessages,
        location = testLocation
    ))
}

def private run_queued_benchmarks(var suite_ctx : SuiteCtx; var file_ctx : FileCtx, var res : SuiteResult&) {
    for (q in suite_ctx.queued_benchmarks) {
        let failed = res.failed
        for (i in range(suite_ctx.bench_count)) {
            run_benchmark(suite_ctx, file_ctx, res, q.name, q.fn)
            if (res.failed > failed) {
                break
            }
        }
    }
}

[export]
def private bench_on_start_impl(var suite_ctx : SuiteCtx; var benchmarking : B?; var file_ctx : FileCtx; func_type, name, sub_name : string) {
    let is_first_bench = !benchmarking.started
    benchmarking.started = true
    benchmarking.current_sub_name = sub_name

    if (suite_ctx.bench_format == BenchmarkOutputFormat.native) {
        if (is_first_bench) {
            log::info("{file_ctx.indenting}=== RUN BENCHMARK '{benchmarking.name}' [{blue_str(func_type)}]")
        }
        let full_name = "\t{sub_name}"
        let padded_name = pad_right(full_name, 22, ' ')
        info_raw("{file_ctx.indenting}{padded_name} ")
        return
    }

    if (suite_ctx.bench_format == BenchmarkOutputFormat.go_export) {
        var go_name = name
        if (starts_with(name, "benchmark_")) {
            go_name = slice(name, length("benchmark_"))
        }
        info_raw("BenchmarkDas_{go_name}/sub={sub_name}-1\t\t")
        return
    }
}

[export]
def private bench_on_finished_impl(var suite_ctx : SuiteCtx; var benchmarking : B?; stats : BenchmarkRunStats) {
    let n = stats.n
    let time_ns = stats.time_ns
    let allocs = stats.allocs
    let heap_bytes = stats.heap_bytes

    benchmarking.finished = true

    let avg_time = double(time_ns) / double(n)
    let avg_heap_bytes = double(ceil(float(heap_bytes) / float(n)))
    let avg_num_allocs = double(ceil(float(allocs) / float(n)))
    let avg_string_heap_bytes = double(ceil(float(stats.string_heap_bytes) / float(n)))
    let avg_string_num_allocs = double(ceil(float(stats.string_allocs) / float(n)))

    // Collect plain-text stats for JSON report messages (always, regardless of format)
    bench_messages |> push("{stats.sub_name}: {avg_time:.1f} ns/op, {avg_heap_bytes:.0f} B/op, {avg_num_allocs:.0f} allocs/op [{stats.func_type}]")

    if (suite_ctx.bench_format == BenchmarkOutputFormat.native) {
        let entries = [
            (int64(avg_time), "ns/op", 18, false),
            (int64(avg_heap_bytes), "B/op", 14, true),
            (int64(avg_num_allocs), "allocs/op", 16, true),
            (int64(avg_string_heap_bytes), "SB/op", 14, true),
            (int64(avg_string_num_allocs), "strings/op", 16, true),
        ]
        let msg = build_string() $(var w) {
            for (i, (val, measure, width, is_alloc) in range(length(entries)), entries) {
                let val_s = "{val}"
                let printed_width = length(val_s + " " + measure)
                let sep = repeat(" ", max(width - printed_width, 1))
                let is_last = i == length(entries) - 1
                var colored_val = yellow_str(val_s)
                if (is_alloc && val == 0l) {
                    colored_val = green_str(val_s)
                }
                w |> write(colored_val + " " + measure)
                if (!is_last) {
                    w |> write(sep)
                }
            }
        }
        info_raw("{msg}\n")
        return
    }

    if (suite_ctx.bench_format == BenchmarkOutputFormat.go_export) {
        info_raw("{n}\t\t{avg_time:.1f} ns/op\t{avg_heap_bytes} B/op\t{avg_num_allocs} allocs/op\t{avg_string_heap_bytes} SB/op\t{avg_string_num_allocs} strings/op\n")
        return
    }

    if (suite_ctx.bench_format == BenchmarkOutputFormat.json) {
        var s = sprint_json(stats, false)
        info_raw("{s}\n")
        return
    }

}

def private run_benchmark(var suite_ctx : SuiteCtx; var file_ctx : FileCtx; var res : SuiteResult&; name : string; func : function<(b : B?) : void>) {
    var benchmarking = new B(name = name)

    let t0 = ref_time_ticks()
    var deliberateRecover = false
    delete bench_messages

    var func_type = "INTERP"
    if (is_in_aot()) {
        func_type = "AOT"
    } elif (is_jit_function(func)) {
        func_type = "JIT"
    }

    try {
        unsafe {
            var selfCtx & = this_context()

            benchmarking.on_log <- @ capture(& context, & file_ctx, & suite_ctx) (msg : string; at : LineInfo) {
                log::info("{file_ctx.indenting}{file_info_hr(at, suite_ctx.uriPaths)}: {msg}")
            }

            benchmarking.on_fail <- @ capture(& benchmarking, & deliberateRecover) (now : bool) {
                deliberateRecover = now
                benchmarking.failed = true
            }

            benchmarking.on_start <- @capture(& selfCtx, & suite_ctx, & benchmarking, & file_ctx) (sub_name : string) {
                unsafe {
                    selfCtx |> invoke_in_context("bench_on_start_impl", suite_ctx, benchmarking, file_ctx, func_type, name, sub_name)
                }
            }

            benchmarking.on_finished <- @capture(& selfCtx, & suite_ctx, & benchmarking) (stats : BenchmarkRunStats) {
                unsafe {
                    var with_func_type = stats
                    with_func_type.func_type = func_type
                    selfCtx |> invoke_in_context("bench_on_finished_impl", suite_ctx, benchmarking, with_func_type)
                }
            }
        }

        unsafe {
            *file_ctx.context |> invoke_in_context(func, benchmarking)
        }
    } recover {
        benchmarking.failed = true
    }

    let dt = get_time_usec(t0)

    // A failed bench increases the failure count and may
    // print an exception if it wasn't a controlled panic via fail or failNow.
    if (benchmarking.failed) {
        res.failed++
        if (benchmarking.started && !benchmarking.finished) {
            if (suite_ctx.bench_format != BenchmarkOutputFormat.json) {
                print("\n")
            }
        }
        log::error("{file_ctx.indenting}--- FAIL '{name}' in '{benchmarking.current_sub_name}' {time_dt_hr(dt)}")
        if (!deliberateRecover) {
            unsafe {
                let context & = this_context()
                if (!empty(context.exception)) {
                    log::error("{file_info_hr(context.exceptionAt, file_ctx.uriPaths)}: {file_ctx.indenting}{context.exception}")
                }
                if (!empty(context.last_exception)) {
                    log::error("{file_info_hr(context.exceptionAt, file_ctx.uriPaths)}: {file_ctx.indenting}{context.last_exception}")
                }
            }
        }
    }

    test_results |> emplace(SuiteTestResult(
        name = clone_string(name),
        passed = !benchmarking.failed,
        skipped = false,
        time_usec = dt,
        messages <- bench_messages
    ))
}

def private errorOrBlue(expectFailure : bool; msg : string) {
    if (expectFailure) {
        log::blue(msg)
    } else {
        log::error(msg)
    }
}
