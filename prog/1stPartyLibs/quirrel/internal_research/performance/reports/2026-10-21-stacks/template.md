# Quirrel VM stacks: fast call, fixed call, tables (2026-10-21)

Three open Gerrit stacks, each measured against the master commit it is based on:

- Fast call chain: 804156, 804157, 804158, 804159, 804160 (base d7def63a2287).
- Fixed-call series: 803099, 803100, 804165, 803363, 804166 (base 1f6dd78f795e). This is the Gerrit order: 803099
  and 803100 are under 804165 and 803363.
- Table stack: 803059, 803060, 803061, 797123, 803062, 803063 (base c4714ee1c0bd). On Gerrit the stack also has
  803058 (sqtable.md, no code, under 803059) and 797123 (a table slot is 32 bytes, between 803061 and 803062).
  803062 is built on 797123, so 797123 is a measured point too.

Each stack is measured on its own base, not on 4.44.1: master merged f2ae9bea8e83 (a proto without typed
parameters keeps no type mask array) after the base of the fast call chain, and 804156 then reads a null mask
array. A port to 4.44.1 would measure a merge of mine, not the changes. The three stacks together are not measured.

## How it was measured

- Builds: csq of each point, `jam -sConfig=rel -sPlatformSpec=clang -sUseWholeOpt=yes -sPlatformArch=x86_64` in
  the consoleSq tool (clang 21.1.8, LTO, mimalloc). The game builds also link with LTO, but these builds keep the
  float mode and SSE level of the tools (`-fp:precise`, SSE2; no `-sGameCodegen=yes`), so they are not the code of a
  game build: vmperf README says that each of the two changes the code of the VM. `data/builds.txt`
  lists each point with its sha. The test suite (`testRunner.py`, dev sq) passes on the three tips.
- Machine: Threadripper 3970X, Windows 10, idle: no game and no build ran during the runs (a load log every 30 s,
  total load 5% to 10%, of which 1.6% is the measured process).
- `measure_builds.py`: CPU 2 pinned, 3 interleaved rounds, the order of the builds turns from round to round. Each
  process runs a workload with the timing helper of vmperf: 5 reps of N calls (N set for at least 200 ms a rep of the base), the fastest rep.
  The tables give the fastest round in ms for one call, the delta of each point against the base, and the largest
  spread of the 3 rounds over the builds of that row. One call of queen solves 50 boards (`_bench.json` gives one).
- Workloads: the documentation suite (15, `bench/quirrel`), the VM suite (10, the workloads of `run_vm_bench.py`),
  and the 19 callback and call workloads of change 804629, which the commit messages of the stacks cite.
- Each build computed the same result as its base in each workload; fibonacci-loop and float2string report none.

## Noise and what it means for the numbers

- Same binary: the base of the call chain was timed twice (`base-again` column). The two agree within 3% in each
  workload, 0.5% on average. The spread of the 3 rounds is under 1% in half of all cells and under 2.7% in 90%.
- Code layout: these are plain LTO builds, one layout each, and the layout shifts of vmperf do not apply to them.
  Workloads that a change cannot reach move a lot: fibonacci-loop (an arithmetic loop, no call, no table) and
  primes-loop (a script call each iteration, no table) move from -20% to +18% between the points of the table
  stack, and 804165 (each and findindex only) makes
  array_apply 40% slower, which 803363 then takes back. Thus a step of less than about 10% on one workload is not
  proof of an effect, and the sign of a small step can be wrong. A tip against its base is also one layout sample.
  Not verified: which of the moves are layout; no layout-shifted builds were made.

## Results in short

Geometric mean of the tip against its base, by workload group:

| stack | documentation suite | VM suite | callback workloads | all 44 |
| --- | ---: | ---: | ---: | ---: |
| fast call (804160 against d7def63a2287) | -7.6% | -8.7% | -15.2% | -11.2% |
| fixed call (804166 against 1f6dd78f795e) | -1.5% | +0.3% | -14.8% | -7.1% |
| tables (803063 against c4714ee1c0bd) | -4.2% | -8.3% | +1.3% | -2.8% |

### Fast call chain

For the hypothesis (the call fast path of 804158 and the native call of 804159 make calls cheaper):

- 804158 gives the large step: fib -24%, fibonacci-recursive -23%, method_calls -17%, binarytrees -16%,
  spectral-norm -18%. At the tip: fib -32%, fibonacci-recursive -35%, binarytrees -20%, closure_storm -13%,
  method_calls -10.5%, tree -16%, darg-ui-benchmark -11%. The commit message of 804158 gives fib -33% (vmperf, layout
  means); this run gives -24% for the same step, because 804157 already made fib 8% faster here (its message gives
  -2.6%).
- The callback workloads get faster along the chain: array_map -27%, array_reduce -33%, array_map_apply_filter_reduce
  -20% at the tip. native_calls is -13% at the tip (the commit message of 804159 gives -12%).
- call_typed (804160): -11% at the tip, -8% for the step. The commit message gives -23% in relinked builds.

Against the hypothesis, or not explained:

