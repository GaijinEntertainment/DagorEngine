# benchctl Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture doc:
`README.md`.

**Never put a `[test]` file outside `tests/`, and never let a test touch the filesystem outside a
`temp_directory`-rooted path or leave behind what it creates.**

**A module file orders its top level types, then `private` helpers, then its public functions, so
the tail of the file is the module's whole API.**

**Never add a run-record field - a field of `RunRecord` or `FileResult` in `bench_history.das`,
what `run` writes per night - without saying in `README.md`'s "Run records" section what reads
it - the viewer, or a person opening the record.** A field nobody named is one nobody notices
going wrong.

**A diff that changes a run-record field keeps the new reader parsing a record written by the old
code, a missing field keeping its declared default.** Records already on the box are never
rewritten.

**Never let a child's own output overwrite a `timeout` status in `run_bench_file` - a killed child
that printed a passing report is still killed.** Loosen `timeout_seconds` in `suite.json` instead.

**Never read a benchmark's identity from anywhere but its path under `benchmarks/`** - the group
is the directory, the id is the path without `.das`.

**Statistics live in `benchstat.das` and nowhere else** - it depends on no storage, so a second
median or spread helper anywhere in this folder is a defect.

**Placement - one file, one line: a diff keeps each file inside its line, and a new file adds its
line here, with its tests, in the same change.**

- `main.das` - argv, subcommand dispatch, exit codes.
- `bench_args.das` - the database verbs' flags.
- `bench_table.das` - the stored row and its mapping to the shape the statistics read.
- `benchstat.das` - every statistic: median, spread, outlier filtering, geomean, Welch. No storage.
- `bench_suite.das` - `suite.json` and file discovery. No processes.
- `suite.json` - what is benchmarked, in which lanes, with which limits.
- `bench_runner.das` - one file in one lane: spawn, timeout, output to samples and verdict.
- `_jit_probe.das` - the program `probe_jit` runs to prove the jit lane works.
- `bench_history.das` - run records to dataset. No processes, no argv, no statistics.
- `bench_stand.das` - the `run` and `report` verbs the nightly box drives.
- `table_fmt.das`, `utils.das` - output formatting and small shared helpers.
- `tests/test_bench_suite.das`, `tests/test_bench_runner.das`, `tests/test_bench_history.das` -
  the module suites; `tests/test_bench_cli.das` - the verbs, spawned; `tests/_test_common.das` -
  the fixtures they share; `tests/_fake_dastest.das` - the stand-in dastest a spawn test measures
  against.

