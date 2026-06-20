options gen2

require strings
require daslib/clargs
require daslib/fio
require daslib/algorithm
require daslib/jobque_boost
// `anthropic/anthropic` is fetched at runtime by daspkg, not built in.
// If this require fails with "module not found", run from project root:
//   bin/daslang utils/daspkg/main.das -- install --root utils/find-dupe
require anthropic/anthropic
require cluster_input
require source_extract
require judge
require judge_live
require report


let DEFAULT_HAIKU = "claude-haiku-4-5-20251001"
let DEFAULT_SONNET = "claude-sonnet-4-6"


[CommandLineArgs]
struct Config {
    @clarg_short = "i"
    @clarg_doc = "detect-dupe JSON report file (input)"
    input : string

    @clarg_short = "o"
    @clarg_doc = "output directory (default ./find-dupe-out)"
    out : string = "./find-dupe-out"

    @clarg_doc = "basename for verdict files (default find_dupe_verdicts → <basename>.json + <basename>.md)"
    out_basename : string = "find_dupe_verdicts"

    @clarg_doc = "model: haiku | sonnet (default haiku)"
    model : string = "haiku"

    @clarg_doc = "skip clusters where every member is shorter than this many lines (default 6)"
    min_lines : int = 6

    @clarg_doc = "hard cap on clusters analyzed after sort (0 = no cap)"
    max_clusters : int = 0

    @clarg_doc = "sort: default | size | fuzzy_first (default = composite)"
    sort_by : string = "default"

    @clarg_doc = "concurrent judge calls (default 8). 1 = sequential."
    parallel : int = 8

    @clarg_doc = "filter reports to actionable rows only: real + partial. Summary still reflects all clusters."
    positives_only : bool

    @clarg_doc = "estimate tokens & cost without making API calls"
    dry_run : bool

    @clarg_doc = "send a single test request to verify ANTHROPIC_API_KEY + das-claude wiring"
    smoke_test : bool

    @clarg_short = "v"
    @clarg_doc = "verbose progress to stdout"
    verbose : bool

    @clarg_short = "?"
    @clarg_name = "show-help"
    @clarg_doc = "Show this help and exit"
    help : bool
}


// Internal job — uniform shape for both exact clusters and fuzzy
// pairs. Skip metadata is kept inline so the report writer can render
// "this cluster was skipped" rows alongside actual verdicts.
struct JudgeJob {
    cluster_id : int
    kind : string                       //!< "exact" | "fuzzy"
    similarity : float = 0.0f           //!< 0.0 for exact; >0 for fuzzy pairs
    members : array<InputMemberRef>
    skipped : bool = false
    skip_reason : string
}


def smoke_test() {
    to_log(LOG_INFO, "Running das-claude smoke test (model={DEFAULT_HAIKU})...\n")
    var config <- claude_config_from_env(DEFAULT_HAIKU)
    var params : MessageCreateParams
    params.system := "Reply with the single word OK."
    params.messages |> emplace(user_message("ping"))
    var error : string
    var msg = create_message(config, params, error)
    if (!empty(error)) {
        to_log(LOG_ERROR, "Smoke test FAILED: {error}\n")
        panic("smoke test failed")
    }
    let reply = get_text(msg)
    to_log(LOG_INFO, "Reply: {reply}\n")
    to_log(LOG_INFO, "Tokens: in={msg.usage.input_tokens} out={msg.usage.output_tokens}\n")
    if (msg.usage.input_tokens == 0) {
        panic("Smoke test FAILED: zero input tokens reported (suspicious)")
    }
    to_log(LOG_INFO, "Smoke test PASSED.\n")
}


def resolve_model(name : string) : string {
    if (name == "sonnet") {
        return DEFAULT_SONNET
    }
    return DEFAULT_HAIKU
}