- n-bodies is 10% slower from 804158 on (1512 to 1665 ms): it makes one script call (`advance`) a step and a
  native `sqrt` call in its inner loop, so the call changes can reach it; the cause is not known. primes-loop (a
  script call each iteration) is 7% slower, mandel 5% and fibonacci-loop 9%; mandel and fibonacci-loop make no
  call in their loops, so layout is the probable cause there; not verified.
- 804159 alone makes 20 workloads slower by 3% or more (array_filter +19%, array_each +13%), and 804160 takes them
  back. exp-loop is 18% faster from 804156 on, which only moves the default fill and the type check into helpers.
  Both look like layout.

### Fixed-call series

For the hypothesis (a container method calls its script callback through one fixed call):

- 803099: sort -24% (the commit message gives table-sort -19%), array_apply -21%, array_reduce -19%.
- 803100, for the step against 803099: array_map -40%, array_map_apply_filter_reduce -22%, array_map_filter_reduce -19%.
- 803363: table_map_filter_reduce -13%, table_map_filter_reduce_findvalue -12%, array_findvalue_middle -26% at that
  point.
- 804166: join_filter -22% for the step (the commit message gives -35%).
- At the tip: the callback workloads are 15% faster (geometric mean), sort -24%, the other suites do not change by
  more than the noise.

Against the hypothesis, or not explained:

- string2float is 12.5% to 18% slower at each point from 803099 on but 803363 (+5.9%). It calls a native once per element and no
  callback; probably layout, not verified.
- method_calls is 5% slower at each point from 803099 on, the same in the Lua and Luau run (513 to 539 ms). Not
  explained: a change in sqvm.cpp can change the register allocation of all of Execute.
- 804165 (each and findindex) makes the map, apply and reduce workloads 21% to 40% slower, 803363 takes it back;
  804166 (join only) makes array_map 20% slower again. The tip keeps part of the gain of 803100 and 803363 for map:
  array_map -16% at the tip against -30% at 803363.

### Table stack

For the hypothesis (faster table lookups and method calls):

- 803060 (a?.b reads use the literal hint): nullable_probe -19% for the step, -22% at the tip (the commit message
  gives -16%).
- method_calls -17% at the tip; most of it comes at 803060 (-15%), not at 803059, whose commit message gives -8% for
  the class member cache (measured: -2.5%).
- dictionary -10%, desc_churn -12% and binarytrees -8% at the tip; desc_churn gets its step at 803063 (-10%, the
  commit message gives -7%).

Against the hypothesis, or not explained:

- The commit message of 797123 gives particles-kinematics -15% and probe_storm -18%; here particles-kinematics is
  +3.4% at 797123 and +2.1% at the tip, and probe_storm is -2% at 797123 and +6% at the tip.
- native_calls is 22% slower at the tip, float2string 8.6% slower, string2float 5% slower, array_map 12% slower.
- 803061 (the 64-bit key filter) makes 29 workloads slower by 3% or more, primes-loop +18% and fibonacci-loop +15%
  among them, which cannot reach a table miss: layout, probably; the later points take most of it back.

The base of this stack is older than the other two: it does not have the merge sort, so sort takes 211 ms here and
84 to 92 ms in the other bases.

## Tables of all points

The delta of each column is against the base of the stack. Green: 3% or more faster; red: 3% or more slower.
The group "workloads of --extra-dir" is the 19 callback and call workloads of change 804629.

### Fast call chain

{{table:call}}

### Fixed-call series

{{table:fixed}}

### Table stack

{{table:table}}

## Quirrel against Lua and Luau

`run_vm_bench.py` (CPU 2 pinned, 3 processes, the median of the best rep of each; the clock steps by 1 ms). The first
column is 4.44.1 of master (`data/vm_bench.json`, a copy of `../../vm_bench.json` of 2026-10-21); the other Quirrel columns are the bases and tips above. A ratio is
Quirrel time over the other VM, so above 1.0 means Quirrel is slower: next to a Quirrel time it is over Luau -O2 of
the same run, in the Lua and Luau columns (the times of the 4.44.1 run) it is of 4.44.1.

{{luau}}

4.44.1 is 2% to 14% slower than the three older bases on fib, life and mandel in this run. Not explained: the code
layout, or a master change after the bases; not verified.

With the fast call chain, fib comes from 2.18 to 1.47 times the time of Luau -O2, and binarytrees from 2.42 to 1.98.
With the table stack, nullable_probe comes from 4.53 to 3.54 times and method_calls from 2.29 to 1.95. The largest
gaps stay at nullable_probe (3.5x after the table stack), mandel (3.3x), closure_storm (2.7x to 3.4x at the tips)
and life (2.7x); the stacks move mandel and life by -7% to +5%, which is in the range of the layout noise.

## Files

- `data/call.json`, `data/fixed.json`, `data/table.json`: each round of each build and workload (`measure_builds.py`).
- `data/vm_bench-*.json`: `run_vm_bench.py` of each base and tip.
- `data/builds.txt`: the point names and their shas.
- `template.md`: the source of `report.md` and `report.html` (`python ../../stacks_report.py template.md data .`).
- The exes and pdbs are in `F:\research\quirrel-perf\23-stacks\exe` on the measurement machine, not in git.
