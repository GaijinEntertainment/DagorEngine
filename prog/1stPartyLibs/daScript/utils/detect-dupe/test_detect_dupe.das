options gen2

require dastest/testing_boost public
require strings
require math
require daslib/defer
require daslib/fio
require daslib/command_line
require daslib/json_boost

require canonical
require minhash
require cluster
require exchange
require pipeline
require patterns
require report
require ../common/parallel_workers.das


def make_record(name : string; canonical : string; line : int; want_sigs : bool) : FuncRecord {
    return <- make_record(MemberRef(file = "test.das", name = name, line = line, end_line = line + 5, is_lambda = false),
        canonical, want_sigs, 0)
}


def find_canonical_by_name(records : array<FuncRecord>; name : string) : string {
    for (rec in records) {
        if (rec.ref.name == name) return rec.canonical
    }
    return ""
}


def find_record_by_name(records : array<FuncRecord>; name : string) : int {
    for (i in range(length(records))) {
        if (records[i].ref.name == name) return i
    }
    return -1
}


def cluster_has_member(c : Cluster; name : string) : bool {
    for (m in c.members) {
        if (m.name == name) return true
    }
    return false
}


def find_cluster_with_member(clusters : array<Cluster>; name : string) : int {
    for (i in range(length(clusters))) {
        if (cluster_has_member(clusters[i], name)) return i
    }
    return -1
}


def parse_fixture(file : string; var records : array<FuncRecord>; lambdas_only : bool; want_sigs : bool) : bool {
    var seen_loc : table<string; void?>
    return parse_and_collect(file, records, seen_loc, false, lambdas_only, want_sigs, 0)
}


def fabs_f(x : float) : float {
    return abs(x)
}


def parse_canonical_fixture(t : T?; var records : array<FuncRecord>) : bool {
    let ok = parse_fixture(CANONICAL_FIXTURE, records, false, false)
    t |> success(ok, "canonical_cases.das parsed")
    return ok
}


def find_canonical_required(t : T?; records : array<FuncRecord>; name : string) : string {
    let canon = find_canonical_by_name(records, name)
    t |> success(!empty(canon), "canonical present for {name}")
    return canon
}


[test]
def test_tokenize_basic(t : T?) {
    let toks <- tokenize_canonical("A B C")
    t |> equal(length(toks), 3)
    t |> equal(toks[0], "A")
    t |> equal(toks[1], "B")
    t |> equal(toks[2], "C")
}


[test]
def test_tokenize_empty(t : T?) {
    let toks <- tokenize_canonical("")
    t |> equal(length(toks), 0)
}


[test]
def test_tokenize_collapses_whitespace(t : T?) {
    let toks <- tokenize_canonical("  A   B  ")
    t |> equal(length(toks), 2)
    t |> equal(toks[0], "A")
    t |> equal(toks[1], "B")
}


[test]
def test_tokenize_trailing_space(t : T?) {
    let toks <- tokenize_canonical("FN ENDFN ")
    t |> equal(length(toks), 2)
}


[test]
def test_minhash_signature_length(t : T?) {
    let toks <- ["alpha", "beta", "gamma"]
    let sig <- minhash_signature(toks)
    t |> equal(length(sig), 64)
}


[test]
def test_minhash_padding_for_short_input(t : T?) {
    let toks <- ["only_one"]
    let sig <- minhash_signature(toks)
    t |> equal(length(sig), 64)
}


[test]
def test_minhash_deterministic(t : T?) {
    var toks_a : array<string>
    var toks_b : array<string>
    toks_a |> reserve(20)
    toks_b |> reserve(20)
    for (i in range(20)) {
        toks_a |> push("token_{i}")
        toks_b |> push("token_{i}")
    }
    let sig_a <- minhash_signature(toks_a)
    let sig_b <- minhash_signature(toks_b)
    var all_match = true
    for (a, b in sig_a, sig_b) {
        if (a != b) {
            all_match = false
        }
    }
    t |> success(all_match, "signatures of identical token lists must match slot-for-slot")
}


[test]
def test_jaccard_identical(t : T?) {
    var toks : array<string>
    toks |> reserve(15)
    for (i in range(15)) {
        toks |> push("t{i}")
    }
    let sig_a <- minhash_signature(toks)
    let sig_b <- minhash_signature(toks)
    t |> equal(jaccard_estimate(sig_a, sig_b), 1.0f)
}


[test]
def test_jaccard_empty(t : T?) {
    let empty_a : array<uint64>
    let empty_b : array<uint64>
    t |> equal(jaccard_estimate(empty_a, empty_b), 0.0f)
}


[test]
def test_jaccard_length_mismatch(t : T?) {
    let toks <- ["x", "y"]
    let sig <- minhash_signature(toks)
    let short_sig <- [0ul]
    t |> equal(jaccard_estimate(sig, short_sig), 0.0f)
}


[test]
def test_jaccard_unrelated_low(t : T?) {
    var toks_a : array<string>
    var toks_b : array<string>
    toks_a |> reserve(30)
    toks_b |> reserve(30)
    for (i in range(30)) {
        toks_a |> push("aaa_{i}")
        toks_b |> push("zzz_{i + 1000}")
    }
    let sig_a <- minhash_signature(toks_a)
    let sig_b <- minhash_signature(toks_b)
    let j = jaccard_estimate(sig_a, sig_b)
    t |> success(j < 0.3f, "unrelated inputs should have jaccard < 0.3, got {j}")
}


[test]
def test_exact_groups_duplicates(t : T?) {
    var records <- array<FuncRecord>(
        <- make_record("a", "FN ARG <var_0> RET <var_0> ENDFN", 1, false),
        <- make_record("b", "FN ARG <var_0> RET <var_0> ENDFN", 2, false),
        <- make_record("c", "FN ARG <var_0> RET LIT ENDFN", 3, false),
        <- make_record("d", "FN ARG <var_0> RET <var_0> ENDFN", 4, false))
    let clusters <- exact_buckets(records, 0)
    t |> equal(length(clusters), 1)
    t |> equal(length(clusters[0].members), 3)
    t |> success(cluster_has_member(clusters[0], "a"), "cluster contains a")
    t |> success(cluster_has_member(clusters[0], "b"), "cluster contains b")
    t |> success(cluster_has_member(clusters[0], "d"), "cluster contains d")
    t |> success(!cluster_has_member(clusters[0], "c"), "cluster does NOT contain c")
}


[test]
def test_exact_min_tokens_filter(t : T?) {
    var records <- array<FuncRecord>(
        <- make_record("a", "FN ENDFN", 1, false),
        <- make_record("b", "FN ENDFN", 2, false),
        <- make_record("c", "FN BODY BLK STMT ENDBLK ENDFN", 3, false),
        <- make_record("d", "FN BODY BLK STMT ENDBLK ENDFN", 4, false))
    let clusters <- exact_buckets(records, 5)
    t |> equal(length(clusters), 1)
    t |> equal(length(clusters[0].members), 2)
    t |> success(cluster_has_member(clusters[0], "c"), "c clustered")
    t |> success(cluster_has_member(clusters[0], "d"), "d clustered")
}


