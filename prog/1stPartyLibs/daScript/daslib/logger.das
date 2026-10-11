options gen2
options indenting = 4
options no_global_variables = false

module logger shared public

//! Structured logging for daslang tools.
//!
//! Writes one JSON object per line ("JSON Lines" / ``ndjson``) to
//! ``{get_das_root()}/logs/<name>.log``. Reuses the runtime ``LOG_TRACE`` /
//! ``LOG_DEBUG`` / ``LOG_INFO`` / ``LOG_WARNING`` / ``LOG_ERROR`` /
//! ``LOG_CRITICAL`` levels. Each record carries an ISO 8601 UTC timestamp
//! (millisecond precision), level name, category, message, and optional
//! ``fields`` object.
//!
//! All public names carry the ``logger_`` prefix — short verbs like
//! ``flush`` / ``close`` / ``info`` / ``debug`` would collide with user
//! code when called unqualified. The intended call shape is
//! ``logger_info("mcp.tool", "...")``, not ``logger::info(...)``.
//!
//! **Categories** are free-form strings; the convention is dotted
//! hierarchy from tool name down to subsystem:
//!
//! - ``"mcp"`` — top-level events for the MCP server
//! - ``"mcp.req"`` / ``"mcp.res"`` — JSON-RPC request / response framing
//! - ``"mcp.tool"`` — tool dispatch (start / complete / error)
//! - ``"mcp.live"``, ``"mcp.live.launch"`` — live-reload subsystem
//! - ``"mcp.heap"`` — heap-collection ticks
//!
//! Filters apply at the most-specific dotted prefix:
//! :func:`logger_set_category_level` ``("mcp.live", LOG_DEBUG)`` enables
//! debug for ``"mcp.live"``, ``"mcp.live.launch"``, etc. — anything else
//! stays at the global :func:`logger_set_min_level`.
//!
//! :func:`logger_install_hook` plugs a GLOBAL debug agent into the runtime
//! so accidental ``print()`` / ``to_log()`` calls are diverted off
//! stdout/stderr — important for stdio-transport processes (MCP server,
//! language servers) where any stdout write corrupts the wire format.
//! What reaches the file is still the level filter's to decide, and
//! ``print`` is a debug-level line: under the ``LOG_INFO`` default it is
//! dropped rather than recorded. One install covers every Context including
//! future ``new_thread`` workers. A leak dump or a fatal is not ``print``
//! and no hook sees it: :func:`logger_capture_diagnostics` is what puts
//! those beside the log.

require daslib/json public
require daslib/fio public
require strings
require daslib/jsonrpc
require daslib/debugger
require daslib/rtti

// ── State (per-Context) ──────────────────────────────────────────────

var private log_file : FILE const? = null
var private log_path : string
var private log_name : string
var private log_open_failed = false
var private log_min_level = env_min_level()
var private log_category_overrides : table<string; int>
var private log_fallback_to_stderr = true
var private log_tee = false
var private log_echo_stderr = false

def private env_min_level() : int {
    var text = get_env_variable("DAS_LOGGER_LEVEL")
    if (empty(text)) {
        text = get_env_variable("DAS_LOG_LEVEL")
    }
    if (empty(text)) {
        return LOG_WARNING
    }
    let name = to_lower(text)
    if (name == "trace") return LOG_TRACE
    if (name == "debug") return LOG_DEBUG
    if (name == "info") return LOG_INFO
    if (name == "warning") return LOG_WARNING
    if (name == "error") return LOG_ERROR
    if (name == "critical") return LOG_CRITICAL
    return to_int(name)
}

def private level_name(level : int) : string {
    return "critical" if (level >= LOG_CRITICAL)
    return "error" if (level >= LOG_ERROR)
    return "warning" if (level >= LOG_WARNING)
    return "info" if (level >= LOG_INFO)
    return "debug" if (level >= LOG_DEBUG)
    return "trace"
}

def private resolve_level(category : string) : int {
    if (key_exists(log_category_overrides, category)) return log_category_overrides[category]
    var found = false
    var level = 0
    peek_data(category) $(d) {
        var end = length(d)
        while (true) {
            let dot = rfind(d, ".", end - 1)
            break if (dot < 0)
            let probe = slice(d, 0, dot)
            if (key_exists(log_category_overrides, probe)) {
                level = log_category_overrides[probe]
                found = true
                return
            }
            end = dot
        }
    }
    return found ? level : log_min_level
}