def flatten_jobs(report : InputReport) : array<JudgeJob> {
    var out : array<JudgeJob>
    var idx = 0
    for (c in report.exact_clusters) {
        var j = JudgeJob()
        j.cluster_id = idx
        j.kind := "exact"
        j.members := c.members
        out |> emplace(j)
        idx++
    }
    for (p in report.fuzzy_pairs) {
        var j = JudgeJob()
        j.cluster_id = idx
        j.kind := "fuzzy"
        j.similarity = p.similarity
        j.members |> push(p.a)
        j.members |> push(p.b)
        out |> emplace(j)
        idx++
    }
    return <- out
}


// Worth-it gate: skip if every member spans fewer than `min_lines`
// lines. We only have line counts (not token counts) per member, so
// line-count is the gate. Cluster-of-one degenerate jobs are also
// skipped.
def apply_worth_it_gate(var jobs : array<JudgeJob>; min_lines : int) {
    for (j in jobs) {
        if (length(j.members) < 2) {
            j.skipped = true
            j.skip_reason := "degenerate cluster (< 2 members)"
            continue
        }
        var max_span = 0
        for (m in j.members) {
            let span = (m.end_line > 0 ? m.end_line - m.line + 1 : 1)
            if (span > max_span) {
                max_span = span
            }
        }
        if (max_span < min_lines) {
            j.skipped = true
            j.skip_reason := "max member span {max_span} lines < min_lines {min_lines}"
        }
    }
}


// Sort priority — descending. Fuzzy pairs first (those are where the
// AI earns its keep), then bigger clusters, then bigger total span.
def sort_priority(j : JudgeJob; key : string) : int {
    if (key == "size") {
        return length(j.members)
    }
    if (key == "fuzzy_first") {
        return j.kind == "fuzzy" ? 1 : 0
    }
    // default = composite: fuzzy(1000) + members*10 + total_span/10
    var total_span = 0
    for (m in j.members) {
        let span = (m.end_line > 0 ? m.end_line - m.line + 1 : 1)
        total_span += span
    }
    let fuzzy_bonus = j.kind == "fuzzy" ? 1000 : 0
    return fuzzy_bonus + length(j.members) * 10 + total_span / 10
}


def sort_jobs(var jobs : array<JudgeJob>; key : string) {
    sort(jobs) <| $(a, b) => sort_priority(a, key) > sort_priority(b, key)
}


// Rough chars/4 estimate — daslang doesn't expose Anthropic's
// tokenizer. Order-of-magnitude is fine for a "should I run this?"
// gate.
def estimate_tokens(prompt_chars : int) : int {
    return prompt_chars / 4
}


def cost_estimate(model : string; in_tokens, out_tokens : int) : double {
    // Haiku 4.5: $1 / MTok in, $5 / MTok out
    // Sonnet 4.6: $3 / MTok in, $15 / MTok out
    if (model == "sonnet") {
        return double(in_tokens) * 3.0lf / 1000000.0lf + double(out_tokens) * 15.0lf / 1000000.0lf
    }
    return double(in_tokens) * 1.0lf / 1000000.0lf + double(out_tokens) * 5.0lf / 1000000.0lf
}


// Hold per-cluster sources for the report writer. Keyed by
// `cluster_id`. Built once per run; used by both the judge call and
// the Markdown report.
//
// If any member's source can't be extracted (file missing,
// out-of-range line, etc.), the cluster is marked `skipped = true`
// with an explanatory `skip_reason` instead of getting passed to the
// model with empty source — that would waste an API call and produce
// a verdict over missing data. Skipped jobs still appear in the
// report (under the "skipped" header) so failures are visible.
def collect_sources(var cache : FileCache; var jobs : array<JudgeJob>;
                    var sources_by_id : table<int; array<string>>) {
    for (j in jobs) {
        if (j.skipped) {
            continue
        }
        var arr : array<string>
        var failure : string
        for (m in j.members) {
            var sub_err : string
            let src = extract_source(cache, m.file, m.line, m.end_line, sub_err)
            if (!empty(sub_err)) {
                failure := "{m.file}:{m.line}: {sub_err}"
                break
            }
            if (empty(src)) {
                failure := "{m.file}:{m.line}: empty source extracted"
                break
            }
            arr |> push(src)
        }
        if (!empty(failure)) {
            j.skipped = true
            j.skip_reason := "source extraction failed: {failure}"
            continue
        }
        sources_by_id[j.cluster_id] <- arr
    }
}


