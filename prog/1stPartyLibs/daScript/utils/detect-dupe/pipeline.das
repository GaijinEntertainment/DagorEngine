options gen2

require daslib/ast
require daslib/ast_boost
require daslib/fio
require daslib/strings_boost
require math
require strings
require canonical
require cluster
require exchange
require patterns


// `normalize_path` lives in exchange.das — pulled in via `require exchange`
// above so callers in this file (and downstream of `require pipeline`) keep
// working unchanged.


// ── filesystem scanning ──────────────────────────────────────────────
//
// Walks a directory tree (or accepts a single file) and collects `.das`
// paths to compile. Lives here (rather than in `main.das`) so MCP tools
// — which can't shell out to the CLI — can build a corpus end-to-end
// from inside the same process.

// Read newline-delimited paths from `f` (either `fstdin()` for the
// `--paths-stdin`/`--against-from-stdin` flows, or a freshly-opened file
// for `--paths-from`). Skips blank lines and `#`-prefixed comments so
// callers can hand-roll a path list with annotations. Lives here (rather
// than main.das) so the unit tests can exercise it directly without
// shelling out to a subprocess.
def public read_path_lines(f : FILE const?; var paths : array<string>) {
    if (f == null) return
    while (!feof(f)) {
        let line = fgets(f) |> strip
        if (empty(line) || line |> starts_with("#")) continue
        paths |> push(line)
    }
}


// Read newline-delimited paths from a file. Wraps `read_path_lines` and
// returns false (with `error` populated) if the file can't be opened, so
// the caller can emit a usage error rather than silently producing an
// empty seed list.
def public read_path_lines_from_file(path : string; var paths : array<string>;
                                     var error : string&) : bool {
    var ok = false
    fopen(path, "rb") $(f) {
        if (f == null) return
        ok = true
        read_path_lines(f, paths)
    }
    if (!ok) {
        error := "could not open paths-from file: {path}"
    }
    return ok
}


// `daslib/debugger.das` and `daslib/profiler.das` install thread-local
// agents at compile time; the second install in the same process throws
// and aborts the scan. Match by daslib/ path suffix, not bare basename
// — a project's own `myapp/debugger.das` should not be silently skipped.
def public is_daslib_skip(path : string; basename : string) : bool {
    let normalized = normalize_path(path)
    let prefix = "daslib/{basename}"
    return normalized == prefix || normalized |> ends_with("/{prefix}")
}


def public is_skip_file(path : string; basename : string) : bool {
    if (basename == "builtin.das") return true
    return (is_daslib_skip(path, "debugger.das")
        || is_daslib_skip(path, "profiler.das"))
}


def public is_skip_dir(name : string) : bool {
    return name |> starts_with(".")
}


// `expect` directives mark intentionally-broken test files (the test
// framework runs the compiler and checks the produced error). Skipping
// avoids polluting the corpus with junk records from files that aren't
// supposed to compile.
def public has_expect_directive(file : string) : bool {
    let text = fread(file)
    if (empty(text)) return false
    let lines <- text |> split("\n")
    for (line in lines) {
        let trimmed = line |> strip
        if (trimmed |> starts_with("expect ")) return true
    }
    return false
}


def public scan_das_files(path : string; var files : array<string>; var cache : table<string; void?>) {
    let st = stat(path)
    if (!st.is_valid) return
    if (st.is_reg) {
        if (path |> ends_with(".das")) {
            // Normalize before keying/pushing so seeds spelled differently
            // (`a/b/c.das` vs `a\b\c.das` on Windows, or with `./` segments)
            // don't sneak duplicate entries past the cache.
            let np = normalize_path(path)
            if (!cache |> key_exists(np)) {
                let fname = base_name(np)
                if (!is_skip_file(np, fname)) {
                    cache |> insert(np, null)
                    files |> push(np)
                }
            }
        }
        return
    }
    if (!st.is_dir) return
    fio::dir(path) $(name) {
        if (name == "." || name == ".." || name |> starts_with("_") || is_skip_dir(name)) return
        let full = path_join(path, name)
        let fst = stat(full)
        if (!fst.is_valid) return
        if (fst.is_dir) {
            scan_das_files(full, files, cache)
        } elif (fst.is_reg && name |> ends_with(".das") && !is_skip_file(full, name)) {
            let np = normalize_path(full)
            if (!cache |> key_exists(np)) {
                cache |> insert(np, null)
                files |> push(np)
            }
        }
    }
}


