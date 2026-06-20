options gen2

require strings
require daslib/json_boost
require daslib/strings_boost
require daslib/fio
require cluster_input
require judge


// Output JSON shape — distinct from the detect-dupe input shape.
// `schema_version` lets future bumps reject older readers loudly,
// same convention as utils/detect-dupe/exchange.das.
let public FIND_DUPE_SCHEMA_VERSION = 1


enum public VerdictKind {
    real            //!< single group covers every member; cluster is a real duplicate
    partial         //!< cluster splits — multiple groups OR groups + false_positives
    false_positive  //!< no group has size >= 2; cluster is noise
    skipped         //!< worth-it gate skipped this cluster (no API call)
    error           //!< API or parse failure; verdict is unknown
}


struct public VerdictRow {
    cluster_id : int
    kind : string                          //!< "exact" | "fuzzy"
    similarity : float                     //!< 0.0 for exact; >0 for fuzzy
    members : array<InputMemberRef>
    verdict : string                       //!< VerdictKind name
    groups : array<array<int>>
    false_positives : array<int>
    reason : string
    input_tokens : int
    output_tokens : int
    error : string
    skipped : bool
    skip_reason : string
}


struct public OutputSummary {
    total : int
    judged : int                           //!< actually sent to the model
    skipped : int                          //!< worth-it gate
    real : int
    partial : int
    false_positive : int
    errored : int
    input_tokens : int
    output_tokens : int
}


struct public OutputReport {
    schema_version : int = FIND_DUPE_SCHEMA_VERSION
    model : string
    summary : OutputSummary
    verdicts : array<VerdictRow>
}


// Decide the high-level verdict label from groups / false_positives.
// Empty cluster (size 0) shouldn't happen but the rule still works.
def public classify_verdict(n_members : int; v : Verdict) : VerdictKind {
    if (!empty(v.error)) return VerdictKind.error
    var any_real_group = false
    for (g in v.groups) {
        if (length(g) >= 2) {
            any_real_group = true
            break
        }
    }
    let all_false = !any_real_group && length(v.false_positives) == n_members
    if (all_false) return VerdictKind.false_positive
    let single_full_group = (length(v.groups) == 1 && length(v.groups[0]) == n_members && empty(v.false_positives))
    if (single_full_group) return VerdictKind.real
    return VerdictKind.partial
}


def public verdict_kind_to_string(k : VerdictKind) : string {
    if (k == VerdictKind.real) return "real"
    if (k == VerdictKind.partial) return "partial"
    if (k == VerdictKind.false_positive) return "false_positive"
    if (k == VerdictKind.skipped) return "skipped"
    return "error"
}


def public write_json_report(report : OutputReport; path : string) : bool {
    let body = sprint_json(report, true)
    var ok = false
    fopen(path, "wb") $(fw) {
        if (fw == null) return
        ok = true
        fw |> fprint(body)
    }
    return ok
}


def private verdict_emoji(k : VerdictKind) : string {
    if (k == VerdictKind.real) return "[REAL]"
    if (k == VerdictKind.partial) return "[PARTIAL]"
    if (k == VerdictKind.false_positive) return "[FALSE-POS]"
    if (k == VerdictKind.skipped) return "[SKIP]"
    return "[ERR]"
}


def private member_label(m : InputMemberRef) : string {
    let kind = m.is_lambda ? "lambda" : "function"
    // Same `end_line <= 0` guard as `format_cluster_prompt` — v1 inputs
    // would otherwise render as `file:24-0` in the markdown report.
    let location = m.end_line > 0 ? "{m.file}:{m.line}-{m.end_line}" : "{m.file}:{m.line}"
    return "{location}  {m.name}  ({kind})"
}


def public write_markdown_report(report : OutputReport;
                                 var sources_by_id : table<int; array<string>>;
                                 path : string) : bool {
    var ok = false
    fopen(path, "wb") $(fw) {
        if (fw == null) return
        ok = true
        fw |> fprint("# find-dupe verdicts\n\n")
        fw |> fprint("**Model:** `{report.model}`  \n")
        let s & = unsafe(report.summary)
        fw |> fprint("**Summary:** {s.judged} judged / {s.total} total  \n")
        if (length(report.verdicts) < s.total) {
            fw |> fprint("_Filtered:_ showing {length(report.verdicts)} of {s.total} rows (positives only).  \n")
        }
        fw |> fprint("- real: {s.real}\n")
        fw |> fprint("- partial: {s.partial}\n")
        fw |> fprint("- false_positive: {s.false_positive}\n")
        fw |> fprint("- skipped: {s.skipped}\n")
        fw |> fprint("- errored: {s.errored}\n")
        fw |> fprint("- tokens: in={s.input_tokens}  out={s.output_tokens}\n\n")
        fw |> fprint("---\n\n")
        for (row in report.verdicts) {
            let kind = parse_verdict_string(row.verdict)
            let tag = verdict_emoji(kind)
            fw |> fprint("## {tag} Cluster #{row.cluster_id} — {row.kind}")
            if (row.kind == "fuzzy") {
                fw |> fprint("  (similarity {row.similarity})")
            }
            fw |> fprint("\n\n")
            if (!empty(row.reason)) {
                fw |> fprint("**Reason:** {row.reason}\n\n")
            }
            if (row.skipped) {
                fw |> fprint("_Skipped:_ {row.skip_reason}\n\n")
            }
            if (!empty(row.error)) {
                fw |> fprint("_Error:_ `{row.error}`\n\n")
            }
            fw |> fprint("**Members ({length(row.members)}):**\n\n")
            for (i in iter_range(row.members)) {
                fw |> fprint("- `[{i}]` {member_label(row.members[i])}\n")
            }
            fw |> fprint("\n")
            // Partition annotation
            if (!empty(row.groups) || !empty(row.false_positives)) {
                fw |> fprint("**Partition:**\n\n")
                for (g in row.groups) {
                    fw |> fprint("- group: ")
                    for (i in iter_range(g)) {
                        if (i > 0) {
                            fw |> fprint(", ")
                        }
                        fw |> fprint("`[{g[i]}]`")
                    }
                    fw |> fprint("\n")
                }
                if (!empty(row.false_positives)) {
                    fw |> fprint("- false_positives: ")
                    for (i in iter_range(row.false_positives)) {
                        if (i > 0) {
                            fw |> fprint(", ")
                        }
                        fw |> fprint("`[{row.false_positives[i]}]`")
                    }
                    fw |> fprint("\n")
                }
                fw |> fprint("\n")
            }
            // Source code (collapsed). `key_exists` first so we don't
            // insert an empty default into the cache.
            if (key_exists(sources_by_id, row.cluster_id)) {
                fw |> fprint("<details><summary>source</summary>\n\n")
                let sources & = unsafe(sources_by_id[row.cluster_id])
                for (si in iter_range(sources)) {
                    fw |> fprint("`[{si}]` `{row.members[si].name}`\n")
                    fw |> fprint("```daslang\n{sources[si]}\n```\n\n")
                }
                fw |> fprint("</details>\n\n")
            }
            fw |> fprint("---\n\n")
        }
    }
    return ok
}


def private parse_verdict_string(s : string) : VerdictKind {
    if (s == "real") return VerdictKind.real
    if (s == "partial") return VerdictKind.partial
    if (s == "false_positive") return VerdictKind.false_positive
    if (s == "skipped") return VerdictKind.skipped
    return VerdictKind.error
}
