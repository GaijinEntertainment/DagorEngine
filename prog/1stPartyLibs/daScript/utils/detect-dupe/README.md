# detect-dupe — cross-file similar-function detector

Walks a directory tree of `.das` files, normalises each user function into
an alpha-renamed token stream (identifiers, types, and literals all
collapsed), and reports:

- **Exact-clone clusters** — functions that hash to the same canonical
  text. Pure structural duplicates, modulo names/types/values.
- **Fuzzy near-duplicates** — pairs scored as `sqrt(jaccard × len_ratio)`
  on top of a 64-slot MinHash signature and a hard `len_ratio ≥ threshold`
  gate. The length gate suppresses MinHash false-positives on highly
  periodic boilerplate (otherwise a 4-statement and a 7-statement copy
  of the same `t |> run(...)` block both look 100% identical to MinHash).
  Note that the geometric mean admits Jaccard somewhat below `threshold`
  when lengths match closely — this is intentional and biases toward
  recall.

Useful for surfacing test-suite boilerplate that could be factored,
near-clones that drifted apart, or copy-pasted helpers that escaped
review.

## Usage

```sh
./bin/daslang utils/detect-dupe/main.das -- -p tests --json /tmp/dupes.json
./bin/daslang utils/detect-dupe/main.das -- -p tests/strings -t 0.85 -n 20
./bin/daslang utils/detect-dupe/main.das -- -p daslib --no-fuzzy --min-tokens 32
./bin/daslang utils/detect-dupe/main.das -- -p tests -L --min-tokens 16    # cluster dastest run() bodies
./bin/daslang utils/detect-dupe/main.das -- -p tests --keep all            # disable pattern filter (legacy view)
./bin/daslang utils/detect-dupe/main.das -- -p daslib -p tests -j 8 --export-functions corpus.json
                                                                            # parallel export across 8 child processes
git diff --name-only master | grep '\.das$' \
  | ./bin/daslang utils/detect-dupe/main.das -- --paths-stdin --export-functions pr.json
                                                                            # PR-scoped export
./bin/daslang utils/detect-dupe/main.das -- -?
```

`./bin/daslang -jit utils/detect-dupe/main.das -- …` works too — detect-dupe itself JITs cleanly. Net runtime improvement is modest because per-file cost is dominated by interpreter compilation of the scanned files.

Flags:

| Flag | Default | Meaning |
|---|---|---|
| `-p / --path` | required* | File or directory to scan; repeatable. `*` one of `-p`, `--paths-from`, `--paths-stdin` is required (or `--import-functions` / `--against`) |
| `--paths-from` | (off) | Read newline-delimited paths from a file (`#`-comments and blank lines skipped). Composes with `-p`. Each entry can be a file or directory; directories recurse |
| `--paths-stdin` | off | Read newline-delimited paths from stdin (`#`-comments and blank lines skipped). Composes with `-p`. Mutually exclusive with `--against-from-stdin` (one stdin reader per run) |
| `-j / --workers` | 0 (auto) | Worker count for parallel `--export-functions` runs. 0 = hardware threads. 1 = sequential. Files are sorted, split into N contiguous chunks, each compiled by a child detect-dupe process; shards are merged in chunk-index order so output is byte-identical across worker counts. Below 16 input files the export stays sequential regardless. Ignored without `--export-functions` |
| `-t / --threshold` | 0.7 | Fuzzy similarity floor (0..1). Score is `sqrt(jaccard × len_ratio)` plus a hard `len_ratio ≥ threshold` gate |
| `-n / --top` | 20 | Top-N entries shown in stdout summary |
| `--json` | (off) | Path for full JSON report |
| `-x / --no-fuzzy` | off | Skip MinHash pass — exact clusters only |
| `--min-tokens` | 8 | Drop functions with fewer than N tokens (filters trivial wrappers) |
| `-L / --lambdas-only` | off | Skip top-level functions, keep only lambdas — useful for clustering dastest `t \|> run("…") @(t) { … }` bodies |
| `--export-functions` | (off) | Write all extracted functions to a JSON file and exit before clustering |
| `--import-functions` | (off) | Load functions from a JSON file (produced by `--export-functions`) instead of compiling. Mutually exclusive with `--path` and `--export-functions` |
| `-v / --verbose` | off | Per-file progress |
| `--baseline` | (off) | B1: load corpus JSON; tag records whose member identity (`file:line:name`) isn't in the baseline as candidates and filter to those |
| `--baseline-strict` | off | B1 modifier: also drop clusters whose canonical exists in the baseline (only fully-new clusters survive) |
| `--against` | (off) | B2 candidate path (file or directory). Repeatable. Compiled in-process; their functions are tagged candidates and the report is filtered |
| `--against-from-stdin` | off | B2: read newline-delimited candidate paths from stdin (use with `git diff --name-only … \| detect-dupe --against-from-stdin …`) |
| `--check` | off | Exit non-zero when the post-filter report contains any clusters/pairs (CI gate) |
| `--flat` | off | In `--against` mode, force the flat clusters+pairs writer (default is the per-candidate rollup) |
| `-k / --keep` | (off) | Pattern name to KEEP despite default skip (repeatable). Special value `all` disables pattern filtering entirely. See "Patterns" below |
| `-?` | | Help |