// Process one function (template OR reified) and append a FuncRecord
// if it survives the filters. Shared by the `for_each_generic` and
// `for_each_function` walks below — they only differ in which list they
// iterate, not in how each entry is filtered/canonicalized.
//
// The per-source-line `seen_loc` dedup serves two purposes:
//   1. Generic templates and their reified instances share the same
//      `(file, line)`. Walking generics FIRST means the unmangled
//      template name wins; the reified copies (stored under the
//      mangled `` `name`<hash> `` form) are then skipped here.
//   2. A generic reified for N types appears N times in
//      `module->functions`. Only the first survives; the others are
//      pure noise (perfect duplicates of themselves).
def private collect_one_function(var func : FunctionPtr;
                                 source_norm : string;
                                 var records : array<FuncRecord>;
                                 var seen_loc : table<string; void?>;
                                 lambdas_only : bool;
                                 want_sigs : bool;
                                 min_tokens : int) {
    if (func.flags.builtIn) return
    // Lambdas are emitted as `generated` synthesized functions
    // (`__lambda_function_at_line_xxx`). Default mode skips them via
    // the generated filter (the dispatcher already references them
    // via ADDR). Lambda-only mode lets them through and drops the
    // top-level functions instead.
    if (lambdas_only) {
        if (!func.flags._lambda) return
    } else {
        if (func.flags.generated) return
    }
    let fn_file = (func.at.fileInfo != null) ? string(func.at.fileInfo.name) : ""
    // Normalize slash direction — Windows runners can mix `\` and
    // `/`, and a literal mismatch silently drops every function
    // from a file that compiled fine.
    if (normalize_path(fn_file) != source_norm) return
    let loc = "{fn_file}:{int(func.at.line)}"
    if (key_exists(seen_loc, loc)) return
    seen_loc |> insert(loc, null)
    let canon = canonicalize_function(func)
    if (empty(canon)) return
    var rec = FuncRecord()
    rec.ref.file = fn_file
    rec.ref.name = string(func.name)
    rec.ref.line = int(func.at.line)
    // `func.at` spans only the `def name(args)` declaration line — not
    // the body. The body's closing `}` lives in `func.body.at.last_line`.
    if (func.body != null) {
        rec.ref.end_line = int(func.body.at.last_line)
    } else {
        rec.ref.end_line = int(func.at.last_line)
    }
    rec.ref.is_lambda = func.flags._lambda
    rec.canonical := canon
    derive_sig_and_count(rec, want_sigs, min_tokens)
    records |> emplace(rec)
}


// Walk one compiled program and append a FuncRecord per user
// function (lambdas included; only builtIn / generated skipped).
//
// Generic-template functions (e.g. dastest's `equal<T>`) get reified
// into every caller's module, so without filtering we see hundreds of
// copies of the same library helpers. The file-source filter drops
// any function whose `at.fileInfo` points outside the file we just
// compiled, leaving only what was actually written in that file.
//
// Templates are walked BEFORE reified functions: an uncalled generic
// only exists in `module->generics` (so without this pass we lose it
// entirely), and a called generic exists in BOTH lists — visiting the
// template first lets the source-name version win the per-line dedup
// over the mangled `` `name`<hash> `` reification.
def public collect_from_program(program : ProgramPtr; source_file : string; var records : array<FuncRecord>; var seen_loc : table<string; void?>; lambdas_only : bool; want_sigs : bool; min_tokens : int) {
    let thisMod = get_this_module(program)
    let source_norm = normalize_path(source_file)
    // Generic templates can't be lambdas — skip the generics walk in
    // lambdas-only mode entirely.
    if (!lambdas_only) {
        for_each_generic(thisMod, "") $(func) {
            collect_one_function(func, source_norm, records, seen_loc,
                                 lambdas_only, want_sigs, min_tokens)
        }
    }
    for_each_function(thisMod, "") $(func) {
        collect_one_function(func, source_norm, records, seen_loc,
                             lambdas_only, want_sigs, min_tokens)
    }
}


