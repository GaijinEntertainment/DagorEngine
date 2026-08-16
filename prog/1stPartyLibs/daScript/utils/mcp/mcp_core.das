// Provider-neutral MCP protocol core.
//
// Generic JSON-RPC framing + the data-driven tool registry (array<ToolDef>)
// with no assumptions about what the tools do or how they resolve modules.
// handle_tools_list / handle_tools_call iterate a registry the entry point
// builds; the McpServer config carries the advertised serverInfo plus the
// names of any extra arguments the server wants extracted for every tool.
// A das compiler server points those at the module-resolution args
// (project / project_root / load_modules); a plain server leaves them empty.
// The registrations and the do_* handlers live in the server's own modules
// (see protocol_core.das for the daslang layer), never here.

options gen2
options rtti

module mcp_core

require daslib/json
require daslib/json_boost public
require daslib/jsonrpc
require daslib/jobque_boost
require strings
require daslib/strings_boost
require daslib/fio
require daslib/logger public

let HEAP_COLLECT_THRESHOLD = 1024ul * 1024ul  // 1 MB
let MCP_LOG_NAME = "mcp_server"

// ── Logging ───────────────────────────────────────────────────────────
//
// Thin wrappers around daslib/logger. Categories are dotted so consumers
// can filter (e.g. `logger_set_category_level("mcp.tool", LOG_DEBUG)`).
//
// CRITICAL: MCP uses stdio JSON-RPC. ANY write to the parent process's
// stdout corrupts protocol framing. Every diagnostic must go to the log
// file (or stderr fallback) — never stdout. `logger_install_hook()` in
// main() enforces this for accidental print() / to_log() calls — a
// single global install covers main + every new_thread worker.

var ast_grep_available : bool = false

def log_request(method, body : string) {
    let truncated = length(body) > 500 ? slice(body, 0, 500) + "..." : body
    logger_info("mcp.req", "{method} {truncated}")
}

def log_response(method : string; elapsed_sec : double) {
    let ms = int(elapsed_sec * 1000.0lf)
    logger_info("mcp.res", "{method} ({ms}ms)")
}

def log_tool(tool_name : string; args : string) {
    logger_info("mcp.tool", "{tool_name} {args}")
}

def log_tool_done(tool_name : string; elapsed_sec : double; is_error : bool) {
    let ms = int(elapsed_sec * 1000.0lf)
    let status = is_error ? "ERR" : "OK"
    if (is_error) {
        logger_warning("mcp.tool", "{tool_name} {status} ({ms}ms)")
    } else {
        logger_info("mcp.tool", "{tool_name} {status} ({ms}ms)")
    }
}

def log_error(msg : string) {
    logger_error("mcp", msg)
}

// ── MCP protocol structs ─────────────────────────────────────────────

struct ServerInfo {
    name : string
    version : string
}

struct ToolsCapability {}

struct Capabilities {
    tools : ToolsCapability
}

struct InitializeResult {
    protocolVersion : string
    capabilities : Capabilities
    serverInfo : ServerInfo
}

// The server configuration threaded through dispatch. `info` is the advertised
// serverInfo. The extra-arg names are the one place a server declares the
// arguments it wants pulled out of every tool call on top of the positional
// arg_names: str_extra_args (up to two names) fill the handler's extra1/extra2
// slots, arr_extra_arg (one name) fills extra_list. A server that needs none
// leaves them empty and its handlers just ignore those slots.
struct McpServer {
    info : ServerInfo
    str_extra_args : array<string>
    arr_extra_arg : string
    // Directory for per-call progress files. Empty disables progress: a tool's
    // wants_progress is honoured only when the server also names a dir here.
    progress_dir : string
}

// Optional post-processing applied to every tool result before it is wrapped
// in a JSON-RPC response. A layer installs one to annotate results (the
// daslang layer installs its cross-tree guard here); a plain server leaves it
// unset, so the core stays provider-agnostic. (result, raw args) -> result.
var tool_result_filter : function<(result : string; args : JsonValue?) : string>

def install_tool_result_filter(f : function<(result : string; args : JsonValue?) : string>) {
    tool_result_filter = f
}

// ── MCP protocol handlers ─────────────────────────────────────────────
// JSON-RPC envelope helpers (response, error, serialize_id) live in daslib/jsonrpc.