`builtin.das`, `daslib/debugger.das`, and `daslib/profiler.das` are skipped automatically — the latter two install thread-local debug agents at compile time, which would abort the scanner on second use.

## Patterns (default-skip filter)

A "pattern" is a structural shape that signals "this function is boilerplate, not real code" — typically a wrapper or dispatcher whose canonical token stream contains zero unique signal beyond its repetition count. Pattern-matched functions are dropped from clustering by default, on the theory that they explode cluster sizes and fuzzy-pair counts without surfacing real duplicates.

Override per-pattern via `--keep <name>` (repeatable), or disable filtering wholesale via `--keep all`.

Currently shipped patterns:

| Name | Detects | Why it's boilerplate |
|---|---|---|
| `visitor` | Class-method whose hook name starts with `visit`, `preVisit`, `postVisit`, `before`, or `after` (matched by name, regardless of body) | `AstVisitor` overrides — the dispatch contract requires one method per AST node type, so cross-class duplication at the canonical level is structural. Common in `daslib/aot_cpp.das`, `daslib/ast_print.das`, `daslib/templates_boost.das`, `daslib/rst_comment.das`, `daslib/perf_lint.das` |
| `dispatch` | Function whose body is `N >= 2` byte-identical top-level statement chunks (`STMT … STMT … STMT …` with all chunks equal) | dastest's `t \|> run("X") @(t) { … }` outer functions. Lambda bodies are collapsed to `ADDR` upstream, so two `run` calls look identical regardless of what the lambdas do — the outer function carries zero unique structure beyond its call count. Same shape catches `t \|> bench(…)`, repeated-init blocks, and any uniform call list |
| `emit` | 1..6 top-level statements, each a single trivial `CALL:foo(...)` (only literal/var/field args — no nested calls, no control flow) or a single `RET ...` | Emitter shells like `def visitX(...) { write(*ss, ")") ; return that }` — non-visitor variants of the same pattern (free-function literal-emit wrappers). The visitor matcher covers the named-by-convention case; this one catches the body-shape case for free functions or unconventionally-named class methods |

Pattern detection lives in `patterns.das`. Adding a new pattern is a three-step change:

1. Add a `try_<name>(name?, canonical, var hit) : bool` predicate that returns `true` and populates `hit.name` / `hit.note` when matched. Body-shape matchers take only the canonical; name-shape matchers take both. Match order in `classify()` is name-first, then body-shape (more specific → more general).
2. Wire it into `classify(name, canonical)` in priority order (more specific shapes first; name-based matchers usually win over body-shape matchers).
3. Add at least one positive test and one negative test in `test_detect_dupe.das` (under `── patterns / classify ──`).

