options gen2

require common public

//! Dispatcher: ast_dump has two paths — expression-based (spawns its own
//! daslang.exe subprocess with a generated stub) and file-based (forwards to
//! subtools/ast_dump.das). Both isolate compile state from the MCP server.

def do_ast_dump_expression(expression, project_root : string; load_modules : array<string>) : string {
    let exe = get_daslang_exe()
    if (empty(exe)) return make_tool_result("Cannot determine daslang executable path", true)
    let script_path = make_temp_das_file()
    let code = "options gen2\noptions rtti\nrequire daslib/ast\nrequire daslib/ast_boost\n\n[export]\ndef main() \{\n    var ed <- quote({expression})\n    print(\"\{debug_expression(ed)\}\n\")\n\}\n"
    var write_ok = false
    fopen(script_path, "wb") $(fw) {
        if (fw == null) return
        write_ok = true
        fw |> fprint(code)
    }
    if (!write_ok) return make_tool_result("Cannot write temp file", true)
    var output : string
    var argv <- [exe]
    if (!empty(project_root)) {
        argv |> push("-project_root")
        argv |> push(project_root)
    }
    argv |> reserve(length(argv) + 2 * length(load_modules) + 1)
    for (lm in load_modules) {
        argv |> push("-load_module")
        argv |> push(lm)
    }
    argv |> push(script_path)
    let exit_code = run_and_capture(argv, output)
    remove(script_path)
    if (exit_code != 0) return make_tool_result("AST dump failed (exit code {exit_code}):\n{output}", true)
    return make_tool_result(output)
}

def do_ast_dump(expression, file, func_name, mode, lineinfo : string; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    if (!empty(expression)) {
        return do_ast_dump_expression(expression, project_root, load_modules)
    } elif (!empty(file)) {
        return run_mcp_subtool("ast_dump", [file, func_name, mode, lineinfo, project], project_root, load_modules)
    } else {
        return make_tool_result("Provide either 'expression' or 'file' argument", true)
    }
}
