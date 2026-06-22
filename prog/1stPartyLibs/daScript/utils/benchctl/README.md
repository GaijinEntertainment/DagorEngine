# benchctl

A command-line tool for storing, querying, and comparing daslang benchmark results across commits.

Can also be handy to do some incremental tests of performance by measuring the changes one after another (e.g. try algorithm 1, then compare it against algorithm 2 results).

You can browse the data using SQLite3 client if you want to. Use SQLite3 dumping to analyze the data using external tools and/or scripts.

## Overview

benchctl stores benchmark output in a local SQLite database and provides statistical comparison between two sets of results. It integrates directly with the daslang benchmark runner (`dastest` with `--bench` argument).

Key capabilities:

- Insert benchmark JSON output files into a persistent database, tagged by commit hash (and optionally custom tags)
- Query the database with structured filter flags (`--commit`, `--tag`)
- Compare two sets of results
- Compute geometric mean deltas across all benchmarks in a comparison

> benchctl uses a port of Go's benchstat command Welch's t-test for statistical significance implementation

## Prerequisites

- daslang compiler with `sqlite` module support

> You will need to use a `-DDAS_SQLITE_DISABLED=off` when running a cmake

## Usage

```
daslang utils/benchctl/main.das -- <command> [options...]
```

All commands accept `--db <path>` to specify the database file (default: `benchdata.db`) and `--no-color` to disable ANSI color output.

Consider using an explicit name for a long-term database while using the default `benchdata.db` as a scratch db you can reset between the experiments.

---

### `reset`

Initializes (or reinitializes) the benchmark database.

> **Warning:** drops all existing data.

```
daslang utils/benchctl/main.das -- reset --db benchdata.db
```

> Note, running most other commands without an explicit `reset` on a non-existing database file would call a `reset` implicitly.

---

### `insert`

Reads one or more benchmark JSON output files and inserts their records into the database.

> Use `--bench-format json` to make dastest benchmarks output the JSON files.

```
daslang utils/benchctl/main.das -- insert [options...] <file>...
```

Options:

| Flag | Description |
|------|-------------|
| `--db <path>` | Database file path (default: `benchdata.db`) |
| `--commit <hash>` | Git commit hash to tag results with (default: `git rev-parse HEAD`) |
| `--tag <name>` | Tag label to attach to results - can be repeated (must not contain `[` or `]`) |

Input files must contain newline-delimited JSON records as produced by the dastest benchmark runner. Non-JSON lines are silently skipped.

```
# Adds all samples from result1.txt and result2.txt tagging them with 2 provided tags,
# the commit hash will be "git rev-parse HEAD" (use --commit to override that)
daslang utils/benchctl/main.das -- insert --tag example1 --tag foo result1.txt result2.txt
```

**Example:**

```sh
# Run benchmarks and save output.
# --test specifies the benchmarks (and tests) source dir or files
# --bench tells dastest to run the benchmarks along with the tests (they're off by default)
# --bench-format json selects the JSON export format
# --count 10 specifies how many "samples" we want per every benchmarks
# More samples - more precision, but also much more time for the benchmarking to finish.
# Note: you need at least 5 samples for a benchmark to be analyzeable.
daslang dastest/dastest.das -- --test benchmarks/ --bench --bench-format json --count 10 | tee results.txt

# Insert these samples into the database (with no extra tags)
daslang utils/benchctl/main.das -- insert results.txt
```

---

### `query`

Displays benchmark records from the database. Filters compose with AND.

```
daslang utils/benchctl/main.das -- query [--db benchdata.db] [--commit <hash>] [--tag <name>]
```

Options:

| Flag | Description |
|------|-------------|
| `--db <path>` | Database file path |
| `--commit <hash>` | Filter to records with this exact commit hash |
| `--tag <name>` | Filter to records carrying this tag (single-value; uses LIKE under the hood — empty `--tag` means no filter) |

This is mostly needed to debug the selection filters before using a more useful `compare` command.

**Examples:**

```sh
# Show all records
daslang utils/benchctl/main.das -- query

# Show records for a specific commit
daslang utils/benchctl/main.das -- query --commit abc12345

# Show records with a tag
daslang utils/benchctl/main.das -- query --tag before

# Combine filters (AND)
daslang utils/benchctl/main.das -- query --commit abc12345 --tag before
```

