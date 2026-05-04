options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/json
require daslib/json_boost

def private try_ast_grep(cmd : string) : bool {
    var found = false
    unsafe {
        popen("{cmd} --version") <| $(f) {
            if (f == null) {
                return
            }
            let line = fgets(f)
            if (find(line, "ast-grep") >= 0) {
                found = true
            }
        }
    }
    return found
}

def detect_ast_grep() : string {
    // try bare "sg" first (works when .venv is activated or sg is on PATH)
    if (try_ast_grep("sg")) {
        return "sg"
    }
    // try .venv paths (not activated): Scripts/ on Windows, bin/ on Linux/macOS
    let das_root = get_das_root()
    for (subdir in ["Scripts", "bin"]) {
        let venv_sg = "{das_root}/.venv/{subdir}/sg"
        if (try_ast_grep(venv_sg)) {
            return venv_sg
        }
    }
    return ""
}

def get_ast_grep_cmd() : string {
    if (empty(ast_grep_cmd)) {
        ast_grep_cmd = detect_ast_grep()
    }
    return ast_grep_cmd
}

def json_to_int(v : JsonValue?) : int {
    if (v == null) {
        return 0
    }
    if (v.value is _number) {
        return int(v.value as _number)
    }
    if (v.value is _longint) {
        return int(v.value as _longint)
    }
    return 0
}

def get_start_line(m : JsonValue?) : int {
    if (m == null || !(m.value is _object)) {
        return 0
    }
    unsafe {
        let rng = (m.value as _object)?["range"] ?? null
        if (rng == null) {
            return 0
        }
        let start = rng?.start
        if (start == null) {
            return 0
        }
        let ln = start?.line
        return json_to_int(ln)
    }
}

def do_grep_usage(symbol, directory, context_lines_str, glob_filter : string) : string {
    if (empty(symbol)) {
        return make_tool_result("'symbol' argument is required", true)
    }
    let das_root = get_das_root()
    let search_dir = resolve_path(!empty(directory) ? directory : ".")
    // build ast-grep command
    let sg = get_ast_grep_cmd()
    let cd_root = "cd \"{das_root}\" && "
    var cmd = "{cd_root}{sg} run -p \"{symbol}\" -l daslang --json \"{search_dir}\""
    if (!empty(glob_filter)) {
        cmd = "{cd_root}{sg} run -p \"{symbol}\" -l daslang --globs \"{glob_filter}\" --json \"{search_dir}\""
    }
    var raw_json : string
    unsafe {
        popen(cmd) <| $(f) {
            if (f == null) {
                return
            }
            raw_json = build_string() <| $(var w) {
                while (!feof(f)) {
                    write(w, fgets(f))
                }
            }
        }
    }
    if (empty(raw_json) || raw_json == "[]") {
        return make_tool_result("No matches found for '{symbol}' in {search_dir}")
    }
    // parse JSON array from ast-grep
    var err : string
    var parsed = read_json(raw_json, err)
    if (parsed == null) {
        let truncated = length(raw_json) > 500 ? slice(raw_json, 0, 500) : raw_json
        return make_tool_result("Failed to parse ast-grep output: {err}\nRaw: {truncated}", true)
    }
    if (!(parsed.value is _array)) {
        unsafe { delete parsed; }
        return make_tool_result("Unexpected ast-grep output format", true)
    }
    // group matches by file and format output
    var by_file : table<string; array<string>>
    var file_order : array<string>
    unsafe {
        let matches & = parsed.value as _array
        for (m in matches) {
            let file_val = m?.file
            let lines_val = m?.lines
            if (file_val == null) {
                continue
            }
            let display_path = make_relative_path(file_val.value as _string, das_root)
            let line_num = get_start_line(m) + 1
            var context = ""
            if (lines_val != null && (lines_val.value is _string)) {
                context = strip(lines_val.value as _string)
            }
            let entry = "  {line_num}: {context}"
            if (!key_exists(by_file, display_path)) {
                file_order |> push(display_path)
            }
            by_file[display_path] |> push(entry)
        }
    }
    unsafe { delete parsed; }
    let total = length(by_file)
    var match_count = 0
    for (fp in file_order) {
        unsafe {
            match_count += length(by_file[fp])
        }
    }
    var output = build_string() <| $(var w) {
        write(w, "Found {match_count} match")
        if (match_count != 1) {
            write(w, "es")
        }
        write(w, " for '{symbol}' in {total} file")
        if (total != 1) {
            write(w, "s")
        }
        write(w, "\n\n")
        for (fp in file_order) {
            write(w, "{fp}:\n")
            unsafe {
                for (entry in by_file[fp]) {
                    write(w, "{entry}\n")
                }
            }
            write(w, "\n")
        }
    }
    return make_tool_result(output)
}
