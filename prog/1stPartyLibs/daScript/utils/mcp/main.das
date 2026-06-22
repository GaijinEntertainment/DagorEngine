// MCP server for daslang — stdio transport
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

require protocol public
require strings

[export]
def main() {
    logger_init(MCP_LOG_NAME)
    logger_info("mcp", "Starting daslang MCP server (stdio transport)")
    // detect ast-grep for grep_usage/outline tools
    let sg_check = detect_ast_grep()
    if (!empty(sg_check)) {
        ast_grep_available = true
        logger_info("mcp", "ast-grep detected ({sg_check}) — grep_usage tool enabled")
    } else {
        logger_warning("mcp", "ast-grep not found — grep_usage tool disabled")
    }

    let sin = fstdin()
    let sout = fstdout()

    while (!feof(sin)) {
        // read one line (up to 16KB per fgets call; loop for longer messages)
        let msg = build_string() $(var w) {
            while (!feof(sin)) {
                let chunk = fgets(sin)
                let len = length(chunk)
                if (len == 0) break
                if (character_at(chunk, len - 1) == '\n') { // nolint:PERF003
                    // got full line — append without the trailing newline
                    if (len > 1 && character_at(chunk, len - 2) == '\r') { // nolint:PERF003
                        write(w, slice(chunk, 0, len - 2))
                    } else {
                        write(w, slice(chunk, 0, len - 1))
                    }
                    break
                } else {
                    // buffer full, line continues — append and keep reading
                    write(w, chunk)
                }
            }
        }

        if (empty(msg)) continue

        var response = dispatch_jsonrpc(msg)
        if (!empty(response)) {
            // A JSON-RPC frame is one line; the trailing "\n" below is the only
            // delimiter. Embedded newlines mean stray output (a subtool GC-leak
            // dump, a panic backtrace, a stray print) leaked into the payload and
            // would desync the client's line framing for the rest of the session.
            // Collapse to a single line so only this one request fails, not the
            // stream, and log it loudly so the leak is diagnosable.
            if (find(response, "\n") >= 0) {
                logger_error("mcp.frame", "response contains embedded newline(s) — stray output leaked into JSON-RPC frame; collapsing to keep the stream aligned. preview: {slice(response, 0, 240)}")
                response = replace(response, "\n", "\\n")
            }
            fprint(sout, response)
            fprint(sout, "\n")
            fflush(sout)
        }

        // collect heap after each request if over threshold
        let heap_used = heap_bytes_allocated()
        let str_used = string_heap_bytes_allocated()
        if (heap_used > HEAP_COLLECT_THRESHOLD || str_used > HEAP_COLLECT_THRESHOLD) {
            logger_info("mcp.heap", "collecting — heap: {int(heap_used)}, string heap: {int(str_used)}")
            unsafe {
                heap_collect(str_used > HEAP_COLLECT_THRESHOLD, false)
            }
        }
    }
    logger_info("mcp", "stdin closed, shutting down")
}
