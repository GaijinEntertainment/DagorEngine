options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost public
require daslib/json_boost
require daslib/fio
require strings

def dastest_root() : string {
    return "{get_das_root()}/dastest"
}

def fixture_path(name : string) : string {
    return "{dastest_root()}/tests/{name}"
}

def json_out_path() : string {
    return "{get_das_root()}/.dastest_json_test.json"
}

def run_dastest_json(test_file : string; var json_content : string&; var exit_code : int&) {
    let exe = get_command_line_arguments()[0]
    let json_path = json_out_path()
    let cmd = "{exe} {dastest_root()}/dastest.das -- --test {test_file} --json-file {json_path}"
    var output : string
    unsafe {
        exit_code = popen(cmd) <| $(f) {
            if (f != null) {
                while (!feof(f)) {
                    output += fgets(f)
                }
            }
        }
    }
    json_content = ""
    fopen(json_path, "rb") <| $(f) {
        if (f != null) {
            json_content = fread(f)
        }
    }
    remove(json_path)
}

[test]
def test_json_file_passing(tt : T?) {
    tt |> run("produces valid JSON for passing tests") <| @(t : T?) {
        var json_content : string
        var exit_code : int
        run_dastest_json(fixture_path("_fixture_passing.das"), json_content, exit_code)
        t |> equal(exit_code, 0)
        t |> success(!empty(json_content), "JSON file should not be empty")

        var error : string
        var js <- read_json(json_content, error)
        t |> success(js != null, "JSON parse failed: {error}")
        if (js == null) {
            return
        }

        // Check all required fields exist
        t |> success(js?.file != null, "should have 'file' field")
        t |> success(js?.total != null, "should have 'total' field")
        t |> success(js?.passed != null, "should have 'passed' field")
        t |> success(js?.failed != null, "should have 'failed' field")
        t |> success(js?.errors != null, "should have 'errors' field")
        t |> success(js?.skipped != null, "should have 'skipped' field")
        t |> success(js?.success != null, "should have 'success' field")
        t |> success(js?.time_usec != null, "should have 'time_usec' field")
        t |> success(js?.tests != null, "should have 'tests' field")

        // Check values
        let success_val = js?.success ?? false
        t |> success(success_val, "success should be true")
        let total = js?.total ?? -1
        let passed_count = js?.passed ?? -1
        let failed_count = js?.failed ?? -1
        let errors_count = js?.errors ?? -1
        t |> equal(int(total), 3)  // test_pass_one has sub_a, test_pass_two = 3 tests total
        t |> equal(int(passed_count), 3)
        t |> equal(int(failed_count), 0)
        t |> equal(int(errors_count), 0)
    }

    tt |> run("individual test results have correct fields") <| @(t : T?) {
        var json_content : string
        var exit_code : int
        run_dastest_json(fixture_path("_fixture_passing.das"), json_content, exit_code)
        if (exit_code != 0 || empty(json_content)) {
            t |> failure("dastest did not produce JSON output")
            return
        }

        var error : string
        var js <- read_json(json_content, error)
        if (js == null) {
            t |> failure("JSON parse failed: {error}")
            return
        }

        let tests_jv = js?.tests
        t |> success(tests_jv != null, "tests field should exist")
        if (tests_jv == null) {
            return
        }

        let tests & = unsafe(tests_jv.value as _array)
        t |> equal(length(tests), 3)
        if (length(tests) < 1) {
            return
        }

        // Check first test entry has required fields
        let first = tests[0]
        t |> success(first?.name != null, "test entry should have 'name'")
        t |> success(first?.passed != null, "test entry should have 'passed'")
        t |> success(first?.skipped != null, "test entry should have 'skipped'")
        t |> success(first?.time != null, "test entry should have 'time' (renamed from time_usec)")

        // All tests should pass
        for (test_jv in tests) {
            let test_passed = test_jv?.passed ?? false
            let test_name = test_jv?.name ?? "unknown"
            t |> success(test_passed, "test '{test_name}' should have passed")
        }
    }

    tt |> run("time_usec is positive") <| @(t : T?) {
        var json_content : string
        var exit_code : int
        run_dastest_json(fixture_path("_fixture_passing.das"), json_content, exit_code)
        if (exit_code != 0 || empty(json_content)) {
            t |> failure("dastest did not produce JSON output")
            return
        }

        var error : string
        var js <- read_json(json_content, error)
        if (js == null) {
            t |> failure("JSON parse failed")
            return
        }

        let time_usec = int(js?.time_usec ?? 0)
        t |> success(time_usec > 0, "time_usec should be positive")

        let tests_jv = js?.tests
        if (tests_jv == null) {
            return
        }
        let tests & = unsafe(tests_jv.value as _array)
        for (test_jv in tests) {
            let test_time = int(test_jv?.time ?? 0)
            let test_name = test_jv?.name ?? "unknown"
            t |> success(test_time >= 0, "test '{test_name}' time should be >= 0")
        }
    }
}

[test]
def test_json_file_not_created_without_flag(tt : T?) {
    tt |> run("no JSON file without --json-file flag") <| @(t : T?) {
        let json_path = json_out_path()
        remove(json_path)
        let exe = get_command_line_arguments()[0]
        let fix_path = fixture_path("_fixture_passing.das")
        let cmd = "{exe} {dastest_root()}/dastest.das -- --test {fix_path}"
        unsafe {
            popen(cmd) <| $(f) {
                if (f != null) {
                    while (!feof(f)) {
                        fgets(f)
                    }
                }
            }
        }
        let st = stat(json_path)
        t |> success(!st.is_reg, "JSON file should not exist without --json-file flag")
    }
}
