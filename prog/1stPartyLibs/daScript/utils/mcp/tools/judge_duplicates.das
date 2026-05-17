options gen2
options rtti
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require common public
require daslib/fio
require daslib/strings_convert
require daslib/result

// Subprocess dispatch: the find-dupe judge requires the `anthropic/anthropic`
// daspkg, which isn't linked into the MCP server's daslang process. Calling
// the CLI as a child process keeps the dependency local to that process and
// surfaces a clean install hint when the package isn't there.

// Return shape: an envelope with exit_code, output paths, the embedded
// verdict JSON (or "" on dry-run / failure), and the tail of subprocess
// stdout for diagnostics. `@embed` lets the verdict JSON pass through
// without double-encoding.
struct JudgeDuplicatesEnvelope {
    exit_code : int
    out_dir : string
    json_path : string
    md_path : string
    @embed verdicts : string         //!< raw verdict JSON; "" on dry-run / failure
    stdout_tail : string             //!< last bit of stdout — handy for dry-run summary
    dry_run : bool
}


// Match the daspkg-install failure mode. The require error from daslang reads
// "module not found: anthropic/anthropic" — match a generous prefix so a
// future wording change still trips this branch.
def private looks_like_anthropic_missing(output : string) : bool {
    if (find(output, "anthropic/anthropic") >= 0 && find(output, "not found") >= 0) {
        return true
    }
    return find(output, "module not found: anthropic") >= 0
}


def private looks_like_api_key_missing(output : string) : bool {
    return find(output, "ANTHROPIC_API_KEY") >= 0
}


def private daspkg_install_hint() : string {
    return "find-dupe requires the `anthropic/anthropic` daspkg. From the project root run:\n  bin/daslang utils/daspkg/main.das -- install --root utils/find-dupe\nThen retry."
}


def private api_key_hint() : string {
    return "ANTHROPIC_API_KEY is not set in the daslang subprocess's environment. On macOS, add to ~/.zshenv (which is sourced by every zsh, including non-interactive subshells):\n  export ANTHROPIC_API_KEY=\"sk-ant-...\"\nThen restart the MCP server."
}


// Trim oversized stdout for the envelope. The full output already went into
// the structured error; this is just a last-N-chars tail for context.
def private tail_chars(s : string; n : int) : string {
    let len = length(s)
    if (len <= n) {
        return s
    }
    return slice(s, len - n)
}


def public run_find_dupe_judge(input_path : string;
                               out_dir : string;
                               model : string;
                               parallel : int;
                               max_clusters : int;
                               min_lines : int;
                               positives_only : bool;
                               dry_run : bool;
                               verbose : bool) : string {
    let exe = get_daslang_exe()
    if (empty(exe)) {
        return make_tool_result("Cannot determine daslang executable path", true)
    }
    if (!(empty(model) || model == "haiku" || model == "sonnet")) {
        return make_tool_result("judge_duplicates: 'model' must be 'haiku' or 'sonnet' (got '{model}')", true)
    }
    if (empty(input_path)) {
        return make_tool_result("judge_duplicates: missing required 'input' parameter (path to detect-dupe JSON report)", true)
    }
    let resolved_input = resolve_path(input_path)
    let input_st = stat(resolved_input)
    if (!input_st.is_valid || !input_st.is_reg) {
        return make_tool_result("judge_duplicates: input file not found: {resolved_input}", true)
    }
    let resolved_out = resolve_path(empty(out_dir) ? "{get_das_root()}/_mcp_find_dupe_{ref_time_ticks()}" : out_dir)
    let main_das = "{get_das_root()}/utils/find-dupe/main.das"
    let main_st = stat(main_das)
    if (!main_st.is_valid || !main_st.is_reg) {
        return make_tool_result("judge_duplicates: utils/find-dupe/main.das not found at {main_das}", true)
    }
    var argv <- [exe, main_das, "--",
                 "--input", resolved_input,
                 "--out", resolved_out]
    if (!empty(model)) {
        argv |> push("--model")
        argv |> push(string(model))
    }
    if (parallel > 0) {
        argv |> push("--parallel")
        argv |> push("{parallel}")
    }
    if (max_clusters > 0) {
        argv |> push("--max-clusters")
        argv |> push("{max_clusters}")
    }
    if (min_lines > 0) {
        argv |> push("--min-lines")
        argv |> push("{min_lines}")
    }
    if (positives_only) {
        argv |> push("--positives-only")
    }
    if (dry_run) {
        argv |> push("--dry-run")
    }
    if (verbose) {
        argv |> push("-v")
    }
    var output : string
    let exit_code = run_and_capture(argv, output, 0.0)
    if (exit_code != 0) {
        if (looks_like_anthropic_missing(output)) {
            return make_tool_result("{daspkg_install_hint()}\n\n--- subprocess output ---\n{output}", true)
        }
        if (looks_like_api_key_missing(output)) {
            return make_tool_result("{api_key_hint()}\n\n--- subprocess output ---\n{output}", true)
        }
        return make_tool_result("find-dupe failed (exit code {exit_code}):\n{output}", true)
    }
    var envelope : JudgeDuplicatesEnvelope
    envelope.exit_code = exit_code
    envelope.out_dir := resolved_out
    envelope.dry_run = dry_run
    envelope.stdout_tail := tail_chars(output, 4096)
    if (dry_run) {
        return make_tool_result(sprint_json(envelope, false))
    }
    let json_path = path_join(resolved_out, "find_dupe_verdicts.json")
    let md_path = path_join(resolved_out, "find_dupe_verdicts.md")
    envelope.json_path := json_path
    envelope.md_path := md_path
    let verdict_text = fread(json_path)
    if (empty(verdict_text)) {
        return make_tool_result("find-dupe completed but verdict JSON missing or empty: {json_path}\nSubprocess output:\n{output}", true)
    }
    envelope.verdicts := verdict_text
    return make_tool_result(sprint_json(envelope, false))
}


def public do_judge_duplicates(input_path : string;
                               out_dir : string;
                               model : string;
                               max_clusters_str : string;
                               positives_only_str : string;
                               dry_run_str : string) : string {
    var max_clusters = 0
    if (!empty(max_clusters_str)) {
        let parsed = try_to_int(max_clusters_str)
        if (is_err(parsed)) {
            return make_tool_result("judge_duplicates: 'max_clusters' must be a non-negative integer (got '{max_clusters_str}')", true)
        }
        max_clusters = unwrap(parsed)
        if (max_clusters < 0) {
            return make_tool_result("judge_duplicates: 'max_clusters' must be >= 0 (got {max_clusters})", true)
        }
    }
    let positives_only = positives_only_str == "true" || positives_only_str == "1"
    let dry_run = dry_run_str == "true" || dry_run_str == "1"
    return run_find_dupe_judge(input_path, out_dir, model, 8, max_clusters,
                               6, positives_only, dry_run, false)
}
