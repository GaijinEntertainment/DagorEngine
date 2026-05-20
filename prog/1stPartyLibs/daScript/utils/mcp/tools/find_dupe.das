options gen2
options rtti
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/fio
require daslib/strings_convert
require daslib/result
require judge_duplicates


// `find_dupe` runs detect-dupe against `paths` (file/dir/glob list) and
// hands the resulting JSON report off to find-dupe's judge. Both stages
// are subprocess calls — detect-dupe because daslang's `require` grammar
// can't reach `utils/detect-dupe/` (hyphen), judge because of the daspkg
// dependency.

def public do_find_dupe(paths : string;
                        out_dir : string;
                        model : string;
                        threshold_str : string;
                        max_clusters_str : string;
                        dry_run_str : string) : string {
    let exe = get_daslang_exe()
    if (empty(exe)) {
        return make_tool_result("Cannot determine daslang executable path", true)
    }
    if (empty(paths)) {
        return make_tool_result("find_dupe: missing required 'paths' parameter", true)
    }
    var files : array<string>
    parse_file_list(paths, files)
    if (empty(files)) {
        return make_tool_result("find_dupe: no files matched: {paths}", true)
    }
    let detect_dupe_main = "{get_das_root()}/utils/detect-dupe/main.das"
    let detect_dupe_st = stat(detect_dupe_main)
    if (!detect_dupe_st.is_valid || !detect_dupe_st.is_reg) {
        return make_tool_result("find_dupe: utils/detect-dupe/main.das not found at {detect_dupe_main}", true)
    }
    if (!(empty(model) || model == "haiku" || model == "sonnet")) {
        return make_tool_result("find_dupe: 'model' must be 'haiku' or 'sonnet' (got '{model}')", true)
    }
    let dry_run = dry_run_str == "true" || dry_run_str == "1"
    var max_clusters = 0
    if (!empty(max_clusters_str)) {
        let parsed = try_to_int(max_clusters_str)
        if (is_err(parsed)) {
            return make_tool_result("find_dupe: 'max_clusters' must be a non-negative integer (got '{max_clusters_str}')", true)
        }
        max_clusters = unwrap(parsed)
        if (max_clusters < 0) {
            return make_tool_result("find_dupe: 'max_clusters' must be >= 0 (got {max_clusters})", true)
        }
    }
    var threshold : float = -1.0
    if (!empty(threshold_str)) {
        let parsed = try_to_float(threshold_str)
        if (is_err(parsed)) {
            return make_tool_result("find_dupe: 'threshold' must be a number in [0, 1] (got '{threshold_str}')", true)
        }
        threshold = unwrap(parsed)
        if (threshold < 0.0 || threshold > 1.0) {
            return make_tool_result("find_dupe: 'threshold' must be in [0, 1] (got {threshold})", true)
        }
    }
    let resolved_out = resolve_path(empty(out_dir) ? "{get_das_root()}/_mcp_find_dupe_{ref_time_ticks()}" : out_dir)
    mkdir(resolved_out)
    let dupes_json = path_join(resolved_out, "dupes.json")
    // Stage 1: run detect-dupe to produce the cluster JSON.
    var stage1_argv <- [exe, detect_dupe_main, "--"]
    stage1_argv |> reserve(length(stage1_argv) + 2 * length(files) + 4)
    for (f in files) {
        stage1_argv |> push("-p")
        stage1_argv |> push(resolve_path(f))
    }
    stage1_argv |> push("--json")
    stage1_argv |> push(string(dupes_json))
    if (threshold >= 0.0) {
        stage1_argv |> push("-t")
        stage1_argv |> push("{threshold}")
    }
    var stage1_out : string
    let stage1_rc = run_and_capture(stage1_argv, stage1_out, 0.0)
    if (stage1_rc != 0) {
        return make_tool_result("find_dupe: detect-dupe failed (exit code {stage1_rc}):\n{stage1_out}", true)
    }
    let dupes_st = stat(dupes_json)
    if (!dupes_st.is_valid || !dupes_st.is_reg) {
        return make_tool_result("find_dupe: detect-dupe did not produce {dupes_json}\nstdout:\n{stage1_out}", true)
    }
    // Stage 2: hand off to the judge. Reuse the judge_duplicates implementation
    // so failure handling (anthropic-missing, API-key-missing) stays consistent.
    return run_find_dupe_judge(dupes_json, resolved_out, model, 8, max_clusters,
                               6, false, dry_run, false)
}
