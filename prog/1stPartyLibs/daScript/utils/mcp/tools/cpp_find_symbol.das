options gen2

require cpp_common public
require daslib/strings_boost public
require strings public

// `do_cpp_find_symbol` — search C++ symbol declarations by name + kind.
//
// `query`: case-insensitive substring; prefix `=` for exact case-sensitive match.
// `kind`: optional — one of "function", "class", "struct", "enum", "union",
//          "typedef", "namespace", "macro". Empty matches any kind.
// `directory`: optional — repo-relative or absolute. Empty -> default cpp dirs (src/include/modules).
// `glob_filter`: optional --globs filter (gitignore syntax; `!pattern` for exclusion).
//
// Caveats:
//   - Best-effort name extraction; complex templates and function-pointer typedefs may
//     report partial names. Specializations are NOT separately listed (primary only).
//   - Macro-expanded declarations (e.g. `DAS_BIND_FN(foo)`) are invisible to ast-grep.
def do_cpp_find_symbol(query : string; kind : string = ""; directory : string = ""; glob_filter : string = "") : string {
    if (empty(query)) return make_tool_result("'query' argument is required", true)
    let search_dirs <- cpp_resolve_search_dirs(directory)
    var err : string
    var entries <- cpp_run_scan(search_dirs, glob_filter, err)
    if (!empty(err)) return make_tool_result(err, true)
    var matched : array<int>
    for (i in range(length(entries))) {
        let e & = unsafe(entries[i])
        // try matching against both unqualified and qualified forms
        if (empty(e.name)
            || (!empty(kind) && e.kind != kind)
            || (!symbol_matches(e.name, query) && !symbol_matches(e.qualified, query))) continue
        matched |> push(i)
        if (length(matched) >= CPP_MAX_FIND_RESULTS) break
    }
    if (empty(matched)) return make_tool_result("No matches for '{query}'" + (empty(kind) ? "" : " (kind={kind})"))
    let truncated = length(matched) >= CPP_MAX_FIND_RESULTS
    let output = build_string() $(var w) {
        let header_kind = empty(kind) ? "" : " (kind={kind})"
        w |> write("Found {length(matched)} symbol(s) matching '{query}'{header_kind}:\n")
        // group by file, preserve discovery order
        var by_file : table<string; array<int>>
        var file_order : array<string>
        for (idx in matched) {
            let fp = entries[idx].file
            if (!key_exists(by_file, fp)) {
                file_order |> push(fp)
            }
            by_file[fp] |> push(idx)
        }
        for (fp in file_order) {
            w |> write("\n{fp}:\n")
            for (idx in by_file[fp]) {
                let e & = unsafe(entries[idx])
                let display = empty(e.qualified) ? e.name : e.qualified
                w |> write("  [{e.kind}] {display}  // line {e.line}\n")
            }
        }
        if (truncated) {
            w |> write("\n(truncated at {CPP_MAX_FIND_RESULTS} results)\n")
        }
    }
    return make_tool_result(output)
}