If you need a one-off filter that the structured flags don't cover (e.g. `WHERE string_allocs > 0`), open the DB directly with the `sqlite3` shell — `benchctl` no longer accepts arbitrary SQL via the CLI.

> Stored columns: `id`, `commit_hash`, `tags`, `insert_date`, `full_name`, `name`, `sub_name`, `mode`, `n`, `time_ns`, `allocs`, `heap_bytes`, `string_allocs`, `string_heap_bytes`. Tags are stored as `[tag1][tag2]` strings.

---

### `compare`

Compares two sets of benchmark results using statistical analysis.

```
daslang utils/benchctl/main.das -- compare [options...]
```

Options:

| Flag | Description |
|------|-------------|
| `--db <path>` | Database file path |
| `--old-commit <hash>` | Baseline commit filter |
| `--old-tag <name>` | Baseline tag filter |
| `--new-commit <hash>` | New commit filter |
| `--new-tag <name>` | New tag filter |
| `--s <from=>to>` | Regex rename: rewrite old benchmark names to match new names |
| `--no-color` | Disable colored output |

Each side's filters compose with AND. Empty side filters mean "all rows for that side" — usually you want at least one filter per side.

**Example - compare two commits:**

```sh
daslang utils/benchctl/main.das -- compare \
    --old-commit abc12345 \
    --new-commit def67890
```

**Example - compare using tags:**

```sh
daslang utils/benchctl/main.das -- compare \
    --old-tag before \
    --new-tag after
```

Both sample sets (old and new) will be compared over the matching `full_name`. This means only the same benchmark results (but across different revisions) can be compared. Unless you use a renaming rule.

**Example - rename benchmarks between two sets:**

If benchmarks have different names, but logically can be compared to one another (e.g. "BenchmarkBad/foo" and "BenchmarkGood/foo"), you can use a renaming rule to make otherwise non-matching benchmarks match.

```sh
daslang utils/benchctl/main.das -- compare \
    --old-tag before \
    --new-tag after \
    --s "BenchmarkBad=>BenchmarkGood"
```

The `--s` argument is a `from=>to` regex substitution applied to **old names** before trying to pair the names.

---

## Output format (`compare`)

For each paired benchmark:

```
BenchmarkName/SubName
    150.3 ns/op ±2%    148.1 ns/op ±1%    -1.47%    (p<0.001 n=10+10)
```

Column layout: `old value` | `new value` | `delta` | `significance`

- **Green** delta: lower values (improvement for time/memory metrics)
- **Red** delta: higher values (regression)
- `~`: result not statistically significant (p ≥ 0.05)
- **Yellow** variation: coefficient of variation exceeds 5% (noisy benchmark)

A geometric mean summary across all benchmarks is printed at the end.

Some general advice:

* You want the `p` to be as low as possible (p<0.001 is perfect)
* You want the `n` to be around 10 or higher, **5 is the bare minimum**
* You want the sample variation (±value) to be less than 5%

If all of the above are satisfied, you can generally consider the results legit and stable.

---

## Statistical methods

| Method | Description |
|--------|-------------|
| Outlier filtering | Tukey's fences (1.5 × IQR) applied before any statistics |
| Mean | Arithmetic mean of per-operation values |
| Variation | Coefficient of variation (stddev / mean); highlighted yellow if > 5% |
| Significance | Welch's t-test; p < 0.05 is considered significant |
| Summary | Geometric mean of per-benchmark means |

A minimum of 5 samples per benchmark is required for statistics to be computed.

## Typical workflow

```sh
# 1. Initialize the database (once, or to wipe existing data).
# You can usually skip this step as it can be executed implictly,
# but it's a good place to test whether your benchctl is working properly.
daslang utils/benchctl/main.das -- reset

# 2. Run benchmarks on the baseline and insert results
daslang dastest/dastest.das -- --test mybench.das --bench --bench-format json | tee old.json
daslang utils/benchctl/main.das -- insert --tag before old.json

# 3. Make your changes, then run benchmarks again
daslang dastest/dastest.das -- --test mybench.das --bench --bench-format json | tee new.json
daslang utils/benchctl/main.das -- insert --tag after new.json

# 4. Compare the two runs
daslang utils/benchctl/main.das -- compare --old-tag before --new-tag after
```
