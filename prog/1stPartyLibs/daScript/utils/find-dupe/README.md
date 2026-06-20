# find-dupe — AI judge for detect-dupe clusters

`utils/detect-dupe` produces clusters of suspected duplicate functions (exact + fuzzy). On a real codebase those clusters mix **real duplicates** with **false positives** — pattern-matching boilerplate, generic accessors, shared-shape glue code that happens to canonicalize alike. Manually triaging clusters is the bottleneck for actually deleting duplicates.

`find-dupe` feeds each cluster's actual source to Claude (via the `das-claude` daspkg, module `anthropic/anthropic`) and asks it to **partition** the cluster into real-duplicate groups vs false positives, with a one-line reason. Output is JSON (machine, for CI gates / future tooling) + Markdown (human, for review).

> ⚠️ **Sends source code to Anthropic's API.** Each cluster's full function bodies (verbatim, with file paths and line numbers) are uploaded as part of every judge call. Do **not** run this tool on proprietary, confidential, or otherwise restricted code unless your project's data-handling policy permits sending that source to Anthropic. Anthropic's API terms apply (see [anthropic.com/legal](https://www.anthropic.com/legal)). Use `--dry-run` to preview cluster size and token estimates without making any API calls.

The default model is **Claude Haiku 4.5** — cheap, fast, and reliable for "are these the same function?" judgments. Use `--model sonnet` for harder clusters.

## Install

```bash
# from the project root
bin/daslang utils/daspkg/main.das -- install --root utils/find-dupe
```

This fetches `das-claude` per `.das_package` and unpacks it under `utils/find-dupe/modules/`. If you skip this step, `bin/daslang utils/find-dupe/main.das` fails at compile time with `module 'anthropic/anthropic' not found` — re-run the install command above.

## API key

Set `ANTHROPIC_API_KEY` so daslang's non-interactive subshells can see it. On macOS the easiest path is `~/.zshenv` (sourced by every zsh invocation, interactive or not):

```bash
echo 'export ANTHROPIC_API_KEY="sk-ant-..."' >> ~/.zshenv
```

`~/.zshrc` works for terminals you opened yourself but **not** for GUI-launched processes (VS Code, Claude Code's MCP server) — those inherit env from launchctl. `~/.zshenv` covers both.

Verify the wiring with the built-in smoke test:

```bash
bin/daslang utils/find-dupe/main.das -- --smoke-test
```

Expect:

```
Running das-claude smoke test (model=claude-haiku-4-5-20251001)...
Reply: OK
Tokens: in=15 out=4
Smoke test PASSED.
```

## Workflow

1. **Run detect-dupe** to produce a JSON report:
   ```bash
   bin/daslang utils/detect-dupe/main.das -- -p <paths> --json ./dupes.json
   ```

2. **Dry-run** find-dupe for a cost preview (no API calls):
   ```bash
   bin/daslang utils/find-dupe/main.das -- --input ./dupes.json --dry-run -v
   ```
   Output: cluster counts, estimated input/output tokens, dollar estimate.

3. **Live run** to produce verdicts:
   ```bash
   bin/daslang utils/find-dupe/main.das -- --input ./dupes.json -v
   ```
   Writes `find_dupe_verdicts.json` (machine) and `find_dupe_verdicts.md` (human) to `./find-dupe-out/` (override the directory with `--out`, the basename with `--out-basename`).

> **Run from the project root.** `detect-dupe` records source paths relative to its cwd. `find-dupe` extracts each function's body from disk using those paths, so its cwd must match.

> **Cross-platform paths.** Examples use cwd-relative paths (`./dupes.json`) so they work on Linux, macOS, and Windows alike. If you'd rather drop intermediates in the OS tempdir, substitute your shell's convention — `$TMPDIR/dupes.json` (zsh/bash on macOS), `/tmp/dupes.json` (Linux), or `%TEMP%\dupes.json` (PowerShell/cmd) — none of those are required by the tool.

## Flags

| Flag | Default | Description |
|---|---|---|
| `-i`, `--input` | (required) | detect-dupe JSON report file |
| `-o`, `--out` | `./find-dupe-out` | output directory |
| `--model` | `haiku` | `haiku` (Haiku 4.5) or `sonnet` (Sonnet 4.6) |
| `--min-lines` | `6` | skip clusters where every member is shorter than this |
| `--max-clusters` | `0` | hard cap on clusters analyzed (0 = no cap) |
| `--sort-by` | `default` | `default` (composite), `size`, or `fuzzy_first` |
| `--parallel` | `8` | concurrent judge calls (1 = sequential) |
| `--positives-only` | off | filter reports to actionable rows only: real + partial |
| `--dry-run` | off | estimate tokens & cost without making API calls |
| `--smoke-test` | off | one-shot API call to verify the env wiring |
| `-v`, `--verbose` | off | per-cluster progress to stdout |
| `-?`, `--show-help` | — | print help and exit |

The default sort puts fuzzy pairs first (highest AI value), then larger clusters, then larger total span — useful with `--max-clusters` and when interrupting a long run.

## Output shape

**`find_dupe_verdicts.json`** — top-level fields:

| Field | Type | |
|---|---|---|
| `schema_version` | int | bump on schema changes |
| `model` | string | the resolved model id |
| `summary` | object | totals + token usage |
| `verdicts` | array of `VerdictRow` | one row per cluster |

Each `VerdictRow` carries `cluster_id`, `kind` (`exact`/`fuzzy`), `similarity` (for fuzzy), `members` (with file/line/end_line), `verdict` (`real`/`partial`/`false_positive`/`skipped`/`error`), `groups` (the partition), `false_positives` (indices), `reason`, and per-call token usage.

**`find_dupe_verdicts.md`** — human-readable, per-cluster sections with verdict tag, reason, member list, partition, and the original source (collapsed under `<details>`).

## MCP integration

Two MCP tools shell out to this CLI so AI assistants can run the full pipeline without leaving the chat:

- `judge_duplicates` — takes a detect-dupe JSON report path; returns the verdict envelope.
- `find_dupe` — convenience: runs detect-dupe against `paths`, then judges the resulting clusters.

Both surface a structured "anthropic daspkg not installed" error with the install command when the subprocess fails for that reason. See `doc/source/reference/utils/find_dupe.rst` and `utils/mcp/README.md` for parameter details.

## Tests

```bash
bin/daslang dastest/dastest.das -- --test utils/find-dupe/tests
```

Hermetic — no API calls. Validates `parse_verdict`, `classify_verdict`, `extract_source`, and `read_input_report`. Test fixtures use `daslib/fio`'s `create_temp_file_result(prefix, ext)` so they're cross-platform clean. Wired into `run_utils_tests` via `utils/CMakeLists.txt`.

## Pricing

Approximate, as of 2026-04:

| Model | Input ($/MTok) | Output ($/MTok) |
|---|---|---|
| Haiku 4.5 | 1.00 | 5.00 |
| Sonnet 4.6 | 3.00 | 15.00 |

A typical cluster (3-4 functions, 50 lines total) consumes ~1 KTok in + 0.3 KTok out → ~$0.0025 on Haiku. Use `--dry-run` first if a run looks large.

## See also

- `utils/detect-dupe/README.md` — the cluster-producing pipeline whose JSON we consume.
- `examples/claude/` — the daslang helper bot, the source pattern this tool was built from (also uses das-claude).