def make_verdict_row(j : JudgeJob; v : Verdict) : VerdictRow {
    var row = VerdictRow()
    row.cluster_id = j.cluster_id
    row.kind := j.kind
    row.similarity = j.similarity
    row.members := j.members
    row.skipped = j.skipped
    row.skip_reason := j.skip_reason
    row.input_tokens = v.input_tokens
    row.output_tokens = v.output_tokens
    row.error := v.error
    if (j.skipped) {
        row.verdict := verdict_kind_to_string(VerdictKind.skipped)
        return <- row
    }
    if (!empty(v.error)) {
        row.verdict := verdict_kind_to_string(VerdictKind.error)
        return <- row
    }
    row.groups := v.groups
    row.false_positives := v.false_positives
    row.reason := v.reason
    let kind = classify_verdict(length(j.members), v)
    row.verdict := verdict_kind_to_string(kind)
    return <- row
}


def update_summary(var s : OutputSummary; row : VerdictRow) {
    s.total++
    if (row.skipped) {
        s.skipped++
        return
    }
    s.judged++
    s.input_tokens += row.input_tokens
    s.output_tokens += row.output_tokens
    if (row.verdict == "real") {
        s.real++
    } elif (row.verdict == "partial") {
        s.partial++
    } elif (row.verdict == "false_positive") {
        s.false_positive++
    } elif (row.verdict == "error") {
        s.errored++
    }
}


