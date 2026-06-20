options gen2
options rtti

require strings
require math
require daslib/strings_boost
require daslib/json_boost
require daslib/fio
require cluster


struct public Summary {
    files_scanned : int
    functions_analyzed : int
    exact_clusters : int
    fuzzy_pairs : int
    threshold : float
    min_tokens : int
}


struct public Report {
    summary : Summary
    exact_clusters : array<Cluster>
    fuzzy_pairs : array<FuzzyPair>
}


// JSON serialization is delegated to `sprint_json` from `daslib/json_boost`,
// which walks struct fields via RTTI and escapes correctly (Unicode-safe,
// emits `\u00XX` for control bytes < 0x20). Annotations on Cluster /
// FuzzyPair / Summary fields would let us rename or omit fields if needed.
def public write_json_report(report : Report; path : string) : bool {
    let body = sprint_json(report, true)
    var ok = false
    fopen(path, "wb") $(fw) {
        if (fw == null) return
        ok = true
        fw |> fprint(body)
    }
    return ok
}


def member_label(m : MemberRef) : string {
    // Synthetic lambda names already encode line:col, which makes
    // multiple `@(...)` blocks on the same line distinguishable.
    let base = m.is_lambda ? "<lambda> ({m.name})" : m.name
    return "{m.file}:{m.line}  {base}"
}


// Per-candidate JSON shape: "for each function in the focus set, here
// are the existing functions it resembles." Used by --against mode and
// the detect_duplicates MCP tool — both want the result keyed on the
// user's mental model ("what should I look at for *this* function I
// just wrote") rather than the flat clusters/pairs view.

struct public CandidateMatch {
    name : string
    file : string
    line : int
    is_lambda : bool
    // 1.0 for exact-cluster siblings, < 1.0 (the geometric-mean score)
    // for fuzzy matches. Using one struct for both keeps the JSON shape
    // uniform.
    similarity : float = 0.0f
}


struct public CandidateRollup {
    name : string
    file : string
    line : int
    is_lambda : bool
    exact_matches : array<CandidateMatch>
    fuzzy_matches : array<CandidateMatch>
}


struct public PerCandidateReport {
    candidates : array<CandidateRollup>
}


def make_match(m : MemberRef; similarity : float) : CandidateMatch {
    var cm = CandidateMatch(
        name = m.name,
        file = m.file,
        line = m.line,
        is_lambda = m.is_lambda,
        similarity = similarity)
    return <- cm
}


// Build the per-candidate pivot. `candidate_refs` is the caller's list
// of focus-set members (typically `[r.ref for r in records if
// r.is_candidate]`). The rollup includes one entry per candidate even
// when it has no matches — useful for the MCP consumer that wants to
// confirm "I checked these N functions and found nothing."
def public build_per_candidate_report(report : Report;
                                      candidate_refs : array<MemberRef>;
                                      top_per_candidate : int) : PerCandidateReport {
    var cand_keys : table<string; int>
    for (i in range(length(candidate_refs))) {
        let k = member_key(candidate_refs[i])
        if (!key_exists(cand_keys, k)) {
            cand_keys |> insert(k, i)
        }
    }
    var rollups : array<CandidateRollup>
    rollups |> reserve(length(candidate_refs))
    for (cref in candidate_refs) {
        var r = CandidateRollup(
            name = cref.name,
            file = cref.file,
            line = cref.line,
            is_lambda = cref.is_lambda)
        rollups |> emplace(r)
    }
    // Exact siblings: walk every cluster the candidate belongs to,
    // collecting the OTHER members. A candidate could in theory belong
    // to multiple clusters across a multi-file rebuild — be permissive.
    for (c in report.exact_clusters) {
        for (m in c.members) {
            let k = member_key(m)
            if (!key_exists(cand_keys, k)) continue
            let idx = cand_keys[k]
            for (other in c.members) {
                if (member_key(other) == k) continue
                rollups[idx].exact_matches |> emplace(make_match(other, 1.0f))
            }
        }
    }
    // Fuzzy partners: pairs where either endpoint matches a candidate.
    // When BOTH endpoints are candidates, each side gets the other in
    // its rollup — the pair is symmetric, the rollup is per-candidate.
    for (p in report.fuzzy_pairs) {
        let ka = member_key(p.a)
        let kb = member_key(p.b)
        let a_is_cand = key_exists(cand_keys, ka)
        let b_is_cand = key_exists(cand_keys, kb)
        if (a_is_cand) {
            rollups[cand_keys[ka]].fuzzy_matches |> emplace(make_match(p.b, p.similarity))
        }
        if (b_is_cand) {
            rollups[cand_keys[kb]].fuzzy_matches |> emplace(make_match(p.a, p.similarity))
        }
    }
    // Trim to top_per_candidate (fuzzy_matches were inserted in
    // sort_pairs order so they're already similarity-DESC; exact_matches
    // are append-order, fine for stable diffs since clusters are sorted).
    if (top_per_candidate >= 0) {
        for (r in rollups) {
            if (length(r.exact_matches) > top_per_candidate) {
                unsafe(resize(r.exact_matches, top_per_candidate))
            }
            if (length(r.fuzzy_matches) > top_per_candidate) {
                unsafe(resize(r.fuzzy_matches, top_per_candidate))
            }
        }
    }
    var out = PerCandidateReport()
    out.candidates <- rollups
    return <- out
}


