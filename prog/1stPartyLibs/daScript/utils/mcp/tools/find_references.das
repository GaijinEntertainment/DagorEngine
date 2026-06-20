options gen2

require common public

//! Thin popen wrapper. Real logic lives in subtools/find_references.das so
//! macro state from compile_file doesn't leak across MCP calls AND so
//! any shared module the compiled program requires (dasModuleImgui, etc.)
//! loads + unloads on the subtool's process lifecycle, not the MCP server's.

def public do_find_references(file : string; line_str, col_str, scope_str, no_opt_str : string; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("find_references", [file, line_str, col_str, scope_str, no_opt_str, project], project_root, load_modules)
}