Pattern names are the user-visible contract — they appear in `--keep`, in the per-record `--verbose` skip log, and in the summary line. Pick a short, stable identifier.

The summary line surfaces what was filtered:

```
collected 66 record(s); 0 compile failure(s), 1 skipped (expect-directive)
patterns skipped: 35 dispatch (--keep <name>|all to include)
```

`--verbose` prints one `pattern-skip [name] file:line func (note)` line per filtered record — useful for confirming the filter isn't dropping real signal on a new corpus.

## Canonical form

Each function emits a flat tag stream. Examples:

| Source | Canonical (excerpt) |
|---|---|
| `def add(a,b:int):int { return a+b }` | `FN ARG <var_0> TYP ARG <var_1> TYP TYP BODY BLK STMT RET OP2:+ <var_0> <var_1> ENDBLK ENDFN` |
| `def add(a,b:float):float { return a+b }` | (same — types collapse) |
| `def double(a:int) { return a*2 }` | `FN ARG <var_0> TYP BODY BLK STMT RET OP2:* <var_0> LIT ENDBLK ENDFN` |

User identifiers → `<var_0>`, `<var_1>`, …
All types → `TYP`. All literals → `LIT`. Field/swizzle names → `.FLD` /
`.SWZ`. Called function names are kept (`CALL:push` vs `CALL:emplace`
is real signal).

## Implementation

| File | Role |
|---|---|
| `canonical.das` | `CanonicalVisitor` (extends `daslib/ast` `AstVisitor`) and `tokenize_canonical` |
| `minhash.das` | 64-slot MinHash signatures over 5-grams, Jaccard estimate |
| `cluster.das` | Exact-bucket clustering + fuzzy all-pairs with length gate |
| `report.das` | JSON + stdout summary writer |
| `main.das` | CLI (`daslib/clargs`), file scan, compile-and-collect orchestration |
| `pipeline.das` | `compile_and_collect` / `collect_from_program` — compile-and-extract orchestration, shared by `main.das` and the test suite. `apply_pattern_filter` drops records matched by `patterns.das` |
| `patterns.das` | Pattern matchers — `classify(name, canonical) → PatternHit`. Default-skipped shapes (visitor methods, dispatchers, emitters); override via `--keep <name>` |
| `exchange.das` | On-disk JSON schema + writer/reader for `--export-functions` / `--import-functions` |
| `fixture/synth.das` | Hand-crafted fixture for smoke-testing the visitor end-to-end |
| `fixture/canonical_cases.das` | Narrowly-targeted fixture (one function per canonicalization concern) for unit-testing `CanonicalVisitor` |
| `test_detect_dupe.das` | dastest suite — run with `bin/daslang dastest/dastest.das -- --test utils/detect-dupe/test_detect_dupe.das` |

Notes:

- Compile policy mirrors `utils/lint`: `ignore_shared_modules`,
  `export_all`. Optimisations and infer-time folding stay ON so dastest
  macros (e.g. `unroll`) compile.
- **Default mode** drops everything `generated` (which includes
  lambdas). The dispatcher already references each lambda via an `ADDR`
  token, so the lambda's structural fingerprint is partially preserved
  in the parent function. Use `-L` to flip this and cluster the lambda
  bodies themselves.
- **Lambda-only mode (`-L`)** is dominated at the top by linq's `each`
  macro emissions (a 100+-token `GOTO/LABEL/_builtin_iterator_first /
  next/close` shell that recurs hundreds of times). Real test-body
  signal starts a few clusters down; sort/grep accordingly.
- Functions whose `at.fileInfo` points outside the compiled file are
  filtered out — without this, reified generics from required modules
  (e.g. `dastest/testing.das`) flood the report.
- Per-source-line dedup: a generic reified for N types becomes N
  FunctionPtrs all pointing at the same `(file, line)`. We keep the
  first to avoid the same source location being counted N times.

## Export / import

