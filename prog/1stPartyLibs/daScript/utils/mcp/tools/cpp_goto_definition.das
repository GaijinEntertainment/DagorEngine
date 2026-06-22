options gen2

require cpp_common public
require daslib/strings_boost public
require daslib/fio public
require strings public
require math

// `do_cpp_goto_definition` — given a cursor position in a C++ file, return up to
// five plausible definition locations.
//
// **Approximate by design.** No scope resolution, no overload disambiguation,
// no template-specialization tracking. Multiple candidates are listed in
// preference order (same file > same dir > shorter path). For substring or
// usage searches, prefer `cpp_grep_usage`. For precise resolution, wait for the
// clangd-backed v2.
def do_cpp_goto_definition(file, line_str, column_str : string) : string {
    if (empty(file) || empty(line_str) || empty(column_str)) return make_tool_result("'file', 'line', and 'column' are all required", true)
    let line_num = to_int(line_str)
    let col_num = to_int(column_str)
    if (line_num <= 0 || col_num <= 0) return make_tool_result("'line' and 'column' must be positive integers (1-based)", true)
    let abs_path = resolve_path(file)
    var f = fopen(abs_path, "r")
    if (f == null) return make_tool_result("Failed to open '{file}'", true)
    var lines : array<string>
    while (!feof(f)) {
        var ln = fgets(f)
        while (!empty(ln)) {
            let last_ch = character_at(ln, length(ln) - 1) // nolint:PERF003
            if (last_ch == '\n' || last_ch == '\r') {
                ln = slice(ln, 0, length(ln) - 1)
            } else {
                break
            }
        }
        lines |> push(ln)
    }
    fclose(f)
    if (line_num > length(lines)) return make_tool_result("Line {line_num} is past the end of '{file}' ({length(lines)} lines)", true)
    let cursor_line = lines[line_num - 1]
    let line_len = length(cursor_line)
    // Allow col == line_len + 1 (cursor positioned just past the last character —
    // common IDE convention). Clamp the index used for identifier extraction.
    if (col_num > line_len + 1) return make_tool_result("Column {col_num} is past the end of line {line_num}", true)
    if (line_len == 0) return make_tool_result("No identifier at cursor (line {line_num}, col {col_num})", true)
    // Walk left and right from cursor over [A-Za-z0-9_] to extract identifier.
    var start = min(col_num - 1, line_len - 1)
    var endx = start
    var ident = ""
    peek_data(cursor_line) $(buf) {
        if (!is_cpp_ident_word_char(int(buf[start]))) {
            // try the char immediately before (cursor often sits just past the name)
            if (start > 0 && is_cpp_ident_word_char(int(buf[start - 1]))) {
                start--
                endx = start
            } else {
                return
            }
        }
        while (start > 0 && is_cpp_ident_word_char(int(buf[start - 1]))) {
            start--
        }
        while (endx + 1 < line_len && is_cpp_ident_word_char(int(buf[endx + 1]))) {
            endx++
        }
        ident = slice(cursor_line, start, endx + 1)
    }
    if (empty(ident) || !is_cpp_ident_start_char(first_character(ident))) return make_tool_result("No identifier at cursor (line {line_num}, col {col_num})", true)
    // Look up definitions in the lazily-built C++ index. Shared with the
    // with_cpp_source redirect on find_symbol/goto_definition — pays the ~2s
    // scan cost once per server lifetime, then O(1) per goto.
    var candidates <- cpp_lookup_all_by_name(ident)
    if (empty(candidates)) {
        let why = cpp_index_status()
        // Disambiguate "really no matches" from "the index couldn't build".
        if (!empty(why)) return make_tool_result("No definition found for '{ident}' (cpp index unavailable: {why})", true)
        return make_tool_result("No definition found for '{ident}'")
    }
    // Rank: same file > same dir > shorter path
    let cursor_rel = make_relative_path(abs_path, get_das_root())
    let cursor_dir = dir_name(cursor_rel)
    sort(candidates) $(a, b) {
        let af = a.file == cursor_rel ? 0 : 1
        let bf = b.file == cursor_rel ? 0 : 1
        if (af != bf) return af < bf
        let ad = dir_name(a.file) == cursor_dir ? 0 : 1
        let bd = dir_name(b.file) == cursor_dir ? 0 : 1
        if (ad != bd) return ad < bd
        return length(a.file) < length(b.file)
    }
    let max_show = min(CPP_MAX_GOTO_CANDIDATES, length(candidates))
    let output = build_string() $(var w) {
        if (length(candidates) > max_show) {
            w |> write("Found {length(candidates)} candidates for '{ident}' (showing top {max_show}; approximate — no scope resolution):\n\n")
        } else {
            w |> write("Found {length(candidates)} candidate(s) for '{ident}' (approximate — no scope resolution):\n\n")
        }
        for (i in range(max_show)) {
            let e & = unsafe(candidates[i])
            let display = empty(e.qualified) ? e.name : e.qualified
            w |> write("  [{e.kind}] {display}  // {e.file}:{e.line}\n")
        }
    }
    return make_tool_result(output)
}

def private is_cpp_ident_word_char(ch : int) : bool {
    return is_alpha(ch) || is_number(ch) || ch == '_'
}

def private is_cpp_ident_start_char(ch : int) : bool {
    return is_alpha(ch) || ch == '_'
}
