options gen2
options rtti
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/fio
require daslib/strings_convert
require daslib/result


// `export_corpus` shells out to `daslang utils/detect-dupe/main.das`
// (daslang's `require` grammar can't accept hyphens in path components,
// so the in-process pipeline isn't reachable from this tool). The MCP
// envelope returns the corpus path and subprocess output for diagnostics
// — the corpus body itself is not embedded (potentially many MB).

struct ExportCorpusEnvelope {
    exit_code : int
    out : string
    stdout : string                  //!< captured subprocess output
}


def public do_export_corpus(paths : string; paths_file : string; out : string;
                            min_tokens_str : string; workers_str : string) : string {
    let exe = get_daslang_exe()
    if (empty(exe)) {
        return make_tool_result("Cannot determine daslang executable path", true)
    }
    if (empty(out)) {
        return make_tool_result("export_corpus: missing required 'out' parameter (output corpus.json path)", true)
    }
    if (empty(paths) && empty(paths_file)) {
        return make_tool_result("export_corpus: missing input — provide 'paths' (comma/newline list, dirs, globs) and/or 'paths_file' (path to a newline-delimited file)", true)
    }
    let detect_dupe_main = "{get_das_root()}/utils/detect-dupe/main.das"
    let detect_dupe_st = stat(detect_dupe_main)
    if (!detect_dupe_st.is_valid || !detect_dupe_st.is_reg) {
        return make_tool_result("export_corpus: utils/detect-dupe/main.das not found at {detect_dupe_main}", true)
    }
    // Union the two path sources into one seed list. parse_file_list
    // handles comma/newline splitting + glob expansion; paths_file
    // contributes raw newline-delimited entries.
    var seeds : array<string>
    if (!empty(paths)) {
        parse_file_list(paths, seeds)
    }
    if (!empty(paths_file)) {
        let resolved_pf = resolve_path(paths_file)
        let pf_st = stat(resolved_pf)
        if (!pf_st.is_valid || !pf_st.is_reg) {
            return make_tool_result("export_corpus: paths_file not found: {resolved_pf}", true)
        }
        let body = fread(resolved_pf)
        for (raw in split_by_chars(body, "\n")) {
            let line = strip(raw)
            if (empty(line)) {
                continue
            }
            if (line |> starts_with("#")) {
                continue
            }
            seeds |> push(line)
        }
    }
    if (empty(seeds)) {
        return make_tool_result("export_corpus: no paths matched (paths=\"{paths}\", paths_file=\"{paths_file}\")", true)
    }
    let resolved_out = resolve_path(out)
    // Always pass the seed list via a temp --paths-from file. Avoids
    // ARG_MAX blow-ups on PR-sized lists, and keeps the argv path the
    // same whether the input came from `paths`, `paths_file`, or both.
    let temp_paths_file = "{resolved_out}.mcp.paths.{ref_time_ticks()}.txt"
    var write_ok = false
    fopen(temp_paths_file, "wb") $(fw) {
        if (fw == null) {
            return
        }
        write_ok = true
        for (s in seeds) {
            fw |> fprint("{resolve_path(s)}\n")
        }
    }
    if (!write_ok) {
        return make_tool_result("export_corpus: could not write temp paths file: {temp_paths_file}", true)
    }
    var argv <- [
        exe, detect_dupe_main, "--",
        "--paths-from", temp_paths_file,
        "--export-functions", resolved_out
    ]
    if (!empty(min_tokens_str)) {
        let parsed = try_to_int(min_tokens_str)
        if (is_err(parsed)) {
            remove(temp_paths_file)
            return make_tool_result("export_corpus: 'min_tokens' must be a non-negative integer (got '{min_tokens_str}')", true)
        }
        let n = unwrap(parsed)
        if (n < 0) {
            remove(temp_paths_file)
            return make_tool_result("export_corpus: 'min_tokens' must be >= 0 (got {n})", true)
        }
        argv |> push("--min-tokens")
        argv |> push("{n}")
    }
    if (!empty(workers_str)) {
        let parsed = try_to_int(workers_str)
        if (is_err(parsed)) {
            remove(temp_paths_file)
            return make_tool_result("export_corpus: 'workers' must be a non-negative integer (got '{workers_str}')", true)
        }
        let n = unwrap(parsed)
        if (n < 0) {
            remove(temp_paths_file)
            return make_tool_result("export_corpus: 'workers' must be >= 0 (got {n})", true)
        }
        argv |> push("--workers")
        argv |> push("{n}")
    }
    var output : string
    let exit_code = run_and_capture(argv, output, 0.0)
    remove(temp_paths_file)
    if (exit_code != 0) {
        return make_tool_result("export_corpus: detect-dupe failed (exit code {exit_code}):\n{output}", true)
    }
    let out_st = stat(resolved_out)
    if (!out_st.is_valid || !out_st.is_reg) {
        return make_tool_result("export_corpus: detect-dupe completed but corpus missing: {resolved_out}\nSubprocess output:\n{output}", true)
    }
    let envelope = ExportCorpusEnvelope(
        exit_code = exit_code,
        out = resolved_out,
        stdout = output)
    return make_tool_result(sprint_json(envelope, false), false)
}
