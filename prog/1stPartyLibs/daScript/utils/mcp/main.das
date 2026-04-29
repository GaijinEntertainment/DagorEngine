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
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require protocol public

[export]
def main() {
    log_to_stderr = true
    log_path = "{get_das_root()}/{LOG_FILE}"
    log_open()
    log_write("Starting daslang MCP server (stdio transport)")
    // detect ast-grep for grep_usage/outline tools
    let sg_check = detect_ast_grep()
    if (!empty(sg_check)) {
        ast_grep_available = true
        log_write("ast-grep detected ({sg_check}) — grep_usage tool enabled")
    } else {
        log_write("WARNING: ast-grep not found — grep_usage tool disabled")
    }

    let sin = fstdin()
    let sout = fstdout()

    while (!feof(sin)) {
        // read one line (up to 16KB per fgets call; loop for longer messages)
        var msg = build_string() <| $(var w) {
            while (!feof(sin)) {
                let chunk = fgets(sin)
                let len = length(chunk)
                if (len == 0) {
                    break
                }
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

        if (empty(msg)) {
            continue
        }

        let response = dispatch_jsonrpc(msg)
        if (!empty(response)) {
            fprint(sout, response)
            fprint(sout, "\n")
            fflush(sout)
        }

        // collect heap after each request if over threshold
        let heap_used = heap_bytes_allocated()
        let str_used = string_heap_bytes_allocated()
        if (heap_used > HEAP_COLLECT_THRESHOLD || str_used > HEAP_COLLECT_THRESHOLD) {
            log_write("HEAP collecting — heap: {int(heap_used)}, string heap: {int(str_used)}")
            unsafe {
                heap_collect(str_used > HEAP_COLLECT_THRESHOLD, false)
            }
        }
    }
    log_write("stdin closed, shutting down")
}
