// Shared MCP protocol logic for daslang MCP server
// Used by both HTTP (main.das) and stdio (main_stdio.das) transports

options gen2
options rtti
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/json
require daslib/json_boost public
require daslib/jobque_boost
require strings
require daslib/fio

require tools/compile_check public
require tools/list_functions public
require tools/list_types public
require tools/run_test public
require tools/list_module_api public
require tools/list_modules public
require tools/find_symbol public
require tools/format_file public
require tools/run_script public
require tools/ast_dump public
require tools/list_requires public
require tools/convert_to_gen2 public
require tools/goto_definition public
require tools/type_of public
require tools/find_references public
require tools/program_log public
require tools/eval_expression public
require tools/describe_type public
require tools/grep_usage public
require tools/outline public
require tools/aot public
require tools/lint_tool public
require tools/live public

let HEAP_COLLECT_THRESHOLD = 1024ul * 1024ul  // 1 MB
let LOG_FILE = "utils/mcp/mcp_server.log"

// ── Logging ───────────────────────────────────────────────────────────

var log_path : string
var log_start_clock : clock
var log_to_stderr : bool = false
var ast_grep_available : bool = false

def log_open() {
    log_start_clock = get_clock()
    fopen(log_path, "ab") <| $(f) {
        if (f != null) {
            fprint(f, "\n--- server start ---\n")
        }
    }
}

def log_timestamp() : string {
    let elapsed = get_clock() - log_start_clock
    let total_sec = int(elapsed)
    let h = total_sec / 3600
    let m = (total_sec % 3600) / 60
    let s = total_sec % 60
    return build_string() <| $(var w) {
        if (h > 0) {
            write(w, "{h}h")
        }
        write(w, "{m}m{s}s")
    }
}

def log_write(msg : string) {
    let line = "[{log_timestamp()}] {msg}\n"
    if (log_to_stderr) {
        fprint(fstderr(), line)
        fflush(fstderr())
    } else {
        print(line)
    }
    fopen(log_path, "ab") <| $(f) {
        if (f != null) {
            fprint(f, line)
        }
    }
}

def log_request(method, body : string) {
    let truncated = length(body) > 500 ? slice(body, 0, 500) + "..." : body
    log_write("REQ {method} {truncated}")
}

def log_response(method : string; elapsed_sec : double) {
    let ms = int(elapsed_sec * 1000.0lf)
    log_write("RES {method} ({ms}ms)")
}

def log_tool(tool_name : string; args : string) {
    log_write("TOOL {tool_name} {args}")
}

def log_tool_done(tool_name : string; elapsed_sec : double; is_error : bool) {
    let ms = int(elapsed_sec * 1000.0lf)
    let status = is_error ? "ERR" : "OK"
    log_write("TOOL {tool_name} {status} ({ms}ms)")
}

