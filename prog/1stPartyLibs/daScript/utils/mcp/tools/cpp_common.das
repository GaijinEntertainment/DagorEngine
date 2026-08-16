options gen2

module cpp_common

require common public
require grep_usage public
require outline public
require ../cpp_search_config.das public
require ../../common/git_signature.das
require daslib/json public
require daslib/json_boost public
require daslib/strings_boost public
require strings public
require math public
require daslib/fio public

let CPP_RULES_FILE = "tree-sitter-daslang/cpp_outline_rules.yml"

let private CPP_SEARCH_CONFIG_PATH = "utils/mcp/cpp_search_config.das"

struct CppMatch {
    file : string         // relative to das_root, generic-path
    line : int            // 1-based start line
    end_line : int        // 1-based end line (inclusive); equals `line` for single-line matches
    kind : string         // function/class/struct/enum/union/typedef/namespace/macro
    name : string         // extracted identifier (best-effort, unqualified)
    qualified : string    // fully-qualified name where available (e.g. "Module::addFunction")
    template_args : string // "<float>" / "<T, U>" with surrounding angles for templated entries; "" otherwise
    signature : string    // for functions: collapsed param list with return type (e.g. "void foo(int x) const"); "" for non-functions
    text : string         // first line of declaration (kept for fallback rendering)
}

def cpp_default_search_dirs() : array<string> {
    let root = get_das_root()
    var dirs : array<string>
    dirs |> reserve(length(CPP_SEARCH_DIRS))
    for (d in CPP_SEARCH_DIRS) {
        dirs |> push(path_join(root, d))
    }
    return <- dirs
}

def cpp_rule_id_to_kind(rule_id : string) : string {
    if (find(rule_id, "functions") >= 0) {
        return "function"
    } elif (find(rule_id, "classes") >= 0) {
        return "class"
    } elif (find(rule_id, "structs") >= 0) {
        return "struct"
    } elif (find(rule_id, "enums") >= 0) {
        return "enum"
    } elif (find(rule_id, "unions") >= 0) {
        return "union"
    } elif (find(rule_id, "typedefs") >= 0) {
        return "typedef"
    } elif (find(rule_id, "namespaces") >= 0) {
        return "namespace"
    } elif (find(rule_id, "macros") >= 0) return "macro"
    return "unknown"
}

def is_cpp_ident_char(ch : int) : bool {
    return is_alpha(ch) || is_number(ch) || ch == '_' || ch == ':' || ch == '~'
}

def is_cpp_op_char(ch : int) : bool {
    // chars that may appear as part of operator overloads (operator+, operator==, operator<<, etc.)
    return ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' || ch == '=' || ch == '<' || ch == '>' || ch == '!' || ch == '&' || ch == '|' || ch == '^' || ch == ',' || ch == '(' || ch == ')' || ch == '[' || ch == ']'
}

def cpp_first_line(text : string) : string {
    let nl = find(text, "\n")
    return strip(nl >= 0 ? slice(text, 0, nl) : text)
}

// Find the position of the first '(' that is NOT inside <...> brackets (skips template arg lists).
def find_open_paren(line : string) : int {
    var depth = 0
    for (i in range(length(line))) {
        let ch = character_at(line, i) // nolint:PERF002,PERF003
        if (ch == '<') {
            depth++
        } elif (ch == '>') {
            if (depth > 0) {
                depth--
            }
        } elif (ch == '(' && depth == 0) return i
    }
    return -1
}