[test]
def test_exact_assigns_cluster_id(t : T?) {
    var records <- array<FuncRecord>(
        <- make_record("a", "FN X Y Z ENDFN", 1, false),
        <- make_record("b", "FN X Y Z ENDFN", 2, false),
        <- make_record("c", "FN P Q R ENDFN", 3, false))
    let clusters <- exact_buckets(records, 0)
    let i_a = find_record_by_name(records, "a")
    let i_b = find_record_by_name(records, "b")
    let i_c = find_record_by_name(records, "c")
    t |> success(records[i_a].cluster_id >= 0, "a got a cluster_id")
    t |> success(records[i_b].cluster_id >= 0, "b got a cluster_id")
    t |> equal(records[i_a].cluster_id, records[i_b].cluster_id)
    t |> equal(records[i_c].cluster_id, -1)
}


[test]
def test_exact_singleton_not_reported(t : T?) {
    var records : array<FuncRecord>
    records |> emplace <| make_record("solo", "FN UNIQUE STREAM ENDFN", 1, false)
    let clusters <- exact_buckets(records, 0)
    t |> equal(length(clusters), 0)
}


[test]
def test_sort_clusters_by_size(t : T?) {
    var records <- array<FuncRecord>(
        <- make_record("a1", "FN AAA ENDFN", 1, false),
        <- make_record("a2", "FN AAA ENDFN", 2, false),
        <- make_record("b1", "FN BBB ENDFN", 3, false),
        <- make_record("b2", "FN BBB ENDFN", 4, false),
        <- make_record("b3", "FN BBB ENDFN", 5, false))
    var clusters <- exact_buckets(records, 0)
    sort_clusters(clusters)
    t |> equal(length(clusters), 2)
    t |> equal(length(clusters[0].members), 3)
    t |> equal(length(clusters[1].members), 2)
    t |> success(cluster_has_member(clusters[0], "b1"), "size-3 cluster ranked first")
}


[test]
def test_sort_pairs_by_similarity(t : T?) {
    var pairs : array<FuzzyPair>
    var p1 : FuzzyPair
    p1.similarity = 0.75f
    p1.a.name = "low_a"
    p1.a.file = "f"
    p1.a.line = 1
    p1.b.name = "low_b"
    p1.b.file = "f"
    p1.b.line = 2
    var p2 : FuzzyPair
    p2.similarity = 0.95f
    p2.a.name = "hi_a"
    p2.a.file = "f"
    p2.a.line = 3
    p2.b.name = "hi_b"
    p2.b.file = "f"
    p2.b.line = 4
    pairs |> emplace(p1)
    pairs |> emplace(p2)
    sort_pairs(pairs)
    t |> equal(pairs[0].similarity, 0.95f)
    t |> equal(pairs[1].similarity, 0.75f)
}


[test]
def test_fuzzy_near_duplicates_pair(t : T?) {
    var records : array<FuncRecord>
    let canon_a = "FN ARG <var_0> TYP BODY BLK STMT LET LET_VAR <var_1> LIT STMT FOR FOR_VAR <var_2> <var_0> FOR_BODY BLK STMT OP2:+= <var_1> <var_2> ENDBLK STMT RET <var_1> ENDBLK ENDFN"
    let canon_b = "FN ARG <var_0> TYP BODY BLK STMT LET LET_VAR <var_1> LIT STMT FOR FOR_VAR <var_2> <var_0> FOR_BODY BLK STMT OP2:*= <var_1> <var_2> ENDBLK STMT RET <var_1> ENDBLK ENDFN"
    records |> emplace <| make_record("a", canon_a, 1, true)
    records |> emplace <| make_record("b", canon_b, 2, true)
    let pairs <- fuzzy_pairs(records, 0.7f, 0)
    t |> equal(length(pairs), 1)
    t |> success(pairs[0].similarity >= 0.7f, "similarity above threshold, got {pairs[0].similarity}")
}


[test]
def test_fuzzy_skips_identical(t : T?) {
    var records : array<FuncRecord>
    let canon = "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN"
    records |> emplace <| make_record("a", canon, 1, true)
    records |> emplace <| make_record("b", canon, 2, true)
    let pairs <- fuzzy_pairs(records, 0.7f, 0)
    t |> equal(length(pairs), 0)
}


[test]
def test_fuzzy_skips_same_cluster(t : T?) {
    var records : array<FuncRecord>
    let canon = "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN"
    records |> emplace <| make_record("a", canon, 1, true)
    records |> emplace <| make_record("b", canon, 2, true)
    let clusters <- exact_buckets(records, 0)
    t |> equal(length(clusters), 1)
    let pairs <- fuzzy_pairs(records, 0.7f, 0)
    t |> equal(length(pairs), 0)
}


[test]
def test_fuzzy_length_ratio_gate(t : T?) {
    let records <- array<FuncRecord>(
        <- make_record(
            "short",
            "FN ARG <var_0> TYP BODY BLK STMT RET <var_0> ENDBLK ENDFN",
            1, true),
        <- make_record(
            "long",
            "FN ARG <var_0> TYP ARG <var_1> TYP ARG <var_2> TYP TYP BODY BLK STMT LET LET_VAR <var_3> LIT STMT FOR FOR_VAR <var_4> <var_0> FOR_BODY BLK STMT OP2:+= <var_3> <var_4> ENDBLK STMT FOR FOR_VAR <var_5> <var_1> FOR_BODY BLK STMT OP2:*= <var_3> <var_5> ENDBLK STMT RET <var_3> ENDBLK ENDFN",
            2, true))
    let pairs <- fuzzy_pairs(records, 0.7f, 0)
    t |> equal(length(pairs), 0)
}


[test]
def test_fuzzy_min_tokens_filter(t : T?) {
    let records <- array<FuncRecord>(
        <- make_record("a", "FN A B C ENDFN", 1, true),
        <- make_record("b", "FN A B C ENDFN", 2, true))
    let pairs <- fuzzy_pairs(records, 0.7f, 100)
    t |> equal(length(pairs), 0)
}


def make_temp_export_path(t : T?) : string {
    let r = create_temp_file_result("detect_dupe_test", ".json")
    if (!(r is value)) {
        t |> failure("create_temp_file_result failed: {unsafe(r.error)}")
        return ""
    }
    return unsafe(r.value)
}


[test]
def test_export_import_basic(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    var records <- array<FuncRecord>(
        <- make_record("alpha", "FN A B C ENDFN", 10, false),
        <- make_record("beta", "FN X Y Z ENDFN", 20, false))
    records[1].ref.is_lambda = true

    let wrote = write_export(records, path)
    t |> success(wrote, "write_export returned true")

    var loaded : array<FuncRecord>
    var err : string
    let read_ok = read_import(path, loaded, false, 0, err)
    t |> success(read_ok, "read_import returned true; err={err}")
    t |> equal(length(loaded), 2)
    t |> equal(loaded[0].ref.name, "alpha")
    t |> equal(loaded[0].ref.line, 10)
    t |> equal(loaded[0].ref.is_lambda, false)
    t |> equal(loaded[0].canonical, "FN A B C ENDFN")
    t |> equal(loaded[1].ref.name, "beta")
    t |> equal(loaded[1].ref.is_lambda, true)
    remove(path)
}


