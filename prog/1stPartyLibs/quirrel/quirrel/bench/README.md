# Quirrel benchmarks

Single home for all interpreter benchmarks. Sources are grouped per language:

- `quirrel/` - Quirrel workloads (`*.nut`), plus prebuilt `sq3-64.exe`
  (Squirrel 3.1 baseline) used by the docs suite
- `lua/` - Lua workloads, plus prebuilt `lua.exe` (5.4.6) and `luajit.exe`
- `luau/` - Luau ports of the docs workloads, plus prebuilt `luau.exe`
- `js/` - JavaScript ports, plus prebuilt `qjs.exe` (QuickJS)

Quirrel itself is not committed as a binary: both harnesses run the release
csq built from this repo (`jam -sConfig=rel` in the consoleSq
tool sources; deploys to `tools/dagor_cdk/<platform>/csq.exe`). That is the shipped
runtime configuration (mimalloc); a plain cmake `sq.exe` sits on the CRT
heap and is up to 2x slower on table-sweep workloads (50k-table particles
sweep: 0.20s vs 0.11s), which misrepresents shipped performance.

Two harnesses run different slices of these sources:

## benchmarks.py - cross-language documentation suite

Compares Quirrel, Squirrel 3.1, Lua 5.4, LuaJIT (-joff), Luau and QuickJS on
the classic workloads (nbodies, particles, fib, primes, dict, darg, ...).
Each workload times itself (best of 10-20 in-process iterations) and prints
`"<name>", <seconds>, <iterations>`. Results are written to
`../doc/source/performance/results.json` and rendered to `results.rst` for the
documentation; commit both when refreshing published numbers.

    python benchmarks.py             # run everything, update doc results
    python benchmarks.py -l Quirrel-4.35.1 -t queen sort   # subset
    python benchmarks.py --only_rst  # re-render rst from committed json

When the interpreter version changes, bump the Quirrel row label in
benchmarks.py so committed results state what was measured.

## run_vm_bench.py - VM acceptance harness

Head-to-head Quirrel (csq) vs Lua 5.4 and Luau (-O2) on paired workloads with
identical algorithms (`quirrel/<name>.nut` and `lua/<name>.lua`): general
interpreter loads (fib, binarytrees, life, mandel, strings) and daRg-UI-shaped
loads (desc_churn, probe_storm, nullable_probe, closure_storm, method_calls).
Workloads self-report five in-process reps as `BENCH <name> <ms>` lines; the
harness takes the best rep per process, median over `--runs` processes, and
prints a ratio table. Run it before and after any interpreter change.

    python run_vm_bench.py --csq <path-to-release-csq.exe>

Lua/Luau default to the prebuilt binaries above. A snapshot of results is
committed at `../doc/source/performance/vm_bench_results.json`.
