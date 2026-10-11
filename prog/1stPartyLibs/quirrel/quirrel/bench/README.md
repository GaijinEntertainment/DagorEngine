# Quirrel benchmarks

Single home for all interpreter benchmarks. One folder per language, and each
folder holds idiomatic code for that language and the prebuilt interpreter:

- `quirrel/` - modern Quirrel (`*.nut`): `import`, `const`, `let`, `[pure]`
  functions, string interpolation. Both suites.
- `squirrel/` - Squirrel 3.2 (`*.nut`) and the prebuilt `sq3-64.exe`. The two
  languages are no longer close enough to share sources.
- `lua/` - Lua 5.5 (`<const>`, integers, bitwise operators, `table.create`) and
  the prebuilt `lua.exe` (5.5.1). Both suites.
- `luajit/` - Lua 5.1 dialect with the `bit` library and `table.new`, and the
  prebuilt `luajit.exe` (2.1 rolling), which benchmarks.py runs with `-joff`.
- `luau/` - modern Luau (type annotations, generalized iteration, compound
  assignment) and the prebuilt `luau.exe` (0.735). Both suites, each at `-O0`,
  `-O1` and `-O2`.
- `js/` - modern JavaScript (ES modules, classes, `Map`, `performance.now()`)
  and the prebuilt `qjs.exe` (QuickJS-ng 0.16.2).

Quirrel itself is not committed as a binary: both harnesses run
`tools/dagor_cdk/windows-x86_64/csq.exe`, the release build from this tree
(`jam -sConfig=rel` in `prog/tools/consoleSq`), with `--opt-closure-hoisting`,
because the game runtime turns closure hoisting on (`bindQuirrelEx`). That is the
shipped runtime configuration (clang, mimalloc); a build on the CRT heap, such as
a plain cmake `sq.exe`, is up to 2x slower on table-sweep workloads, which
misrepresents shipped performance. `--quirrel <exe>` measures another build.

## Rules for a port

`quirrel/<name>.nut` is the reference for every workload. A port in another
folder does the same work: the same sizes and loop counts, the same data shape
(named fields stay named fields), the same arithmetic in the same order, the same
deterministic generators (Park-Miller `seed * 16807 % 2147483647` and a lagged
Fibonacci in `table-sort`) and the same result. Inside that, each port uses what
an expert of its language writes for speed: constants, locals for library
functions, preallocation, the fastest idiomatic loop. Ports use the generic
container of their language only: no typed arrays, buffers or vector types.

Each workload except fibonacci-loop and float2string prints its result (see the
table below). Both harnesses compare the results of all interpreters and fail on
a mismatch, which means that a port does other work. Quirrel and Squirrel use
32-bit floats and the other four use doubles, so a float result of Quirrel or
Squirrel only has to agree to 1e-3 relative; integer results and the float
results of the double-precision interpreters must be equal.

Each folder has one timing helper, `profile.<ext>`, that every workload imports
(`import`, `require`, `dofile`, ES `import`, as the language does it).

## Workloads

| Workload | Measures | Result |
| --- | --- | --- |
| n-bodies | float arithmetic on named fields | energy -0.169083134 |
| n-bodies-arrays | the same, bodies as arrays | energy -0.169083134 |
| particles-kinematics | field update sweep over 50000 tables | 56199.300000 |
| exp-loop | native math call in a loop | 10000017.773511 |
| dictionary | string-keyed table inserts and reads | 13 |
| darg-ui-benchmark | daRg-shaped builders and tree walk | 399 |
| fibonacci-recursive | call overhead | 1346269 |
| fibonacci-loop | local arithmetic | none, the sum overflows |
| primes-loop | integer loop and modulo | 1652 |
| float2string | default float to string | none, the format differs per language |
| string2float | string to float | 5001000050.0000 |
| queen | 8 queens, array reads and recursion | 92 |
| sort | sort with a script comparator | 2147481491 |
| spectral-norm | nested loops, float arithmetic | 1.274224116 |
| tree | treap: allocation and pointer chasing | 64999 |

The float results are double-precision values after all runs of one process:
the particles and n-bodies state carries over from run to run.

## Rebuilding the committed interpreters

Each one is a release console exe that imports KERNEL32 alone, so a folder holds
the exe and the workloads and nothing else. All of them come from the upstream
release sources, built with the clang-cl in `devtools/LLVM-21.1.8` against the
`vc2022_17.14.4` headers, `/O2` and the static CRT (`/MT`; cmake builds take
`-DLUAU_STATIC_CRT=ON` and `-DCMAKE_C_FLAGS_RELEASE="/MT /O2 /Ob2 /DNDEBUG"`).
LuaJIT is the exception: `src/msvcbuild.bat static` with cl, which is the
upstream Windows recipe, and `static` is what drops `lua51.dll`. `sq3-64.exe` is
older than this recipe: a 64-bit build of the Squirrel 3.2 stable release, whose
build flags are not recorded.

Two of them must be asked for their jump-table interpreter loop, because both
gate it on `__GNUC__`, which clang-cl does not define:

- Lua takes `-DLUA_USE_JUMPTABLE=1`, the override its own guard provides.
- QuickJS has no such knob, so the line that reads
  `#if defined(EMSCRIPTEN) || defined(_MSC_VER)` in `quickjs.c` becomes
  `#if defined(EMSCRIPTEN) || (defined(_MSC_VER) && !defined(__clang__))`.
  Without it the primes workload takes 2x longer, which measures the MSVC
  command line rather than QuickJS.

LuaJIT keeps the one source change the earlier binary carried, and the perf notes
page has the diff: `os.clock()` reads the performance counter, because the CRT
`clock()` ticks at 1 ms, which is the whole time a short rep takes. Quirrel,
Squirrel and Lua 5.5 time with that 1 ms clock; Luau and QuickJS have a
high-resolution one.

Two harnesses run different slices of these sources, to compare Quirrel with
the other languages. To measure what a Quirrel change does, use `vmperf/` (see
`vmperf/README.md`): code layout alone moves the VM by up to 17%, so one run
before and one after the change show nothing.

## benchmarks.py - cross-language documentation suite

Compares Quirrel, Squirrel 3.2, Lua 5.5, LuaJIT (-joff), Luau at -O0, -O1 and
-O2, and QuickJS on the 15 workloads above. Each workload times itself (best of
10-20 in-process iterations) and prints
`"<name>", <seconds>, <iterations>[, <result>]`. queen solves the board 50 times
inside one timed run and reports the time of a single board, because one board
takes about as long as the coarsest clock here ticks: 20 timed runs of 50 boards
take one to four seconds per interpreter and still resolve the row to a percent.
Results are written to `../doc/content/_bench.json`, which the documentation's
Performance page renders; commit it when refreshing published numbers.

Every interpreter is pinned to one logical CPU. The mask is set on the harness,
which each interpreter inherits at creation; `--cpu N` picks the core (2 by
default, a P-core on Intel hybrid parts, where they come first) and `--no-pin`
measures without one. The core alone decides tens of percent: on an Intel hybrid
part n-bodies took 0.87s on a P-core, 1.34s with the sibling hyperthread busy
and 1.88s on an E-core.

Each cell is `--runs` whole processes and the fastest is published. Best of N
iterations inside one process cannot see a process that was slow for its whole
life, and what makes a whole process slow - a shared core, the code alignment that
process happened to get - only adds time, so the fastest run is the reproducible
one; run_vm_bench.py publishes the median of its runs instead. A cell whose runs
spread more than 5% says so, as long as they also differ by more than 5 ms: on a
row of tens of milliseconds the clock alone is worth percents. Such a row is worth
measuring again on a quiet machine with `-u`, which keeps the lower of the
committed and the new time of every cell it measures, for the same reason. So
`-u` can only make a cell faster: after an interpreter change that can make a row
slower, bump its label or run without `-u`.

    python benchmarks.py             # run everything, update the doc side-car
    python benchmarks.py -l Quirrel-4.43.1 -t queen sort   # subset
    python benchmarks.py -u -l Lua-5.5.1 -t tree   # re-measure, keep the lowest
    python benchmarks.py -r /tmp/try.json   # trial run, side-car untouched
    python benchmarks.py --quirrel <csq.exe>   # another Quirrel build

Refresh a renamed interpreter with a full run, not with `-u`: `-u` keeps the rows
this run did not measure, so the old label would stay on the page beside the new
one. `../doc/gen/check.py` fails on exactly that. `-u` also writes this run's
machine and run count over every row it keeps, so it warns when the kept rows were
measured with another CPU, platform or `--runs`; a changed machine or protocol
wants a full run.

When the interpreter version changes, bump the Quirrel row label in
benchmarks.py so committed results state what was measured; the harness warns
when the label and `csq --sqversion` differ.

## run_vm_bench.py - VM against Lua and Luau

Head-to-head Quirrel vs Lua 5.5 and Luau (-O0, -O1, -O2) on paired workloads with
identical algorithms (`quirrel/<name>.nut`, `lua/<name>.lua`, `luau/<name>.luau`):
general interpreter loads (fib, binarytrees, life, mandel, strings) and
daRg-UI-shaped loads (desc_churn, probe_storm, nullable_probe, closure_storm,
method_calls). Workloads self-report five in-process reps as
`BENCH <name> <ms> ms (r=<result>)` lines; the harness takes the best rep per
process, median over `--runs` processes, checks that every VM computed the same
result, and prints a ratio table.

    python run_vm_bench.py                        # measures the release csq
    python run_vm_bench.py --quirrel <other.exe>  # or any other build

Lua/Luau default to the prebuilt binaries above. A snapshot of results is
committed at `../doc/content/_vm_bench.json`, which the Performance page renders.

## run.bat

Each folder has a `run.bat` that runs the documentation suite once with that
interpreter, unpinned, and `run_all.bat` calls all of them. Use it to check a
port; measure with benchmarks.py. `quirrel/run.bat` takes the interpreter from
`CSQ` when it is set.