// Drop records whose `file` matches any path in `against_files` (paths
// must already be normalized via `normalize_path`). Used after loading
// a corpus when --against / candidate-mode is active, to override "the
// old version of this file" with the freshly-compiled one. Returns the
// count of dropped records.
def public drop_records_under_files(var records : array<FuncRecord>;
                                    against_files : table<string; void?>) : int {
    var kept : array<FuncRecord>
    kept |> reserve(length(records))
    var dropped = 0
    for (r in records) {
        if (key_exists(against_files, normalize_path(r.ref.file))) {
            dropped++
            continue
        }
        kept |> push_clone(r)
    }
    records <- kept
    return dropped
}


// Walk records, classify each by `patterns/classify`, and drop any whose
// matched pattern is not in `kept`. Returns a per-pattern count of what
// was dropped so the caller can surface a summary line.
//
// `keep_all == true` is the legacy passthrough — no classification, no
// drops. Useful for `--keep all` and any future "pattern-skip is broken,
// give me everything" escape hatch.
//
// In-place: rebuilds `records` to keep matched-and-not-kept entries out.
// Uses push_clone to avoid worrying about non-copyable fields (`sig`).
def public apply_pattern_filter(var records : array<FuncRecord>;
                                kept : table<string; void?>;
                                keep_all : bool;
                                verbose : bool) : table<string; int> {
    var skip_counts : table<string; int>
    if (keep_all) return <- skip_counts
    var kept_records : array<FuncRecord>
    kept_records |> reserve(length(records))
    for (r in records) {
        let hit = classify(r.ref.name, r.canonical)
        if (empty(hit.name) || key_exists(kept, hit.name)) {
            kept_records |> push_clone(r)
            continue
        }
        if (verbose) {
            to_log(LOG_INFO, "  pattern-skip [{hit.name}]: {r.ref.file}:{r.ref.line} {r.ref.name} ({hit.note})\n")
        }
        if (key_exists(skip_counts, hit.name)) {
            skip_counts[hit.name] = skip_counts[hit.name] + 1
        } else {
            skip_counts |> insert(hit.name, 1)
        }
    }
    records <- kept_records
    return <- skip_counts
}


// Collect MemberRefs of all candidate records — used to feed the
// per-candidate report writer.
def public collect_candidate_refs(records : array<FuncRecord>) : array<MemberRef> {
    return <- [for (r in records); r.ref; where r.is_candidate]
}


// `quiet=true` suppresses the "FAIL {file}" stdout print — required when
// called from the MCP server (stdio transport reserves stdout for
// JSON-RPC). The boolean return value is the caller's signal anyway. CLI
// keeps the print for human visibility.
//
// `project` lets callers point at a `.das_project` for custom module
// resolution, mirroring other compilation-based MCP tools. Empty string
// preserves the original "no project" behavior.
def public compile_and_collect(file : string; var records : array<FuncRecord>;
                               var seen_loc : table<string; void?>;
                               verbose : bool; lambdas_only : bool;
                               want_sigs : bool; min_tokens : int;
                               quiet : bool = false;
                               project : string = "") : bool {
    var inscope access <- make_file_access(project)
    var ok_compile = true
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.ignore_shared_modules = true
            cop.export_all = true
            cop.version_2_syntax = true
            // Leave optimisations and infer-time folding ON: dastest macros
            // (e.g. unroll) need constant folding to compile, and folded-equal
            // expressions are exactly the kind of duplicate we want to catch.
            compile_file(file, access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    ok_compile = false
                    if (!quiet) {
                        if (verbose) {
                            to_log(LOG_ERROR, "FAIL {file}\n  {issues}\n")
                        } else {
                            to_log(LOG_ERROR, "FAIL {file}\n")
                        }
                    }
                    return
                }
                collect_from_program(program, file, records, seen_loc, lambdas_only, want_sigs, min_tokens)
            }
        }
    }
    return ok_compile
}
