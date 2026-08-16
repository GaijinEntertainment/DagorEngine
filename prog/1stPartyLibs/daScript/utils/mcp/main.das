// MCP server for daslang — stdio transport (full tool set)
//
// Reads JSON-RPC from stdin, writes responses to stdout (newline-delimited).
// No dasHV dependency — Claude Code manages the process lifecycle.
//
// Run:   daslang.exe utils/mcp/main.das
// Config in .mcp.json:
//   "mcpServers": { "daslang": { "command": "daslang.exe", "args": ["utils/mcp/main.das"] } }

options gen2
options persistent_heap
options gc
options multiple_contexts
options rtti

require protocol_core public
require registry_das public
require registry_cpp public

[export]
def main() {
    logger_init(MCP_LOG_NAME)
    logger_info("mcp", "Starting daslang MCP server (stdio transport)")
    // detect ast-grep for grep_usage/outline/cpp_* tools
    let sg_check = detect_ast_grep()
    if (!empty(sg_check)) {
        ast_grep_available = true
        logger_info("mcp", "ast-grep detected ({sg_check}) — grep_usage / outline / cpp_grep_usage / cpp_find_symbol / cpp_outline / cpp_goto_definition enabled")
    } else {
        logger_warning("mcp", "ast-grep not found — grep_usage / outline / cpp_grep_usage / cpp_find_symbol / cpp_outline / cpp_goto_definition disabled (cpp_compile_check / cpp_build_info / cpp_format_file are unaffected)")
    }

    var reg : array<ToolDef>
    build_das_tools(reg)
    build_cpp_tools(reg)
    register_shutdown(reg)

    serve_stdio(das_mcp_server("daslang", "0.1.0"), reg)
}