[test]
def test_export_import_recomputes_sig(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    var records : array<FuncRecord>
    records |> emplace <| make_record(
        "x",
        "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN",
        1, false)
    write_export(records, path)
    var loaded : array<FuncRecord>
    var err : string
    read_import(path, loaded, true, 0, err)
    t |> equal(length(loaded), 1)
    t |> equal(length(loaded[0].sig), 64)
    t |> success(loaded[0].token_count > 0, "token_count was recomputed")
    remove(path)
}


[test]
def test_export_import_skips_sig_when_no_fuzzy(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    var records : array<FuncRecord>
    records |> emplace <| make_record("x", "FN A B C D E F G H I J ENDFN", 1, false)
    write_export(records, path)
    var loaded : array<FuncRecord>
    var err : string
    read_import(path, loaded, false, 0, err)
    t |> equal(length(loaded), 1)
    t |> equal(length(loaded[0].sig), 0)
    t |> success(loaded[0].token_count > 0, "token_count still computed")
    remove(path)
}


[test]
def test_import_missing_file(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    remove(path)
    var loaded : array<FuncRecord>
    var err : string
    let ok = read_import(path, loaded, false, 0, err)
    t |> success(!ok, "read_import returns false for missing file")
    t |> success(!empty(err), "error message set")
}


[test]
def test_import_malformed_json(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    fopen(path, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint("this is not valid json !!!")
        }
    }
    var loaded : array<FuncRecord>
    var err : string
    let ok = read_import(path, loaded, false, 0, err)
    t |> success(!ok, "read_import returns false for garbage")
    t |> success(!empty(err), "error message set")
    remove(path)
}


[test]
def test_import_bad_schema_version(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    var bad = ExportEnvelope()
    bad.schema_version = 99
    let body = sprint_json(bad, false)
    fopen(path, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint(body)
        }
    }
    var loaded : array<FuncRecord>
    var err : string
    let ok = read_import(path, loaded, false, 0, err)
    t |> success(!ok, "read_import rejects unknown schema_version")
    t |> success(find(err, "schema_version") >= 0, "error mentions schema_version: {err}")
    remove(path)
}


[test]
def test_import_rejects_zero_end_line(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    let body = "\{\"schema_version\":{EXCHANGE_SCHEMA_VERSION},\"functions\":[\{\"name\":\"foo\",\"file\":\"a.das\",\"line\":10,\"end_line\":0,\"is_lambda\":false,\"canonical\":\"FN A B ENDFN\"\}]\}"
    fopen(path, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint(body)
        }
    }
    var loaded : array<FuncRecord>
    var err : string
    let quiet = true
    let ok = read_import(path, loaded, false, 0, err, quiet)
    t |> success(ok, "read_import succeeds (envelope is valid; record is dropped as malformed)")
    t |> equal(length(loaded), 0)
    remove(path)
}


[test]
def test_import_rejects_zero_line(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    let body = "\{\"schema_version\":{EXCHANGE_SCHEMA_VERSION},\"functions\":[\{\"name\":\"foo\",\"file\":\"a.das\",\"line\":0,\"end_line\":5,\"is_lambda\":false,\"canonical\":\"FN A B ENDFN\"\}]\}"
    fopen(path, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint(body)
        }
    }
    var loaded : array<FuncRecord>
    var err : string
    let quiet = true
    let ok = read_import(path, loaded, false, 0, err, quiet)
    t |> success(ok, "envelope is valid; line=0 record is dropped as malformed")
    t |> equal(length(loaded), 0)
    remove(path)
}


[test]
def test_read_path_lines_basic(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    fopen(path, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint("# comment line\n")
            fw |> fprint("\n")
            fw |> fprint("daslib/foo.das\n")
            fw |> fprint("    \n")
            fw |> fprint("  daslib/bar.das  \n")
            fw |> fprint("# trailing comment\n")
        }
    }
    var paths : array<string>
    var err : string
    let ok = read_path_lines_from_file(path, paths, err)
    t |> success(ok, "read_path_lines_from_file returned true; err={err}")
    t |> equal(length(paths), 2)
    t |> equal(paths[0], "daslib/foo.das")
    t |> equal(paths[1], "daslib/bar.das")
    remove(path)
}


[test]
def test_read_path_lines_missing_file(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    remove(path)
    var paths : array<string>
    var err : string
    let ok = read_path_lines_from_file(path, paths, err)
    t |> success(!ok, "read_path_lines_from_file returns false for missing file")
    t |> success(!empty(err), "error message set: {err}")
    t |> equal(length(paths), 0)
}


let SCAN_FIXTURE_DIR = "utils/detect-dupe/fixture"


[test]
def test_scan_das_files_dedup(t : T?) {
    var files1 : array<string>
    var cache1 : table<string; void?>
    scan_das_files(SCAN_FIXTURE_DIR, files1, cache1)
    let n1 = length(files1)
    t |> success(n1 > 0, "fixture dir scan picked up at least one file")
    if (n1 == 0) return
    let before = length(files1)
    scan_das_files(SCAN_FIXTURE_DIR, files1, cache1)
    t |> equal(length(files1), before, "second scan with same cache adds nothing")
    var files2 : array<string>
    scan_das_files(SCAN_FIXTURE_DIR, files2, cache1)
    t |> equal(length(files2), 0, "second scan with different out-array but same cache yields zero")
}


[test]
def test_skip_ast_fuzz_selftest(t : T?) {
    t |> success(is_skip_file("repo/tools/ast-fuzz/selftest/alias.das", "alias.das"),
        "ast-fuzz selftest victims are excluded")
    t |> success(is_skip_file("repo\\tools\\ast-fuzz\\selftest\\cycle.das", "cycle.das"),
        "backslash paths normalize before the check")
    t |> success(!is_skip_file("repo/tools/ast-fuzz/main.das", "main.das"),
        "the ast-fuzz driver itself stays in the corpus")
    var files : array<string>
    var cache : table<string; void?>
    scan_das_files("{SCAN_FIXTURE_DIR}/ast-fuzz/selftest", files, cache)
    t |> equal(length(files), 0, "directory scan yields nothing from a selftest dir")
}


def synth_paths(n : int) : array<string> {
    var out : array<string>
    out |> reserve(n)
    for (i in range(n)) {
        out |> push("file_{i}.das")
    }
    return <- out
}


[test]
def test_chunk_files_contiguous_and_complete(t : T?) {
    let files <- synth_paths(11)
    let chunks <- chunk_files(files, 4)
    t |> equal(length(chunks), 4, "11 files / 4 workers => 4 chunks")
    t |> equal(length(chunks[0]), 3)
    t |> equal(length(chunks[1]), 3)
    t |> equal(length(chunks[2]), 3)
    t |> equal(length(chunks[3]), 2)
    var rebuilt : array<string>
    rebuilt |> reserve(11)
    for (c in chunks) {
        rebuilt |> push_from(c)
    }
    t |> equal(length(rebuilt), 11)
    for (i in range(11)) {
        t |> equal(rebuilt[i], "file_{i}.das")
    }
}


