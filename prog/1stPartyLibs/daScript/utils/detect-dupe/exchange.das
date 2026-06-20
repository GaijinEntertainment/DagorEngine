options gen2
options rtti

require strings
require daslib/json_boost
require daslib/fio
require cluster
require minhash
require canonical


// Slim per-function record for the on-disk export. Drops the MinHash
// signature (recomputed on import — deterministic and cheap) and
// `cluster_id` (assigned by `exact_buckets`, not part of the input
// shape).
struct public ExportedFunction {
    name : string
    file : string
    line : int
    end_line : int
    is_lambda : bool
    canonical : string
}


// `schema_version` lets a future schema bump reject older readers
// loudly instead of silently mis-parsing. Bump on any breaking change
// to ExportedFunction's field set. Default to 0 (an invalid version)
// so JSON missing the key is rejected by the explicit version check
// in `read_import` rather than silently treated as v1.
struct public ExportEnvelope {
    schema_version : int = 0
    functions : array<ExportedFunction>
}


// v2 (2026-04-27): added `end_line` to ExportedFunction so consumers
// can extract the full function body from disk by line range. Required
// by `utils/find-dupe/`. v1 readers reject v2 files (and vice
// versa) via the explicit version check in `read_import` /
// `load_baseline_canonicals`.
let public EXCHANGE_SCHEMA_VERSION = 2


// Single source of truth for "given a canonical string, finish
// populating the FuncRecord" — used both by the compile path
// (`collect_from_program`) and by the import path. Keeps the two
// in sync if tokenization or signing ever changes.
//
// Always (re)sets `sig`: when `want_sigs` is false (or token_count
// is below the floor), `sig` is cleared so a reused record can't
// carry a stale signature into fuzzy_pairs.
def public derive_sig_and_count(var rec : FuncRecord; want_sigs : bool; min_tokens : int) {
    let tokens <- tokenize_canonical(rec.canonical)
    rec.token_count = length(tokens)
    if (want_sigs && rec.token_count >= min_tokens) {
        rec.sig <- minhash_signature(tokens)
    } else {
        rec.sig |> clear()
    }
}


def public write_export(records : array<FuncRecord>; path : string) : bool {
    var env = ExportEnvelope(schema_version = EXCHANGE_SCHEMA_VERSION)
    env.functions |> reserve(length(records))
    for (rec in records) {
        env.functions |> emplace(ExportedFunction(
            name = rec.ref.name,
            file = rec.ref.file,
            line = rec.ref.line,
            end_line = rec.ref.end_line,
            is_lambda = rec.ref.is_lambda,
            canonical = rec.canonical))
    }
    let body = sprint_json(env, true)
    var ok = false
    fopen(path, "wb") $(fw) {
        if (fw == null) return
        ok = true
        fw |> fprint(body)
    }
    return ok
}


// Merge N shard ExportEnvelope JSON files (each produced by `write_export`)
// into a single envelope written to `out_path`. Functions are concatenated
// in the order shards are listed — caller controls determinism by listing
// shards in stable chunk-index order. Used by the parallel `--workers`
// path: each child writes its own shard, the parent merges in order.
//
// Strict: any unreadable shard, parse failure, or schema_version mismatch
// fails the whole merge (no partial output). Same gate `read_import` uses.
def public merge_envelope_files(shard_paths : array<string>; out_path : string;
                                var error : string&) : bool {
    var merged = ExportEnvelope()
    merged.schema_version = EXCHANGE_SCHEMA_VERSION
    for (p in shard_paths) {
        let text = fread(p)
        if (empty(text)) {
            error := "could not read shard (empty or missing): {p}"
            return false
        }
        var env = ExportEnvelope()
        if (!sscan_json(text, env)) {
            error := "could not parse shard JSON: {p}"
            return false
        }
        if (env.schema_version != EXCHANGE_SCHEMA_VERSION) {
            error := "shard schema_version {env.schema_version} (expected {EXCHANGE_SCHEMA_VERSION}): {p}"
            return false
        }
        merged.functions |> reserve(length(merged.functions) + length(env.functions))
        for (ef in env.functions) {
            merged.functions |> emplace(ef)
        }
    }
    let body = sprint_json(merged, true)
    var ok = false
    fopen(out_path, "wb") $(fw) {
        if (fw == null) return
        ok = true
        fw |> fprint(body)
    }
    if (!ok) {
        error := "could not write merged envelope to {out_path}"
    }
    return ok
}