def public write_per_candidate_report(report : Report;
                                      candidate_refs : array<MemberRef>;
                                      top_per_candidate : int) : string {
    let pcr = build_per_candidate_report(report, candidate_refs, top_per_candidate)
    return sprint_json(pcr, true)
}


def public print_per_candidate_summary(report : Report;
                                       candidate_refs : array<MemberRef>;
                                       top_per_candidate : int) {
    let pcr = build_per_candidate_report(report, candidate_refs, top_per_candidate)
    to_log(LOG_INFO, "\n=== detect-dupe per-candidate report ===\n")
    to_log(LOG_INFO, "candidates scanned: {length(pcr.candidates)}\n")
    var n_with_hits = 0
    for (r in pcr.candidates) {
        if (!empty(r.exact_matches) || !empty(r.fuzzy_matches)) {
            n_with_hits++
        }
    }
    to_log(LOG_INFO, "candidates with matches: {n_with_hits}\n")
    for (r in pcr.candidates) {
        if (empty(r.exact_matches) && empty(r.fuzzy_matches)) continue
        let header = r.is_lambda ? "<lambda> ({r.name})" : r.name
        to_log(LOG_INFO, "\n* {r.file}:{r.line}  {header}\n")
        if (!empty(r.exact_matches)) {
            to_log(LOG_INFO, "  exact:\n")
            for (m in r.exact_matches) {
                let mhdr = m.is_lambda ? "<lambda> ({m.name})" : m.name
                to_log(LOG_INFO, "    - {m.file}:{m.line}  {mhdr}\n")
            }
        }
        if (!empty(r.fuzzy_matches)) {
            to_log(LOG_INFO, "  fuzzy:\n")
            for (m in r.fuzzy_matches) {
                let mhdr = m.is_lambda ? "<lambda> ({m.name})" : m.name
                to_log(LOG_INFO, "    - {m.similarity}  {m.file}:{m.line}  {mhdr}\n")
            }
        }
    }
}


def public print_summary(report : Report; top : int) {
    to_log(LOG_INFO, "\n=== detect-dupe summary ===\n")
    to_log(LOG_INFO, "files scanned     : {report.summary.files_scanned}\n")
    to_log(LOG_INFO, "functions analyzed: {report.summary.functions_analyzed}\n")
    to_log(LOG_INFO, "exact clusters    : {length(report.exact_clusters)}\n")
    to_log(LOG_INFO, "fuzzy pairs       : {length(report.fuzzy_pairs)}  (threshold {report.summary.threshold})\n")
    to_log(LOG_INFO, "min tokens        : {report.summary.min_tokens}\n")
    let n_ex = min(top, length(report.exact_clusters))
    if (n_ex > 0) {
        to_log(LOG_INFO, "\n--- top {n_ex} exact-clone clusters ---\n")
        for (i in range(n_ex)) {
            let size = length(report.exact_clusters[i].members)
            let tcount = report.exact_clusters[i].token_count
            to_log(LOG_INFO, "\n[{i + 1}] size={size} tokens={tcount}\n")
            to_log(LOG_INFO, "    {report.exact_clusters[i].canonical_sample}\n")
            for (m in report.exact_clusters[i].members) {
                to_log(LOG_INFO, "    - {member_label(m)}\n")
            }
        }
    }
    let n_fz = min(top, length(report.fuzzy_pairs))
    if (n_fz > 0) {
        to_log(LOG_INFO, "\n--- top {n_fz} fuzzy pairs ---\n")
        for (i in range(n_fz)) {
            to_log(LOG_INFO, "\n[{i + 1}] similarity={report.fuzzy_pairs[i].similarity}\n")
            to_log(LOG_INFO, "    a: {member_label(report.fuzzy_pairs[i].a)}\n")
            to_log(LOG_INFO, "    b: {member_label(report.fuzzy_pairs[i].b)}\n")
            to_log(LOG_INFO, "    a canonical: {report.fuzzy_pairs[i].canonical_a}\n")
            to_log(LOG_INFO, "    b canonical: {report.fuzzy_pairs[i].canonical_b}\n")
        }
    }
}