def log_error(msg : string) {
    log_write("ERROR {msg}")
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

struct RpcError {
    code : int
    message : string
}

// ── JSON-RPC envelope (built as string since id is dynamic) ──────────

def json_value_to_string(v : JsonValue?) : string {
    if (v == null) {
        return "null"
    }
    return write_json(v)
}

def json_rpc_response(id_json, result_json : string) : string {
    return "\{\"jsonrpc\":\"2.0\",\"id\":{id_json},\"result\":{result_json}\}"
}

def json_rpc_error(id_json : string; code : int; message : string) : string {
    let err = RpcError(code = code, message = message)
    return "\{\"jsonrpc\":\"2.0\",\"id\":{id_json},\"error\":{sprint_json(err, false)}\}"
}

// ── MCP protocol handlers ─────────────────────────────────────────────

def handle_initialize(id_json : string) : string {
    let result = InitializeResult(
        protocolVersion = "2025-11-25",
        capabilities = Capabilities(),
        serverInfo = ServerInfo(name = "daslang", version = "0.1.0")
    )
    return json_rpc_response(id_json, sprint_json(result, false))
}

struct MakeFileTool {
    name : string
    description : string
    inputSchema : InputSchema
}

struct InputSchema {
    @rename = "type" _type : string
    properties : table<string; PropertySchema>
    required : array<string>
}

struct PropertySchema {
    @rename = "type" _type : string
    description : string
}

struct ToolsListResult {
    tools : array<MakeFileTool>
}

let PROJECT_PROP = PropertySchema(_type = "string", description = "Path to a .das_project file for custom module resolution")

def make_file_tool(name, description : string) : MakeFileTool {
    return MakeFileTool(
        name = name,
        description = description,
        inputSchema = InputSchema(
            _type = "object",
            properties = {
                "file" => PropertySchema(
                    _type = "string",
                    description = "Path to the .das file"
                ),
                "project" => PROJECT_PROP
            },
            required = ["file"]
        )
    )
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

def handle_tools_list(id_json : string) : string {
    var result : ToolsListResult
    result.tools |> emplace(make_tool(
        "compile_check",
        "Compile daScript file(s) and return errors or success. Supports single file, comma-separated list, or glob pattern (e.g. 'utils/mcp/tools/*.das'). Batch mode reports pass/fail per file with summary.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to .das file, comma-separated paths, or glob pattern (e.g. 'dir/*.das')"),
            "project" => PROJECT_PROP,
            "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON (array of CompileResult with file, success, errors, warnings)")
        },
        ["file"]
    ))
    result.tools |> emplace(make_file_tool("list_functions", "Compile a daScript file and list all functions (after macro expansion)"))
    result.tools |> emplace(make_file_tool("list_types", "Compile a daScript file and list all structs, classes, enums, and type aliases"))
    result.tools |> emplace(make_tool(
        "run_test",
        "Run dastest on a .das test file and return pass/fail results",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das test file"),
            "timeout" => PropertySchema(_type = "string", description = "Timeout in seconds (default: 120). Process tree is killed if exceeded"),
            "project" => PROJECT_PROP,
            "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON (TestSummary with file, tests, total, passed, failed, errors, skipped, success, time)"),
            "failures_only" => PropertySchema(_type = "string", description = "If 'true', only show failed tests in output")
        },
        ["file"]
    ))
    result.tools |> emplace(make_file_tool("format_file", "Format a .das file using the daslang code formatter"))
    result.tools |> emplace(make_tool(
        "run_script",
        "Run a daslang script and return its stdout output. Provide either a file path or inline code.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das file to run"),
            "code" => PropertySchema(_type = "string", description = "Inline daslang code to run (written to temp file and executed)"),
            "timeout" => PropertySchema(_type = "string", description = "Timeout in seconds (default: 30). Process tree is killed if exceeded")
        },
        []
    ))
    result.tools |> emplace(make_tool(
        "eval_expression",
        "Evaluate a daslang expression and return its printed result. Wraps the expression in a string builder print statement. Use 'require' to add module imports (comma-separated).",
        {
            "expression" => PropertySchema(_type = "string", description = "daslang expression to evaluate (e.g. 'to_float(42) + 1.0', 'length(\"hello\")')"),
            "require" => PropertySchema(_type = "string", description = "Comma-separated module imports (e.g. 'math', 'strings, daslib/json')")
        },
        ["expression"]
    ))
    result.tools |> emplace(make_tool(
        "describe_type",
        "Describe a type in detail: fields, methods, enum values, bitfield flags, variant options. Works for structs, classes, handled types, enums, bitfields, variants, tuples, and typedefs.",
        {
            "name" => PropertySchema(_type = "string", description = "Type name (e.g. 'TypeDecl', 'ExprCall', 'Type', 'FunctionFlags')"),
            "module" => PropertySchema(_type = "string", description = "Module to require for the type (e.g. 'ast', 'daslib/json')"),
            "project" => PROJECT_PROP
        },
        ["name"]
    ))
    result.tools |> emplace(make_tool(
        "ast_dump",
        "Dump AST of a daslang expression or function. mode='source' shows post-macro daslang code (via describe), mode='ast' (default) shows S-expression with node types.",
        {
            "expression" => PropertySchema(_type = "string", description = "A daslang expression to dump (e.g. 'a.b[0]', 'foo(x, y + 1)')"),
            "file" => PropertySchema(_type = "string", description = "Path to a .das file - dumps AST of compiled functions"),
            "function" => PropertySchema(_type = "string", description = "Function name to dump (optional, defaults to all functions)"),
            "mode" => PropertySchema(_type = "string", description = "Output mode: 'ast' (default) for S-expression, 'source' for post-macro daslang code"),
            "lineinfo" => PropertySchema(_type = "string", description = "If 'true', include LineInfo (file, line:col spans) on each AST node. Only for mode='ast'"),
            "project" => PROJECT_PROP
        },
        []
    ))
    result.tools |> emplace(make_tool(
        "list_modules",
        "List all available daslang modules (builtin and daslib)",
        {
            "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON (ModuleList with builtin and daslib arrays)")
        },
        []
    ))
    result.tools |> emplace(make_tool(
        "find_symbol",
        "Search for a symbol (function, struct, enum, global) across all registered modules by substring match",
        {
            "query" => PropertySchema(_type = "string", description = "Case-insensitive substring search. Prefix with '=' for exact case-sensitive match (e.g. '=print')"),
            "kind" => PropertySchema(_type = "string", description = "Limit to kind: 'function', 'generic', 'struct', 'handled', 'field', 'enum', 'global'"),
            "file" => PropertySchema(_type = "string", description = "Optional .das file - if provided, searches all modules loaded by that file (including daslib)"),
            "project" => PROJECT_PROP
        },
        ["query"]
    ))
    result.tools |> emplace(make_tool(
        "list_requires",
        "Compile a .das file and list all require dependencies (direct and transitive)",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
            "project" => PROJECT_PROP,
            "json" => PropertySchema(_type = "string", description = "If 'true', return structured JSON (RequiresResult with direct and transitive arrays of RequireEntry)")
        },
        ["file"]
    ))
    result.tools |> emplace(make_tool(
        "convert_to_gen2",
        "Convert a daslang source file from v1 (indentation-based) syntax to gen2 (braces, parentheses) syntax using das-fmt. Returns converted code or writes in place.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das file to convert"),
            "inplace" => PropertySchema(_type = "string", description = "If 'true', overwrite the file in place. Otherwise return converted code to stdout.")
        },
        ["file"]
    ))
    result.tools |> emplace(make_tool(
        "goto_definition",
        "Given a cursor position (file, line, column), resolve the definition of the symbol under the cursor. Returns the definition location, kind, and source snippet.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
            "line" => PropertySchema(_type = "string", description = "Line number (1-based)"),
            "column" => PropertySchema(_type = "string", description = "Column number (1-based)"),
            "no_opt" => PropertySchema(_type = "string", description = "If 'true', disable optimizations to preserve original AST (constant folding, inlining)"),
            "project" => PROJECT_PROP
        },
        ["file", "line", "column"]
    ))
    result.tools |> emplace(make_tool(
        "type_of",
        "Given a cursor position (file, line, column), return the type of the expression under the cursor. Shows all expressions at position from innermost to outermost with their resolved types.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
            "line" => PropertySchema(_type = "string", description = "Line number (1-based)"),
            "column" => PropertySchema(_type = "string", description = "Column number (1-based)"),
            "no_opt" => PropertySchema(_type = "string", description = "If 'true', disable optimizations to preserve original AST (constant folding, inlining)"),
            "project" => PROJECT_PROP
        },
        ["file", "line", "column"]
    ))
    result.tools |> emplace(make_tool(
        "find_references",
        "Find all references to the symbol under the cursor. Returns locations where the symbol is used.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
            "line" => PropertySchema(_type = "string", description = "Line number (1-based)"),
            "column" => PropertySchema(_type = "string", description = "Column number (1-based)"),
            "scope" => PropertySchema(_type = "string", description = "Search scope: 'file' (default) for current file only, 'all' for all loaded modules"),
            "no_opt" => PropertySchema(_type = "string", description = "If 'true', disable optimizations to preserve original AST (constant folding, inlining)"),
            "project" => PROJECT_PROP
        },
        ["file", "line", "column"]
    ))
    result.tools |> emplace(make_tool(
        "program_log",
        "Produce post-compilation program text (like 'options log'). Shows all types, globals, and functions after macro expansion, template instantiation, and inference.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das file to compile"),
            "function" => PropertySchema(_type = "string", description = "Limit output to a specific function name (optional)"),
            "project" => PROJECT_PROP
        },
        ["file"]
    ))
    result.tools |> emplace(make_tool(
        "list_module_api",
        "List all functions, types, enums, globals, and annotations exported by a daslang module (e.g. 'math', 'strings', 'fio', 'ast', 'daslib/json')",
        {
            "module" => PropertySchema(_type = "string", description = "Module name (e.g. 'math', 'strings', 'fio', 'ast', 'daslib/json')"),
            "filter" => PropertySchema(_type = "string", description = "Substring filter on names (e.g. 'for_each', 'json')"),
            "section" => PropertySchema(_type = "string", description = "Limit to section: 'functions', 'generics', 'operators', 'structs', 'handled', 'enums', 'globals', 'annotations'"),
            "compact" => PropertySchema(_type = "string", description = "Set to 'true' for compact output: function signatures show types only (no arg names), structs/enums omit fields/values. Use with large modules like 'ast', then drill down with describe_type or filter"),
            "project" => PROJECT_PROP
        },
        ["module"]
    ))
    if (ast_grep_available) {
        result.tools |> emplace(make_tool(
            "grep_usage",
            "Parse-aware symbol search across .das files using ast-grep + tree-sitter. Finds identifier occurrences excluding comments and strings. Much faster than compilation-based search.",
            {
                "symbol" => PropertySchema(_type = "string", description = "Identifier name to search for (e.g. 'compile_program', 'for_each_function')"),
                "directory" => PropertySchema(_type = "string", description = "Directory to search in (default: project root). Relative to project root."),
                "glob" => PropertySchema(_type = "string", description = "File filter pattern (e.g. 'daslib/*.das', 'utils/**/*.das'). Prefix with ! to exclude (e.g. '!modules/**')"),
                "context_lines" => PropertySchema(_type = "string", description = "Lines of context around each match (default: 1)")
            },
            ["symbol"]
        ))
        result.tools |> emplace(make_tool(
            "outline",
            "List all declarations (functions, structs, classes, enums, globals, typedefs) in a file or set of files using tree-sitter. Works on broken/incomplete code — no compilation needed.",
            {
                "file" => PropertySchema(_type = "string", description = "Path to a .das file (relative to project root)"),
                "glob" => PropertySchema(_type = "string", description = "Glob pattern to scan multiple files (e.g. 'daslib/*.das')")
            },
            ["file"]
        ))
    }
    result.tools |> emplace(make_tool(
        "aot",
        "Generate AOT (ahead-of-time) C++ code for a daScript file or a single function. Without 'function', returns the full AOT output. With 'function', returns C++ for that function only. If multiple functions match, returns the list of matches.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das file"),
            "function" => PropertySchema(_type = "string", description = "Function name to extract AOT for. Supports exact name, method name (matches ClassName`method), and generic origin."),
            "project" => PROJECT_PROP
        },
        ["file"]
    ))
    result.tools |> emplace(make_tool(
        "lint",
        "Run lint and performance checks on daScript file(s). Combines paranoid lint (unused variables, const suggestions, unreachable code, naming) with performance lint (PERF001-009: string concat in loops, character_at misuse, push without reserve, unnecessary string/get_ptr conversions, redundant move-return). Supports single file, comma-separated list, or glob pattern.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to .das file, comma-separated paths, or glob pattern"),
            "project" => PROJECT_PROP
        },
        ["file"]
    ))
    result.tools |> emplace(make_tool(
        "live_status",
        "Get status of a running daslang-live instance (fps, uptime, paused, dt, has_error). Returns 503 JSON with compilation error if the script failed to compile.",
        { "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)") },
        []
    ))
    result.tools |> emplace(make_tool(
        "live_error",
        "Get the last compilation error from a running daslang-live instance. Returns null if no error.",
        { "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)") },
        []
    ))
    result.tools |> emplace(make_tool(
        "live_reload",
        "Trigger a reload of the running daslang-live script. Works even when there is a compilation error.",
        {
            "full" => PropertySchema(_type = "string", description = "If 'true', force full recompile instead of incremental reload"),
            "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)")
        },
        []
    ))
    result.tools |> emplace(make_tool(
        "live_pause",
        "Pause or unpause the running daslang-live script. Returns 503 if there is a compilation error.",
        {
            "paused" => PropertySchema(_type = "string", description = "'true' to pause, 'false' to unpause"),
            "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)")
        },
        ["paused"]
    ))
    result.tools |> emplace(make_tool(
        "live_command",
        "Dispatch a [live_command] to the running daslang-live instance. Use name='help' to list all available commands. Returns 503 if there is a compilation error.",
        {
            "name" => PropertySchema(_type = "string", description = "Command name (e.g. 'help', 'screenshot', 'cmd_spawn_ball')"),
            "args" => PropertySchema(_type = "string", description = "Command arguments as JSON object string (e.g. '\{\"x\":1\}')"),
            "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)")
        },
        ["name"]
    ))
    result.tools |> emplace(make_tool(
        "live_shutdown",
        "Gracefully shut down the running daslang-live instance.",
        { "port" => PropertySchema(_type = "string", description = "Live API port (default: 9090)") },
        []
    ))
    result.tools |> emplace(make_tool(
        "live_launch",
        "Launch a daslang-live instance with a script file. If an instance is already running, returns its status instead of starting a new one. Polls for up to 10 seconds to confirm startup.",
        {
            "file" => PropertySchema(_type = "string", description = "Path to the .das script file to run"),
            "project" => PROJECT_PROP,
            "port" => PropertySchema(_type = "string", description = "Live API port to check (default: 9090)")
        },
        ["file"]
    ))
    result.tools |> emplace(make_tool(
        "shutdown",
        "Shut down the MCP server. Claude Code will auto-restart it, picking up any code changes to MCP tools. Tool registration changes still require a manual restart.",
        { "unused" => PropertySchema(_type = "string", description = "ignored") },
        []
    ))
    return json_rpc_response(id_json, sprint_json(result, false))
}

