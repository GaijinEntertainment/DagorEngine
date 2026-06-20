options gen2

require common public

//! Thin popen wrapper. Real logic lives in subtools/find_symbol.das so
//! macro state from compile_file doesn't leak across MCP calls.

def do_find_symbol(query : string; kind : string = ""; file : string = ""; project : string = ""; project_root : string = ""; with_cpp_source : bool = false; cpp_dirs : string = ""; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("find_symbol", [query, kind, file, project, with_cpp_source ? "true" : "false", cpp_dirs], project_root, load_modules)
}