// Extract the qualified declared name from a C++ first-line.
// Best-effort. Misses some edge cases (function-pointer typedefs returning fn-ptrs,
// trailing-return-type lambdas, complex template specializations).
// Returns ("name", "qualified") — `qualified` includes ::-scope; `name` is the last segment.
def cpp_extract_name_pair(text : string; kind : string) : tuple<string; string> {
    let line = cpp_first_line(text)
    var qualified = ""
    if (kind == "function") {
        let op = find_open_paren(line)
        if (op > 0) {
            // Walk backwards from `op` over ident/op chars to find the function name span.
            var i = op - 1
            while (i >= 0 && (character_at(line, i) == ' ' || character_at(line, i) == '\t')) { // nolint:PERF002,PERF003
                i--
            }
            let span_end = i + 1
            // First, check if this looks like an `operator<X>` form by walking back over op chars.
            while (i >= 0 && is_cpp_op_char(character_at(line, i))) { // nolint:PERF002,PERF003
                i--
            }
            // Now walk back over ident chars (covers regular names + qualifiers + ~dtor + "operator")
            while (i >= 0 && is_cpp_ident_char(character_at(line, i))) { // nolint:PERF002,PERF003
                i--
            }
            let span_start = i + 1
            if (span_end > span_start) {
                qualified = slice(line, span_start, span_end)
            }
        }
    } elif (kind == "class" || kind == "struct" || kind == "union") {
        let kw = kind == "class" ? "class " : (kind == "struct" ? "struct " : "union ")
        let kw_pos = find(line, kw)
        if (kw_pos >= 0) {
            var rest = strip(slice(line, kw_pos + length(kw)))
            // skip optional API macros (any UPPER_SNAKE token before the name)
            while (!empty(rest) && (first_character(rest) >= 'A' && first_character(rest) <= 'Z')) { // nolint:PERF014
                let first_space = find(rest, " ")
                if (first_space < 0) break
                let head = slice(rest, 0, first_space)
                // Heuristic: if the head is all uppercase/underscore and ≥ 3 chars, treat it as macro.
                var all_upper = true
                for (i in range(length(head))) {
                    let c = character_at(head, i) // nolint:PERF002,PERF003
                    if (!((c >= 'A' && c <= 'Z') || c == '_' || is_number(c))) { // nolint:PERF014
                        all_upper = false
                        break
                    }
                }
                if (!all_upper || length(head) < 3) break
                // Don't skip when the next token isn't another identifier — that would mean
                // `head` is the actual class name (e.g. `class DAS_API LOG : public ...` —
                // we already skipped DAS_API; LOG looks like a macro by shape, but the
                // following `:` proves it's the class name).
                let after = strip(slice(rest, first_space + 1))
                if (empty(after) || first_character(after) == ':' || first_character(after) == '<' || first_character(after) == '{' || first_character(after) == ';') break
                rest = strip(slice(rest, first_space + 1))
            }
            // name ends at '{', ':', '<', whitespace, or ';'
            var nend = length(rest)
            for (i in range(length(rest))) {
                let c = character_at(rest, i) // nolint:PERF002,PERF003
                if (c == '{' || c == ':' || c == '<' || c == ' ' || c == '\t' || c == ';') {
                    nend = i
                    break
                }
            }
            qualified = slice(rest, 0, nend)
        }
    } elif (kind == "enum") {
        let kw_pos = find(line, "enum ")
        if (kw_pos >= 0) {
            var rest = strip(slice(line, kw_pos + 5))
            if (starts_with(rest, "class ")) {
                rest = strip(slice(rest, 6))
            } elif (starts_with(rest, "struct ")) {
                rest = strip(slice(rest, 7))
            }
            if (!empty(rest) && first_character(rest) != '{') {
                var nend = length(rest)
                for (i in range(length(rest))) {
                    let c = character_at(rest, i) // nolint:PERF002,PERF003
                    if (c == '{' || c == ':' || c == ' ' || c == '\t' || c == ';') {
                        nend = i
                        break
                    }
                }
                qualified = slice(rest, 0, nend)
            }
        }
    } elif (kind == "namespace") {
        let kw_pos = find(line, "namespace ")
        if (kw_pos >= 0) {
            let rest = strip(slice(line, kw_pos + 10))
            var nend = length(rest)
            for (i in range(length(rest))) {
                let c = character_at(rest, i) // nolint:PERF002,PERF003
                if (c == '{' || c == ' ' || c == '\t' || c == ';') {
                    nend = i
                    break
                }
            }
            qualified = slice(rest, 0, nend)
        }
    } elif (kind == "macro") {
        let pos = find(line, "#define")
        if (pos >= 0) {
            let rest = strip(slice(line, pos + 7))
            var nend = length(rest)
            for (i in range(length(rest))) {
                let c = character_at(rest, i) // nolint:PERF002,PERF003
                if (c == '(' || c == ' ' || c == '\t') {
                    nend = i
                    break
                }
            }
            qualified = slice(rest, 0, nend)
        }
    } elif (kind == "typedef") {
        // C++11 alias form: "using NAME = ...;"  (matches cpp-outline-typedefs-using rule).
        // Take the identifier between "using " and the first "=".
        if (starts_with(line, "using ")) {
            let after_using = strip(slice(line, 6))
            let eq_pos = find(after_using, "=")
            if (eq_pos > 0) {
                qualified = strip(slice(after_using, 0, eq_pos))
            }
        }
        if (empty(qualified)) {
            // function-pointer form: "typedef <ret> (*NAME)(args);"
            let star_paren = find(line, "(*")
            if (star_paren >= 0) {
                let rest = strip(slice(line, star_paren + 2))
                var nend = length(rest)
                for (i in range(length(rest))) {
                    let c = character_at(rest, i) // nolint:PERF002,PERF003
                    if (c == ')' || c == ' ' || c == ',' || c == '\t') {
                        nend = i
                        break
                    }
                }
                qualified = slice(rest, 0, nend)
            } else {
                // simple form: take last identifier before ;
                var clean = line
                let semi = find(clean, ";")
                if (semi >= 0) {
                    clean = slice(clean, 0, semi)
                }
                clean = strip(clean)
                var ce = length(clean)
                while (ce > 0 && !is_cpp_ident_char(character_at(clean, ce - 1))) { // nolint:PERF002,PERF003
                    ce--
                }
                var cs = ce
                while (cs > 0 && is_cpp_ident_char(character_at(clean, cs - 1))) { // nolint:PERF002,PERF003
                    cs--
                }
                qualified = slice(clean, cs, ce)
            }
        }
    }
    // unqualified: split off the last ::-segment
    var unq = qualified
    let last_sep = rfind(qualified, "::")
    if (last_sep >= 0) {
        unq = slice(qualified, last_sep + 2)
    }
    return unq => qualified
}