[test]
def test_chunk_files_workers_one(t : T?) {
    let files <- synth_paths(7)
    let chunks <- chunk_files(files, 1)
    t |> equal(length(chunks), 1, "workers=1 => single chunk")
    t |> equal(length(chunks[0]), 7)
}


[test]
def test_chunk_files_workers_more_than_files(t : T?) {
    let files <- synth_paths(3)
    let chunks <- chunk_files(files, 8)
    t |> equal(length(chunks), 3, "workers > files => one chunk per file, no empty trailers")
    for (c in chunks) {
        t |> equal(length(c), 1)
    }
}


[test]
def test_chunk_files_empty_input(t : T?) {
    let files : array<string>
    let chunks <- chunk_files(files, 4)
    t |> equal(length(chunks), 0, "empty input => empty chunk list")
}


def write_shard(t : T?; functions : array<ExportedFunction>; schema : int) : string {
    let path = make_temp_export_path(t)
    if (empty(path)) return ""
    var env = ExportEnvelope(schema_version = schema)
    env.functions |> reserve(length(functions))
    for (ef in functions) {
        env.functions |> emplace(ExportedFunction(
            name = ef.name,
            file = ef.file,
            line = ef.line,
            end_line = ef.end_line,
            is_lambda = ef.is_lambda,
            canonical = ef.canonical))
    }
    let body = sprint_json(env, false)
    fopen(path, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint(body)
        }
    }
    return path
}


def make_export_func(name : string; line : int) : ExportedFunction {
    return ExportedFunction(
        name = name,
        file = "test.das",
        line = line,
        end_line = line + 5,
        is_lambda = false,
        canonical = "FN {name}")
}


[test]
def test_merge_envelope_files_concat_in_order(t : T?) {
    let s0 <- [make_export_func("alpha", 10), make_export_func("beta", 20)]
    let s1 <- [make_export_func("gamma", 30)]
    let s2 <- [make_export_func("delta", 40), make_export_func("epsilon", 50)]

    let p0 = write_shard(t, s0, EXCHANGE_SCHEMA_VERSION)
    let p1 = write_shard(t, s1, EXCHANGE_SCHEMA_VERSION)
    let p2 = write_shard(t, s2, EXCHANGE_SCHEMA_VERSION)
    if (empty(p0) || empty(p1) || empty(p2)) return

    let out = make_temp_export_path(t)
    if (empty(out)) return
    let shards <- [p0, p1, p2]
    var err : string
    let ok = merge_envelope_files(shards, out, err)
    t |> success(ok, "merge_envelope_files returned true; err={err}")

    var loaded : array<FuncRecord>
    var rerr : string
    let read_ok = read_import(out, loaded, false, 0, rerr)
    t |> success(read_ok, "read_import on merged file; err={rerr}")
    t |> equal(length(loaded), 5)
    t |> equal(loaded[0].ref.name, "alpha")
    t |> equal(loaded[1].ref.name, "beta")
    t |> equal(loaded[2].ref.name, "gamma")
    t |> equal(loaded[3].ref.name, "delta")
    t |> equal(loaded[4].ref.name, "epsilon")
    remove(p0)
    remove(p1)
    remove(p2)
    remove(out)
}


[test]
def test_merge_envelope_files_rejects_bad_schema(t : T?) {
    let good <- [make_export_func("ok", 10)]
    let bad <- [make_export_func("nope", 20)]
    let p_good = write_shard(t, good, EXCHANGE_SCHEMA_VERSION)
    let p_bad = write_shard(t, bad, 99)
    if (empty(p_good) || empty(p_bad)) return

    let out = make_temp_export_path(t)
    if (empty(out)) return
    let shards <- [p_good, p_bad]
    var err : string
    let ok = merge_envelope_files(shards, out, err)
    t |> success(!ok, "merge rejects bad schema_version")
    t |> success(find(err, "schema_version") >= 0, "error mentions schema_version: {err}")
    remove(p_good)
    remove(p_bad)
    remove(out)
}


[test]
def test_merge_envelope_files_missing_shard(t : T?) {
    let missing = make_temp_export_path(t)
    if (empty(missing)) return
    remove(missing)
    let out = make_temp_export_path(t)
    if (empty(out)) return
    let shards <- [missing]
    var err : string
    let ok = merge_envelope_files(shards, out, err)
    t |> success(!ok, "merge rejects missing shard")
    t |> success(!empty(err), "error message set: {err}")
    remove(out)
}


[test]
def test_derive_sig_and_count_no_sigs(t : T?) {
    var rec = FuncRecord()
    rec.canonical = "FN A B C D E ENDFN"
    derive_sig_and_count(rec, false, 0)
    t |> equal(rec.token_count, 7)
    t |> equal(length(rec.sig), 0)
}


[test]
def test_derive_sig_and_count_with_sigs(t : T?) {
    var rec = FuncRecord()
    rec.canonical = "FN A B C D E F G H I J K ENDFN"
    derive_sig_and_count(rec, true, 0)
    t |> equal(length(rec.sig), 64)
}


[test]
def test_derive_sig_and_count_below_floor(t : T?) {
    var rec = FuncRecord()
    rec.canonical = "FN A B ENDFN"
    derive_sig_and_count(rec, true, 100)
    t |> equal(length(rec.sig), 0)
    t |> equal(rec.token_count, 4)
}


let CANONICAL_FIXTURE = "utils/detect-dupe/fixture/canonical_cases.das"


[test]
def test_canonical_alpha_rename(t : T?) {
    var records : array<FuncRecord>
    if (!parse_canonical_fixture(t, records)) return
    let a = find_canonical_required(t, records, "case_add_baseline")
    let b = find_canonical_required(t, records, "case_add_renamed")
    t |> equal(a, b)
}


[test]
def test_canonical_type_collapse(t : T?) {
    var records : array<FuncRecord>
    if (!parse_canonical_fixture(t, records)) return
    let a = find_canonical_required(t, records, "case_add_baseline")
    let c = find_canonical_required(t, records, "case_add_typed_float")
    t |> equal(a, c)
}


[test]
def test_canonical_literal_collapse(t : T?) {
    var records : array<FuncRecord>
    if (!parse_canonical_fixture(t, records)) return
    let a = find_canonical_required(t, records, "case_lit_one")
    let b = find_canonical_required(t, records, "case_lit_big")
    t |> equal(a, b)
    t |> success(find(a, "LIT") >= 0, "LIT token present")
}


[test]
def test_canonical_call_name_preserved(t : T?) {
    var records : array<FuncRecord>
    if (!parse_canonical_fixture(t, records)) return
    let a = find_canonical_required(t, records, "case_call_push")
    let b = find_canonical_required(t, records, "case_call_reserve")
    t |> success(a != b, "different called fns must produce different canonical")
    t |> success(find(a, "push") >= 0, "case_call_push canonical mentions push")
    t |> success(find(b, "reserve") >= 0, "case_call_reserve canonical mentions reserve")
}


[test]
def test_canonical_field_marker(t : T?) {
    var records : array<FuncRecord>
    if (!parse_canonical_fixture(t, records)) return
    let a = find_canonical_required(t, records, "case_field_read")
    t |> success(find(a, ".FLD") >= 0, ".FLD token present in: {a}")
}


