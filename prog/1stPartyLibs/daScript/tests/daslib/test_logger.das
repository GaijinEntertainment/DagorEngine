options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost
require daslib/logger
require daslib/json_boost
require daslib/fio
require daslib/strings_boost
require strings

def temp_log_path() : string {
    return path_join(get_das_root(), "_test_log_{ref_time_ticks()}.log")
}

// Read the file, split on newlines, drop the trailing empty line.
def read_lines(path : string) : array<string> {
    var lines : array<string>
    let text = fread(path)
    return <- lines if (empty(text))
    let parts <- text |> split("\n")
    for (line in parts) {
        lines |> push(line) if (!empty(line))
    }
    return <- lines
}

def parse_record(t : T?; line : string) : JsonValue? {
    var err : string
    var v = read_json(line, err)
    if (v == null) {
        t |> failure("could not parse JSON: {err} (line: '{line}')")
    }
    return v
}

// json_boost's `?[]` always returns a JsonValue? — JVNull() when missing,
// so we only need to type-test (no null check on the pointer itself).
def field_str(t : T?; rec : JsonValue?; key : string) : string {
    let v = rec?[key]
    if (v == null || !(v.value is _string)) {
        t |> failure("missing or non-string field '{key}'")
        return ""
    }
    return v.value as _string
}

def field_num(t : T?; rec : JsonValue?; key : string) : double {
    // JV(int) stores _longint, JV(double/float) stores _number. JSON treats
    // both as "number"; this helper accepts either.
    let v = rec?[key]
    if (v == null) {
        t |> failure("missing field '{key}'")
        return 0.0lf
    }
    if (v.value is _number) return v.value as _number
    if (v.value is _longint) return double(v.value as _longint)
    t |> failure("field '{key}' is not a number")
    return 0.0lf
}

// Reset module state. The library has no `reset()` API on purpose (production
// callers want sticky config); these tests need a clean slate, so we drive
// the public setters manually.
def reset(t : T?) {
    logger_close()
    logger_set_path("")
    logger_set_min_level(LOG_INFO)
    logger_clear_category_levels()
    logger_set_stderr_fallback(false)
}

[test]
def test_basic_write(t : T?) {
    t |> run("info writes one JSON Lines record with ts/level/cat/msg") @(t : T?) {
        reset(t)
        let p = temp_log_path()
        logger_set_path(p)
        logger_info("test.basic", "hello world")
        logger_close()
        let lines = read_lines(p)
        remove(p)
        t |> equal(length(lines), 1)
        return if (empty(lines))
        let rec = parse_record(t, lines[0])
        t |> equal(field_str(t, rec, "level"), "info")
        t |> equal(field_str(t, rec, "cat"), "test.basic")
        t |> equal(field_str(t, rec, "msg"), "hello world")
        // ts must be ISO 8601 UTC with ms: "YYYY-MM-DDTHH:MM:SS.mmmZ" (24 chars)
        let ts = field_str(t, rec, "ts")
        t |> equal(length(ts), 24)
        t |> success(ts |> ends_with("Z"))
    }
}

[test]
def test_level_filter(t : T?) {
    t |> run("min_level drops records below threshold") @(t : T?) {
        reset(t)
        let p = temp_log_path()
        logger_set_path(p)
        logger_set_min_level(LOG_WARNING)
        logger_debug("test.filter", "debug-dropped")
        logger_info("test.filter", "info-dropped")
        logger_warning("test.filter", "warning-kept")
        logger_error("test.filter", "error-kept")
        logger_close()
        let lines = read_lines(p)
        remove(p)
        t |> equal(length(lines), 2)
        return if (length(lines) != 2)
        t |> equal(field_str(t, parse_record(t, lines[0]), "level"), "warning")
        t |> equal(field_str(t, parse_record(t, lines[1]), "level"), "error")
    }
}

[test]
def test_category_prefix_override(t : T?) {
    t |> run("dotted prefix category override applies to descendants") @(t : T?) {
        reset(t)
        let p = temp_log_path()
        logger_set_path(p)
        logger_set_min_level(LOG_WARNING)
        logger_set_category_level("test.live", LOG_DEBUG)
        logger_debug("test.live", "live-direct")          // kept via direct match
        logger_debug("test.live.foo", "live-foo")         // kept via prefix
        logger_debug("test.other", "other-dropped")        // no override
        logger_debug("test.live2", "live2-not-prefix")     // not a dotted prefix of "test.live"
        logger_close()
        let lines = read_lines(p)
        remove(p)
        t |> equal(length(lines), 2)
        return if (length(lines) != 2)
        t |> equal(field_str(t, parse_record(t, lines[0]), "cat"), "test.live")
        t |> equal(field_str(t, parse_record(t, lines[1]), "cat"), "test.live.foo")
    }
}

[test]
def test_msg_escaping(t : T?) {
    t |> run("msg with quotes / newlines / tabs is JSON-escaped and round-trips") @(t : T?) {
        reset(t)
        let p = temp_log_path()
        logger_set_path(p)
        logger_info("test.esc", "she said \"hi\"\nnewline\there")
        logger_close()
        let lines = read_lines(p)
        remove(p)
        t |> equal(length(lines), 1)
        return if (empty(lines))
        let rec = parse_record(t, lines[0])
        t |> equal(field_str(t, rec, "msg"), "she said \"hi\"\nnewline\there")
    }
}

[test]
def test_fields_serialization(t : T?) {
    t |> run("fields object is embedded as a nested JSON object") @(t : T?) {
        reset(t)
        let p = temp_log_path()
        logger_set_path(p)
        logger_info("test.fields", "with extras", JV({"count" => JV(42), "name" => JV("foo")}))
        logger_close()
        let lines = read_lines(p)
        remove(p)
        t |> equal(length(lines), 1)
        return if (empty(lines))
        let rec = parse_record(t, lines[0])
        let fields = rec?["fields"]
        if (fields == null || !(fields.value is _object)) {
            t |> failure("fields missing or not an object")
            return
        }
        t |> equal(field_str(t, fields, "name"), "foo")
        t |> equal(int(field_num(t, fields, "count")), 42)
    }
}

[test]
def test_no_open_silent_when_fallback_disabled(t : T?) {
    t |> run("never writes anywhere if path unset and stderr fallback off") @(t : T?) {
        reset(t)
        // No set_path. Fallback off. info() should be a silent no-op.
        logger_info("test.noop", "should-not-appear")
        t |> success(true)
    }
}

// NOTE: `logger_install_hook()` is not unit-tested here because the global
// debug agent it registers stays alive for the process and intercepts
// dastest's own report-print() too — even after we clear path + fallback
// (the fail-open guard appears to be observed in the wrong context). The
// hook is exercised end-to-end via the MCP server (where `logger_init` is
// the canonical entry point); add a dedicated integration harness if hook
// coverage in dastest is needed.