def handle_initialize(server_info : ServerInfo; id_json : string) : string {
    let result = InitializeResult(
        protocolVersion = "2025-11-25",
        capabilities = Capabilities(),
        serverInfo = server_info
    )
    return jsonrpc::response(id_json, sprint_json(result, false))
}

// Declared dependency-first (PropertySchema -> InputSchema -> MakeFileTool):
// the AOT C++ emitter writes structs in declaration order, and a by-value
// member needs its type complete first, so MakeFileTool (which holds an
// InputSchema by value) must come after InputSchema, which must come after
// PropertySchema (held by value inside its `properties` table).
struct PropertySchema {
    @rename = "type" _type : string
    description : string
    @optional items : PropertySchema?
}

struct InputSchema {
    @rename = "type" _type : string
    properties : table<string; PropertySchema>
    required : array<string>
}

struct MakeFileTool {
    name : string
    description : string
    inputSchema : InputSchema
}

def make_tool(name, description : string; var props : table<string; PropertySchema>; var req : array<string>) : MakeFileTool {
    return MakeFileTool(
        name = name,
        description = description,
        inputSchema = InputSchema(
            _type = "object",
            properties <- props,
            required <- req
        )
    )
}

// ── Tool registry ────────────────────────────────────────────────────
//
// A ToolHandler maps the extracted positional args (arg1..arg6) plus the
// server's declared extra args (extra1/extra2 strings, extra_list array) to a
// `do_*` call; it's a no-capture function pointer so it copies freely across
// the new_thread boundary in run_tool (same program -> same function table).
// The extra slots are neutral: the daslang server uses them for project /
// project_root / load_modules, a plain server ignores them.

typedef ToolHandler = function<(arg1 : string; arg2 : string; arg3 : string; arg4 : string; arg5 : string; arg6 : string; extra1 : string; extra2 : string; extra_list : array<string>) : string>

struct ToolDef {
    tool : MakeFileTool                 // advertised name + description + inputSchema
    required : array<string>            // every name here must be non-empty, else -32602
    any_required : array<string>        // at least one must be non-empty, else -32602 (empty = no constraint)
    arg_names : array<string>           // JSON arg names mapped positionally to arg1..arg6
    needs_ast_grep : bool               // only advertised when ast-grep is available
    main_thread : bool                  // run on the main thread (live_* / shutdown), not a worker
    wants_progress : bool               // stream notifications/progress when the client sends a token
    handler : ToolHandler
}

def find_tool(reg : array<ToolDef>; name : string) : int {
    for (i in range(length(reg))) {
        if (reg[i].tool.name == name) return i
    }
    return -1
}

def handle_tools_list(reg : array<ToolDef>; id_json : string) : string {
    // Serialize each MakeFileTool in place rather than cloning into a
    // ToolsListResult — PropertySchema carries a pointer field (`items`) that
    // can't be cloned from const, and the list is static so a fresh build per
    // (rare) call is fine.
    let json = build_string() $(var w) {
        w |> write("\{\"tools\":[")
        var first = true
        for (td in reg) {
            if (td.needs_ast_grep && !ast_grep_available) continue
            if (!first) w |> write(",")
            first = false
            w |> write(sprint_json(td.tool, false))
        }
        w |> write("]\}")
    }
    return jsonrpc::response(id_json, json)
}

// ── Tool dispatch (runs in cloned context via new_thread) ────────────

struct ToolMessage {
    text : string
}

def get_string_arg(args : JsonValue?; name : string) : string {
    if (args == null || !(args.value is _object)) return ""
    let v = (args.value as _object)?[name] ?? null
    if (v == null) return ""
    if (v.value is _string) return v.value as _string
    if (v.value is _bool) return v.value as _bool ? "true" : "false"
    // A tool may declare an integer/number arg (e.g. a bookmark index);
    // stringify it so the required-arg check and positional extraction see the
    // value instead of reading a JSON number as absent. Integers land in
    // _longint (the parser's extension), non-integers in _number.
    if (v.value is _longint) return "{v.value as _longint}"
    if (v.value is _number) {
        let d = v.value as _number
        let i = int64(d)
        return double(i) == d ? "{i}" : "{d}"
    }
    return ""
}

