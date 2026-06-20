options gen2

require common public

//! Thin popen wrapper. Real logic lives in subtools/list_module_api.das so
//! macro state from compile_file doesn't leak across MCP calls.

def do_list_module_api(module_name : string; filter : string = ""; section : string = ""; compact_str : string = ""; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("list_module_api", [module_name, filter, section, compact_str, project], project_root, load_modules)
}