// Convenience accessor — returns just the unqualified name.
def cpp_extract_name(text : string; kind : string) : string {
    let pair = cpp_extract_name_pair(text, kind)
    return pair._0
}

// Collapse all whitespace runs (newlines, tabs, multi-space) to a single space.
// Used to normalize multi-line declarations down to one display line.
def cpp_collapse_whitespace(s : string) : string {
    var out : array<string>
    let n = length(s)
    var i = 0
    while (i < n) {
        while (i < n && (character_at(s, i) == ' ' || character_at(s, i) == '\t' || character_at(s, i) == '\n' || character_at(s, i) == '\r')) { // nolint:PERF002,PERF003
            i++
        }
        if (i >= n) break
        let start = i
        while (i < n && character_at(s, i) != ' ' && character_at(s, i) != '\t' && character_at(s, i) != '\n' && character_at(s, i) != '\r') { // nolint:PERF002,PERF003
            i++
        }
        out |> push(slice(s, start, i))
    }
    return join(out, " ")
}

// For functions: extract the signature — return type + name + balanced (params) + trailing
// qualifiers (const/noexcept/override/final). Multi-line input is collapsed to a single line.
// Returns "" for non-functions or when the text doesn't look parsable.
def cpp_extract_signature(full_text : string; kind : string) : string {
    if (kind != "function") return ""
    let paren_open = find_open_paren(full_text)
    if (paren_open <= 0) return ""
    // Walk balanced parens forward to find matching ')'.
    var depth = 1
    var i = paren_open + 1
    let n = length(full_text)
    while (i < n && depth > 0) {
        let ch = character_at(full_text, i) // nolint:PERF002,PERF003
        if (ch == '(') {
            depth++
        } elif (ch == ')') {
            depth--
        }
        i++
    }
    if (depth != 0) return ""
    let paren_close = i - 1
    // Walk back from '(' over whitespace, then over operator chars + identifier chars
    // to find the start of the function name.
    var j = paren_open - 1
    while (j >= 0 && (character_at(full_text, j) == ' ' || character_at(full_text, j) == '\t' || character_at(full_text, j) == '\n')) { // nolint:PERF002,PERF003
        j--
    }
    while (j >= 0 && is_cpp_op_char(character_at(full_text, j))) { // nolint:PERF002,PERF003
        j--
    }
    while (j >= 0 && is_cpp_ident_char(character_at(full_text, j))) { // nolint:PERF002,PERF003
        j--
    }
    let name_start = j + 1
    // Return type spans from the previous statement boundary up to name_start.
    var ret_start = name_start
    while (ret_start > 0) {
        let prev = character_at(full_text, ret_start - 1) // nolint:PERF002,PERF003
        if (prev == '{' || prev == '}' || prev == ';') break
        ret_start--
    }
    // Skip leading whitespace in return type.
    while (ret_start < name_start && (character_at(full_text, ret_start) == ' ' || character_at(full_text, ret_start) == '\t' || character_at(full_text, ret_start) == '\n')) { // nolint:PERF002,PERF003
        ret_start++
    }
    let ret_type = strip(cpp_collapse_whitespace(slice(full_text, ret_start, name_start)))
    let name_part = strip(cpp_collapse_whitespace(slice(full_text, name_start, paren_open)))
    let params = cpp_collapse_whitespace(slice(full_text, paren_open, paren_close + 1))
    // Trailing qualifiers between ')' and '{' or ';'.
    var trailing_end = n
    for (k in range(paren_close + 1, n)) {
        let ch = character_at(full_text, k) // nolint:PERF002,PERF003
        if (ch == '{' || ch == ';') {
            trailing_end = k
            break
        }
    }
    let trailing = strip(cpp_collapse_whitespace(slice(full_text, paren_close + 1, trailing_end)))
    var result = empty(ret_type) ? "" : "{ret_type} "
    result = "{result}{name_part}{params}"
    if (!empty(trailing)) {
        result = "{result} {trailing}"
    }
    return result
}

