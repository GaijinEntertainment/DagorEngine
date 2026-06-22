options gen2

require common public

def do_run_script(file, code : string; timeout_str : string = ""; track_allocations : bool = false; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    let project_err = validate_project_arg(project)
    if (!empty(project_err)) return make_tool_result(project_err, true)
    let exe = get_daslang_exe()
    if (empty(exe)) return make_tool_result("Cannot determine daslang executable path", true)
    // if code is provided, write to temp file; otherwise resolve relative path
    var script_path = !empty(file) ? resolve_path(file) : ""
    var is_temp = false
    if (!empty(code)) {
        script_path = make_temp_das_file()
        is_temp = true
        var write_ok = false
        fopen(script_path, "wb") $(fw) {
            if (fw == null) return
            write_ok = true
            fw |> fprint(code)
        }
        if (!write_ok) return make_tool_result("Cannot write temp file: {script_path}", true)
    }
    if (empty(script_path)) return make_tool_result("Provide either 'file' or 'code' argument", true)
    let timeout_sec = empty(timeout_str) ? 30.0 : float(to_int(timeout_str))
    var output : string
    var argv <- [exe]
    if (!empty(project_root)) {
        argv |> push("-project_root")
        argv |> push(project_root)
    }
    argv |> reserve(length(argv) + 2 * length(load_modules) + 5)
    for (lm in load_modules) {
        argv |> push("-load_module")
        argv |> push(lm)
    }
    if (!empty(project)) {
        argv |> push("-project")
        argv |> push(project)
    }
    if (track_allocations) {
        argv |> push("-track-allocations")
        argv |> push("-heap-report")
    }
    argv |> push(script_path)
    let exit_code = run_and_capture(argv, output, timeout_sec)
    if (is_temp) {
        remove(script_path)
    }
    if (exit_code == popen_timed_out) return make_tool_result("Script timed out after {timeout_sec}s:\n{output}", true)
    if (exit_code != 0) return make_tool_result("Script failed (exit code {exit_code}):\n{output}", true)
    return make_tool_result(output)
}