[test]
def test_canonical_swizzle_is_a_field_before_infer(t : T?) {
    var records : array<FuncRecord>
    if (!parse_canonical_fixture(t, records)) return
    let a = find_canonical_required(t, records, "case_swizzle")
    t |> success(find(a, ".FLD") >= 0, "`v.xy` is a plain field access to the parser: {a}")
    t |> success(find(a, ".SWZ") < 0, "no .SWZ token before infer: {a}")
}


[test]
def test_canonical_for_vs_while(t : T?) {
    var records : array<FuncRecord>
    if (!parse_canonical_fixture(t, records)) return
    let f = find_canonical_required(t, records, "case_for_loop")
    let w = find_canonical_required(t, records, "case_while_loop")
    t |> success(find(f, "FOR") >= 0, "FOR token in for-loop")
    t |> success(find(w, "WHILE") >= 0, "WHILE token in while-loop")
    t |> success(find(f, "WHILE") < 0, "no WHILE in for-loop")
    t |> success(find(w, "FOR") < 0, "no FOR in while-loop")
}


[test]
def test_canonical_lambda_emits_mk_blk(t : T?) {
    var records : array<FuncRecord>
    if (!parse_canonical_fixture(t, records)) return
    let l = find_canonical_required(t, records, "case_lambda_use")
    t |> success(find(l, "MK_BLK") >= 0, "MK_BLK present in lambda use: {l}")
}


[test]
def test_canonical_let_var_token(t : T?) {
    var records : array<FuncRecord>
    if (!parse_canonical_fixture(t, records)) return
    let a = find_canonical_required(t, records, "case_let_var")
    t |> success(find(a, "LET_VAR") >= 0, "LET_VAR present in: {a}")
}


let SYNTH_FIXTURE = "utils/detect-dupe/fixture/synth.das"


[test]
def test_pipeline_synth_collects_functions(t : T?) {
    var records : array<FuncRecord>
    let ok = parse_fixture(SYNTH_FIXTURE, records, false, false)
    t |> success(ok, "synth.das parsed")
    t |> equal(length(records), 10)
    t |> success(find_record_by_name(records, "add_int") >= 0, "add_int present")
    t |> success(find_record_by_name(records, "loop_sum") >= 0, "loop_sum present")
    t |> success(find_record_by_name(records, "with_lambda") >= 0, "with_lambda present")
}


[test]
def test_pipeline_synth_exact_clusters(t : T?) {
    var records : array<FuncRecord>
    parse_fixture(SYNTH_FIXTURE, records, false, false)
    var clusters <- exact_buckets(records, 0)
    sort_clusters(clusters)
    t |> equal(length(clusters), 2)
    t |> equal(length(clusters[0].members), 3)
    t |> success(cluster_has_member(clusters[0], "add_int"), "add_int in first cluster")
    t |> success(cluster_has_member(clusters[0], "add_int_renamed_args"), "add_int_renamed_args")
    t |> success(cluster_has_member(clusters[0], "add_float"), "add_float")
    t |> equal(length(clusters[1].members), 2)
    t |> success(cluster_has_member(clusters[1], "with_lambda"), "with_lambda")
    t |> success(cluster_has_member(clusters[1], "with_lambda_renamed"), "with_lambda_renamed")
}


[test]
def test_pipeline_synth_fuzzy_pair(t : T?) {
    var records : array<FuncRecord>
    parse_fixture(SYNTH_FIXTURE, records, false, true)
    let clusters <- exact_buckets(records, 0)
    let pairs <- fuzzy_pairs(records, 0.7f, 0)
    t |> equal(length(pairs), 1)
    let names_match = (
        (pairs[0].a.name == "loop_sum" && pairs[0].b.name == "loop_product") ||
        (pairs[0].a.name == "loop_product" && pairs[0].b.name == "loop_sum"))
    t |> success(names_match, "loop_sum/loop_product pair (got {pairs[0].a.name} / {pairs[0].b.name})")
    t |> success(pairs[0].similarity >= 0.7f, "similarity above threshold: {pairs[0].similarity}")
}


[test]
def test_pipeline_lambdas_only_mode(t : T?) {
    var records : array<FuncRecord>
    let ok = parse_fixture(SYNTH_FIXTURE, records, true, false)
    t |> success(ok, "parse in lambdas-only mode")
    t |> equal(find_record_by_name(records, "add_int"), -1)
    t |> equal(find_record_by_name(records, "main"), -1)
    t |> equal(find_record_by_name(records, "loop_sum"), -1)
    t |> equal(find_record_by_name(records, "with_lambda"), -1)
    t |> equal(length(records), 4, "one record per make-block: with_lambda, with_lambda_renamed, two on one line")
    var same_line : array<string>
    var with_lambda_name = ""
    for (rec in records) {
        t |> success(rec.ref.is_lambda, "lambdas-only record has is_lambda=true: {rec.ref.name}")
        if (rec.ref.name |> starts_with("two_lambdas_one_line`lambda@")) {
            same_line |> push(rec.ref.name)
        } elif (rec.ref.name |> starts_with("with_lambda`lambda@")) {
            with_lambda_name = rec.ref.name
        }
    }
    t |> success(!empty(with_lambda_name), "a lambda record is named <function>`lambda@<column>")
    t |> equal(length(same_line), 2, "two make-blocks on one source line are two records, not one")
    t |> success(length(same_line) == 2 && same_line[0] != same_line[1],
        "the two records on one line differ by column in the name, so reports and baselines keep them apart: {same_line}")
    let a = find_canonical_by_name(records, with_lambda_name)
    t |> success(find(a, "MK_BLK") >= 0 && find(a, "BLK_ARG") >= 0, "a lambda record is the make-block's own canonical: {a}")
}


[test]
def test_pipeline_export_import_clusters_match(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    var records_parsed : array<FuncRecord>
    parse_fixture(SYNTH_FIXTURE, records_parsed, false, true)
    write_export(records_parsed, path)

    var records_imported : array<FuncRecord>
    var err : string
    let read_ok = read_import(path, records_imported, true, 0, err)
    t |> success(read_ok, "read_import OK; err={err}")

    var c_parsed <- exact_buckets(records_parsed, 0)
    var c_imported <- exact_buckets(records_imported, 0)
    sort_clusters(c_parsed)
    sort_clusters(c_imported)
    t |> equal(length(c_parsed), length(c_imported))
    for (cc, ci in c_parsed, c_imported) {
        t |> equal(cc.canonical_sample, ci.canonical_sample)
        t |> equal(length(cc.members), length(ci.members))
    }

    var p_parsed <- fuzzy_pairs(records_parsed, 0.7f, 0)
    var p_imported <- fuzzy_pairs(records_imported, 0.7f, 0)
    sort_pairs(p_parsed)
    sort_pairs(p_imported)
    t |> equal(length(p_parsed), length(p_imported))
    for (pc, pi in p_parsed, p_imported) {
        let diff = fabs_f(pc.similarity - pi.similarity)
        t |> success(diff < 1.0e-5f, "similarity matches: {pc.similarity} vs {pi.similarity}")
        t |> equal(pc.canonical_a, pi.canonical_a)
        t |> equal(pc.canonical_b, pi.canonical_b)
    }
    remove(path)
}


