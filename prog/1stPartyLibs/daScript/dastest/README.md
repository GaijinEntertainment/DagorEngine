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
- `--bench`: Enable benchmark execution (all of them)
- `--bench-names <namePrefix>`: Run top-level benchmark matching "namePrefix"
- `--bench-format <format>`: Specifies the benchmark output format ("native", "go" or "json")
- `--count <n>`: Run all benchmarks this number of times (useful for sample collection)

Note that benchmarks will be executed only if all module tests have already passed. If you want to run benchmarks only, skipping the tests, consider using something like `--test-names none`.

#### Internal arguments

- `--run`: Path to the single script file to run tests in isolated mode

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
