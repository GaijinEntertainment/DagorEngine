// C++-only MCP server for daslang — stdio transport (cpp/agnostic subset)
//
// Exposes only the curated cpp/agnostic tool surface — ast-grep-backed .das +
// .cpp search/outline and the compile-DB-driven C++ build tools — plus the
// shared shutdown control. None of the daslang compiler-backed tools
// (compile_check, find_symbol, AOT, lint, live_*) are registered, so a C++-only
// consumer gets a focused tool list. It shares protocol_core's dispatch with
// main.das; only the registry differs.
//
// Run:   daslang.exe utils/mcp/cpp_main.das
// Config in .mcp.json:
//   "mcpServers": { "daslang-cpp": { "command": "daslang.exe", "args": ["utils/mcp/cpp_main.das"] } }

options gen2
options persistent_heap
options gc
options multiple_contexts
options rtti

require protocol_core public
require registry_cpp public

[export]
def main() {
    logger_init("mcp_cpp_server")
    logger_info("mcp", "Starting daslang cpp MCP server (stdio transport)")
    // detect ast-grep for grep_usage/outline/cpp_* tools
    let sg_check = detect_ast_grep()
    if (!empty(sg_check)) {
        ast_grep_available = true
        logger_info("mcp", "ast-grep detected ({sg_check}) — grep_usage / outline / cpp_grep_usage / cpp_find_symbol / cpp_outline / cpp_goto_definition enabled")
    } else {
        logger_warning("mcp", "ast-grep not found — grep_usage / outline / cpp_grep_usage / cpp_find_symbol / cpp_outline / cpp_goto_definition disabled (cpp_compile_check / cpp_build_info / cpp_format_file are unaffected)")
    }

    var reg : array<ToolDef>
    build_cpp_tools(reg)
    register_shutdown(reg)

    serve_stdio(das_mcp_server("daslang-cpp", "0.1.0"), reg)
}
