options gen2

require common public

//! Thin popen wrapper. Real logic lives in subtools/aot.das so macro
//! state from compile_file doesn't leak across MCP calls.

def do_aot(file, func_name : string; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("aot", [file, func_name, project], project_root, load_modules)
}