// ── Tool dispatch (runs in cloned context via new_thread) ────────────

struct ToolMessage {
    text : string
}

def get_string_arg(args : JsonValue?; name : string) : string {
    if (args == null || !(args.value is _object)) {
        return ""
    }
    let v = (args.value as _object)?[name] ?? null
    if (v == null) {
        return ""
    }
    if (v.value is _string) {
        return v.value as _string
    }
    if (v.value is _bool) {
        return v.value as _bool ? "true" : "false"
    }
    return ""
}

def dispatch_tool(tool_name, arg1, arg2, arg3, arg4, arg5, project : string) : string {
    if (tool_name == "compile_check") {
        return do_compile_check(arg1, project, arg2 == "true")
    } elif (tool_name == "list_functions") {
        return do_list_functions(arg1, project)
    } elif (tool_name == "list_types") {
        return do_list_types(arg1, project)
    } elif (tool_name == "run_test") {
        return do_run_test(arg1, project, arg2, arg3 == "true", arg4 == "true")
    } elif (tool_name == "format_file") {
        return do_format_file(arg1)
    } elif (tool_name == "run_script") {
        return do_run_script(arg1, arg2, arg3)
    } elif (tool_name == "eval_expression") {
        return do_eval_expression(arg1, arg2)
    } elif (tool_name == "describe_type") {
        return do_describe_type(arg1, arg2, project)
    } elif (tool_name == "ast_dump") {
        return do_ast_dump(arg1, arg2, arg3, arg4, arg5, project)
    } elif (tool_name == "list_requires") {
        return do_list_requires(arg1, project, arg2 == "true")
    } elif (tool_name == "convert_to_gen2") {
        return do_convert_to_gen2(arg1, arg2 == "true")
    } elif (tool_name == "goto_definition") {
        return do_goto_definition(arg1, arg2, arg3, arg4, project)
    } elif (tool_name == "type_of") {
        return do_type_of(arg1, arg2, arg3, arg4, project)
    } elif (tool_name == "find_references") {
        return do_find_references(arg1, arg2, arg3, arg4, arg5, project)
    } elif (tool_name == "program_log") {
        return do_program_log(arg1, arg2, project)
    } elif (tool_name == "list_modules") {
        return do_list_modules(arg1 == "true")
    } elif (tool_name == "find_symbol") {
        return do_find_symbol(arg1, arg2, arg3, project)
    } elif (tool_name == "list_module_api") {
        return do_list_module_api(arg1, arg2, arg3, arg4, project)
    } elif (tool_name == "grep_usage") {
        return do_grep_usage(arg1, arg2, arg3, arg4)
    } elif (tool_name == "outline") {
        return do_outline(arg1, arg2)
    } elif (tool_name == "aot") {
        return do_aot(arg1, arg2, project)
    } elif (tool_name == "lint") {
        return do_lint(arg1, project)
    } elif (tool_name == "live_status") {
        return do_live_status(arg1)
    } elif (tool_name == "live_error") {
        return do_live_error(arg1)
    } elif (tool_name == "live_reload") {
        return do_live_reload(arg1, arg2)
    } elif (tool_name == "live_pause") {
        return do_live_pause(arg1, arg2)
    } elif (tool_name == "live_command") {
        return do_live_command(arg1, arg2, arg3)
    } elif (tool_name == "live_shutdown") {
        return do_live_shutdown(arg1)
    } elif (tool_name == "live_launch") {
        return do_live_launch(arg1, arg2, arg3)
    } elif (tool_name == "shutdown") {
        unsafe { exit(0); }
        return make_tool_result("shutting down")
    } else {
        return make_tool_result("unknown tool: {tool_name}", true)
    }
}

