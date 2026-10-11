options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dastest/testing_boost
require daslib/logger
require daslib/json_boost
require daslib/defer
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
def reset() {
    logger_close()
    logger_set_path("")
    logger_set_min_level(LOG_INFO)
    logger_clear_category_levels()
    logger_set_stderr_fallback(false)
}

[test]
def test_basic_write(t : T?) {
    t |> run("info writes one JSON Lines record with ts/level/cat/msg") @(t : T?) {
        reset()
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
        reset()
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
        reset()
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
        reset()
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
        reset()
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
        reset()
        // No set_path. Fallback off. info() should be a silent no-op.
        logger_info("test.noop", "should-not-appear")
        t |> success(true)
    }
}

def private das_exe() : string {
    let args <- get_command_line_arguments()
    return empty(args) ? "" : args[0]
}

def private run_child(cmd : string; var output : string&) : int {
    return unsafe(popen_timeout(cmd, 120.0, $(f) {
        if (f != null) {
            output = fread(f)
        }
    }))
}

// `logger_install_hook()` is tested through a child process (`_logger_hook_child.das`): the
// global debug agent it registers keeps the installing context, so an in-process cell would leave
// a dead agent behind for the harness's next line.
[test]
def test_hook_at_field_decimal(t : T?) {
    set_env_variable("DAS_LOG_LEVEL", "info")
    defer() {
        set_env_variable("DAS_LOG_LEVEL", "")
    }
    t |> run("the hook's at field names the call site's line in decimal") @(t : T?) {
        let p = temp_log_path()
        var out : string
        var cmd = "\"{das_exe()}\" -dasroot \"{get_das_root()}\" \"{get_das_root()}/tests/daslib/_logger_hook_child.das\" -- \"{p}\" 2>&1"
        if (get_platform_name() == "windows") {
            cmd = "\"{cmd}\""   // cmd.exe strips the first and last quote of a line that opens with one
        }
        let rc = run_child(cmd, out)
        t |> equal(rc, 0, "the probe process exits clean: {out}")
        t |> success(find(out, "[info] hook at-field probe") < 0, "without the echo the console sees no record: {out}")
        let lines = read_lines(p)
        remove(p)
        var at = ""
        for (line in lines) {
            continue if (find(line, "hook at-field probe") < 0)
            let rec = parse_record(t, line)
            let fields = rec?["fields"]
            if (fields == null || !(fields.value is _object)) {
                t |> failure("the hook's record carries no fields object: '{line}'")
                return
            }
            at = field_str(t, fields, "at")
        }
        t |> success(!empty(at), "the hook wrote the probe line with an at field")
        let tail = slice(at, rfind(at, ":") + 1)
        t |> success(find(at, "0x") < 0, "the line is not hex: '{at}'")
        t |> success(to_int(tail) > 0 && "{to_int(tail)}" == tail, "the line is a decimal number: '{at}'")
    }
    t |> run("the stderr echo prints the record as one plain line on stderr alone, beside the file's JSON") @(t : T?) {
        let p = temp_log_path()
        let windows = get_platform_name() == "windows"
        let child = "\"{das_exe()}\" -dasroot \"{get_das_root()}\" \"{get_das_root()}/tests/daslib/_logger_hook_child.das\" -- echo \"{p}\""
        // stderr alone: stdout to the null device after the merge
        var err_cmd = "{child} 2>&1 >{windows ? "NUL" : "/dev/null"}"
        var out_cmd = "{child} 2>{windows ? "NUL" : "/dev/null"}"
        if (windows) {
            err_cmd = "\"{err_cmd}\""
            out_cmd = "\"{out_cmd}\""
        }
        var err_only : string
        let rc = run_child(err_cmd, err_only)
        t |> equal(rc, 0, "the probe process exits clean: {err_only}")
        t |> success(find(err_only, "[info] hook at-field probe") >= 0, "the echo line reached stderr: {err_only}")
        t |> success(find(err_only, "\"level\":\"info\"") < 0, "the console line is plain text, not the file's JSON record")
        let lines = read_lines(p)
        remove(p)
        var in_file = false
        for (line in lines) {
            in_file ||= find(line, "hook at-field probe") >= 0
        }
        t |> success(in_file, "the file still receives the record")
        var out_only : string
        t |> equal(run_child(out_cmd, out_only), 0, "the probe process exits clean: {out_only}")
        t |> success(find(out_only, "hook at-field probe") < 0, "stdout carries no echo: {out_only}")
        remove(p)
    }
}