// Extract template specialization args ("<float>" / "<T*>") for a struct/class/union
// or function template specialization. Returns empty when there are no args.
def cpp_extract_template_args(full_text : string; kind : string; name : string) : string {
    if (empty(name)) return ""
    var search_name = name
    let last_sep = rfind(search_name, "::")
    if (last_sep >= 0) {
        search_name = slice(search_name, last_sep + 2)
    }
    if (starts_with(search_name, "operator")) return ""  // operator<< / operator< would false-match
    let n = length(full_text)
    // Bound the search to the declaration prefix.
    var bound = n
    if (kind == "function") {
        let p = find_open_paren(full_text)
        if (p > 0) {
            bound = p
        }
    } elif (kind == "struct" || kind == "class" || kind == "union") {
        for (i in range(n)) {
            let c = character_at(full_text, i) // nolint:PERF002,PERF003
            if (c == '{' || c == ';') {
                bound = i
                break
            }
        }
    } else {
        return ""
    }
    let needle = "{search_name}<"
    let pos = find(full_text, needle)
    if (pos < 0 || pos >= bound) return ""
    // 'pos' is start of name; '<' is at pos + length(name).
    let lt = pos + length(search_name)
    var depth = 1
    var i2 = lt + 1
    while (i2 < n && depth > 0) {
        let ch = character_at(full_text, i2) // nolint:PERF002,PERF003
        if (ch == '<') {
            depth++
        } elif (ch == '>') {
            depth--
        }
        i2++
    }
    if (depth != 0) return ""
    return slice(full_text, lt, i2)
}

// Escape regex meta-characters so the input string matches itself literally.
// Used when generating ast-grep rules from arbitrary user-supplied symbol names
// (e.g. operator overloads with `+` / `<` characters).
def cpp_regex_escape(s : string) : string {
    return build_string() $(var w) {
        let n = length(s)
        for (i in range(n)) {
            let c = character_at(s, i) // nolint:PERF002,PERF003
            if (c == '.' || c == '+' || c == '*' || c == '?' || c == '(' || c == ')'
                || c == '[' || c == ']' || c == '{' || c == '}' || c == '|'
                || c == '^' || c == '$' || c == '\\') {
                w |> write("\\")
            }
            w |> write(slice(s, i, i + 1))
        }
    }
}

// Process-local temp path for the generated grep rule file. Allocated lazily
// on first call via `create_temp_file_result` (which yields a unique path
// inside the system temp directory — `$TMPDIR` on macOS/Linux, `%TEMP%` on
// Windows — so concurrent MCP server instances don't race on the same path)
// and reused for subsequent calls so we don't accumulate one file per call.
var private CPP_GREP_RULE_PATH : string

// Generate an ast-grep multi-rule YAML file that matches `symbol` as any C++
// identifier-leaf node kind: identifier, type_identifier, namespace_identifier,
// field_identifier. The single-pattern form (`sg run -p X`) only matches bare
// identifier expressions, missing every use where the symbol is a child of a
// larger node (`X writer;`, `X::method()`, `class Y : public X`).
//
// On success, returns the path to the written file (process-local, overwritten
// on every call). On failure (path-allocation or write error), returns `""` and
// populates `err_message` so the caller can surface a clear error instead of
// running `sg scan -r <stale_or_missing_path>`.
def cpp_write_grep_rule_file(symbol : string; var err_message : string&) : string {
    if (empty(CPP_GREP_RULE_PATH)) {
        let r = create_temp_file_result("das_mcp_grep", ".yml")
        if (r is value) {
            CPP_GREP_RULE_PATH = unsafe(r.value)
        } else {
            // Fallback to system tmpdir with a stable name; concurrent server
            // instances may race here but that's strictly better than a hard
            // failure on platforms where create_temp_file misbehaves.
            let td = temp_directory_result()
            let dir = (td is value) ? unsafe(td.value) : "."
            CPP_GREP_RULE_PATH = path_join(dir, "das_mcp_grep_rule.yml")
        }
    }
    let escaped = cpp_regex_escape(symbol)
    let kinds = ["identifier", "type_identifier", "namespace_identifier", "field_identifier"]
    let body = build_string() $(var w) {
        for (i in range(length(kinds))) {
            if (i > 0) {
                w |> write("---\n")
            }
            w |> write("id: cpp-grep-{kinds[i]}\n")
            w |> write("language: cpp\n")
            w |> write("rule:\n")
            w |> write("  kind: {kinds[i]}\n")
            w |> write("  regex: '^{escaped}$'\n")
        }
    }
    var success = false
    fopen(CPP_GREP_RULE_PATH, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint(body)
            success = true
        }
    }
    if (!success) {
        err_message = "Failed to write ast-grep rule file: {CPP_GREP_RULE_PATH}"
        return ""
    }
    return CPP_GREP_RULE_PATH
}

