options gen2

require common public

//! Thin popen wrapper. Real logic lives in subtools/describe_type.das so
//! macro state from compile_file doesn't leak across MCP calls.

def do_describe_type(name, module_name : string; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("describe_type", [name, module_name, project], project_root, load_modules)
}