def get_string_array_arg(args : JsonValue?; name : string) : array<string> {
    var result : array<string>
    if (args == null || !(args.value is _object)) return <- result
    let v = (args.value as _object)?[name] ?? null
    if (v == null || !(v.value is _array)) return <- result
    result |> reserve(length(v.value as _array))
    // Filter out non-string and empty-string elements so callers don't have
    // to re-validate. Empty path entries would otherwise resolve to das_root
    // downstream (resolve_path("") -> get_das_root()) and load it as a module.
    for (elem in v.value as _array) {
        if (elem != null && elem.value is _string) {
            let s = elem.value as _string
            if (!empty(s)) {
                result |> push(s)
            }
        }
    }
    return <- result
}

def run_tool(handler : ToolHandler; tool_name : string; main_thread : bool;
             arg1, arg2, arg3, arg4, arg5, arg6, extra1, extra2 : string;
             var extra_list : array<string>) : string {
    log_tool(tool_name, "{arg1} {arg2} {arg3} {arg4} {arg5} {arg6}")
    let t0 = get_clock()
    var result : string
    // Live tools (and shutdown) run on the main thread — they use system() and
    // sleep() which don't work well from new_thread, and they don't need
    // compilation context.
    if (main_thread) {
        result = invoke(handler, arg1, arg2, arg3, arg4, arg5, arg6, extra1, extra2, extra_list)
    } else {
        with_channel(1) $(ch) {
            new_thread() <| @ capture(<- extra_list) () {
                // logger_install_hook in main() registered a GLOBAL debug
                // agent — worker contexts inherit it automatically, no
                // re-install needed here. Worker print()/to_log() routes
                // through the agent's owning context (main) into the
                // shared log file.
                let tool_result = invoke(handler, arg1, arg2, arg3, arg4, arg5, arg6, extra1, extra2, extra_list)
                ch |> push_clone(ToolMessage(text = tool_result))
                ch |> notify_and_release()
            }
            for_each_clone(ch) $(msg : ToolMessage#) {
                result = clone_string(msg.text)
            }
        }
    }
    let is_error = find(result, "\"isError\":true") >= 0
    log_tool_done(tool_name, get_clock() - t0, is_error)
    return result
}

// ── Universal progress ───────────────────────────────────────────────
//
// A tool that opts in (wants_progress) and whose server names a progress_dir
// gets, on any call carrying an MCP _meta.progressToken, a unique temp file the
// downstream command writes its current stage into. The core owns that file's
// whole lifetime: it hands the path to the handler (mcp_progress_path), runs the
// command on a worker thread while the main thread polls the file every
// PROGRESS_POLL_MS and emits a notifications/progress with a monotonic count,
// then deletes it. With no token (or no opt-in) run_with_progress collapses to a
// plain run_and_capture — no file, no worker, no polling — so untouched tools and
// unaware clients behave exactly as before.

let PROGRESS_POLL_MS = 5000

var current_progress_file : string    // "" when the current call streams no progress
var current_progress_token : string   // raw JSON of the progressToken, "" when none
var current_tool_name : string        // current call's tool; prefixes every progress message
var progress_count : int               // monotonic progress value for the current call
var progress_seq : int                 // disambiguates progress file names within a process

struct RunResult {
    code : int
    output : string
}

// The file the downstream command should write progress into, or "" when this
// call streams none. The handler forwards it (e.g. as a --progress-file
// argument) only when non-empty.
def mcp_progress_path() : string {
    return current_progress_file
}

// The token lives at params._meta.progressToken and is a string or integer per
// the MCP spec. Return its JSON verbatim (so the same type is echoed back), or
// "" when absent or not a scalar — write_json keeps a scalar on one line.
def extract_progress_token(params : JsonValue?) : string {
    if (params == null || !(params.value is _object)) return ""
    let meta = (params.value as _object)?["_meta"] ?? null
    if (meta == null || !(meta.value is _object)) return ""
    let tok = (meta.value as _object)?["progressToken"] ?? null
    if (tok == null) return ""
    if (tok.value is _string || tok.value is _longint || tok.value is _number) {
        return write_json(tok)
    }
    return ""
}

def make_progress_path(dir : string) : string {
    mkdir_rec(dir)
    progress_seq++
    let ticks = ref_time_ticks()
    return path_join(dir, "mcp_progress_{ticks}_{progress_seq}.txt")
}

// Fold a progress line into a safe JSON string body: escape the backslash and
// quote JSON needs, drop the whitespace that would split the one-line frame.
def sanitize_message(s : string) : string {
    var r = replace(s, "\\", "\\\\")
    r = replace(r, "\"", "\\\"")
    r = replace(r, "\r", "")
    r = replace(r, "\n", " ")
    r = replace(r, "\t", " ")
    return r
}