// Walk `search_dirs` looking for sub-folders that contain a `.git` file or
// directory — submodules, daspkg-installed packages, FetchContent destinations,
// ad-hoc clones. Returns `--globs` exclusion patterns for each, except those
// listed in `overrides`. Compares overrides as repo-relative generic paths.
def cpp_collect_git_excludes(search_dirs : array<string>; overrides : array<string>) : array<string> {
    var excl : array<string>
    var seen : table<string; bool>
    let das_root = get_das_root()
    for (sd in search_dirs) {
        // Use the full repo-relative prefix, not just `base_name(sd)` — so that
        // a nested search root like `modules/dasSQLITE` produces overrides
        // keyed `modules/dasSQLITE/...`, matching how `CPP_SEARCH_INCLUDE_OVERRIDES`
        // is documented (repo-relative generic paths).
        let sd_rel = to_generic_path(make_relative_path(sd, das_root))
        dir_rec(sd) $(filename, _is_dir) {
            if (base_name(filename) != ".git") return
            let inner_repo = dir_name(filename)  // ".daspkg_cache/index"
            let rel = empty(inner_repo) ? sd_rel : "{sd_rel}/{to_generic_path(inner_repo)}"
            if (key_exists(seen, rel)) return
            seen[rel] = true
            for (ov in overrides) {
                if (ov == rel) return
            }
            excl |> push("!**/{rel}/**")
        }
    }
    return <- excl
}

// Append the standard `--globs` arguments (always-excludes + auto-`.git`-folder
// excludes for `search_dirs` + `*.cpp/*.h/*.hpp` extension lock + caller's
// optional `extra_glob_filter`) to `args`. Used by both `cpp_run_scan` (via
// rules-yaml) and `cpp_grep_usage` (via `sg run -p`).
def cpp_push_glob_args(var args : array<string>; search_dirs : array<string>; extra_glob_filter : string) {
    let git_excludes <- cpp_collect_git_excludes(search_dirs, CPP_SEARCH_INCLUDE_OVERRIDES)
    args |> reserve(length(args)
        + 2 * length(CPP_SEARCH_ALWAYS_EXCLUDE)
        + 2 * length(git_excludes)
        + 2 * length(CPP_SEARCH_INCLUDE_GLOBS)
        + 2)
    for (g in CPP_SEARCH_ALWAYS_EXCLUDE) {
        args |> push("--globs")
        args |> push(g)
    }
    for (g in git_excludes) {
        args |> push("--globs")
        args |> push(g)
    }
    for (g in CPP_SEARCH_INCLUDE_GLOBS) {
        args |> push("--globs")
        args |> push(g)
    }
    if (!empty(extra_glob_filter)) {
        args |> push("--globs")
        args |> push(extra_glob_filter)
    }
}

