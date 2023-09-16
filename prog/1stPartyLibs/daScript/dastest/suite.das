options indenting = 4

module suite shared

require ast
require strings
require debugapi
require fio
require daslib/defer

require log
require testing
require suite_result public


struct SuiteCtx
    dastestRoot: string
    projectPath: string
    uriPaths: bool = false
    testNames: array<string>
    verbose: bool = false


def createSuiteCtx(): SuiteCtx
    return <- SuiteCtx(get_command_line_arguments())


def private collect_tests_names(args: array<string>; var names: array<string>)
    for i in range(length(args) - 1)
        if args[i] == "--test-names"
            names |> push <| args[i + 1]


def get_str_arg(args : array<string>; name : string; def_val : string) : string
    let idx = find_index(args, name)
    return idx >= 0 && idx + 1 < length(args) ? args[idx + 1] : def_val


def SuiteCtx(args: array<string>): SuiteCtx
    var ctx <- SuiteCtx()
    let at = get_line_info()
    ctx.dastestRoot = dir_name(at.fileInfo != null ? string(at.fileInfo.name) : args[1])
    ctx.verbose = args |> has_value("--verbose")
    ctx.uriPaths = args |> has_value("--uri-paths")
    ctx.projectPath = args |> get_str_arg("--test-project", "")
    collect_tests_names(args, ctx.testNames)
    return <- ctx


struct FileCtx
    [[do_not_delete]] context: rtti::Context?
    uriPaths: bool = false
    indenting: string = ""
    verbose: bool = false
    stackOnRecover: bool = true
    expectFailure: bool = false


def FileCtx(ctx: SuiteCtx): FileCtx
    var res <- internalFileCtx(ctx)
    unsafe
        res.context = addr(this_context())
    return <- res


def private internalFileCtx(ctx: SuiteCtx): FileCtx
    return <- [[FileCtx uriPaths=ctx.uriPaths, verbose=ctx.verbose]]


def private match_test_name(name: string; ctx: SuiteCtx)
    if length(ctx.testNames) == 0
        return true
    for match in ctx.testNames
        if name |> starts_with(match)
            return true
    return false


def test_file(file_name: string; ctx: SuiteCtx): SuiteResult
    var fileCtx <- internalFileCtx(ctx)
    var res = test_file(file_name, ctx, fileCtx)
    delete fileCtx
    return res


