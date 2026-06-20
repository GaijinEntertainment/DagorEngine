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
- `--timeout <seconds>`: If tests run longer than duration d, panic. If d is 0, the timeout is disabled. The default is 10 minutes
- `--isolated-mode`: Run tests in isolated processes, useful to catch crashes
- `--isolated-mode-threads <n>`: Number of worker threads in isolated mode (defaults to 2x hardware threads when 0)
- `--batch <n>`: Files per worker subprocess in isolated mode (semi-isolated sharding). `1` (default) is one process per test (full isolation). `>1` amortizes process/compile cold-start across a batch — much faster, especially on Windows. A crash in a batch is auto-recovered: the file that died is reported as crashed and the rest of the batch is re-run one-process-per-file, so isolation is preserved exactly where it is needed.
- `--stack-on-exception`: On a test panic, walk the call stack *at throw time* (frames intact) and print a real `CALL STACK` trace, instead of the default that reports only the panic message and location. Off by default (a test that swallows a panic via `try`/`recover` would also emit a walk); enable for one-iteration debugging of a failing/crashing test. Works in isolated mode too — the trace is folded under the failing test's log.
- `--bench`: Enable benchmark execution (all of them)
- `--bench-names <namePrefix>`: Run top-level benchmark matching "namePrefix"
- `--bench-format <format>`: Specifies the benchmark output format ("native", "go" or "json")
- `--count <n>`: Run all benchmarks this number of times (useful for sample collection)

Note that benchmarks will be executed only if all module tests have already passed. If you want to run benchmarks only, skipping the tests, consider using something like `--test-names none`.

#### Internal arguments

- `--run`: Script file(s) to run in a single subprocess (repeatable). Used by isolated/semi-isolated mode to dispatch a batch to one worker; each file streams its own JSON report so the parent can attribute results and recover from a mid-batch crash

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
