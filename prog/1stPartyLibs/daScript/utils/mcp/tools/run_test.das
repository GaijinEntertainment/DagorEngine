options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/fio

def do_run_test(file : string; project : string = ""; timeout_str : string = ""; json : bool = false; failures_only : bool = false) : string {
    let exe = get_daslang_exe()
    if (empty(exe)) {
        return make_tool_result("Cannot determine daslang executable path", true)
    }
    let test_file = resolve_path(file)
    var cmd = "{exe} {get_das_root()}/dastest/dastest.das -- --test {test_file}"
    if (!empty(project)) {
        cmd = "{cmd} --project {project}"
    }
    if (failures_only) {
        cmd = "{cmd} --failures-only"
    }
    let timeout_sec = empty(timeout_str) ? 120.0 : float(to_int(timeout_str))
    if (json) {
        // Use dastest --json-file for native JSON output
        let json_path = "{get_das_root()}/.dastest_result.json"
        cmd = "{cmd} --json-file {json_path}"
        var output : string
        let exit_code = run_and_capture(cmd, output, timeout_sec)
        if (exit_code == popen_timed_out) {
            return make_tool_result("\{\"file\":\"{file}\",\"success\":false,\"output\":\"Timed out after {timeout_sec}s\"}", true)
        }
        // Read the JSON file written by dastest
        var json_content = ""
        fopen(json_path, "rb") <| $(f) {
            if (f != null) {
                json_content = fread(f)
            }
        }
        remove(json_path)
        if (empty(json_content)) {
            // dastest crashed before writing JSON — return raw output
            return make_tool_result("\{\"file\":\"{file}\",\"success\":false,\"output\":\"{output}\"}", true)
        }
        let is_error = find(json_content, "\"success\":false") >= 0
        return make_tool_result(json_content, is_error)
    }
    // Text mode
    var output : string
    let exit_code = run_and_capture(cmd, output, timeout_sec)
    if (exit_code == popen_timed_out) {
        return make_tool_result("Tests timed out after {timeout_sec}s:\n{output}", true)
    }
    if (exit_code != 0) {
        return make_tool_result("Tests FAILED (exit code {exit_code}):\n{output}", true)
    }
    return make_tool_result(output)
}
