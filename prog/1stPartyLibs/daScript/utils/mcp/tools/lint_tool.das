options gen2

require common public

//! Thin popen wrapper. Real logic lives in subtools/lint_tool.das so
//! macro state from compile_file doesn't leak across MCP calls.

def do_lint(file : string; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("lint_tool", [file, project], project_root, load_modules)
}
