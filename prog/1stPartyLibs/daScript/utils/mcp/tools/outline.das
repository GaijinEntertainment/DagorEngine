options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require grep_usage public
require daslib/json
require daslib/json_boost
require daslib/strings_boost

// Outline entry: kind + name + line + first-line text
struct OutlineEntry {
    kind : string
    name : string
    line : int
    text : string
}

def get_json_string(v : JsonValue?; field : string) : string {
    if (v == null || !(v.value is _object)) {
        return ""
    }
    unsafe {
        let fld = (v.value as _object)?[field] ?? null
        if (fld != null && (fld.value is _string)) {
            return fld.value as _string
        }
    }
    return ""
}

def get_json_int(v : JsonValue?; field : string) : int {
    if (v == null || !(v.value is _object)) {
        return 0
    }
    unsafe {
        let fld = (v.value as _object)?[field] ?? null
        if (fld == null) {
            return 0
        }
        if (fld.value is _number) {
            return int(fld.value as _number)
        }
        if (fld.value is _longint) {
            return int(fld.value as _longint)
        }
    }
    return 0
}

def rule_id_to_kind(rule_id : string) : string {
    if (find(rule_id, "functions") >= 0) {
        return "function"
    } elif (find(rule_id, "structures") >= 0) {
        return "struct"
    } elif (find(rule_id, "enums") >= 0) {
        return "enum"
    } elif (find(rule_id, "globals") >= 0) {
        return "global"
    } elif (find(rule_id, "typedefs") >= 0) {
        return "typedef"
    } elif (find(rule_id, "bitfields") >= 0) {
        return "bitfield"
    } elif (find(rule_id, "variants") >= 0) {
        return "variant"
    }
    return "unknown"
}

def find_earliest(text : string; chars : array<int>) : int {
    var best = length(text)
    for (ch in chars) {
        let p = find(text, ch)
        if (p >= 0 && p < best) {
            best = p
        }
    }
    return best
}

def extract_name_from_text(text, kind : string) : string {
    // Extract the declaration name from the first line of text
    // e.g. "def foo(x : int) : string {" -> "foo"
    //      "struct Point {" -> "Point"
    //      "[export]\ndef main() {" -> "main" (annotated)
    var line = strip(skip_annotations(text))
    if (kind == "function") {
        // "def name(" or "def name {" or "def name<"
        let def_pos = find(line, "def ")
        if (def_pos >= 0) {
            var rest = strip(slice(line, def_pos + 4))
            // skip visibility: "def private foo" / "def public foo" / "def abstract foo" / "def override foo" / "def static foo" / "def sealed foo" / "def const foo"
            let modifiers = ["private ", "public ", "abstract ", "override ", "static ", "sealed ", "const "]
            for (m in modifiers) {
                if (starts_with(rest, m)) {
                    rest = strip(slice(rest, length(m)))
                }
            }
            // name ends at '(' or '{' or ' ' or end
            return slice(rest, 0, find_earliest(rest, ['(', '{', ' ']))
        }
    } elif (kind == "struct") {
        // "struct Name {" or "class Name {"
        let kw_pos = max(find(line, "struct "), find(line, "class "))
        if (kw_pos >= 0) {
            var rest = strip(slice(line, kw_pos))
            // skip keyword
            let sp = find(rest, " ")
            if (sp >= 0) {
                rest = strip(slice(rest, sp + 1))
            }
            // skip modifiers
            let modifiers = ["public ", "private ", "sealed ", "template "]
            for (m in modifiers) {
                if (starts_with(rest, m)) {
                    rest = strip(slice(rest, length(m)))
                }
            }
            return slice(rest, 0, find_earliest(rest, ['{', ':', ' ']))
        }
    } elif (kind == "enum") {
        let kw_pos = find(line, "enum ")
        if (kw_pos >= 0) {
            var rest = strip(slice(line, kw_pos + 5))
            // skip visibility
            let modifiers = ["public ", "private "]
            for (m in modifiers) {
                if (starts_with(rest, m)) {
                    rest = strip(slice(rest, length(m)))
                }
            }
            return slice(rest, 0, find_earliest(rest, ['{', ' ', ':']))
        }
    } elif (kind == "global") {
        // "let name : type" or "var name = ..."
        let kw_pos = max(find(line, "let "), find(line, "var "))
        if (kw_pos >= 0) {
            var rest = strip(slice(line, kw_pos))
            let sp = find(rest, " ")
            if (sp >= 0) {
                rest = strip(slice(rest, sp + 1))
            }
            // skip modifiers
            let modifiers = ["shared ", "public ", "private "]
            for (m in modifiers) {
                if (starts_with(rest, m)) {
                    rest = strip(slice(rest, length(m)))
                }
            }
            // Handle block form: "let private {" -> skip
            if (first_character(rest) == '{') {
                return ""
            }
            // Handle annotations before name
            if (first_character(rest) == '[') {
                let close = find(rest, "]")
                if (close >= 0) {
                    rest = strip(slice(rest, close + 1))
                }
            }
            return slice(rest, 0, find_earliest(rest, [' ', ':', '=']))
        }
    } elif (kind == "typedef") {
        let kw_pos = find(line, "typedef ")
        if (kw_pos >= 0) {
            var rest = strip(slice(line, kw_pos + 8))
            return slice(rest, 0, find_earliest(rest, [' ', '=']))
        }
    } elif (kind == "bitfield") {
        let kw_pos = find(line, "bitfield ")
        if (kw_pos >= 0) {
            var rest = strip(slice(line, kw_pos + 9))
            let modifiers = ["public ", "private "]
            for (m in modifiers) {
                if (starts_with(rest, m)) {
                    rest = strip(slice(rest, length(m)))
                }
            }
            return slice(rest, 0, find_earliest(rest, ['{', ' ', ':']))
        }
    } elif (kind == "variant") {
        let kw_pos = find(line, "variant ")
        if (kw_pos >= 0) {
            var rest = strip(slice(line, kw_pos + 8))
            let modifiers = ["public ", "private "]
            for (m in modifiers) {
                if (starts_with(rest, m)) {
                    rest = strip(slice(rest, length(m)))
                }
            }
            return slice(rest, 0, find_earliest(rest, ['{', ' ']))
        }
    }
    return ""
}