def emit_progress(message : string) {
    if (empty(current_progress_token)) return
    progress_count++
    // The client shows only the server name on the live call, so name the tool
    // in the message body itself.
    let labeled = empty(current_tool_name) ? message : "{current_tool_name}: {message}"
    let frame = (
        "\{\"jsonrpc\":\"2.0\",\"method\":\"notifications/progress\"," +
        "\"params\":\{\"progressToken\":{current_progress_token}," +
        "\"progress\":{progress_count}," +
        "\"message\":\"{sanitize_message(labeled)}\"\}\}")
    let sout = fstdout()
    fprint(sout, frame)
    fprint(sout, "\n")
    fflush(sout)
    logger_info("mcp.progress", "{progress_count} {labeled}")
}

def emit_progress_from_file() {
    let content = fread(current_progress_file)
    emit_progress(empty(content) ? "working" : content)
}

// Run an external command, streaming progress when the current call set one up.
// A worker runs the blocking capture; the main thread polls the progress file
// every PROGRESS_POLL_MS, emitting a tick each interval, then takes the worker's
// result. Popping that single result is the whole teardown — the worker's
// notify_and_release already balanced its channel ref, so an extra release here
// would under-count and abort the run.
def run_with_progress(argv : array<string>; timeout_sec : float; var output : string&) : int {
    if (empty(current_progress_file) || empty(current_progress_token)) {
        return run_and_capture(argv, output, timeout_sec)
    }
    var command : array<string>
    command := argv
    var err : string
    remove(current_progress_file, err)
    progress_count = 0
    emit_progress("starting")
    var code = 0
    var captured : string
    with_channel(1) $(ch) {
        new_thread() <| @ capture(<- command) () {
            var out : string
            let c = run_and_capture(command, out, timeout_sec)
            ch |> push_clone(RunResult(code = c, output = out))
            ch |> notify_and_release()
        }
        var got = false
        while (!got) {
            got = pop_with_timeout_clone(ch, PROGRESS_POLL_MS) $(r : RunResult#) {
                code = r.code
                captured = clone_string(r.output)
            }
            if (!got) {
                emit_progress_from_file()
            }
        }
    }
    output = captured
    remove(current_progress_file, err)
    return code
}

def handle_tools_call(server : McpServer; reg : array<ToolDef>; id_json : string; params : JsonValue?) : string {
    let tool_name_val = params?.name
    let args = params?.arguments
    if (tool_name_val == null || !(tool_name_val.value is _string)) return jsonrpc::error(id_json, -32602, "missing or non-string tool name")
    let name = tool_name_val.value as _string
    let idx = find_tool(reg, name)
    if (idx < 0) return jsonrpc::error(id_json, -32602, "unknown tool: {name}")
    // ast-grep tools are hidden from tools/list when ast-grep is missing; reject
    // a direct call too, rather than letting do_* build a command off an empty path.
    if (reg[idx].needs_ast_grep && !ast_grep_available) return jsonrpc::error(id_json, -32602, "tool '{name}' requires ast-grep (sg), which was not found on PATH")
    // AND-required: every listed arg must be present.
    for (rn in reg[idx].required) {
        if (empty(get_string_arg(args, rn))) return jsonrpc::error(id_json, -32602, "missing '{rn}' argument")
    }
    // OR-required: at least one of the listed args must be present.
    if (!empty(reg[idx].any_required)) {
        var has_any = false
        for (an in reg[idx].any_required) {
            if (!empty(get_string_arg(args, an))) {
                has_any = true
                break
            }
        }
        if (!has_any) return jsonrpc::error(id_json, -32602, "missing input — provide one of: {join(reg[idx].any_required, ", ")}")
    }
    let cnt = length(reg[idx].arg_names)
    let arg1 = cnt > 0 ? get_string_arg(args, reg[idx].arg_names[0]) : ""
    let arg2 = cnt > 1 ? get_string_arg(args, reg[idx].arg_names[1]) : ""
    let arg3 = cnt > 2 ? get_string_arg(args, reg[idx].arg_names[2]) : ""
    let arg4 = cnt > 3 ? get_string_arg(args, reg[idx].arg_names[3]) : ""
    let arg5 = cnt > 4 ? get_string_arg(args, reg[idx].arg_names[4]) : ""
    let arg6 = cnt > 5 ? get_string_arg(args, reg[idx].arg_names[5]) : ""
    let handler = reg[idx].handler
    let main_thread = reg[idx].main_thread
    // Extra args the server declared: string slots then the one array slot.
    // Absent names extract to "" / an empty array, so a server that declares
    // none passes empty extras and its handlers ignore them.
    let ns = length(server.str_extra_args)
    let extra1 = ns > 0 ? get_string_arg(args, server.str_extra_args[0]) : ""
    let extra2 = ns > 1 ? get_string_arg(args, server.str_extra_args[1]) : ""
    var extra_list : array<string>
    if (!empty(server.arr_extra_arg)) {
        extra_list <- get_string_array_arg(args, server.arr_extra_arg)
    }
    // Universal progress: armed only when the tool opts in, the server names a
    // progress dir, and the client supplied a scalar _meta.progressToken. The
    // handler reads these back through mcp_progress_path / run_with_progress.
    current_progress_token = ""
    current_progress_file = ""
    current_tool_name = ""
    if (reg[idx].wants_progress && !empty(server.progress_dir)) {
        let token = extract_progress_token(params)
        if (!empty(token)) {
            current_progress_token = token
            current_progress_file = make_progress_path(server.progress_dir)
            current_tool_name = name
        }
    }
    let resp = run_tool(handler, name, main_thread, arg1, arg2, arg3, arg4, arg5, arg6, extra1, extra2, extra_list)
    current_progress_token = ""
    current_progress_file = ""
    current_tool_name = ""
    let filtered = tool_result_filter != null ? invoke(tool_result_filter, resp, args) : resp
    return jsonrpc::response(id_json, filtered)
}

