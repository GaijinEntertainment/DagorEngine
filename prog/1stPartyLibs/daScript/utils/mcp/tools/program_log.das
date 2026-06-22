options gen2

require common public

//! Thin popen wrapper. Real logic lives in subtools/program_log.das so
//! macro state from compile_file doesn't leak across MCP calls AND so
//! any shared module the compiled program requires (dasModuleImgui, etc.)
//! loads + unloads on the subtool's process lifecycle, not the MCP server's.

def do_program_log(file, func_name : string; project : string = ""; project_root : string = ""; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("program_log", [file, func_name, project], project_root, load_modules)
}