def skip_annotations(text : string) : string {
    // Skip annotation lines like "[generic]\n[macro_function]\n" to get to the declaration
    var line = text
    while (starts_with(strip(line), "[")) {
        let nl = find(line, "\n")
        if (nl >= 0) {
            line = slice(line, nl + 1)
        } else {
            break
        }
    }
    return line
}

def get_first_line(text : string) : string {
    let clean = skip_annotations(text)
    let nl = find(clean, "\n")
    if (nl >= 0) {
        return strip(slice(clean, 0, nl))
    }
    return strip(clean)
}

def do_outline(file, glob_filter : string) : string {
    if (empty(file) && empty(glob_filter)) {
        return make_tool_result("'file' or 'glob' argument is required", true)
    }
    let das_root = get_das_root()
    let rules_path = "{das_root}/tree-sitter-daslang/outline_rules.yml"
    // build target path(s)
    var target : string
    if (!empty(file)) {
        target = resolve_path(file)
    } elif (!empty(glob_filter)) {
        // glob mode: scan directory
        target = das_root
    }
    let sg = get_ast_grep_cmd()
    let cd_root = "cd \"{das_root}\" && "
    var cmd = "{cd_root}{sg} scan -r \"{rules_path}\" --json \"{target}\""
    if (!empty(glob_filter) && !empty(file)) {
        // both file and glob — use glob as filter
        cmd = "{cd_root}{sg} scan -r \"{rules_path}\" --globs \"{glob_filter}\" --json \"{resolve_path(file)}\""
    } elif (!empty(glob_filter)) {
        cmd = "{cd_root}{sg} scan -r \"{rules_path}\" --globs \"{glob_filter}\" --json \"{das_root}\""
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
        return make_tool_result("No declarations found")
    }
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
    // collect entries
    var entries : array<OutlineEntry>
    var by_file : table<string; array<int>>  // file -> indices into entries
    var file_order : array<string>
    unsafe {
        let matches & = parsed.value as _array
        for (m in matches) {
            let rule_id = get_json_string(m, "ruleId")
            let kind = rule_id_to_kind(rule_id)
            let file_path = get_json_string(m, "file")
            let text = get_json_string(m, "text")
            let rng = (m.value as _object)?["range"] ?? null
            let start = rng != null ? ((rng.value as _object)?["start"] ?? null) : null
            let line_num = start != null ? get_json_int(start, "line") + 1 : 0
            let name = extract_name_from_text(text, kind)
            if (empty(name)) {
                continue  // skip block-form globals like "let private {"
            }
            let display_path = make_relative_path(file_path, das_root)
            let idx = length(entries)
            entries |> emplace(OutlineEntry(kind = kind, name = name, line = line_num, text = get_first_line(text)))
            if (!key_exists(by_file, display_path)) {
                file_order |> push(display_path)
            }
            by_file[display_path] |> push(idx)
        }
    }
    unsafe { delete parsed; }
    // sort entries within each file by line
    for (fp in file_order) {
        unsafe {
            sort(by_file[fp]) <| $(a, b) {
                return entries[a].line < entries[b].line
            }
        }
    }
    // format output
    let multi_file = length(file_order) > 1
    var output = build_string() <| $(var w) {
        for (fp_idx in range(length(file_order))) {
            let fp = file_order[fp_idx]
            if (multi_file) {
                write(w, "{fp}:\n")
            }
            unsafe {
                for (idx in by_file[fp]) {
                    let e & = entries[idx]
                    write(w, "  {e.line}: [{e.kind}] {e.text}\n")
                }
            }
            if (multi_file && fp_idx < length(file_order) - 1) {
                write(w, "\n")
            }
        }
    }
    return make_tool_result(output)
}