def run_pipeline(cfg : Config) : int {
    if (empty(cfg.input)) {
        to_log(LOG_ERROR, "error: --input is required (path to detect-dupe JSON report)\n\n")
        print_help(get_command_info(type<Config>), "find-dupe")
        return 2
    }
    if (cfg.model != "haiku" && cfg.model != "sonnet") {
        to_log(LOG_ERROR, "error: --model must be 'haiku' or 'sonnet', got '{cfg.model}'\n")
        return 2
    }
    // Validate --sort-by so typos surface as errors instead of silently
    // falling through to the composite default in `sort_priority`.
    if (cfg.sort_by != "default" && cfg.sort_by != "size" && cfg.sort_by != "fuzzy_first") {
        to_log(LOG_ERROR, "error: --sort-by must be 'default', 'size', or 'fuzzy_first', got '{cfg.sort_by}'\n")
        return 2
    }
    if (cfg.parallel < 1) {
        to_log(LOG_ERROR, "error: --parallel must be >= 1, got {cfg.parallel}\n")
        return 2
    }
    var input_report : InputReport
    var read_err : string
    if (!read_input_report(cfg.input, input_report, read_err)) {
        to_log(LOG_ERROR, "error: {read_err}\n")
        return 2
    }
    var jobs <- flatten_jobs(input_report)
    if (cfg.verbose) {
        to_log(LOG_INFO, "Loaded {length(input_report.exact_clusters)} exact cluster(s) + {length(input_report.fuzzy_pairs)} fuzzy pair(s) = {length(jobs)} job(s)\n")
    }
    apply_worth_it_gate(jobs, cfg.min_lines)
    sort_jobs(jobs, cfg.sort_by)
    // Cap is applied AFTER sort so the top-priority jobs win when
    // capping. Skipped jobs still appear in the report (not consumed
    // against the cap).
    if (cfg.max_clusters > 0) {
        var kept : array<JudgeJob>
        var live_kept = 0
        for (j in jobs) {
            if (j.skipped) {
                kept |> emplace(j)
                continue
            }
            if (live_kept >= cfg.max_clusters) {
                continue
            }
            kept |> emplace(j)
            live_kept++
        }
        jobs <- kept
        if (cfg.verbose) {
            to_log(LOG_INFO, "--max-clusters {cfg.max_clusters} → kept {live_kept} live job(s)\n")
        }
    }
    var cache = FileCache()
    var sources_by_id : table<int; array<string>>
    collect_sources(cache, jobs, sources_by_id)
    let model_id = resolve_model(cfg.model)
    if (cfg.dry_run) {
        var est_in = 0
        var live_jobs = 0
        for (j in jobs) {
            if (j.skipped) {
                continue
            }
            live_jobs++
            let srcs & = unsafe(sources_by_id[j.cluster_id])
            let prompt = format_cluster_prompt(j.members, srcs)
            est_in += estimate_tokens(length(prompt))
        }
        let est_out = live_jobs * 360        // ~360 output tokens per verdict (groups+reason)
        let cost = cost_estimate(cfg.model, est_in, est_out)
        to_log(LOG_INFO, "DRY-RUN ({cfg.model} → {model_id}):\n")
        to_log(LOG_INFO, "  total jobs:    {length(jobs)}\n")
        to_log(LOG_INFO, "  live (judged): {live_jobs}\n")
        to_log(LOG_INFO, "  skipped:       {length(jobs) - live_jobs}\n")
        to_log(LOG_INFO, "  est. input:    {est_in} tokens\n")
        to_log(LOG_INFO, "  est. output:   {est_out} tokens\n")
        to_log(LOG_INFO, "  est. cost:     ${cost}\n")
        return 0
    }
    var output = OutputReport()
    output.model := model_id
    // Split into skipped (no API call) and live (judge call needed).
    // Skipped rows are built synchronously up front; live rows are
    // judged in parallel and re-merged in original `jobs` order so the
    // priority sort survives.
    var live_results : table<int; VerdictRow>
    var live_indices : array<int>
    for (idx in iter_range(jobs)) {
        if (jobs[idx].skipped) {
            continue
        }
        live_indices |> push(idx)
    }
    let n_live = length(live_indices)
    if (n_live > 0) {
        let workers = cfg.parallel < n_live ? cfg.parallel : n_live
        if (cfg.verbose) {
            to_log(LOG_INFO, "Judging {n_live} live cluster(s) with {workers} worker(s)...\n")
        }
        if (workers <= 1) {
            // Sequential — avoid jobque overhead, keep ordered progress
            // numbering for the single-threaded case.
            var live_count = 0
            for (idx in live_indices) {
                let j & = unsafe(jobs[idx])
                live_count++
                if (cfg.verbose) {
                    to_log(LOG_INFO, "[{live_count}/{n_live}] cluster #{j.cluster_id} ({j.kind}, {length(j.members)} members) → {model_id}...\n")
                }
                let srcs & = unsafe(sources_by_id[j.cluster_id])
                var verdict <- live_judge(j.members, srcs, model_id)
                var row <- make_verdict_row(j, verdict)
                if (cfg.verbose) {
                    to_log(LOG_INFO, "    verdict={row.verdict} reason=\"{row.reason}\" tokens={row.input_tokens}+{row.output_tokens}\n")
                }
                live_results[j.cluster_id] <- row
            }
        } else {
            // Hand-rolled fan-out: parallel_map's macro auto-wraps the
            // body in `@{}` with default capture, which can't carry our
            // non-copyable `jobs` / `sources_by_id`. With direct
            // `new_job(@ capture(ref(...)) {...})` we get the exact
            // capture modes we need. Lifetime is safe — `with_wait_group`
            // blocks the outer scope until every worker has notified, so
            // the referenced arrays/tables outlive every job.
            let verbose = cfg.verbose
            with_job_que() {
                with_channel(n_live) $(ch) {
                    with_wait_group(n_live) $(wg) {
                        for (idx in live_indices) {
                            let captured_idx = idx
                            unsafe {
                                new_job <| @ capture(ref(jobs), ref(sources_by_id)) () {
                                    let j & = unsafe(jobs[captured_idx])
                                    let srcs & = unsafe(sources_by_id[j.cluster_id])
                                    var verdict <- live_judge(j.members, srcs, model_id)
                                    var row <- make_verdict_row(j, verdict)
                                    if (verbose) {
                                        to_log(LOG_INFO, "[done] cluster #{j.cluster_id} verdict={row.verdict} reason=\"{row.reason}\" tokens={row.input_tokens}+{row.output_tokens}\n")
                                    }
                                    ch |> push_clone(row)
                                    ch |> notify_and_release
                                    wg |> notify_and_release
                                }
                            }
                        }
                    }
                    ch |> for_each_clone() $(row : VerdictRow#) {
                        live_results[row.cluster_id] <- clone_to_move(row)
                    }
                }
            }
            if (cfg.verbose) {
                to_log(LOG_INFO, "Parallel judge complete: {length(live_results)}/{n_live}\n")
            }
        }
    }
    // Reassemble in original priority order.
    for (j in jobs) {
        if (j.skipped) {
            var row <- make_verdict_row(j, Verdict())
            if (cfg.verbose) {
                to_log(LOG_INFO, "[skip] cluster #{j.cluster_id} ({j.kind}, {length(j.members)} members): {j.skip_reason}\n")
            }
            update_summary(output.summary, row)
            output.verdicts |> emplace(row)
            continue
        }
        if (key_exists(live_results, j.cluster_id)) {
            var row <- live_results[j.cluster_id]
            update_summary(output.summary, row)
            output.verdicts |> emplace(row)
        }
    }
    mkdir(cfg.out)
    let json_path = path_join(cfg.out, "{cfg.out_basename}.json")
    let md_path = path_join(cfg.out, "{cfg.out_basename}.md")
    // Build the report payload to write — filtered down to actionable
    // rows when --positives-only is set. Always a fresh struct so we
    // don't disturb `output.summary` (printed below).
    var report_to_write = OutputReport()
    report_to_write.schema_version = output.schema_version
    report_to_write.model := output.model
    report_to_write.summary = output.summary
    for (row in output.verdicts) {
        if (cfg.positives_only && row.verdict != "real" && row.verdict != "partial") {
            continue
        }
        report_to_write.verdicts |> push_clone(row)
    }
    if (cfg.positives_only && cfg.verbose) {
        to_log(LOG_INFO, "--positives-only: keeping {length(report_to_write.verdicts)} of {length(output.verdicts)} rows\n")
    }
    if (!write_json_report(report_to_write, json_path)) {
        to_log(LOG_ERROR, "error: could not write JSON report to {json_path}\n")
        return 3
    }
    if (!write_markdown_report(report_to_write, sources_by_id, md_path)) {
        to_log(LOG_ERROR, "error: could not write Markdown report to {md_path}\n")
        return 3
    }
    let s & = unsafe(output.summary)
    to_log(LOG_INFO, "find-dupe done.\n")
    to_log(LOG_INFO, "  total: {s.total}  judged: {s.judged}  skipped: {s.skipped}\n")
    to_log(LOG_INFO, "  real: {s.real}  partial: {s.partial}  false_positive: {s.false_positive}  errored: {s.errored}\n")
    to_log(LOG_INFO, "  tokens: in={s.input_tokens}  out={s.output_tokens}  cost={cost_estimate(cfg.model, s.input_tokens, s.output_tokens)}\n")
    to_log(LOG_INFO, "  JSON:  {json_path}\n")
    to_log(LOG_INFO, "  MD:    {md_path}\n")
    return 0
}


[export]
def main() : int {
    var r <- parse_args(type<Config>)
    if (r |> is_err) {
        to_log(LOG_ERROR, "error: {r |> unwrap_err}\n\n")
        print_help(get_command_info(type<Config>), "find-dupe")
        return 1
    }
    var cfg <- r |> move_unwrap
    if (cfg.help) {
        print_help(get_command_info(type<Config>), "find-dupe")
        return 0
    }
    if (cfg.smoke_test) {
        smoke_test()
        return 0
    }
    return run_pipeline(cfg)
}
