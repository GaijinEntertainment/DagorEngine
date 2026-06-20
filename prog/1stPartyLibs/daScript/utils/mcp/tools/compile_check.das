options gen2
options indenting = 4

require common public

//! `compile_check` MCP wrapper. Forwards to subtools/compile_check.das so
//! every call gets a fresh daslang.exe — no stale [function_macro] /
//! [call_macro] state across calls. The subtool's stdout (already a
//! make_tool_result(...) JSON envelope) is returned verbatim.

def do_compile_check(file : string; project : string = ""; project_root : string = ""; json : bool = false; load_modules : array<string> = array<string>()) : string {
    return run_mcp_subtool("compile_check", [file, project, json ? "true" : "false"], project_root, load_modules)
}