def run_tool(tool_name, arg1, arg2, arg3, arg4, arg5, project : string) : string {
    log_tool(tool_name, "{arg1} {arg2} {arg3} {arg4} {arg5}")
    let t0 = get_clock()
    var result : string
    // Live tools run on the main thread — they use system() and sleep() which
    // don't work well from new_thread, and they don't need compilation context
    if (starts_with(tool_name, "live_")) {
        result = dispatch_tool(tool_name, arg1, arg2, arg3, arg4, arg5, project)
    } else {
        with_channel(1) <| $(ch) {
            new_thread() <| @() {
                let tool_result = dispatch_tool(tool_name, arg1, arg2, arg3, arg4, arg5, project)
                ch |> push_clone(ToolMessage(text = tool_result))
                ch |> notify_and_release()
            }
            for_each_clone(ch) <| $(msg : ToolMessage#) {
                result = clone_string(msg.text)
            }
        }
    }
    let is_error = find(result, "\"isError\":true") >= 0
    log_tool_done(tool_name, get_clock() - t0, is_error)
    return result
}

def handle_tools_call(id_json : string; params : JsonValue?) : string {
    let tool_name_val = params?.name
    let args = params?.arguments
    if (tool_name_val == null) {
        return json_rpc_error(id_json, -32602, "missing tool name")
    }
    let name = tool_name_val.value as _string
    var arg1, arg2, arg3, arg4, arg5 : string
    let project = get_string_arg(args, "project")
    if (name == "compile_check") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
        arg2 = get_string_arg(args, "json")
    } elif (name == "list_functions" || name == "list_types" || name == "format_file") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
    } elif (name == "list_requires") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
        arg2 = get_string_arg(args, "json")
    } elif (name == "run_test") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
        arg2 = get_string_arg(args, "timeout")
        arg3 = get_string_arg(args, "json")
        arg4 = get_string_arg(args, "failures_only")
    } elif (name == "convert_to_gen2") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
        arg2 = get_string_arg(args, "inplace")
    } elif (name == "goto_definition" || name == "type_of") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
        arg2 = get_string_arg(args, "line")
        arg3 = get_string_arg(args, "column")
        arg4 = get_string_arg(args, "no_opt")
    } elif (name == "find_references") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
        arg2 = get_string_arg(args, "line")
        arg3 = get_string_arg(args, "column")
        arg4 = get_string_arg(args, "scope")
        arg5 = get_string_arg(args, "no_opt")
    } elif (name == "run_script") {
        arg1 = get_string_arg(args, "file")
        arg2 = get_string_arg(args, "code")
        arg3 = get_string_arg(args, "timeout")
    } elif (name == "eval_expression") {
        arg1 = get_string_arg(args, "expression")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'expression' argument")
        }
        arg2 = get_string_arg(args, "require")
    } elif (name == "describe_type") {
        arg1 = get_string_arg(args, "name")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'name' argument")
        }
        arg2 = get_string_arg(args, "module")
    } elif (name == "ast_dump") {
        arg1 = get_string_arg(args, "expression")
        arg2 = get_string_arg(args, "file")
        arg3 = get_string_arg(args, "function")
        arg4 = get_string_arg(args, "mode")
        arg5 = get_string_arg(args, "lineinfo")
    } elif (name == "find_symbol") {
        arg1 = get_string_arg(args, "query")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'query' argument")
        }
        arg2 = get_string_arg(args, "kind")
        arg3 = get_string_arg(args, "file")
    } elif (name == "list_module_api") {
        arg1 = get_string_arg(args, "module")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'module' argument")
        }
        arg2 = get_string_arg(args, "filter")
        arg3 = get_string_arg(args, "section")
        arg4 = get_string_arg(args, "compact")
    } elif (name == "program_log") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
        arg2 = get_string_arg(args, "function")
    } elif (name == "grep_usage") {
        arg1 = get_string_arg(args, "symbol")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'symbol' argument")
        }
        arg2 = get_string_arg(args, "directory")
        arg3 = get_string_arg(args, "context_lines")
        arg4 = get_string_arg(args, "glob")
    } elif (name == "outline") {
        arg1 = get_string_arg(args, "file")
        arg2 = get_string_arg(args, "glob")
        if (empty(arg1) && empty(arg2)) {
            return json_rpc_error(id_json, -32602, "missing 'file' or 'glob' argument")
        }
    } elif (name == "list_modules") {
        arg1 = get_string_arg(args, "json")
    } elif (name == "aot") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
        arg2 = get_string_arg(args, "function")
    } elif (name == "lint") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
    } elif (name == "live_status") {
        arg1 = get_string_arg(args, "port")
    } elif (name == "live_error") {
        arg1 = get_string_arg(args, "port")
    } elif (name == "live_reload") {
        arg1 = get_string_arg(args, "full")
        arg2 = get_string_arg(args, "port")
    } elif (name == "live_pause") {
        arg1 = get_string_arg(args, "paused")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'paused' argument")
        }
        arg2 = get_string_arg(args, "port")
    } elif (name == "live_command") {
        arg1 = get_string_arg(args, "name")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'name' argument")
        }
        arg2 = get_string_arg(args, "args")
        arg3 = get_string_arg(args, "port")
    } elif (name == "live_shutdown") {
        arg1 = get_string_arg(args, "port")
    } elif (name == "live_launch") {
        arg1 = get_string_arg(args, "file")
        if (empty(arg1)) {
            return json_rpc_error(id_json, -32602, "missing 'file' argument")
        }
        arg2 = get_string_arg(args, "project")
        arg3 = get_string_arg(args, "port")
    } elif (name == "shutdown") {
        pass
    } else {
        return json_rpc_error(id_json, -32602, "unknown tool: {name}")
    }
    return json_rpc_response(id_json, run_tool(name, arg1, arg2, arg3, arg4, arg5, project))
}

