options gen2

require dastest/testing_boost public
require strings
require daslib/json_boost
require daslib/fio
require ../cluster_input.das
require ../source_extract.das
require ../judge.das
require ../report.das


// ── parse_verdict (judge.das) ──────────────────────────────────────────


[test]
def test_parse_verdict_full_real(t : T?) {
    var error : string
    let json = "\{\"groups\":[[0,1,2]],\"false_positives\":[],\"reason\":\"identical\"\}"
    let input = read_json(json, error)
    t |> success(empty(error), "fixture parses")
    var v : Verdict
    parse_verdict(input, 3, v)
    t |> success(empty(v.error), "no validation error: {v.error}")
    t |> equal(length(v.groups), 1)
    t |> equal(length(v.groups[0]), 3)
    t |> equal(length(v.false_positives), 0)
    t |> equal(v.reason, "identical")
}


[test]
def test_parse_verdict_partial(t : T?) {
    var error : string
    let json = "\{\"groups\":[[0,1]],\"false_positives\":[2],\"reason\":\"only first two match\"\}"
    let input = read_json(json, error)
    var v : Verdict
    parse_verdict(input, 3, v)
    t |> success(empty(v.error), "no validation error: {v.error}")
    t |> equal(length(v.groups), 1)
    t |> equal(length(v.groups[0]), 2)
    t |> equal(length(v.false_positives), 1)
    t |> equal(v.false_positives[0], 2)
}


[test]
def test_parse_verdict_index_out_of_range(t : T?) {
    var error : string
    let json = "\{\"groups\":[[0,1,5]],\"false_positives\":[],\"reason\":\"oops\"\}"
    let input = read_json(json, error)
    var v : Verdict
    parse_verdict(input, 3, v)
    t |> success(!empty(v.error), "should reject index 5 when n=3")
}


[test]
def test_parse_verdict_duplicate_index(t : T?) {
    var error : string
    let json = "\{\"groups\":[[0,1],[1,2]],\"false_positives\":[],\"reason\":\"oops\"\}"
    let input = read_json(json, error)
    var v : Verdict
    parse_verdict(input, 3, v)
    t |> success(!empty(v.error), "should reject duplicate index 1")
}


[test]
def test_parse_verdict_missing_index(t : T?) {
    var error : string
    // n=4 but only 0,1 covered
    let json = "\{\"groups\":[[0,1]],\"false_positives\":[],\"reason\":\"incomplete\"\}"
    let input = read_json(json, error)
    var v : Verdict
    parse_verdict(input, 4, v)
    t |> success(!empty(v.error), "should reject when 2 and 3 not covered")
}


[test]
def test_parse_verdict_null_input(t : T?) {
    var v : Verdict
    parse_verdict(null, 3, v)
    t |> success(!empty(v.error), "should set error on null input")
}


[test]
def test_parse_verdict_singletons_normalized(t : T?) {
    // A model returning all singleton groups + empty false_positives
    // should normalize to "groups: [], false_positives: [0,1,2]" so
    // classify_verdict reports false_positive (no real-duplicate group).
    var error : string
    let json = "\{\"groups\":[[0],[1],[2]],\"false_positives\":[],\"reason\":\"all distinct\"\}"
    let input = read_json(json, error)
    var v : Verdict
    parse_verdict(input, 3, v)
    t |> success(empty(v.error), "no validation error: {v.error}")
    t |> equal(length(v.groups), 0)
    t |> equal(length(v.false_positives), 3)
    let kind = classify_verdict(3, v)
    t |> equal(verdict_kind_to_string(kind), "false_positive")
}


[test]
def test_parse_verdict_resets_stale_error(t : T?) {
    // A reused Verdict instance must not carry a previous error
    // through a successful parse.
    var v : Verdict
    v.error := "stale error from previous attempt"
    var json_error : string
    let json = "\{\"groups\":[[0,1,2]],\"false_positives\":[],\"reason\":\"identical\"\}"
    let input = read_json(json, json_error)
    parse_verdict(input, 3, v)
    t |> success(empty(v.error), "error should be cleared on successful parse: {v.error}")
}


// ── classify_verdict (report.das) ──────────────────────────────────────


[test]
def test_classify_real_full_group(t : T?) {
    var v : Verdict
    var grp <- [0, 1, 2]
    v.groups |> emplace(grp)
    let kind = classify_verdict(3, v)
    t |> equal(verdict_kind_to_string(kind), "real")
}


[test]
def test_classify_partial_subset(t : T?) {
    var v : Verdict
    var grp <- [0, 1]
    v.groups |> emplace(grp)
    v.false_positives |> push(2)
    let kind = classify_verdict(3, v)
    t |> equal(verdict_kind_to_string(kind), "partial")
}


[test]
def test_classify_false_positive_all(t : T?) {
    let v <- Verdict(false_positives <- [0, 1])
    let kind = classify_verdict(2, v)
    t |> equal(verdict_kind_to_string(kind), "false_positive")
}


[test]
def test_classify_error_propagates(t : T?) {
    var v : Verdict
    v.error := "something broke"
    let kind = classify_verdict(2, v)
    t |> equal(verdict_kind_to_string(kind), "error")
}