// An imported function record is "well-formed" when it has a non-empty
// canonical (the only field that affects clustering), a non-empty name
// and file (so reports are interpretable), and a sane line range.
// Empty canonicals are the dangerous case: with `--min-tokens 0` they
// would all collide in a single fake exact-cluster.
//
// `line` must be a real 1-based line, and `end_line` must cover at
// least the start line. v1 envelopes (no `end_line` field) are
// rejected upstream by the `schema_version` check, so requiring v2's
// `end_line` here doesn't break legacy data — a malformed v2 record
// missing `end_line` defaults to 0 and gets rejected by this check
// instead of silently degrading downstream consumers (e.g.
// `find-dupe` would fall back to single-line source extraction).
def is_well_formed(ef : ExportedFunction) : bool {
    return (!empty(ef.canonical) && !empty(ef.name) && !empty(ef.file)
        && ef.line >= 1 && ef.end_line >= ef.line)
}


// Slash-direction normalization for file paths. Lives here (rather than
// in pipeline.das) because exchange is below pipeline in the dep graph
// and `member_id` needs it. pipeline.das re-uses this via `require
// exchange`, so callers see one canonical helper.
def public normalize_path(p : string) : string {
    // `fio::normalize` resolves `..`/`.` and uses the platform separator
    // (backslash on Windows, forward slash on POSIX). The trailing
    // `replace` lands on a canonical forward-slash form so a corpus
    // exported on one OS matches candidates compiled on another.
    return normalize(p) |> replace("\\", "/")
}


// Stable member identity used to compare a freshly-extracted record
// against a baseline. Same shape as cluster.das `member_key`, copied
// here to avoid a require cycle (exchange already needs to know about
// records but not about clustering helpers). Path is normalized so
// Windows runs (mixed `\` and `/` from `fileInfo.name`) match Unix
// baselines.
def member_id(file : string; line : int; name : string) : string {
    return "{normalize_path(file)}:{line}:{name}"
}


// Read an exported envelope and project it down to two sets:
//   * `canonicals`  — set of canonical texts (used by --baseline-strict
//     to drop clusters whose canonical already existed in baseline).
//   * `members`     — set of `file:line:name` identities (used by default
//     baseline mode to tag records that DIDN'T exist in baseline as
//     candidates). Member-level keying catches "growth" — a new copy of
//     an already-tracked canonical added in a new location — which a
//     pure canonical-set diff would miss (the canonical is in baseline,
//     so the new member would never be flagged).
//
// Trade-off of `file:line:name`: an unrelated edit that shifts line
// numbers will look like "new members" and surface those clusters in
// the report. Acceptable for CI gating — re-checking touched code is
// the right default; the user can ignore false positives the same way
// they would for any line-based diff tool.
def public load_baseline_canonicals(path : string;
                                    var canonicals : table<string; void?>;
                                    var members : table<string; void?>;
                                    var error : string&) : bool {
    let text = fread(path)
    if (empty(text)) {
        error := "could not read baseline file (empty or missing): {path}"
        return false
    }
    var env = ExportEnvelope()
    if (!sscan_json(text, env)) {
        error := "could not parse baseline JSON: {path}"
        return false
    }
    if (env.schema_version != EXCHANGE_SCHEMA_VERSION) {
        error := "unsupported baseline schema_version {env.schema_version} (expected {EXCHANGE_SCHEMA_VERSION}): {path}"
        return false
    }
    for (ef in env.functions) {
        if (empty(ef.canonical)) continue
        if (!key_exists(canonicals, ef.canonical)) {
            canonicals |> insert(ef.canonical, null)
        }
        let mid = member_id(ef.file, ef.line, ef.name)
        if (!key_exists(members, mid)) {
            members |> insert(mid, null)
        }
    }
    return true
}


// `quiet=true` suppresses the "skipped N malformed records" warning print —
// required when called from the MCP server, whose stdio transport reserves
// stdout for JSON-RPC. Default false preserves CLI behavior.
def public read_import(path : string; var records : array<FuncRecord>;
                       want_sigs : bool; min_tokens : int;
                       var error : string&;
                       quiet : bool = false) : bool {
    let text = fread(path)
    if (empty(text)) {
        error := "could not read file (empty or missing): {path}"
        return false
    }
    var env = ExportEnvelope()
    if (!sscan_json(text, env)) {
        error := "could not parse JSON: {path}"
        return false
    }
    if (env.schema_version != EXCHANGE_SCHEMA_VERSION) {
        error := "unsupported schema_version {env.schema_version} (expected {EXCHANGE_SCHEMA_VERSION}): {path}"
        return false
    }
    records |> reserve(length(records) + length(env.functions))
    var skipped = 0
    for (ef in env.functions) {
        if (!is_well_formed(ef)) {
            skipped++
            continue
        }
        var rec = FuncRecord()
        rec.ref.name := ef.name
        rec.ref.file := ef.file
        rec.ref.line = ef.line
        rec.ref.end_line = ef.end_line
        rec.ref.is_lambda = ef.is_lambda
        rec.canonical := ef.canonical
        derive_sig_and_count(rec, want_sigs, min_tokens)
        records |> emplace(rec)
    }
    if (skipped > 0 && !quiet) {
        to_log(LOG_WARNING, "warning: skipped {skipped} malformed function record(s) during import\n")
    }
    return true
}