def private should_log(level : int; category : string) : bool {
    return level >= resolve_level(category)
}

def private ensure_dir(path : string) {
    let dir = dir_name(path)
    return if (empty(dir))
    let st = stat(dir)
    return if (st.is_valid && st.is_dir)
    mkdir(dir)
}

def private ensure_open() {
    return if (log_file != null || log_open_failed || empty(log_path))
    ensure_dir(log_path)
    log_file = fopen(log_path, "ab")
    if (log_file == null) {
        log_open_failed = true
    }
}

def private write_record(var w : StringBuilderWriter; ts, lvl, cat, msg : string; fields : JsonValue?) {
    w |> write("\{\"ts\":\"")
    write_escape_string(w, ts)
    w |> write("\",\"level\":\"")
    write_escape_string(w, lvl)
    w |> write("\",\"cat\":\"")
    write_escape_string(w, cat)
    w |> write("\",\"msg\":\"")
    write_escape_string(w, msg)
    if (fields != null) {
        w |> write("\",\"fields\":")
        w |> write(compact_json_whitespace(write_json(fields)))
        w |> write("}\n")
    } else {
        w |> write("\"}\n")
    }
}

def private emit(level : int; category, message : string; fields : JsonValue?) {
    return if (!should_log(level, category))
    ensure_open()
    let line = build_string() $(var w) {
        write_record(w, iso8601_now(), level_name(level), category, message, fields)
    }
    if (log_file != null) {
        fwrite(log_file, line)
        fflush(log_file)
        if (log_echo_stderr) {
            fwrite(fstderr(), "[{level_name(level)}] {strip_right(message)}\n")   // a to_log line carries its own newline
        }
        return
    }
    if (log_fallback_to_stderr) {
        fwrite(fstderr(), line)
    }
}

def logger_set_name(name : string) {
    //! Configure the log file as ``{get_das_root()}/logs/<name>.log``.
    //! Re-opens if the path changes; closes the previous handle. Each
    //! Context tracks its own state — call once per context.
    let new_path = path_join(get_das_root(), "logs/{name}.log")
    logger_set_path(new_path)
    log_name = name
}

def logger_set_path(path : string) {
    //! Override the log file path explicitly. Pass an absolute path or a
    //! path relative to the current working directory.
    if (path == log_path) return
    if (log_file != null) {
        fclose(log_file)
        log_file = null
    }
    log_path = path
    log_name = ""
    log_open_failed = false
}

def logger_get_path() : string {
    //! Returns the currently configured log file path (empty if unset).
    return log_path
}

def logger_get_name() : string {
    //! Returns the logical name passed to :func:`logger_set_name`
    //! (empty if the path was set directly via :func:`logger_set_path`).
    return log_name
}

def logger_set_min_level(level : int) {
    //! Global minimum level. Records below this are dropped before
    //! formatting. Default ``LOG_WARNING``, or the level ``DAS_LOGGER_LEVEL`` /
    //! ``DAS_LOG_LEVEL`` names.
    log_min_level = level
}

def logger_get_min_level() : int {
    //! Returns the currently configured global minimum level.
    return log_min_level
}

def logger_set_category_level(category : string; level : int) {
    //! Override the level filter for one category. Dotted prefix matching: an override on ``"mcp.live"`` applies to ``"mcp.live"``, ``"mcp.live.foo"``, ``"mcp.live.bar"`` (unless they have their own more specific override).
    log_category_overrides[category] = level
}

def logger_clear_category_levels() {
    //! Remove all per-category overrides.
    log_category_overrides |> clear
}

def logger_set_echo_stderr(enabled : bool) {
    //! Echo every record the file receives to stderr as one plain ``[level] message`` line - a
    //! tool's ``--verbose``: stdout stays untouched, unlike :func:`logger_set_tee`, which lets the
    //! runtime's own sink print. Off by default; no effect while the file is not open.
    log_echo_stderr = enabled
}

