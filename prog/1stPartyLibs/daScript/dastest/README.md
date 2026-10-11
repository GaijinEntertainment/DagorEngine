# daslang testing framework

## Introduction

Inspired by [Golang testing framework](https://pkg.go.dev/testing), this framework provides a simple way to write unit tests for your scripts.

## Usage

In the examples below, `$target` refers to "folderWithScriptsOrSingleScript".

Running all tests:

- `daslang dastest.das -- --test $target [--test anotherPath]`

Running all tests **and** benchmarks:

- `daslang dastest.das -- --bench --test $target`

Running benchmarks **only**:

- `daslang dastest.das -- --bench --test $target --test-names none`

Running only **some** selected benchmarks (uses `vector_alloc` as a filtering prefix):

- `daslang dastest.das -- --bench --bench-names vector_alloc --test $target --test-names none`

### dastest.das arguments

- `--test`: Path to the folder with scripts or single script to test
- `--test-names <namePrefix>`: Run top-level tests matching "namePrefix", such as "namePrefix1"
- `--test-project <path.das_project>`: Project file. Will be used to compile given tests
- `--uri-paths`: Print uri paths instead of file paths (vscode friendly)
- `--color`: Print colored output
- `--verbose`: Print verbose output
- `--timeout <seconds>`: The whole suite's timeout. Past it the process ends at once with exit code 124 and no summary, after one line naming the timeout; the `run N/M` lines say which file was running. `0` disables it. The default is 20 minutes
- `--max-file-time <seconds>`: Fail after the suite finishes if any completed test file exceeded this wall-clock duration. `0` (default) disables the limit; this is independent of the global `--timeout` deadlock guard.
- `--isolated-mode`: Run tests in isolated processes, useful to catch crashes
- `--isolated-mode-threads <n>`: Number of worker threads in isolated mode. `0` uses a conservative JIT rule (`min(32, max(4, logical_threads / 2))`) and 2x hardware threads otherwise. Preflight can override the JIT width per machine through its user config.
- `--batch <n>`: Files per worker subprocess in isolated mode (semi-isolated sharding). `1` (default) is one process per test (full isolation). `>1` amortizes process/compile cold-start across a batch - much faster, especially on Windows. A crash in a batch is auto-recovered: the file that died is reported as crashed and the rest of the batch is re-run one-process-per-file, so isolation is preserved exactly where it is needed.
- `--stack-on-exception`: On a test panic, walk the call stack *at throw time* (frames intact) and print a real `CALL STACK` trace, instead of the default that reports only the panic message and location. Off by default (a test that swallows a panic via `try`/`recover` would also emit a walk); enable for one-iteration debugging of a failing/crashing test. Works in isolated mode too - the trace is folded under the failing test's log.
- `--bench`: Enable benchmark execution (all of them)
- `--bench-names <namePrefix>`: Run top-level benchmark matching "namePrefix"
- `--bench-format <format>`: Specifies the benchmark output format ("native", "go" or "json")
- `--count <n>`: Run all benchmarks this number of times (useful for sample collection)

Note that benchmarks will be executed only if all module tests have already passed. If you want to run benchmarks only, skipping the tests, consider using something like `--test-names none`.

#### Internal arguments

- `--run`: Script file(s) to run in a single subprocess (repeatable). Used by isolated/semi-isolated mode to dispatch a batch to one worker; each file streams its own JSON report so the parent can attribute results and recover from a mid-batch crash

### Benchmark measurement {#benchmark-measurement}

`run` measures the way Go's `testing.B` does. The first batch is one call - the warm-up and the calibration. Every batch that finishes under the 1 s budget grows the count from what that batch measured (1.2x the projection, at most 100x per round), so a JIT that was cold on the first call is measured warm, and the batch that meets the budget is the one reported: `ns/op` is its wall time over its count. Batches of 1000 calls or more run ten-fold unrolled under a single timer, which divides the loop's own cost by ten; growth rounds such counts down to a multiple of ten, so the reported count is exactly the number of calls timed. A shorter batch times each call instead, because there the call dominates the loop. The benchmark's name is announced after the first batch, so a benchmark that fails fast never prints it. `--count` repeats the whole pass for sample collection; the `go` output format is what `benchstat` reads.

## Examples

### Benchmarking

```das
options gen2
require dastest/testing_boost public

[benchmark]
def dyn_array(b : B?) {
    let size = 100

    b |> run("fill_vec") <| $() {
        var vec : array<int>
        for (i in range(size)) {
            vec |> push(i)
        }
    }

    b |> run("fill_vec_reserve") <| $() {
        var vec : array<int>
        vec |> reserve(size)
        for (i in range(size)) {
            vec |> push(i)
        }
    }
}
```

## review_gate.das

Not part of the test runner: `dastest/review_gate.das` is the support library for
`REVIEW.das` gates - the executable half of a folder's `REVIEW.md` review checklist (the
contract lives in `REVIEW_COMMON.md` at the repo root, vendored by repos that adopt it). It
provides finding accumulation and the exit verdict (`gate_finding`, `gate_findings`,
`gate_reset`, `gate_verdict`), the descriptor census (`gate_descriptor_census`, two
overloads), the cookie-and-host gate for a served web tree (`gate_web_third_party`, built
on `html_resource_urls`, `script_urls`, `css_urls` and `url_host`), the home-directory gate
for a published record (`gate_home_paths`), the prefix census (`gate_prefixed_names_defined`,
built on `prefixed_tokens`, `css_selector_tokens` and `html_ids`), the page-source extractors
(`html_code_blocks`, `markdown_fences`), plus tree-analysis
helpers: `das_requires`, `strip_line_comments`, `cmake_command_blocks`,
`cmake_command_targets`, `cmake_words`, `cmake_args`, `cmake_list_entries`,
`cmake_test_labels`, `cmake_test_commands`, `cmake_copy_sources`, `shell_cp_sources`,
`prefixed_tokens`, `markdown_table_rows`, `is_cmake_keyword`, `is_kebab_case`, `find_line`. The CMake helpers match command names case-insensitively, as CMake itself
does. It lives under
`dastest/` so an installed SDK carries it the same way it carries the test framework -
dastest itself ships in the SDK as a prebuilt exe, the `DAS_UTILS_SHIPPED_EXES` entry in
`utils/CMakeLists.txt` (repo root).

### The web gate's scanned set {#web-gate-scanned-set}

`gate_web_third_party` reads one JSON shape and no other: an asset manifest (a
`.assets.json` sidecar) lists what the playground fetches, so every entry in it is a load.
Any other JSON can spell an address as a `link` - a click, not a load - so scanning it would
report hosts no page ever reaches.
