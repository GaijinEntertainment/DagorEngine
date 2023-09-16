options indenting = 4

require fio
require strings
require uriparser
require debugapi
require daslib/json_boost
require daslib/jobque_boost

require fs
require suite
require log


let private jsonResultPrefix = "##dastest##\n"


def private collect_input_paths(args: array<string>; var paths: array<string>)
    for i in range(length(args) - 1)
        if args[i] == "--test"
            paths |> push <| args[i + 1]


def private collect_files(input_paths: array<string>; var files: array<string>): bool
    var cache: table<string; void?>
    for path in input_paths
        if path |> ends_with(".das")
            if !cache |> key_exists(path)
                cache[path] = null
                files |> push <| path
        else
            if !fs::scan_dir(path, cache, files, ".das")
                log::error("Unable to scan given path '{path}'")
                return false
    delete cache
    return true


def private get_float_arg(args: array<string>; name: string; def_val: float): float
    let idx = find_index(args, name)
    return idx >= 0 && idx + 1 < length(args) ? float(args[idx + 1]) : def_val


[export]
def main()
    var args <- get_command_line_arguments()
    log::init_log(args)
    let runIdx = args |> find_index("--run")
    if runIdx != -1
        let res = suite::test_file(args[runIdx + 1], SuiteCtx(args))
        log::info_raw("{jsonResultPrefix}{write_json(JV(res))}")
        return

    var res: SuiteResult
    let startTime = ref_time_ticks()

    let timeout = args |> get_float_arg("--timeout", 600.) // 10min
    if timeout > 0.
        unsafe
            var mainCtx & = this_context()
            new_thread <| @[[&res, &mainCtx]]
                fio::sleep(uint(timeout * 1000.))
                unsafe
                    mainCtx |> invoke_in_context("timeout_tests", res, startTime, timeout)

    var inputPaths: array<string>
    collect_input_paths(args, inputPaths)
    var files: array<string>
    if !collect_files(inputPaths, files)
        unsafe
            fio::exit(1)
        return

    let isolatedMode = args |> has_value("--isolated-mode")
    let ctx <- SuiteCtx(args)
    if !isolatedMode
        for file in files
            let uri = ctx.uriPaths ? file_name_to_uri(file) : file
            let fileTime = ref_time_ticks()
            let status = suite::test_file(file, ctx)
            let fileDt = get_time_usec(fileTime)
            if status.errors + status.failed == 0
                if status.total > 0
                    log::green("PASS {uri} {time_dt_hr(fileDt)}")
            else
                log::red("FAIL {uri} {time_dt_hr(fileDt)}")
            res += status
    else
        for file in files
            let uri = ctx.uriPaths ? file_name_to_uri(file) : file
            let singleTest = "{args[0]} {args[1]} -- --run {file} {log::useTtyColors ? " --color" : ""}"
            var exitCode = 0
            var parsedResult = false
            var status: SuiteResult
            let fileTime = ref_time_ticks()
            unsafe
                exitCode = popen(singleTest) <| $(f)
                    if f == null
                        log::error("Internal error: Failed to execute cmd > {singleTest}")
                        return
                    var readJson = false
                    var jsonResult = ""
                    while !feof(f)
                        let st = fgets(f)
                        if st == jsonResultPrefix
                            readJson = true
                            continue
                        if readJson
                            jsonResult += st
                        else
                            log::info_raw(st)
                    var jsError: string
                    var js = jsonResult |> read_json(jsError)
                    if !empty(jsError)
                        log::error("Internal error: failed to parse json result {jsError}")
                    elif js != null
                        parsedResult = true
                        status = SuiteResult(js)
            let fileDt = get_time_usec(fileTime)
            res += status
            if exitCode == 0 && parsedResult && status.failed + status.errors == 0
                log::green("PASS {uri} {time_dt_hr(fileDt)}")
            else
                log::red("FAIL {uri} {time_dt_hr(fileDt)}")
                if exitCode != 0
                    log::red("exit status {exitCode}")
                if !parsedResult
                    res.total += 1
                    res.errors += 1
                    log::red("`{uri}` execution was interrupted, crashed for example")
                    log::info("to get more information, run the test manually\n> {singleTest}")
    finish_tests(res, startTime)


[export]
def timeout_tests(var res: SuiteResult; start_time: int64; timeout_sec: float)
    res.errors += 1
    res.total += 1
    log::error("Test timed out after {timeout_sec}s")
    finish_tests(res, start_time)


def finish_tests(res: SuiteResult; start_time: int64)
    let resCode = res.failed + res.errors
    let totalDt = get_time_usec(start_time)
    log::info("\n{to_string(res)}")
    if resCode == 0
        log::green("SUCCESS! {time_dt_hr(totalDt)}")
    else
        log::red("FAILED! {time_dt_hr(totalDt)}")
    unsafe
        fio::exit(resCode)


// options debugger
// require daslib/debug