def logger_set_stderr_fallback(enabled : bool) {
    //! Enable / disable writing to stderr when the log file can't be
    //! opened. Defaults to true; set to false for strict stdio-transport
    //! servers that must NEVER write to either stream on file failure.
    log_fallback_to_stderr = enabled
}

def logger_log(level : int; category, message : string; fields : JsonValue? = null) {
    //! Generic log entry. Prefer the named-level helpers below.
    emit(level, category, message, fields)
}

def logger_trace(category, message : string; fields : JsonValue? = null) {
    //! Verbose trace-level record. ``LOG_TRACE``.
    emit(LOG_TRACE, category, message, fields)
}

def logger_debug(category, message : string; fields : JsonValue? = null) {
    //! Debug-level record. ``LOG_DEBUG``.
    emit(LOG_DEBUG, category, message, fields)
}

def logger_info(category, message : string; fields : JsonValue? = null) {
    //! Informational record. ``LOG_INFO`` — the default level.
    emit(LOG_INFO, category, message, fields)
}

def logger_warning(category, message : string; fields : JsonValue? = null) {
    //! Warning record. ``LOG_WARNING``.
    emit(LOG_WARNING, category, message, fields)
}

def logger_error(category, message : string; fields : JsonValue? = null) {
    //! Error record. ``LOG_ERROR``.
    emit(LOG_ERROR, category, message, fields)
}

def logger_critical(category, message : string; fields : JsonValue? = null) {
    //! Critical / fatal record. ``LOG_CRITICAL``.
    emit(LOG_CRITICAL, category, message, fields)
}

def logger_flush() {
    //! Explicit flush. Each write already flushes; only useful if you
    //! disable auto-flush in a fork.
    if (log_file != null) {
        fflush(log_file)
    }
}

def logger_close() {
    //! Close the log file. Subsequent writes will lazy-reopen.
    if (log_file != null) {
        fclose(log_file)
        log_file = null
    }
    log_open_failed = false
}

let private LOGGER_AGENT_CATEGORY = "daslib_logger"

class private LogHookAgent : DapiDebugAgent {
    def override onLog(context : Context?; at : LineInfo const?; level : int; text : string#) : bool {
        if (empty(log_path) && !log_fallback_to_stderr) return false
        var fields : JsonValue? = null
        if (at != null && at.fileInfo != null) {
            fields = JV({"at" => JV("{at.fileInfo.name}:{int(at.line)}")})
        }
        emit(level, "runtime", string(text), fields)
        return !log_tee
    }
}

def logger_set_tee(tee : bool) {
    //! Tee mode: hooked ``print``/``to_log`` lines go to the log file AND still reach
    //! stdout/stderr (the default hook behavior diverts them to the file only — right for
    //! stdio-transport tools, wrong for interactive servers watched on a console).
    log_tee = tee
}

def logger_install_hook() {
    //! Install a global debug agent that diverts ``print`` and ``to_log`` (from any Context, current or future) into the configured log file. Idempotent — safe to call from every module's [init], from MCP main, etc. Worker threads spawned via ``new_thread`` inherit the hook automatically; no per-context re-install needed.
    return if (has_debug_agent_context(LOGGER_AGENT_CATEGORY))
    install_new_debug_agent(new LogHookAgent(), LOGGER_AGENT_CATEGORY)
}

def logger_capture_diagnostics() {
    //! Send the runtime's own diagnostics — a leak dump, a fatal, what it reports
    //! about itself — as raw text to ``<log>.diag.log``, or to stderr when that
    //! cannot open. Not a panic report: that is level-tagged, so the hook logs it.
    if (empty(log_path) ||
        !diagnostics_to_file(replace_extension(log_path, ".diag.log"))) {
        diagnostics_to_stderr()
    }
}

def logger_init(name : string) {
    //! Convenience: ``logger_set_name(name)`` + ``logger_install_hook()``.
    //! The common-case one-liner for tools that want both behaviors.
    logger_set_name(name)
    logger_install_hook()
}

def logger_init_tee(name : string) {
    //! ``logger_init`` in tee mode: everything still prints to the console AND lands,
    //! timestamped, in ``logs/<name>.log`` — the one-liner for interactive servers.
    logger_set_tee(true)
    logger_init(name)
}
