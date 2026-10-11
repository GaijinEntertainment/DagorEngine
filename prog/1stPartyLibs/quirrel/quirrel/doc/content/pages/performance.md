---
title: Performance
group: Guides
order: 93
summary: Benchmarks against Lua, LuaJIT, Luau, QuickJS and Squirrel 3.
---

This page has two sets of benchmarks. Both are measured on Windows x64, and
both are committed with the numbers they produced.

Every row times the benchmarked code only, without interpreter startup and
without compilation. Each workload repeats the load inside one process and keeps
its best time, because the best time has the least noise from other work on the
machine.

Each language has its own port of every workload, written as an expert writes
current code in that language: modern Quirrel with `import`, `const` and pure
functions; Squirrel 3.2 as Squirrel; Lua 5.5 with constants and integers; Luau
with type annotations; modern JavaScript. All ports do the same work with the same
algorithm and the same data shapes, and use only the generic containers of their
language. Each workload except fibonacci-loop and float2string prints its result,
and the harness checks that all interpreters computed the same one. Quirrel and
Squirrel use 32-bit floats and the others use 64-bit floats, so their float
results agree to about three digits.

Luau is measured at its three optimization levels, `-O0`, `-O1` (the default)
and `-O2`, because the level changes its speed a lot on some workloads:
exp-loop is 2.7 times faster at `-O1` than at `-O0`, and spectral-norm is 1.8
times faster at `-O2` than at `-O1`.

Each cell in Benchmarks below is measured three times. The harness starts three
processes one after another, all pinned to the same logical CPU, and the page
shows the fastest of the three.

The page shows the fastest run because noise only makes a run slower. A process
can share its core with other work, land on an efficiency core, or get an
unlucky code alignment. None of these makes it faster. The core alone changes
the result by tens of percent on this hardware.

The harness also checks how far the three runs are apart. It reports a spread of
more than five percent when the runs also differ by more than five milliseconds,
because on a row that takes tens of milliseconds the clock alone accounts for a
few percent. A reported row is measured again on an idle machine.

LuaJIT is measured with the JIT off. Quirrel, Lua and QuickJS are built for
platforms where generating code at runtime is not allowed, such as consoles and
phones, so a comparison against a JIT would not be useful.

If you need the fastest interpreter, or an AOT language, use
[Daslang](https://daslang.io/#performance), which has more benchmarks of this
kind. Quirrel, Lua and JS are highly dynamic and much quicker to learn, so this
page compares those.

## Benchmarks

Shorter bars are better.

{{benchmarks}}

## Against Lua and Luau

This head-to-head harness runs Quirrel against Lua and Luau (at `-O0`, `-O1`
and `-O2`) on paired workloads with identical algorithms: general interpreter
loads (fib, binarytrees, life, mandel, strings) and daRg-UI-shaped loads
(desc_churn, probe_storm, nullable_probe, closure_storm, method_calls).

On some CPUs, code layout alone moves these numbers by up to 17%, so one run
before and one after a VM change do not show what the change does. To measure
a change, `bench/vmperf` builds each version in four code layouts and compares
the means.

Times are milliseconds. The ratio is Quirrel over the other interpreter, so a
value above 1.0 means Quirrel is slower.

{{vm_benchmarks}}

## What was built, and how

Everything is built for Windows 64-bit with clang-cl where the source allows it.
The Quirrel rows measure the release build of `csq`, the console interpreter of
the Dagor engine tree, which is the shipped runtime configuration: clang and
mimalloc. A build on the CRT heap, such as a plain cmake `sq.exe`, is up to twice
as slow on table-sweep workloads, so it would not measure shipped performance.
`csq` runs with closure hoisting on, as the game runtime does.

The third-party interpreters are committed next to their workloads. Each is a
release console exe that imports KERNEL32 only. Two of them need a change to get
their jump-table interpreter loop, because both gate it on `__GNUC__`, which
clang-cl does not define. Lua takes `-DLUA_USE_JUMPTABLE=1`. In QuickJS the line
`#if defined(EMSCRIPTEN) || defined(_MSC_VER)` becomes
`#if defined(EMSCRIPTEN) || (defined(_MSC_VER) && !defined(__clang__))`. Without
the QuickJS change the primes workload takes twice as long, which measures the
MSVC command line and not QuickJS.

LuaJIT has one source change, so that a short repetition can be timed:
`os.clock()` reads the performance counter, because the CRT `clock()` ticks once
per millisecond, which is about the time one rep takes.

## Sources

Every workload, harness and prebuilt interpreter is in `bench/` next to `doc/`
in the [Dagor engine](https://github.com/GaijinEntertainment/DagorEngine) tree,
under `prog/1stPartyLibs/quirrel/quirrel`, one folder per language:
`bench/quirrel`, `bench/squirrel`, `bench/lua`, `bench/luajit`, `bench/luau`,
`bench/js`.

- `bench/benchmarks.py` - the cross-language suite above. It writes
  `doc/content/_bench.json`, which this page renders.
- `bench/run_vm_bench.py` - the comparison with Lua and Luau above. It writes
  `doc/content/_vm_bench.json`.
- `bench/vmperf/vmperf.py` - measures one change to the VM or the compiler.

`benchmarks.py` and `run_vm_bench.py` need the Windows interpreters, so the
numbers are refreshed by hand and committed. The site itself builds anywhere. `bench/README.md` says how to run
them and how each committed binary was built.