// ── JSON-RPC dispatcher ──────────────────────────────────────────────

def dispatch_jsonrpc(body : string) : string {
    var err : string
    var parsed = read_json(body, err)
    if (parsed == null) {
        log_error("JSON parse error: {err}")
        return json_rpc_error("null", -32700, "parse error: {err}")
    }
    let id_json = json_value_to_string(parsed?.id)
    let method_val = parsed?.method
    let params_val = parsed?.params
    if (method_val == null || !(method_val.value is _string)) {
        log_error("missing method in request")
        unsafe { delete parsed; }
        return json_rpc_error(id_json, -32600, "invalid request: missing method")
    }
    let method = method_val.value as _string
    log_request(method, body)
    let t0 = get_clock()
    var result : string
    if (method == "initialize") {
        result = handle_initialize(id_json)
    } elif (method == "notifications/initialized" || method == "initialized") {
        unsafe { delete parsed; }
        return ""
    } elif (method == "tools/list") {
        result = handle_tools_list(id_json)
    } elif (method == "tools/call") {
        result = handle_tools_call(id_json, params_val)
    } elif (method == "ping") {
        result = json_rpc_response(id_json, "\{\}")
    } else {
        log_error("method not found: {method}")
        result = json_rpc_error(id_json, -32601, "method not found: {method}")
    }
    log_response(method, get_clock() - t0)
    unsafe { delete parsed; }
    return result
}
