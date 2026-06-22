options gen2

require daslib/json_boost
require daslib/fio


// Shadow structs mirroring the JSON shape produced by
// `utils/detect-dupe/main.das --json <path>`. We don't `require` the
// real `cluster.das` / `report.das` from detect-dupe because module
// resolution from `utils/find-dupe/` doesn't see them, and copying
// their definitions here keeps find-dupe free of any cross-tree
// dependency on detect-dupe' internals (only the on-disk JSON contract).
//
// Field names MUST match detect-dupe' Cluster / FuzzyPair / Summary /
// MemberRef byte-for-byte — sscan_json walks by name and silently
// drops mismatches.
struct public InputMemberRef {
    file : string
    name : string
    line : int
    end_line : int
    is_lambda : bool
}


struct public InputCluster {
    canonical_sample : string
    token_count : int
    members : array<InputMemberRef>
}


struct public InputFuzzyPair {
    similarity : float
    a : InputMemberRef
    b : InputMemberRef
}


struct public InputSummary {
    files_scanned : int
    functions_analyzed : int
    exact_clusters : int
    fuzzy_pairs : int
    threshold : float
    min_tokens : int
}


struct public InputReport {
    summary : InputSummary
    exact_clusters : array<InputCluster>
    fuzzy_pairs : array<InputFuzzyPair>
}


def public read_input_report(path : string; var report : InputReport; var error : string&) : bool {
    let text = fread(path)
    if (empty(text)) {
        error := "could not read detect-dupe JSON report (empty or missing): {path}"
        return false
    }
    if (!sscan_json(text, report)) {
        error := "could not parse detect-dupe JSON report: {path}"
        return false
    }
    // Sanity: end_line==0 across the board signals a v1 (pre-end_line)
    // detect-dupe report. We can still produce verdicts but source
    // extraction degrades to single-line peeks. Warn rather than fail.
    var any_end_line = false
    for (c in report.exact_clusters) {
        for (m in c.members) {
            if (m.end_line > 0) {
                any_end_line = true
                break
            }
        }
    }
    if (!any_end_line) {
        for (p in report.fuzzy_pairs) {
            if (p.a.end_line > 0 || p.b.end_line > 0) {
                any_end_line = true
                break
            }
        }
    }
    let total = length(report.exact_clusters) + length(report.fuzzy_pairs)
    if (!any_end_line && total > 0) {
        to_log(LOG_WARNING, "warning: input report has no `end_line` fields — looks like detect-dupe v1 output. Re-run detect-dupe with the bumped schema for full source extraction.\n")
    }
    return true
}