[test]
def test_candidate_filter_drops_pure_corpus_clusters(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("a", "FN A B C D E F G H ENDFN", 1, false),
        <- make_record("b", "FN A B C D E F G H ENDFN", 2, false),
        <- make_record("c", "FN A B C D E F G H ENDFN", 3, false)]

    let legacy_clusters <- exact_buckets(records, 0)
    t |> equal(length(legacy_clusters), 1)

    let filtered <- exact_buckets(records, 0, true)
    t |> equal(length(filtered), 0)
}


[test]
def test_candidate_filter_keeps_mixed_clusters(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("a", "FN A B C D E F G H ENDFN", 1, false),
        <- make_record("b", "FN A B C D E F G H ENDFN", 2, false),
        <- make_record("c", "FN A B C D E F G H ENDFN", 3, false)]
    records[1].is_candidate = true

    let clusters <- exact_buckets(records, 0, true)
    t |> equal(length(clusters), 1)
    t |> equal(length(clusters[0].members), 3)
}


[test]
def test_candidate_filter_intra_candidate_pair(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("a", "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN", 1, true),
        <- make_record("b", "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT LIT ENDBLK ENDFN", 2, true)]
    records[0].is_candidate = true
    records[1].is_candidate = true

    let pairs <- fuzzy_pairs(records, 0.7f, 0, true)
    t |> equal(length(pairs), 1)
    t |> equal(pairs[0].a.name, "a")
    t |> equal(pairs[0].b.name, "b")
}


[test]
def test_candidate_filter_fuzzy_skips_pure_corpus(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("x", "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN", 1, true),
        <- make_record("y", "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT LIT ENDBLK ENDFN", 2, true),
        <- make_record("z", "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT LIT LIT ENDBLK ENDFN", 3, true),
        <- make_record("cand", "FN ARG <var_0> TYP BODY BLK STMT FOR FOR_VAR <var_1> CALL:range LIT FOR_BODY BLK ENDBLK ENDBLK ENDFN", 4, true)]
    records[3].is_candidate = true

    let legacy <- fuzzy_pairs(records, 0.7f, 0)
    t |> success(!empty(legacy), "legacy mode: at least one corpus pair: got {length(legacy)}")

    let filtered <- fuzzy_pairs(records, 0.7f, 0, true)
    t |> equal(length(filtered), 0)
}


[test]
def test_baseline_load_canonical_set(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    let records : array<FuncRecord> <- [
        <- make_record("a", "FN AAA ENDFN", 1, false),
        <- make_record("b", "FN BBB ENDFN", 2, false),
        <- make_record("c", "FN AAA ENDFN", 3, false)]
    write_export(records, path)

    var canonicals : table<string; void?>
    var members : table<string; void?>
    var err : string
    let ok = load_baseline_canonicals(path, canonicals, members, err)
    t |> success(ok, "load_baseline_canonicals returned true; err={err}")
    t |> equal(length(canonicals), 2)
    t |> success(key_exists(canonicals, "FN AAA ENDFN"), "AAA in set")
    t |> success(key_exists(canonicals, "FN BBB ENDFN"), "BBB in set")
    t |> equal(length(members), 3)
    t |> success(key_exists(members, "test.das:1:a"), "a member-id present")
    t |> success(key_exists(members, "test.das:2:b"), "b member-id present")
    t |> success(key_exists(members, "test.das:3:c"), "c member-id present (the growth case)")
    remove(path)
}


[test]
def test_baseline_load_rejects_bad_schema(t : T?) {
    let path = make_temp_export_path(t)
    if (empty(path)) return
    var bad = ExportEnvelope()
    bad.schema_version = 99
    let body = sprint_json(bad, false)
    fopen(path, "wb") $(fw) {
        if (fw != null) {
            fw |> fprint(body)
        }
    }
    var canonicals : table<string; void?>
    var members : table<string; void?>
    var err : string
    let ok = load_baseline_canonicals(path, canonicals, members, err)
    t |> success(!ok, "load_baseline_canonicals rejects bad schema_version")
    t |> success(find(err, "schema_version") >= 0, "error mentions schema_version: {err}")
    remove(path)
}


[test]
def test_drop_records_under_files(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("a", "FN A ENDFN", 1, false),
        <- make_record("b", "FN B ENDFN", 2, false),
        <- make_record("c", "FN C ENDFN", 3, false)]
    records[0].ref.file = "src/keep.das"
    records[1].ref.file = "src/drop.das"
    records[2].ref.file = "src/drop.das"

    var against : table<string; void?>
    against |> insert("src/drop.das", null)

    let dropped = drop_records_under_files(records, against)
    t |> equal(dropped, 2)
    t |> equal(length(records), 1)
    t |> equal(records[0].ref.name, "a")
    t |> equal(records[0].ref.file, "src/keep.das")
}


[test]
def test_collect_candidate_refs(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("a", "FN A ENDFN", 1, false),
        <- make_record("b", "FN B ENDFN", 2, false),
        <- make_record("c", "FN C ENDFN", 3, false)]
    records[0].is_candidate = true
    records[2].is_candidate = true

    let refs <- collect_candidate_refs(records)
    t |> equal(length(refs), 2)
    t |> equal(refs[0].name, "a")
    t |> equal(refs[1].name, "c")
}


[test]
def test_per_candidate_rollup_shape(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("cand", "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN", 1, true),
        <- make_record("exact_sib", "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN", 2, true),
        <- make_record("fuzzy_partner", "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT LIT ENDBLK ENDFN", 3, true)]
    records[0].is_candidate = true

    var clusters <- exact_buckets(records, 0, true)
    sort_clusters(clusters)
    var pairs <- fuzzy_pairs(records, 0.6f, 0, true)
    sort_pairs(pairs)

    let rep = Report(exact_clusters <- clusters, fuzzy_pairs <- pairs)

    let cand_refs <- collect_candidate_refs(records)
    let pcr = build_per_candidate_report(rep, cand_refs, 5)

    t |> equal(length(pcr.candidates), 1)
    t |> equal(pcr.candidates[0].name, "cand")
    t |> equal(length(pcr.candidates[0].exact_matches), 1)
    t |> equal(pcr.candidates[0].exact_matches[0].name, "exact_sib")
    t |> equal(pcr.candidates[0].exact_matches[0].similarity, 1.0f)
    t |> equal(length(pcr.candidates[0].fuzzy_matches), 1)
    t |> equal(pcr.candidates[0].fuzzy_matches[0].name, "fuzzy_partner")
    t |> success(pcr.candidates[0].fuzzy_matches[0].similarity > 0.6f
        && pcr.candidates[0].fuzzy_matches[0].similarity < 1.0f,
        "fuzzy similarity in (0.6, 1.0): {pcr.candidates[0].fuzzy_matches[0].similarity}")
}


[test]
def test_pattern_dispatch_three_identical_stmts(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT CALL:run <var_0> LIT ADDR STMT CALL:run <var_0> LIT ADDR STMT CALL:run <var_0> LIT ADDR ENDBLK ENDFN"
    let hit = classify("", canon)
    t |> equal(hit.name, "dispatch")
    t |> equal(hit.note, "3x repeated")
}