// Run `sg scan` with cpp rules. Returns array of CppMatch.
// On error, fills err_message and returns empty array.
def cpp_run_scan(search_dirs : array<string>; glob_filter : string; var err_message : string&) : array<CppMatch> {
    var entries : array<CppMatch>
    let sg = get_ast_grep_cmd()
    if (empty(sg)) {
        err_message = "ast-grep not available"
        return <- entries
    }
    let das_root = get_das_root()
    let rules_path = path_join(das_root, CPP_RULES_FILE)
    var dirs_to_scan : array<string>
    if (empty(search_dirs)) {
        dirs_to_scan <- cpp_default_search_dirs()
    } else {
        dirs_to_scan |> push_from(search_dirs)
    }
    var args <- [sg, "scan", "-r", rules_path, "--json"]
    cpp_push_glob_args(args, dirs_to_scan, glob_filter)
    args |> push_from(dirs_to_scan)
    var raw : string
    let rc = run_and_capture(args, raw, 60.0)
    raw = strip_ast_grep_banner(raw)
    if (rc != 0 && empty(raw)) {
        err_message = "sg scan failed (exit {rc})"
        return <- entries
    }
    if (empty(raw) || raw == "[]") return <- entries
    // Parse-first: on Windows, sg sometimes exits non-zero while still emitting
    // valid JSON to stdout (warnings on stderr get merged in by run_and_capture).
    // Try to parse; only fail if parse itself fails, and enrich the message with
    // the rc when sg exited non-zero (likely a real failure with non-JSON text).
    var parse_err : string
    var parsed = read_json(raw, parse_err)
    if (parsed == null) {
        let truncated = length(raw) > 500 ? slice(raw, 0, 500) : raw
        if (rc != 0) {
            err_message = "sg scan failed (exit {rc}): {truncated}"
        } else {
            err_message = "Failed to parse sg scan output: {parse_err}\nRaw: {truncated}"
        }
        return <- entries
    }
    if (!(parsed.value is _array)) {
        unsafe { delete parsed; }
        err_message = "Unexpected sg scan output format"
        return <- entries
    }
    unsafe {
        let matches & = parsed.value as _array
        entries |> reserve(length(matches))
        for (m in matches) {
            let rule_id = get_json_string(m, "ruleId")
            let kind = cpp_rule_id_to_kind(rule_id)
            let file_path = get_json_string(m, "file")
            // Prefer the JSON `lines` field over `text`. `text` is the matched node's
            // exact span — for tree-sitter-cpp misparses (e.g. `class DAS_API Foo`
            // gets parsed as a class_specifier covering only "class DAS_API"), `text`
            // is truncated and the name extractor reads "DAS_API" instead of "Foo".
            // `lines` gives the full source line(s), which is strictly more informative.
            let lines = get_json_string(m, "lines")
            let rng = (m.value as _object)?["range"] ?? null
            let start = rng != null ? ((rng.value as _object)?["start"] ?? null) : null
            let endp = rng != null ? ((rng.value as _object)?["end"] ?? null) : null
            let line_num = start != null ? get_json_int(start, "line") + 1 : 0
            let end_line_num = endp != null ? get_json_int(endp, "line") + 1 : line_num
            let pair = cpp_extract_name_pair(lines, kind)
            let sig = cpp_extract_signature(lines, kind)
            let tmpl = cpp_extract_template_args(lines, kind, pair._1)
            entries |> emplace(CppMatch(
                file = make_relative_path(file_path, das_root),
                line = line_num,
                end_line = end_line_num,
                kind = kind,
                name = pair._0,
                qualified = pair._1,
                template_args = tmpl,
                signature = sig,
                text = cpp_first_line(lines)
            ))
        }
    }
    unsafe { delete parsed; }
    // Tree-sitter-cpp can't parse `class DAS_API Foo : Bar { ... }` — it emits both a
    // truncated class_specifier (range covers only "class DAS_API", no body) AND a
    // bogus function_definition that DOES cover the whole body. Transfer the bogus
    // function's range onto the class match so methods inside still nest properly,
    // then drop the bogus function.
    cpp_extend_class_ranges(entries)
    return <- cpp_filter_entries(entries)
}

// Walk `entries`; for each `function` match that's actually a misparsed class body
// (same file:start_line as a class/struct match, text starts with "class "/"struct "),
// extend the class/struct match's `end_line` to cover the bogus function's range.
// Mutates `entries` in place.
def cpp_extend_class_ranges(var entries : array<CppMatch>) {
    var class_idx_at : table<string; int>
    for (i in range(length(entries))) {
        let e & = unsafe(entries[i])
        if (e.kind == "class" || e.kind == "struct") {
            class_idx_at["{e.file}:{e.line}"] = i
        }
    }
    for (e in entries) {
        if (e.kind != "function") continue
        let key = "{e.file}:{e.line}"
        if (!key_exists(class_idx_at, key)) continue
        let stripped = strip(e.text)
        if (!starts_with(stripped, "class ") && !starts_with(stripped, "struct ")) continue
        let cls_idx = class_idx_at[key]
        if (entries[cls_idx].end_line < e.end_line) {
            entries[cls_idx].end_line = e.end_line
        }
    }
}

