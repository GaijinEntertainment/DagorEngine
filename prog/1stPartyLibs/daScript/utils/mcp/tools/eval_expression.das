options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/strings_boost

def do_eval_expression(expression, req_modules : string) : string {
    if (empty(expression)) {
        return make_tool_result("missing 'expression' argument", true)
    }
    let exe = get_daslang_exe()
    if (empty(exe)) {
        return make_tool_result("Cannot determine daslang executable path", true)
    }
    let stub_path = make_temp_das_file()
    // build script source
    let script = build_string() <| $(var w) {
        write(w, "options gen2\n")
        if (!empty(req_modules)) {
            var req_list <- split(req_modules, ",")
            for (r in req_list) {
                let trimmed = strip(r)
                if (!empty(trimmed)) {
                    write(w, "require {trimmed}\n")
                }
            }
        }
        write(w, "[export]\ndef main() \{\n")
        write(w, "    let _res_ = ")
        write(w, expression)
        write(w, "\n    print(\"\{_res_\}\\n\")\n")
        write(w, "\}\n")
    }
    var write_ok = false
    fopen(stub_path, "wb") <| $(fw) {
        if (fw == null) {
            return
        }
        write_ok = true
        fw |> fwrite(script)
    }
    if (!write_ok) {
        return make_tool_result("Cannot write temp file: {stub_path}", true)
    }
    var output : string
    let exit_code = run_and_capture("{exe} {stub_path}", output)
    remove(stub_path)
    if (exit_code != 0) {
        return make_tool_result("Evaluation failed (exit code {exit_code}):\n{output}", true)
    }
    return make_tool_result(strip(output))
}
