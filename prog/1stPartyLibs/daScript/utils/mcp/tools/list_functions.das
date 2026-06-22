options gen2

require common public

//! Thin popen wrapper. Real logic lives in subtools/list_functions.das so
//! macro state from compile_file doesn't leak across MCP calls.

def do_list_functions(file : string; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("list_functions", [file, project], project_root, load_modules)
}