// Drop two classes of garbage matches:
// 1. tree-sitter-cpp misparses `class DAS_API Foo : Bar { ... }` as a function_definition
//    (DAS_API is unknown, so the parser reads `class DAS_API` as a return type and `Foo`
//    as a function name). These collide with the legitimate class/struct match at the
//    same start line and have "class " or "struct " as their text prefix.
// 2. Anonymous unions/structs/enums (`union { int a; float f; };`,
//    `enum { type = X };` inside a template specialization). Name extractor returns
//    empty `qualified`; these are inline members and add no information.
// Filters entries IN PLACE (mutates `entries`), removing the two classes of garbage.
// Returns the modified `entries` (move-out). `entries` is left empty on the caller side.
def cpp_filter_entries(var entries : array<CppMatch>) : array<CppMatch> {
    // Build a set of (file, start_line) pairs that have a class/struct match.
    let class_struct_at <- {for (e in entries); "{e.file}:{e.line}" => true; where e.kind == "class" || e.kind == "struct"}
    // Walk forward, write the survivors to a write_idx; truncate at the end.
    var write_idx = 0
    for (read_idx in range(length(entries))) {
        let e & = unsafe(entries[read_idx])
        var keep = true
        if (e.kind == "function") {
            let key = "{e.file}:{e.line}"
            if (key_exists(class_struct_at, key)
                && (starts_with(strip(e.text), "class ") || starts_with(strip(e.text), "struct "))) {
                keep = false
            }
        }
        if ((e.kind == "union" || e.kind == "struct" || e.kind == "enum") && empty(e.qualified)) {
            keep = false
        }
        if (keep) {
            if (write_idx != read_idx) {
                entries[write_idx] = entries[read_idx]
            }
            write_idx++
        }
    }
    entries |> resize(write_idx)
    return <- entries
}

// Resolve search directories from a single `directory` arg.
// Empty -> default cpp dirs. Otherwise a single absolute or repo-relative path.
def cpp_resolve_search_dirs(directory : string) : array<string> {
    var dirs : array<string>
    if (empty(directory)) return <- cpp_default_search_dirs()
    dirs |> push(resolve_path(directory))
    return <- dirs
}

// ── Lazy index for redirect + cpp_goto_definition ─────────────────────
//
// Built on first call (~2s scan over `CPP_SEARCH_DIRS`). Subsequent calls
// recompute a cheap signature (~70-200ms: git rev-parse HEAD + git status
// filtered to .cpp/.h/.hpp under search scope + per-file mtimes + config
// file mtime). On signature mismatch, the index is rebuilt; otherwise the
// cached state is reused.
//
// Maps both unqualified (`addFunction`) and qualified (`Module::addFunction`)
// names to CppMatch indices.  Powers find_symbol's `with_cpp_source`
// (first match) and cpp_goto_definition (all matches, ranked by caller).

var cpp_index_signature : string  // empty == not built yet
var cpp_index : table<string; array<int>>
var cpp_index_entries : array<CppMatch>
// Last build error (e.g. "ast-grep not available", "sg scan failed (exit 1)").
// Cleared on a successful build. Surfaced by callers via `cpp_index_status`
// so users get a real reason instead of a silent "no matches".
var cpp_index_error : string

// `*.cpp` / `*.h` / `*.hpp` style patterns are simple suffixes (no embedded
// glob meta-chars) — a substring compare is sufficient and avoids pulling
// in a full glob engine just for extension checks.
def private ends_with_any(path : string; suffix_globs : array<string>) : bool {
    for (g in suffix_globs) {
        // strip leading "*" if present (e.g. "*.cpp" -> ".cpp")
        let suffix = starts_with(g, "*") ? slice(g, 1) : g
        if (ends_with(path, suffix)) return true
    }
    return false
}

// `cpp_collect_git_excludes` returns ast-grep `--globs` patterns of the
// form `!**/<rel>/**`; strip the wrapping to get bare repo-relative
// prefixes suitable for prefix-match pruning.
def private collect_prune_prefixes(search_dirs : array<string>) : array<string> {
    let raw_excludes <- cpp_collect_git_excludes(search_dirs, CPP_SEARCH_INCLUDE_OVERRIDES)
    var pruned : array<string>
    pruned |> reserve(length(raw_excludes))
    for (p in raw_excludes) {
        if (starts_with(p, "!**/") && ends_with(p, "/**")) {
            pruned |> push(slice(p, 4, length(p) - 3))
        }
    }
    return <- pruned
}

def private is_pruned(path : string; prune_dirs : array<string>) : bool {
    for (pr in prune_dirs) {
        if (starts_with(path, "{pr}/")) return true
    }
    return false
}