[test]
def test_pattern_dispatch_two_identical_stmts(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT CALL:run <var_0> LIT ADDR STMT CALL:run <var_0> LIT ADDR ENDBLK ENDFN"
    let hit = classify("", canon)
    t |> equal(hit.name, "dispatch")
    t |> equal(hit.note, "2x repeated")
}


[test]
def test_pattern_single_stmt_not_dispatch(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT CALL:run <var_0> LIT ADDR ENDBLK ENDFN"
    let hit = classify("", canon)
    t |> equal(hit.name, "")
}


[test]
def test_pattern_empty_body_not_dispatch(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK ENDBLK ENDFN"
    let hit = classify("", canon)
    t |> equal(hit.name, "")
}


[test]
def test_pattern_mixed_stmts_not_dispatch(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT CALL:run <var_0> LIT ADDR STMT CALL:bench <var_0> LIT ADDR ENDBLK ENDFN"
    let hit = classify("", canon)
    t |> equal(hit.name, "")
}


[test]
def test_pattern_nested_block_not_dispatch(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT IF <var_0> THEN BLK STMT CALL:foo <var_0> ENDBLK ELSE BLK STMT CALL:foo <var_0> ENDBLK ENDBLK ENDFN"
    let hit = classify("", canon)
    t |> equal(hit.name, "")
}


[test]
def test_pattern_real_code_not_dispatch(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT LET LET_VAR <var_1> STMT RET OP2:+ <var_1> LIT ENDBLK ENDFN"
    let hit = classify("", canon)
    t |> equal(hit.name, "")
}


[test]
def test_pattern_visitor_visit_method(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT LET LET_VAR <var_1> STMT RET <var_1> ENDBLK ENDFN"
    let hit = classify("CppAot`visitExprCopy", canon)
    t |> equal(hit.name, "visitor")
    t |> equal(hit.note, "visitExprCopy")
}


[test]
def test_pattern_visitor_previsit_method(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK ENDBLK ENDFN"
    let hit = classify("PrintVisitor`preVisitExprBlock", canon)
    t |> equal(hit.name, "visitor")
}


[test]
def test_pattern_visitor_before_after_hooks(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK ENDBLK ENDFN"
    let hit = classify("RstComment`beforeStructure", canon)
    t |> equal(hit.name, "visitor")
    let hit2 = classify("RstComment`afterEnumerationEntries", canon)
    t |> equal(hit2.name, "visitor")
}


[test]
def test_pattern_non_visitor_keeps(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT LET LET_VAR <var_1> ENDBLK ENDFN"
    let hit = classify("SqlMacro`apply", canon)
    t |> equal(hit.name, "")
}


[test]
def test_pattern_free_function_not_visitor(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT RET <var_0> ENDBLK ENDFN"
    let hit = classify("visitNode", canon)
    t |> equal(hit.name, "")
}


[test]
def test_pattern_visitor_wins_over_emit(t : T?) {
    let canon = "FN ARG <var_0> TYP ARG <var_1> TYP BODY BLK STMT CALL:write P2R <var_0> .FLD LIT STMT RET <var_1> ENDBLK ENDFN"
    let hit = classify("CppAot`visitExprCopy", canon)
    t |> equal(hit.name, "visitor")
}


[test]
def test_pattern_emit_when_no_visitor_name(t : T?) {
    let canon = "FN ARG <var_0> TYP ARG <var_1> TYP BODY BLK STMT CALL:write P2R <var_0> .FLD LIT STMT RET <var_1> ENDBLK ENDFN"
    let hit = classify("emit_close_paren", canon)
    t |> equal(hit.name, "emit")
}


[test]
def test_pattern_no_test_wrapper_bucket(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT CALL:run <var_0> LIT ASC MK_STR ADDR ADDR ENDBLK ENDFN"
    let hit = classify("test_something", canon)
    t |> equal(hit.name, "", "the post-infer `run` + ADDR shape is no pattern - there is no test_wrapper bucket")
}


[test]
def test_pattern_single_run_with_body_is_signal(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT CALL:run <var_0> LIT MK_BLK BLK BLK_ARG <var_1> STMT CALL:equal <var_1> LIT LIT ENDBLK ENDBLK ENDFN"
    let hit = classify("test_something", canon)
    t |> equal(hit.name, "", "a test wrapper carries its lambda body inline, so it is real signal, not a pattern")
}


[test]
def test_split_top_level_stmts_skips_nested(t : T?) {
    let canon = "FN ARG <var_0> TYP BODY BLK STMT FOR FOR_VAR <var_1> <var_0> FOR_BODY BLK STMT CALL:foo <var_1> STMT CALL:bar <var_1> ENDBLK ENDBLK ENDFN"
    let stmts <- split_top_level_stmts(canon)
    t |> equal(length(stmts), 1)
}


[test]
def test_apply_pattern_filter_drops_dispatch(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("real",
            "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN", 1, false),
        <- make_record("dispatcher",
            "FN ARG <var_0> TYP BODY BLK STMT CALL:run <var_0> LIT ADDR STMT CALL:run <var_0> LIT ADDR ENDBLK ENDFN", 2, false)]
    let kept : table<string; void?>
    var counts <- apply_pattern_filter(records, kept, false, false)
    t |> equal(length(records), 1)
    t |> equal(records[0].ref.name, "real")
    t |> equal(length(counts), 1)
    t |> success(key_exists(counts, "dispatch"), "dispatch counted")
    t |> equal(counts["dispatch"], 1)
}


[test]
def test_apply_pattern_filter_keep_all_passthrough(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("real",
            "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN", 1, false),
        <- make_record("dispatcher",
            "FN ARG <var_0> TYP BODY BLK STMT CALL:run <var_0> LIT ADDR STMT CALL:run <var_0> LIT ADDR ENDBLK ENDFN", 2, false)]
    let kept : table<string; void?>
    let counts <- apply_pattern_filter(records, kept, true, false)
    t |> equal(length(records), 2)
    t |> equal(length(counts), 0)
}


[test]
def test_apply_pattern_filter_explicit_keep_dispatch(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("real",
            "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN", 1, false),
        <- make_record("dispatcher",
            "FN ARG <var_0> TYP BODY BLK STMT CALL:run <var_0> LIT ADDR STMT CALL:run <var_0> LIT ADDR ENDBLK ENDFN", 2, false)]
    var kept : table<string; void?>
    kept |> insert("dispatch", null)
    let counts <- apply_pattern_filter(records, kept, false, false)
    t |> equal(length(records), 2)
    t |> equal(length(counts), 0)
}


[test]
def test_apply_pattern_filter_preserves_candidate_flag(t : T?) {
    var records : array<FuncRecord> <- [
        <- make_record("real",
            "FN ARG <var_0> TYP BODY BLK STMT RET OP2:+ <var_0> LIT ENDBLK ENDFN", 1, false)]
    records[0].is_candidate = true
    let kept : table<string; void?>
    let counts <- apply_pattern_filter(records, kept, false, false)
    t |> equal(length(records), 1)
    t |> success(records[0].is_candidate, "is_candidate flag preserved through filter")
}


