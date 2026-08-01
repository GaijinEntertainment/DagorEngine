# compareDumps

Pure-Python (stdlib-only, 3.6+) tool to compare two Dagor memory flame-graph
dumps and report where allocated bytes and live allocation (pointer) counts
grew and where they were saved.

Dumps come from the `/memory_map` webui page (Memory Flame Graph, see
`webui/plugins/dagor/memmap.cpp`; requires a
`-sUseMemoryDebugLevel=dbg*` build). Accepted inputs:

- a page saved from the browser (`.html` with the JSON tree embedded in a
  `<script id="embeddedData">` tag), or
- the raw JSON tree itself (e.g. from `/memory_map?rc=...`).

## Quick start

```sh
python compare_dumps.py OLD.html NEW.html                  # markdown to stdout
python compare_dumps.py OLD.html NEW.html --format json    # JSON to stdout
python compare_dumps.py OLD.html NEW.html --out report.md --json-out report.json
```

Convention: the **first** argument is the baseline (OLD), the **second** is the
change under test (NEW). A positive delta means NEW allocates more.

## Options

```
--group-by frame|stack  aggregate self-allocations per frame name (default) or
                        per exact call stack
--top N                 rows per markdown section (default 40; JSON always has
                        the full above-threshold list)
--min-bytes N           minimum |byte delta| to report a row (default 16k;
                        suffixes k/m/g)
--min-ptrs N            minimum |pointer-count delta| to report (default 16)
--callers N             caller frames of context per row (default 3)
--raw-names             keep "(line) +offset" in frame names; stripped by
                        default so dumps from different builds still match
--format md|json|both   what goes to stdout (default md)
--out FILE              write the markdown report to a file (always markdown
                        regardless of --format)
--json-out FILE         also write the machine-readable JSON to a file
```

A terse one-line verdict (net bytes/pointers, grown/saved split) always goes to
stderr.

## What it reports

1. **Totals** -- allocated bytes and live pointers, OLD vs NEW, delta, percent.
2. **Suspicious single-site values** -- call sites accounting >= 2 GB. A value
   just below a multiple of 4 GB is usually a wrapped 32-bit size counter and
   inflates that dump's total; check this section before trusting the totals.
3. **Memory grown / saved** -- per frame (or per stack), ranked by self-byte
   delta, with the pointer-count delta and caller context. Frames present in
   only one dump are tagged `[new]` / `[gone]`.
4. **Pointer count grown / saved** -- the same rows ranked by allocation-count
   delta; catches many-small-object churn that byte deltas hide.

Each totals line reconciles exactly: net = grown + saved + below-threshold
remainder.

## Method and semantics

- The dump is a call tree; each node carries **self** values (`sa` bytes, `sp`
  pointers). The tool aggregates self values, so every byte is attributed to
  exactly one row and section sums reconcile with the totals. Inclusive values
  (`a`/`p`) are ignored (they double-count under recursion).
- By default frame names are normalized: the trailing `(713) +46` (Windows
  dbghelp line + offset) or ` + 0x1a2b` (unix module offset) is stripped,
  because line numbers and offsets shift between builds. Use `--raw-names`
  only when both dumps come from the same binary.
- `--group-by frame` answers "which function's direct allocations changed";
  `--group-by stack` splits that per exact call path. Frame mode is the sane
  default: in stack mode a path that merely shifted (same function, slightly
  different caller) shows up as a paired grow+save.

## Caveats

- Two dumps are two **separate runs** unless taken in the same session:
  loading order, streaming state and content differences produce real but
  irrelevant deltas. Take dumps at comparable moments (same level, same camera,
  after loading settles) and treat small deltas as noise.
- The profiler tracks allocations it can see; per-allocation debug overhead
  and untracked pools are not represented.