// Compute the staleness signature for the cpp index. Wraps the shared
// git_signature.compute_signature with cpp's predicate (extension globs
// + vendored-tree pruning) and folds in the cpp_search_config.das mtime
// so config edits invalidate the cache too.
def ensure_cpp_index() {
    let das_root = get_das_root()
    let cfg_path = path_join(das_root, CPP_SEARCH_CONFIG_PATH)
    let cfg_sig = "cfg:{stat_mtime_or_zero(cfg_path)}"
    let prune_dirs <- collect_prune_prefixes(cpp_default_search_dirs())
    let core = compute_signature(das_root,
                                 unsafe(@ capture(& prune_dirs) (p : string) => ends_with_any(p, CPP_SEARCH_INCLUDE_GLOBS) && !is_pruned(p, prune_dirs)),
                                 cpp_default_search_dirs())
    let sig = empty(core) ? "" : "{cfg_sig}:{core}"
    if (!empty(sig) && sig == cpp_index_signature) return  // cache hit
    // rebuild from scratch
    cpp_index |> clear()
    cpp_index_entries |> clear()
    var err : string
    var entries <- cpp_run_scan(cpp_default_search_dirs(), "", err)
    if (!empty(err)) {
        cpp_index_signature = "" // reset; next call retries the scan
        cpp_index_error = err
        return
    }
    for (e in entries) {
        if (empty(e.name)) continue
        cpp_index_entries |> push(e)
        let new_idx = length(cpp_index_entries) - 1
        cpp_index[e.name] |> push(new_idx) // nolint:PERF006
        if (!empty(e.qualified) && e.qualified != e.name) {
            cpp_index[e.qualified] |> push(new_idx) // nolint:PERF006
        }
    }
    cpp_index_signature = sig // commit on success
    cpp_index_error = ""       // clear last error
}

// Returns the last error from `ensure_cpp_index`, or "" if the index built
// successfully (or hasn't been queried yet). Callers that have an opportunity
// to surface a "tool unavailable" message to the user (e.g. `cpp_goto_definition`
// when no candidates are found) should call this to disambiguate "really no
// matches" from "the index couldn't build".
def cpp_index_status() : string {
    return cpp_index_error
}

// Look up a C++ name (qualified or unqualified). Returns true if found and
// fills `match` with the FIRST stored match. Tries the name as given first,
// then strips ::-qualifiers and tries the unqualified suffix.
def cpp_lookup_by_name(cppName : string; var match : CppMatch&) : bool {
    if (empty(cppName)) return false
    ensure_cpp_index()
    if (key_exists(cpp_index, cppName)) {
        let idx = cpp_index[cppName][0]
        match = cpp_index_entries[idx]
        return true
    }
    let last_sep = rfind(cppName, "::")
    if (last_sep >= 0) {
        let unq = slice(cppName, last_sep + 2)
        if (key_exists(cpp_index, unq)) {
            let idx = cpp_index[unq][0]
            match = cpp_index_entries[idx]
            return true
        }
    }
    return false
}

// Like cpp_lookup_by_name but scans `dirs` fresh (no global cache touched).
// Used by `with_cpp_source` callers that want a narrower scan than the
// default CPP_SEARCH_DIRS — typical for tests pointing at a specific subtree
// (e.g. "src/builtin") to avoid the multi-hundred-file index build cost.
//
// Single-symbol callers only — re-scans on every call. Multi-symbol callers
// (e.g. find_symbol) should call cpp_run_scan once and use
// cpp_lookup_by_name_in_entries below to avoid N rescans of the same dirs.
def cpp_lookup_by_name_scoped(cppName : string; dirs : array<string>; var match : CppMatch&) : bool {
    if (empty(cppName) || empty(dirs)) return false
    var err : string
    let entries <- cpp_run_scan(dirs, "", err)
    if (!empty(err)) return false
    return cpp_lookup_by_name_in_entries(cppName, entries, match)
}

// Lookup by name against a pre-scanned entry list. Use when one tool call
// resolves N symbols against the same dirs — scan once, look up N times.
def cpp_lookup_by_name_in_entries(cppName : string; entries : array<CppMatch>; var match : CppMatch&) : bool {
    if (empty(cppName)) return false
    let last_sep = rfind(cppName, "::")
    let unq = last_sep >= 0 ? slice(cppName, last_sep + 2) : ""
    for (e in entries) {
        if (empty(e.name)) continue
        if (e.name == cppName || e.qualified == cppName || (!empty(unq) && e.name == unq)) {
            match = e
            return true
        }
    }
    return false
}

// Return ALL CppMatch entries indexed under `name` (or its unqualified form).
// Empty array if absent.  Used by cpp_goto_definition.  The returned array
// holds copies (not references) so callers can sort freely.
def cpp_lookup_all_by_name(name : string) : array<CppMatch> {
    var out : array<CppMatch>
    if (empty(name)) return <- out
    ensure_cpp_index()
    var indices : array<int>
    if (key_exists(cpp_index, name)) {
        indices := cpp_index[name]
    } else {
        let last_sep = rfind(name, "::")
        if (last_sep >= 0) {
            let unq = slice(name, last_sep + 2)
            if (key_exists(cpp_index, unq)) {
                indices := cpp_index[unq]
            }
        }
    }
    out |> reserve(length(indices))
    for (idx in indices) {
        out |> push(cpp_index_entries[idx])
    }
    return <- out
}