let GENERICS_FIXTURE = "utils/detect-dupe/fixture/generics_cases.das"


[test]
def test_generics_mono_baseline_visible(t : T?) {
    var records : array<FuncRecord>
    let ok = parse_fixture(GENERICS_FIXTURE, records, false, false)
    t |> success(ok, "generics_cases.das parsed")
    if (!ok) return
    let canon = find_canonical_by_name(records, "case_mono_baseline")
    t |> success(!empty(canon), "non-generic baseline visible")
}


[test]
def test_generics_called_visible(t : T?) {
    var records : array<FuncRecord>
    let ok = parse_fixture(GENERICS_FIXTURE, records, false, false)
    t |> success(ok, "generics_cases.das parsed")
    if (!ok) return
    let canon = find_canonical_by_name(records, "case_generic_called")
    t |> success(!empty(canon),
        "called generic must be findable by source-level name")
    var saw_mangled = false
    for (rec in records) {
        if (find(rec.ref.name, "case_generic_called") >= 0 && rec.ref.name != "case_generic_called") {
            saw_mangled = true
        }
    }
    t |> success(!saw_mangled,
        "no mangled `name`<hash> reified copies should appear alongside the template")
}


[test]
def test_generics_uncalled_visible(t : T?) {
    var records : array<FuncRecord>
    let ok = parse_fixture(GENERICS_FIXTURE, records, false, false)
    t |> success(ok, "generics_cases.das parsed")
    if (!ok) return
    let canon = find_canonical_by_name(records, "case_generic_uncalled")
    t |> success(!empty(canon),
        "uncalled generic must be visible — for_each_generic must be walked alongside for_each_function")
}


[test]
def test_generics_called_and_uncalled_canonicalize_equal(t : T?) {
    var records : array<FuncRecord>
    let ok = parse_fixture(GENERICS_FIXTURE, records, false, false)
    t |> success(ok, "generics_cases.das parsed")
    if (!ok) return
    let a = find_canonical_required(t, records, "case_generic_called")
    let b = find_canonical_required(t, records, "case_generic_uncalled")
    t |> equal(a, b)
}


let BROKEN_FIXTURE = "utils/detect-dupe/fixture/_fixture_broken_cases.das"
let UNRESOLVED_FIXTURE = "utils/detect-dupe/fixture/_fixture_unresolved_cases.das"
let READER_FIXTURE = "utils/detect-dupe/fixture/_fixture_reader_cases.das"

[test]
def test_parse_failure_still_collects(t : T?) {
    var records : array<FuncRecord>
    let ok = parse_fixture(BROKEN_FIXTURE, records, false, false)
    t |> success(!ok, "broken_cases.das must fail to parse")
    let canon = find_canonical_by_name(records, "case_broken_survivor")
    t |> success(!empty(canon),
        "a function parsed before the syntax error is still collected")
}


[test]
def test_unresolved_lookups_are_not_failures(t : T?) {
    var records : array<FuncRecord>
    let ok = parse_fixture(UNRESOLVED_FIXTURE, records, false, false)
    t |> success(ok, "a require, annotation and parent class the parse cannot resolve do not fail the file")
    t |> success(!empty(find_canonical_by_name(records, "case_unresolved_annotated")),
        "the annotated function is collected")
    t |> success(!empty(find_canonical_by_name(records, "CaseUnresolvedParent`visit")),
        "the method of a class with an unknown parent is collected")
}


[test]
def test_unknown_reader_macro_does_not_end_the_parse(t : T?) {
    var records : array<FuncRecord>
    let ok = parse_fixture(READER_FIXTURE, records, false, false)
    t |> success(ok, "a `%name~ ... %%` whose reader no module provides reads to its `%%` and is not a failure")
    t |> success(!empty(find_canonical_by_name(records, "case_before_reader")), "the function before it is collected")
    t |> success(!empty(find_canonical_by_name(records, "case_after_reader")), "the function after it is collected")
}


def export_daslib_corpus(t : T?; workers : string; out : string) : string {
    let argv <- [get_das_exe(), "-dasroot", get_das_root(), "{get_das_root()}/utils/detect-dupe/main.das", "--",
                 "-p", "{get_das_root()}/daslib", "-j", workers, "--export-functions", out]
    var output : string
    let rc = run_and_capture(argv, output)
    t |> equal(rc, 0, "daslib export with -j {workers} exits 0:\n{output}")
    return output
}


// The real daslib as a corpus, both sides of the 128-file fan-out: the parallel run and the
// sequential run agree byte for byte, nothing fails to parse, and the record count is a body of
// code rather than a fixture.
[test]
def test_export_daslib_parallel_equals_sequential(t : T?) {
    set_env_variable("DAS_LOG_LEVEL", "info")
    defer() {
        set_env_variable("DAS_LOG_LEVEL", "")
    }
    let out_par = make_temp_export_path(t)
    let out_seq = make_temp_export_path(t)
    if (empty(out_par) || empty(out_seq)) return
    let log_par = export_daslib_corpus(t, "0", out_par)
    let log_seq = export_daslib_corpus(t, "1", out_seq)
    t |> success(find(log_par, "parallel export:") >= 0, "daslib is past the fan-out threshold, so -j 0 fans out:\n{log_par}")
    t |> success(find(log_seq, "0 parse failure(s)") >= 0, "every daslib file parses:\n{log_seq}")
    t |> equal(fread(out_par), fread(out_seq), "parallel and sequential exports are byte-identical")
    var records : array<FuncRecord>
    var err = ""
    let ok = read_import(out_seq, records, false, 0, err, true)
    t |> success(ok, "the export file is readable: {err}")
    t |> success(length(records) > 4000, "daslib yields thousands of records, got {length(records)}")
    remove(out_par)
    remove(out_seq)
}


// --export-functions over a file that fails to parse exits 0 and writes the survivors - the
// sharding scripts read the file, not a partial-run exit code
[test]
def test_export_cli_survives_parse_failure(t : T?) {
    let out = make_temp_export_path(t)
    if (empty(out)) return
    let argv <- [get_das_exe(), "-dasroot", get_das_root(), "{get_das_root()}/utils/detect-dupe/main.das", "--",
                 "-p", "{get_das_root()}/{BROKEN_FIXTURE}", "--export-functions", out]
    var output : string
    let rc = run_and_capture(argv, output)
    t |> equal(rc, 0, "an export over a broken file exits 0 (the run is not partial-failed):\n{output}")
    var records : array<FuncRecord>
    var err = ""
    let ok = read_import(out, records, false, 0, err, true)
    t |> success(ok, "the export file is readable: {err}")
    t |> success(!empty(find_canonical_by_name(records, "case_broken_survivor")),
        "the function parsed before the syntax error is in the export")
    remove(out)
}

[test]
def test_worker_count_auto_is_physical_cores(t : T?) {
    t |> equal(compute_worker_count(1000, 0), min(get_total_hw_cores(), MAX_AUTO_WORKERS),
        "0 = auto means the box's cores under the ceiling; get_total_hw_threads is the jobque's own lane count")
    t |> equal(compute_worker_count(1000, MAX_AUTO_WORKERS * 4), MAX_AUTO_WORKERS * 4,
        "an explicit count is not capped")
}