// ── source_extract (source_extract.das) ────────────────────────────────


[test]
def test_extract_source_known_range(t : T?) {
    var cache = FileCache()
    var error : string
    // synth.das has loop_sum at lines 24-30 — we only assert it
    // contains `def loop_sum` and is non-empty so we don't pin a
    // specific column count.
    let src = extract_source(cache, "utils/detect-dupe/fixture/synth.das", 24, 30, error)
    t |> success(empty(error), "no error: {error}")
    t |> success(!empty(src), "non-empty source")
    t |> success(find(src, "def loop_sum") >= 0, "extracted source contains def loop_sum")
}


[test]
def test_extract_source_caches(t : T?) {
    var cache = FileCache()
    var error : string
    let _ = extract_source(cache, "utils/detect-dupe/fixture/synth.das", 1, 5, error)
    t |> success(key_exists(cache.files, "utils/detect-dupe/fixture/synth.das"), "file in cache after first read")
}


[test]
def test_extract_source_missing_file(t : T?) {
    var cache = FileCache()
    var error : string
    let src = extract_source(cache, "utils/detect-dupe/fixture/__does_not_exist.das", 1, 5, error)
    t |> success(!empty(error), "missing file should set error")
    t |> success(empty(src), "empty source on error")
}


[test]
def test_extract_source_v1_fallback(t : T?) {
    var cache = FileCache()
    var error : string
    // end_line == 0 → single-line peek (legacy v1 input)
    let src = extract_source(cache, "utils/detect-dupe/fixture/synth.das", 24, 0, error)
    t |> success(empty(error), "no error: {error}")
    t |> success(!empty(src), "non-empty source even with end_line=0")
}


[test]
def test_extract_source_end_line_past_eof(t : T?) {
    // Stale detect-dupe report against an edited file: end_line points
    // past EOF. Should error rather than silently clamp + extract.
    var cache = FileCache()
    var error : string
    let src = extract_source(cache, "utils/detect-dupe/fixture/synth.das", 24, 99999, error)
    t |> success(!empty(error), "end_line past EOF should set error")
    t |> success(empty(src), "empty source on error")
}


[test]
def test_extract_source_inverted_range(t : T?) {
    // Degenerate end_line < start_line. Should error rather than
    // silently coerce to a single-line peek.
    var cache = FileCache()
    var error : string
    let src = extract_source(cache, "utils/detect-dupe/fixture/synth.das", 30, 20, error)
    t |> success(!empty(error), "inverted range should set error")
    t |> success(empty(src), "empty source on error")
}


// ── read_input_report (cluster_input.das) ─────────────────────────────


// Cross-platform temp-file helper. Mirrors `make_temp_corpus_file` in
// `utils/mcp/test_tools.das` — `create_temp_file_result` resolves the
// OS tempdir (`%TEMP%` on Windows, `$TMPDIR`/`/var/folders/...` on
// macOS, `/tmp` on Linux), so tests don't hardcode `/tmp` and break
// the Windows extended_checks runner.
def make_temp_input_file(t : T?; body : string) : string {
    let r = create_temp_file_result("find_dupe_test", ".json")
    if (!(r is value)) {
        t |> failure("create_temp_file_result failed")
        return ""
    }
    let path = unsafe(r.value)
    fopen(path, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint(body)
        }
    }
    return path
}


[test]
def test_read_input_report_basic(t : T?) {
    let json_text = "\{\"summary\":\{\"files_scanned\":1,\"functions_analyzed\":3,\"exact_clusters\":1,\"fuzzy_pairs\":0,\"threshold\":0.7,\"min_tokens\":8\},\"exact_clusters\":[\{\"canonical_sample\":\"\",\"token_count\":12,\"members\":[\{\"file\":\"a.das\",\"name\":\"foo\",\"line\":1,\"end_line\":5,\"is_lambda\":false\},\{\"file\":\"b.das\",\"name\":\"bar\",\"line\":10,\"end_line\":15,\"is_lambda\":false\}]\}],\"fuzzy_pairs\":[]\}"
    let path = make_temp_input_file(t, json_text)
    if (empty(path)) return
    var report : InputReport
    var error : string
    let ok = read_input_report(path, report, error)
    t |> success(ok, "read_input_report should succeed: {error}")
    t |> equal(length(report.exact_clusters), 1)
    t |> equal(length(report.exact_clusters[0].members), 2)
    t |> equal(report.exact_clusters[0].members[0].name, "foo")
    t |> equal(report.exact_clusters[0].members[1].end_line, 15)
    remove(path)
}


[test]
def test_read_input_report_missing_file(t : T?) {
    // Generate a unique temp path then immediately delete it — the
    // path is now guaranteed nonexistent and unique to this run, and
    // works on Windows where `/tmp` doesn't exist. Same pattern as
    // utils/mcp/test_tools.das `test_detect_duplicates_corpus_not_found`.
    let bogus = make_temp_input_file(t, "")
    if (empty(bogus)) return
    remove(bogus)
    var report : InputReport
    var error : string
    let ok = read_input_report(bogus, report, error)
    t |> success(!ok, "should fail")
    t |> success(!empty(error), "error should be set")
}
