options gen2
options no_unused_function_arguments = false
options no_aot
options rtti

require dastest/testing_boost
require dastest/suite_result
require daslib/json
require daslib/json_boost
require daslib/strings_boost
require daslib/fio

struct JsonReport {
    file : string
    total : int
    passed : int
    failed : int
    errors : int
    skipped : int
    success : bool
    tests : array<SuiteTestResult>
}

def run_dastest(fixture : string; extra_args : string = "") : JsonReport {
    let das_root = get_das_root()
    let dastest_path = "{das_root}/dastest/dastest.das"
    let json_file = "{das_root}/tests/dastest/_temp_{fixture}.json"
    let test_file = "{das_root}/tests/dastest/_fixture_{fixture}.das"
    // argv, not a shell command line: cmd.exe /c strips the first and last quote
    // of a line that starts with one, which unquotes the interpreter path
    var args <- [
        get_command_line_arguments()[0],
        "-dasroot", das_root,
        dastest_path,
        "--",
        "--test", test_file,
        "--json-file", json_file
    ]
    if (!empty(extra_args)) {
        args |> push_from(split(extra_args, " "))
    }
    var output : string
    let exitCode = run_and_capture(args, output)
    var json_text = ""
    fopen(json_file, "rb") $(f) {
        if (f != null) {
            json_text = fread(f)
        }
    }
    remove(json_file)
    var error = ""
    var json = read_json(json_text, error)
    if (json == null) {
        to_log(LOG_ERROR, "Command output ({exitCode}):\n\n{output}\n")
        panic("Failed to parse JSON from {json_file}: {error}")
    }
    return from_JV(json, type<JsonReport>)
}

def find_test(report : JsonReport; name : string) : SuiteTestResult const? {
    for (tr in report.tests) {
        if (tr.name == name) {
            return unsafe(addr(tr))
        }
    }
    return null
}


// ---- Tests for JSON output with messages and location ----

[test]
def test_passing_json_output(tt : T?) {
    let report = run_dastest("pass")
    tt |> equal(report.total, 2)
    tt |> equal(report.passed, 2)
    tt |> equal(report.failed, 0)
    for (tr in report.tests) {
        tt |> success(tr.passed, "test '{tr.name}' should pass")
        tt |> equal(length(tr.messages), 0)
        tt |> success(empty(tr.location), "location should be empty for passing test")
        tt |> success(tr.time_usec > 0, "time should be > 0 for '{tr.name}'")
    }
}

[test]
def test_failing_json_output(tt : T?) {
    let report = run_dastest("fail")
    tt |> equal(report.total, 2)
    tt |> equal(report.passed, 1)
    tt |> equal(report.failed, 1)
    var fail_result = find_test(report, "test_will_fail")
    tt |> success(fail_result != null, "test_will_fail should be in results")
    if (fail_result != null) {
        tt |> success(!fail_result.passed, "test_will_fail should not pass")
        tt |> success(!empty(fail_result.messages), "failed test should have messages")
        tt |> success(!empty(fail_result.location), "failed test should have location")
    }
    var pass_result = find_test(report, "test_will_pass")
    tt |> success(pass_result != null, "test_will_pass should be in results")
    if (pass_result != null) {
        tt |> success(pass_result.passed)
        tt |> equal(length(pass_result.messages), 0)
        tt |> success(empty(pass_result.location))
    }
}

[test]
def test_skipped_json_output(tt : T?) {
    let report = run_dastest("skip")
    tt |> equal(report.total, 2)
    tt |> equal(report.skipped, 1)
    var skip_result = find_test(report, "test_skipped")
    tt |> success(skip_result != null)
    if (skip_result != null) {
        tt |> success(skip_result.skipped, "should be skipped")
    }
    var pass_result = find_test(report, "test_not_skipped")
    tt |> success(pass_result != null)
    if (pass_result != null) {
        tt |> success(pass_result.passed)
        tt |> success(!pass_result.skipped)
    }
}

[test]
def test_benchmark_json_output(tt : T?) {
    let report = run_dastest("bench", "--bench")
    tt |> success(!empty(report.tests), "should have at least one result in tests array")
    var bench_result = find_test(report, "bench_simple")
    tt |> success(bench_result != null, "bench_simple should be in results")
    if (bench_result != null) {
        tt |> success(bench_result.passed, "benchmark should pass")
        tt |> success(bench_result.time_usec > 0, "benchmark should have positive time")
        tt |> success(!empty(bench_result.messages), "benchmark should have sub-benchmark messages")
        // Verify message contains ns/op stats
        let msg = bench_result.messages[0]
        tt |> success(find(msg, "ns/op") >= 0, "benchmark message should contain 'ns/op'")
        tt |> success(find(msg, "B/op") >= 0, "benchmark message should contain 'B/op'")
        tt |> success(find(msg, "add:") >= 0, "benchmark message should contain sub-bench name 'add'")
    }
}

[test]
def test_test_name_filter(tt : T?) {
    let report = run_dastest("pass", "--test-names test_passing")
    tt |> equal(report.total, 1)
    tt |> equal(report.passed, 1)
    tt |> success(report.tests[0].name == "test_passing")
}
