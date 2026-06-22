options gen2

require common public
require daslib/fio

def do_run_test(file : string; project : string = ""; project_root : string = ""; timeout_str : string = ""; json : bool = false; failures_only : bool = false; load_modules : array<string> = array<string>()) : string {
    let exe = get_daslang_exe()
    if (empty(exe)) return make_tool_result("Cannot determine daslang executable path", true)
    let test_file = resolve_path(file)
    let dastest_main = "{get_das_root()}/dastest/dastest.das"
    // -project_root and -load_module are daslang CLI flags and must come before the script path.
    var argv <- [exe]
    if (!empty(project_root)) {
        argv |> push("-project_root")
        argv |> push(project_root)
    }
    argv |> reserve(length(argv) + 2 * length(load_modules) + 7)
    for (lm in load_modules) {
        argv |> push("-load_module")
        argv |> push(lm)
    }
    argv |> push(dastest_main)
    argv |> push("--")
    argv |> push("--test")
    argv |> push(test_file)
    if (!empty(project)) {
        argv |> push("--project")
        argv |> push(project)
    }
    if (failures_only) {
        argv |> push("--failures-only")
    }
    let timeout_sec = empty(timeout_str) ? 120.0 : float(to_int(timeout_str))
    if (json) {
        // Use dastest --json-file for native JSON output
        let json_path = "{get_das_root()}/.dastest_result.json"
        argv |> push("--json-file")
        argv |> push(json_path)
        var output : string
        let exit_code = run_and_capture(argv, output, timeout_sec)
        if (exit_code == popen_timed_out) return make_tool_result("\{\"file\":\"{file}\",\"success\":false,\"output\":\"Timed out after {timeout_sec}s\"}", true)
        // Read the JSON file written by dastest
        var json_content = ""
        fopen(json_path, "rb") $(f) {
            if (f != null) {
                json_content = fread(f)
            }
        }
        remove(json_path)
        // dastest crashed before writing JSON — return raw output
        if (empty(json_content)) return make_tool_result("\{\"file\":\"{file}\",\"success\":false,\"output\":\"{output}\"}", true)
        let is_error = find(json_content, "\"success\":false") >= 0
        return make_tool_result(json_content, is_error)
    }
    // Text mode
    var output : string
    let exit_code = run_and_capture(argv, output, timeout_sec)
    if (exit_code == popen_timed_out) return make_tool_result("Tests timed out after {timeout_sec}s:\n{output}", true)
    if (exit_code != 0) return make_tool_result("Tests FAILED (exit code {exit_code}):\n{output}", true)
    return make_tool_result(output)
}
