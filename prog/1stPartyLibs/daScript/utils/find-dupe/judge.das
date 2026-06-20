options gen2

require strings
require daslib/json_boost
require cluster_input


// Verdict shape — what the AI emits and what the report writer
// consumes. `groups` partitions the cluster into sub-sets that ARE
// each a real duplicate. `false_positives` lists indices that don't
// belong with anyone else.
//
// Invariant: every member index in [0, N) appears exactly once across
// `groups + false_positives`. `parse_verdict` enforces this and sets
// `error` if violated — the caller decides whether to retry or fall
// through to a "indeterminate" report row.
struct public Verdict {
    groups : array<array<int>>
    false_positives : array<int>
    reason : string
    input_tokens : int
    output_tokens : int
    error : string
}


// JudgeFn is a function pointer so tests can inject a deterministic
// fake judge with no API calls. The live and fake implementations
// MUST match this signature.
typedef public JudgeFn = function<(members : array<InputMemberRef>; sources : array<string>; model : string) : Verdict>


let public SYSTEM_PROMPT = "You are a code-review assistant judging whether a cluster of daslang functions are real duplicates.\n\nReal duplicate = same intent, same effect — only naming, formatting, or trivial cosmetic differences.\nPartial = a subset of the cluster matches; the rest are false positives.\nFalse positive = pattern-shaped match but semantically different (e.g. different operations, different invariants, different domain).\n\nYou MUST call the `report_verdict` tool exactly once. Be terse: the `reason` field is one short sentence.\n\nThe `groups` field partitions the cluster into sub-sets that ARE each real duplicates of one another. A function that doesn't match any other goes in `false_positives`.\nEvery member index in [0, N) must appear exactly once across `groups` + `false_positives`. Singleton groups (size 1) are allowed but should usually be expressed as a `false_positives` entry instead.\n"


// JSON-Schema for the `report_verdict` tool. Daslang gen2 string
// interpolation reserves `{` / `}` so each literal brace is `\{` /
// `\}`. Mirrors the `Verdict` struct's required output fields.
let public VERDICT_SCHEMA = "\{\"type\":\"object\",\"properties\":\{\"groups\":\{\"type\":\"array\",\"items\":\{\"type\":\"array\",\"items\":\{\"type\":\"integer\"\}\}\},\"false_positives\":\{\"type\":\"array\",\"items\":\{\"type\":\"integer\"\}\},\"reason\":\{\"type\":\"string\"\}\},\"required\":[\"groups\",\"false_positives\",\"reason\"]\}"


def public format_cluster_prompt(members : array<InputMemberRef>; sources : array<string>) : string {
    return build_string() $(var w) {
        w |> write("Cluster of {length(members)} function(s) to judge:\n\n")
        for (i in iter_range(members)) {
            let m & = unsafe(members[i])
            let kind = m.is_lambda ? "lambda" : "function"
            // Guard `end_line <= 0` (v1 inputs missing the field) so the
            // prompt doesn't show inverted ranges like `24-0`, which would
            // confuse the judge model.
            let loc = m.end_line > 0 ? "{m.file}:{m.line}-{m.end_line}" : "{m.file}:{m.line}"
            w |> write("[{i}] {loc}  name={m.name}  {kind}\n")
            w |> write("```daslang\n")
            w |> write(sources[i])
            w |> write("\n```\n\n")
        }
        w |> write("Call `report_verdict` now. Use member indices [0..{length(members) - 1}].\n")
    }
}


// Wire shape Claude returns through the `report_verdict` tool. Mirrors
// the JSON Schema in `VERDICT_SCHEMA`. `from_JV` walks this struct via
// reflection, so the field names MUST match the schema byte-for-byte.
struct private VerdictWire {
    groups : array<array<int>>
    false_positives : array<int>
    reason : string
}


def public parse_verdict(input : JsonValue?; n_members : int; var v : Verdict) {
    // Reset output fields so a reused `Verdict` (e.g. retry path) can't
    // carry stale data from a previous parse. Token counters are owned
    // by the caller (live_judge fills them from msg.usage), so we don't
    // touch those here.
    v.error := ""
    v.groups |> clear()
    v.false_positives |> clear()
    v.reason := ""
    if (input == null) {
        v.error := "tool input was null"
        return
    }
    var wire <- from_JV(input, type<VerdictWire>)
    v.groups <- wire.groups
    v.false_positives <- wire.false_positives
    v.reason := wire.reason
    // Normalize singleton groups: a `[i]` group is semantically the
    // same as `i` in `false_positives` (a function that doesn't match
    // anyone else), and the partition reads cleaner in the report
    // when expressed as false_positives. Doing this BEFORE validation
    // lets `classify_verdict` rely on "groups[g].length >= 2" as the
    // definition of a real-duplicate group — without that invariant,
    // a model returning `[[0],[1],[2]]` + empty false_positives would
    // be misclassified as `partial` instead of `false_positive`.
    var canonical_groups : array<array<int>>
    for (g in v.groups) {
        if (length(g) == 1) {
            v.false_positives |> push(g[0])
        } else {
            var keep <- g
            canonical_groups |> emplace(keep)
        }
    }
    v.groups <- canonical_groups
    // Validate: every index in [0, n_members) appears exactly once.
    var seen : array<bool>
    seen |> resize(n_members)
    for (g in v.groups) {
        for (idx in g) {
            if (idx < 0 || idx >= n_members) {
                v.error := "verdict index {idx} out of range [0..{n_members - 1}]"
                return
            }
            if (seen[idx]) {
                v.error := "verdict index {idx} appeared more than once"
                return
            }
            seen[idx] = true
        }
    }
    for (idx in v.false_positives) {
        if (idx < 0 || idx >= n_members) {
            v.error := "verdict false_positive index {idx} out of range [0..{n_members - 1}]"
            return
        }
        if (seen[idx]) {
            v.error := "verdict false_positive index {idx} appeared more than once"
            return
        }
        seen[idx] = true
    }
    for (i in iter_range(seen)) {
        if (!seen[i]) {
            v.error := "verdict missed index {i} (not in any group or false_positives)"
            return
        }
    }
}
