options gen2

require common public

//! Thin popen wrapper. Real logic lives in subtools/goto_definition.das so
//! macro state from compile_file doesn't leak across MCP calls AND so any
//! shared module the compiled program requires (dasModuleImgui, etc.)
//! loads + unloads on the subtool's process lifecycle, not the MCP server's.

def public do_goto_definition(file : string; line_str, col_str : string; no_opt_str : string = ""; project : string = ""; project_root : string = ""; with_cpp_source : bool = false; cpp_dirs : string = ""; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("goto_definition",
        [file, line_str, col_str, no_opt_str, with_cpp_source ? "true" : "false", cpp_dirs, project],
        project_root, load_modules)
}