def test_file(file_name: string; ctx: SuiteCtx; file_ctx: FileCtx): SuiteResult
    var res: SuiteResult
    var access := make_file_access(ctx.projectPath)
    access |> add_file_access_root("dastest", ctx.dastestRoot)
    using <| $(var mg:ModuleGroup#)
        using <| $(var cop:CodeOfPolicies#)
            cop.aot_module = true
            compile_file(file_name, access, addr(mg), cop) <| $(ok, program, output)
                var expectedErrors : table<CompilationError; int>
                if program != null
                    program |> for_each_expected_error <| $ ( err, count )
                        expectedErrors[err] = count
                var failed = !ok
                if ok
                    if !empty(output)
                        log::info("{output}")
                elif program != null
                    failed = false
                    for err in program.errors
                        let count = --expectedErrors[err.cerr]
                        if count < 0
                            failed = true

                if !failed
                    for errC, errN in keys(expectedErrors), values(expectedErrors)
                        if errN > 0
                            failed = true
                            break
                if !ok || failed
                    if failed
                        log::error("Failed to compile {file_name}\n{output}")
                    else
                        log::blue("Failed to compile {file_name}\n{output}")
                    if program != null
                        for err in program.errors
                            if expectedErrors[err.cerr] < 0
                                log::error("{describe(err.at)}: {int(err.cerr)}: {err.what}")
                                if !empty(err.extra)
                                    log::info("{err.extra}")
                                if !empty(err.fixme)
                                    log::info("{err.fixme}")
                    for errC, errN in keys(expectedErrors), values(expectedErrors)
                        if errN > 0
                            log::error("expect {int(errC)}:{errN} // {errC}")
                            log::info("Expect declaration count is greater than the actual errors reported")

                    res.total += 1
                    if failed
                        res.errors += 1
                    else
                        res.passed += 1
                    return
                simulate(program) <| $ (sok; context; serrors)
                    if !sok
                        res.total += 1
                        res.errors += 1
                        log::error("Failed to simulate {file_name}\n{serrors}")
                        return
                    var mod = program |> get_this_module()
                    if mod != null && context != null
                        res += test_module(*mod, *context, ctx, file_ctx)
                    else
                        res.errors += 1
                        res.total += 1
                        var msg = "Failed to execute {file_name}"
                        if mod == null
                            msg = "{msg}. Current module is null."
                        if context == null
                            msg = "{msg}. Current context is null."
                        log::error(msg)
    access := null
    return res


def test_module(var mod: rtti::Module; var context: rtti::Context; suite_ctx: SuiteCtx): SuiteResult
    var fileCtx <- internalFileCtx(suite_ctx)
    var res = test_module(mod, context, suite_ctx, fileCtx)
    delete fileCtx
    return res


def test_module(var mod: rtti::Module; var context: rtti::Context; suite_ctx: SuiteCtx; file_ctx: FileCtx): SuiteResult
    var res: SuiteResult
    var fileCtx := file_ctx
    var modPtr: Module?
    unsafe
        fileCtx.context = addr(context)
        modPtr = addr(mod)
    modPtr |> module_for_each_function <| $(f)
        if f.hash == 0ul
            return
        let func = context |> get_function_by_mnh(f.hash)
        let fn <- modPtr |> find_module_function_via_rtti(func)
        if fn == null
            return
        for ann in fn.annotations
            if ann.annotation.name == "test"
                var name = "{fn.name}"
                for arg in ann.arguments
                    if arg.name == "name"
                        name = "{arg.sValue}"
                if match_test_name(name, suite_ctx)
                    test_any(name, func, length(fn.arguments), fileCtx, res)
                return
    delete fileCtx
    return res


def test_func(name: string; var context: FileCtx; func: function<(t: T?): void>): SuiteResult
    var res: SuiteResult
    test_any(name, func, 1, context, res)
    return res


def test_func(name: string; var context: FileCtx; func: function<(): void>): SuiteResult
    var res: SuiteResult
    test_any(name, func, 0, context, res)
    return res


def test_func(name: string; var context: FileCtx; func: lambda<(t: T?): void>): SuiteResult
    var res: SuiteResult
    test_any(name, func, 1, context, res)
    return res


def test_func(name: string; var context: FileCtx; func: lambda<(): void>): SuiteResult
    var res: SuiteResult
    test_any(name, func, 0, context, res)
    return res


[export]
def private test_any_func(name: string; func: function; args_num: int; var context: FileCtx; var res: SuiteResult&)
    test_any(name, func, args_num, context, res)


[export]
def private test_any_lambda(name: string; func: lambda; args_num: int; var context: FileCtx; var res: SuiteResult&)
    test_any(name, func, args_num, context, res)


[export]
def private sub_test_any_func(name: string; func: function; args_num: int; var context: FileCtx; var res: SuiteResult&)
    var subContext := context
    subContext.indenting = "\t" + subContext.indenting
    test_any(name, func, args_num, subContext, res)
    delete subContext


[export]
def private sub_test_any_lambda(name: string; func: lambda; args_num: int; var context: FileCtx; var res: SuiteResult&)
    var subContext := context
    subContext.indenting = "\t" + subContext.indenting
    test_any(name, func, args_num, subContext, res)
    delete subContext


def private test_any(name: string; func; args_num: int; var context: FileCtx; var res: SuiteResult&)
    log::info("{context.indenting}=== RUN '{name}'")
    let beforeFailed = res.failed
    res.total += 1
    var dt = 0
    var deliberateRecover = false
    var failed = false
    var skipped = false
    let t0 = ref_time_ticks()
    var testing: T?
    defer <| $
        unsafe
            delete testing
    try
        if args_num == 0
            unsafe
                *context.context |> invoke_in_context(func)
            dt = get_time_usec(t0)
        else
            testing = new T(name, t0, context.verbose)
            unsafe
                var selfCtx & = this_context()
                testing.onFail <- @ <| [[&testing, &failed, &deliberateRecover]](now: bool)
                    testing.failed = true
                    failed = true
                    deliberateRecover = now

                testing.onSkipNow <- @ <| [[&testing, &skipped, &deliberateRecover]]()
                    testing.skipped = true
                    skipped = true
                    deliberateRecover = true

                testing.onRun <- @ <| [[&selfCtx, &context, &res]](test_name: string; f: RunT)
                    if f is func1
                        unsafe
                            selfCtx |> invoke_in_context("sub_test_any_func", test_name, f as func1, 1, context, res)
                    elif f is lmd1
                        unsafe
                            selfCtx |> invoke_in_context("sub_test_any_lambda", test_name, f as lmd1, 1, context, res)

                testing.onLog <- @ <| [[&context]](msg: string; at: LineInfo)
                    log::info("{context.indenting}{file_info_hr(at, context.uriPaths)}: {msg}")

                *context.context |> invoke_in_context(func, testing)
            dt = get_time_usec(t0)
    recover
        if !deliberateRecover
            dt = get_time_usec(t0)
            res.errors += 1
            errorOrBlue(context.expectFailure, "{context.indenting}--- FAIL '{name}' {time_dt_hr(dt)}")
            if !empty(context.context.exception)
                errorOrBlue(context.expectFailure, "{file_info_hr(context.context.exceptionAt, context.uriPaths)}: {context.indenting}{context.context.exception}")
            if !empty(context.context.last_exception)
                errorOrBlue(context.expectFailure, "{file_info_hr(context.context.exceptionAt, context.uriPaths)}: {context.indenting}{context.context.last_exception}")
            if context.stackOnRecover
                *context.context |> stackwalk(context.context.exceptionAt)
            return

    var expectFailure = context.expectFailure
    if expectFailure && !failed
        expectFailure = false
        failed = true
        log::error("{context.indenting}--- expect test failure, but it's success '{name}' {time_dt_hr(dt)}")

    if failed
        res.failed += 1
    elif skipped
        res.skipped += 1
        log::info("{context.indenting}--- SKIPPED '{name}' {time_dt_hr(dt)}")
    else
        res.passed += 1

    if beforeFailed < res.failed
        errorOrBlue(expectFailure, "{context.indenting}--- FAIL '{name}' {time_dt_hr(dt)}")
    else
        log::green("{context.indenting}--- PASS '{name}' {time_dt_hr(dt)}")


def private errorOrBlue(expectFailure: bool; msg: string)
    if expectFailure
        log::blue(msg)
    else
        log::error(msg)