Compilation is the expensive step; the canonical-form computation is deterministic. To hand the function list off to an external tool (visualizer, custom clusterer), or to shard compilation across machines and merge later, dump the post-canonicalization records and reload them:

```sh
# compile + extract, write JSON, exit before clustering
./bin/daslang utils/detect-dupe/main.das -- -p tests --export-functions /tmp/funcs.json

# skip compilation; cluster + report from JSON (--json still works as before)
./bin/daslang utils/detect-dupe/main.das -- --import-functions /tmp/funcs.json --json /tmp/dupes.json
```

`--import-functions` is mutually exclusive with both `--path` and `--export-functions`. `--export-functions` always exits before the clustering pass.

The on-disk schema is a small envelope:

```json
{
  "schema_version": 1,
  "functions": [
    { "name": "add_int", "file": "tests/foo.das", "line": 4,
      "is_lambda": false, "canonical": "FN ARG <var_0> TYP …" }
  ]
}
```

MinHash signatures are not included — they're recomputed on import (deterministic and cheap). On import, `--no-fuzzy` and `--min-tokens` apply just like in the compile path.

## Modes

The flat report from `-p` is the firehose — useful once on a new corpus to
calibrate, less useful day-to-day. Two filtered modes are layered on top
via a single `is_candidate` flag inside `FuncRecord`. A cluster or fuzzy
pair is **kept** iff at least one of its members is a candidate.

### B1 — baseline diff (CI gate)

Snapshot the corpus once, commit the JSON, and on every PR re-scan + diff.

```sh
# one-off: build the baseline (commit this)
./bin/daslang utils/detect-dupe/main.das -- -p tests --export-functions tests_baseline.json

# CI: scan again, surface only what isn't in the baseline
./bin/daslang utils/detect-dupe/main.das -- -p tests --baseline tests_baseline.json --check
```

Records are tagged candidate when their **member identity** (`file:line:name`) is absent from the baseline. A cluster appears in the report if any of its members is a candidate, which catches both (a) brand-new canonicals and (b) growth — a new copy of an already-tracked canonical added in a new location. Use `--baseline-strict` to drop case (b) — strict additionally filters out clusters whose canonical was already in the baseline, so only fully-new canonicals survive. Pairs aren't strict-filtered (the baseline doesn't carry MinHash signatures), so strict is cluster-only.

Note: `file:line:name` keying means an unrelated edit that shifts line numbers will look like a "new member" and surface its cluster — acceptable for CI, since touched code is the right default to re-check.

`--check` turns the filtered report into a CI gate — non-zero exit when any cluster or pair survives the filter.

### B2 — PR-files / interactive

"Did I just write something that already exists?" Compare a file list against a pre-built corpus:

```sh
./bin/daslang utils/detect-dupe/main.das -- \
    --import-functions tests_baseline.json --against tests/strings/new_helper.das

# git pipeline:
git diff --name-only master | grep '\.das$' | \
    ./bin/daslang utils/detect-dupe/main.das -- \
        --import-functions tests_baseline.json --against-from-stdin
```

When `--against` and `--import-functions` are both set, corpus records whose `file` matches any candidate path are dropped first, then the candidate is freshly compiled — so the file is compared against the rest of the world, never against its own stale copy in the baseline. Look for the `dropped N corpus records overridden` line.

The default writer in `--against` mode is a per-candidate rollup ("for each function in the focus set, here are its top siblings"). Use `--flat` to revert to the legacy clusters+pairs view.

### MCP integration

The `detect_duplicates` tool in the [daslang MCP server](../mcp/) wraps B2 mode for AI assistants. Pass `paths` (newline- or comma-delimited, or a glob) and `corpus` (the JSON from `--export-functions`); receive a per-candidate JSON envelope. Used by Claude Code et al. during PR review.

## Out of scope

- LSH / banding (4.4K² is fine; revisit at >50K functions).
- Embedding-based similarity (would need an external service).
- Auto-fix or refactor suggestions — discovery only.
