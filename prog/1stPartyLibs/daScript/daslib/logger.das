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
//! so accidental ``print()`` / ``to_log()`` calls are diverted into the
//! log file instead of stdout/stderr — important for stdio-transport
//! processes (MCP server, language servers) where any stdout write
//! corrupts the wire format. One install covers every Context including
//! future ``new_thread`` workers.

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
var private log_open_attempted = false
var private log_open_failed = false
var private log_min_level = LOG_INFO
var private log_category_overrides : table<string; int>
var private log_fallback_to_stderr = true

// ── Internal helpers ─────────────────────────────────────────────────

def private level_name(level : int) : string {
    return "critical" if (level >= LOG_CRITICAL)
    return "error" if (level >= LOG_ERROR)
    return "warning" if (level >= LOG_WARNING)
    return "info" if (level >= LOG_INFO)
    return "debug" if (level >= LOG_DEBUG)
    return "trace"
}

// Resolve filter level for a category. Most-specific dotted prefix wins —
// "mcp.live.foo" first looks up the full path, then "mcp.live", then "mcp",
// before falling back to the global min level.
def private resolve_level(category : string) : int {
    var probe = category
    while (true) {
        if (key_exists(log_category_overrides, probe)) return log_category_overrides[probe]
        let dot = probe |> rfind(".")
        break if (dot < 0)
        probe = probe |> slice(0, dot)
    }
    return log_min_level
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
    log_open_attempted = true
    ensure_dir(log_path)
    log_file = fopen(log_path, "ab")
    if (log_file == null) {
        log_open_failed = true
    }
}

// JSON-encode the four fixed fields directly into the writer, then append
// the optional `fields` object via daslib/json's public `write_json` (no
// json_boost reflection — fast path).
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
        // write_json always pretty-prints (depth=0 still inserts \n/\t between
        // elements). One JSON object per line is load-bearing for JSON Lines,
        // so flatten via the jsonrpc framing helper (preserves whitespace
        // inside string literals).
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
        return
    }
    // Open failed (or path never set). stderr is always safe — it isn't the
    // JSON-RPC channel for stdio-transport servers, so a tail fallback can't
    // corrupt protocol framing.
    if (log_fallback_to_stderr) {
        fwrite(fstderr(), line)
    }
}

// ── Public configuration ─────────────────────────────────────────────
//
// All public names carry the ``logger_`` prefix on purpose. Short verbs
// like ``flush`` / ``close`` / ``info`` / ``debug`` collide with user
// code when callers ``require daslib/logger`` and reach for the
// unqualified form. The prefix keeps the call sites readable
// (``logger_info("foo", "bar")``) without forcing ``logger::`` everywhere.

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
    // Clear logical name so logger_get_name() matches the doc claim:
    // "empty if the path was set directly". logger_set_name() restores
    // log_name immediately after it calls us, so that path still works.
    log_name = ""
    log_open_attempted = false
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
    //! formatting. Default ``LOG_INFO``.
    log_min_level = level
}

def logger_get_min_level() : int {
    //! Returns the currently configured global minimum level.
    return log_min_level
}

def logger_set_category_level(category : string; level : int) {
    //! Override the level filter for one category. Dotted prefix
    //! matching: an override on ``"mcp.live"`` applies to ``"mcp.live"``,
    //! ``"mcp.live.foo"``, ``"mcp.live.bar"`` (unless they have their own
    //! more specific override).
    log_category_overrides[category] = level
}

def logger_clear_category_levels() {
    //! Remove all per-category overrides.
    log_category_overrides |> clear
}

def logger_set_stderr_fallback(enabled : bool) {
    //! Enable / disable writing to stderr when the log file can't be
    //! opened. Defaults to true; set to false for strict stdio-transport
    //! servers that must NEVER write to either stream on file failure.
    log_fallback_to_stderr = enabled
}

// ── Log calls ────────────────────────────────────────────────────────

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

// ── Lifecycle ────────────────────────────────────────────────────────

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
    log_open_attempted = false
    log_open_failed = false
}

// ── Debug-agent hook ─────────────────────────────────────────────────
//
// Intercepts ``Context::to_out`` (the underlying mechanism behind ``print``
// and ``to_log``) and routes every message into the configured log file
// instead of stdout/stderr. Critical for stdio-transport processes — MCP
// servers, language servers, anything where stdout is the wire format.
//
// Registered as a GLOBAL debug agent (not thread-local) under the
// ``"daslib_logger"`` category. One install covers every Context that
// later spawns; ``new_thread`` workers don't need to re-install. The
// agent's ``onLog`` is dispatched into its installing Context, which
// owns the log file handle — cross-context invocations route writes
// through that single handle.

let private LOGGER_AGENT_CATEGORY = "daslib_logger"

class private LogHookAgent : DapiDebugAgent {
    def override onLog(context : Context?; at : LineInfo const?; level : int; text : string#) : bool {
        // Fail-open when logging isn't configured — let the default
        // stdout/stderr path through instead of swallowing silently.
        // Makes accidental "hook before logger_set_name" calls harmless
        // rather than blackholing every print().
        if (empty(log_path) && !log_fallback_to_stderr) return false
        var fields : JsonValue? = null
        if (at != null && at.fileInfo != null) {
            fields = JV({"at" => JV("{at.fileInfo.name}:{at.line}")})
        }
        emit(level, "runtime", string(text), fields)
        return true
    }
}

def logger_install_hook() {
    //! Install a global debug agent that diverts ``print`` and
    //! ``to_log`` (from any Context, current or future) into the
    //! configured log file. Idempotent — safe to call from every
    //! module's [init], from MCP main, etc. Worker threads spawned via
    //! ``new_thread`` inherit the hook automatically; no per-context
    //! re-install needed.
    return if (has_debug_agent_context(LOGGER_AGENT_CATEGORY))
    install_new_debug_agent(new LogHookAgent(), LOGGER_AGENT_CATEGORY)
}

def logger_init(name : string) {
    //! Convenience: ``logger_set_name(name)`` + ``logger_install_hook()``.
    //! The common-case one-liner for tools that want both behaviors.
    logger_set_name(name)
    logger_install_hook()
}