// ── JSON-RPC dispatcher ──────────────────────────────────────────────

def dispatch_one(server : McpServer; reg : array<ToolDef>; req : jsonrpc::ParsedRequest; body : string) : string {
    // Malformed entry — parser already built the envelope. Notifications skip the wire.
    if (!empty(req.error_envelope)) {
        if (!req.is_notification) log_error(req.error_envelope)
        return req.is_notification ? "" : req.error_envelope
    }
    let id_json = req.id_str
    let method = req.method
    log_request(method, body)
    let t0 = get_clock()
    var result : string
    if (method == "initialize") {
        result = handle_initialize(server.info, id_json)
    } elif (method == "notifications/initialized" || method == "initialized") {
        return ""
    } elif (method == "tools/list") {
        result = handle_tools_list(reg, id_json)
    } elif (method == "tools/call") {
        result = handle_tools_call(server, reg, id_json, req.params)
    } elif (method == "ping") {
        result = jsonrpc::response(id_json, "\{\}")
    } else {
        log_error("method not found: {method}")
        result = jsonrpc::error(id_json, jsonrpc::METHOD_NOT_FOUND, "method not found: {method}")
    }
    log_response(method, get_clock() - t0)
    return req.is_notification ? "" : result
}

def dispatch_jsonrpc(server : McpServer; reg : array<ToolDef>; body : string) : string {
    let pb = jsonrpc::parse_batch(body)
    if (!empty(pb.framing_error)) {
        log_error(pb.framing_error)
        return pb.framing_error
    }
    if (!pb.is_batch) return dispatch_one(server, reg, pb.requests[0], body)
    // §6 batch: collect non-empty per-entry responses, wrap as JSON array; "" if all-notifications.
    var parts : array<string>
    parts |> reserve(length(pb.requests))
    for (req in pb.requests) {
        let r = dispatch_one(server, reg, req, body)
        if (!empty(r)) parts |> push(r)
    }
    if (empty(parts)) return ""
    return build_string() $(var w) {
        write_char(w, '[')
        var first = true
        for (p in parts) {
            if (!first) write_char(w, ',')
            first = false
            write(w, p)
        }
        write_char(w, ']')
    }
}

// ── stdio server loop ────────────────────────────────────────────────
//
// Shared by every entry point: read newline-delimited JSON-RPC from stdin,
// dispatch against `reg`, write one-line responses to stdout. The entry point
// owns logger_init + ast-grep detection + registry assembly, then calls this.

def serve_stdio(server : McpServer; reg : array<ToolDef>) {
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

        var response = dispatch_jsonrpc(server, reg, msg)
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
