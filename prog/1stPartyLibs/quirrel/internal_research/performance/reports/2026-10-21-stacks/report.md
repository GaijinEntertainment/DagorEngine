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

| workload | base ms | base-again | 804156 | 804157 | 804158 | 804159 | 804160 | spread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **documentation suite** | | | | | | | | |
| n-bodies | 1512.00 | +0.0% | -2.8% | +1.3% | +10.3% | +11.9% | +10.1% | 4% |
| n-bodies-arrays | 1762.00 | +0.6% | -3.4% | -4.3% | +1.6% | -2.0% | +1.0% | 2% |
| particles-kinematics | 470.00 | +0.2% | -3.8% | +2.3% | +1.9% | +1.9% | +3.6% | 4% |
| exp-loop | 550.00 | +0.2% | -18.0% | -19.1% | -20.7% | -18.4% | -19.5% | 3% |
| dictionary | 731.00 | -0.0% | +0.1% | +0.7% | +0.5% | +0.4% | +0.7% | 1% |
| darg-ui-benchmark | 205.00 | +0.0% | +0.5% | -2.9% | -8.3% | -8.3% | -10.7% | 1% |
| fibonacci-recursive | 189.00 | -0.3% | +0.0% | -11.1% | -31.5% | -33.6% | -34.7% | 1% |
| fibonacci-loop | 81.33 | -0.4% | +8.6% | +6.6% | +1.6% | +7.0% | +9.4% | 5% |
| primes-loop | 196.00 | -1.5% | +5.4% | +9.4% | +4.6% | +16.1% | +6.6% | 5% |
| float2string | 195.50 | +0.3% | +0.0% | -2.3% | -3.6% | -12.3% | -12.0% | 2% |
| string2float | 143.50 | -0.3% | -6.3% | +0.3% | -12.2% | -10.1% | -10.1% | 3% |
| queen | 158.50 | -0.6% | +1.6% | +0.0% | -10.1% | -2.5% | -5.4% | 3% |
| sort | 84.00 | -0.4% | +12.7% | +6.7% | +8.3% | +15.5% | -6.0% | 1% |
| spectral-norm | 933.00 | -0.9% | -0.9% | -0.3% | -17.8% | -9.9% | -18.0% | 2% |
| tree | 1097.00 | -0.4% | -0.7% | -6.4% | -16.0% | -14.5% | -16.2% | 2% |
| **VM suite** | | | | | | | | |
| fib | 113.00 | +0.9% | +0.4% | -8.0% | -30.1% | -30.1% | -31.9% | 1% |
| binarytrees | 34.00 | -1.0% | -1.0% | -2.9% | -18.1% | -17.2% | -19.6% | 2% |
| life | 106.00 | +0.5% | +0.5% | +0.0% | -2.8% | +0.9% | +0.9% | 1% |
| mandel | 79.67 | -0.4% | +2.1% | +0.4% | +6.7% | +5.9% | +5.4% | 2% |
| strings | 90.00 | +0.0% | -3.3% | -0.4% | +0.4% | -3.0% | -2.2% | 1% |
| desc_churn | 127.00 | -0.8% | -2.0% | -4.7% | -5.1% | -7.1% | -7.1% | 2% |
| probe_storm | 101.33 | -3.0% | -2.6% | -3.0% | -2.6% | -4.6% | -3.3% | 6% |
| nullable_probe | 105.00 | -0.5% | -3.8% | -0.5% | +1.4% | +5.7% | +1.0% | 3% |
| closure_storm | 117.50 | -0.4% | +2.6% | -3.8% | -13.2% | -14.5% | -13.2% | 1% |
| method_calls | 513.00 | +0.6% | +6.2% | -2.9% | -19.1% | -11.1% | -10.5% | 3% |
| **workloads of --extra-dir** | | | | | | | | |
| array8_map_filter_reduce_chain | 137.50 | +0.0% | -4.7% | -10.2% | -12.4% | -6.9% | -16.7% | 1% |
| array_apply | 50.75 | +0.5% | -10.8% | -12.8% | -22.7% | -13.8% | -17.2% | 3% |
| array_each | 67.00 | -0.5% | -2.5% | -6.5% | -9.5% | +2.0% | -13.9% | 1% |
| array_each_findindex | 13.93 | +0.5% | -1.9% | -7.2% | -8.1% | -5.7% | -12.0% | 1% |
| array_filter | 55.50 | -0.5% | +4.5% | -5.4% | -9.0% | +8.1% | -11.3% | 5% |
| array_findindex_last | 62.00 | +0.0% | -2.8% | -12.9% | -8.1% | -13.3% | +2.0% | 1% |
| array_findvalue_middle | 30.86 | -0.5% | +2.8% | -10.6% | -8.3% | -12.5% | -13.9% | 3% |
| array_map | 54.50 | -0.5% | -15.1% | -24.3% | -26.6% | -21.1% | -27.1% | 4% |
| array_map_apply_filter_reduce | 26.50 | +0.0% | -8.0% | -15.6% | -21.7% | -15.6% | -20.3% | 1% |
| array_map_filter_reduce | 136.50 | +0.0% | -6.2% | -15.0% | -18.3% | -9.2% | -19.4% | 3% |
| array_map_filter_reduce_chain | 136.50 | +0.4% | -6.2% | -14.7% | -17.6% | -8.8% | -19.0% | 2% |
| array_map_helper_call | 77.00 | -0.4% | -4.8% | -17.3% | -15.2% | -4.3% | -15.6% | 1% |
| array_map_instance_get | 109.50 | -0.5% | +5.0% | -13.7% | -13.7% | -11.0% | -14.6% | 1% |
| array_reduce | 54.25 | -0.5% | -12.4% | -20.3% | -30.9% | -25.8% | -32.7% | 3% |
| call_typed | 25.50 | -0.5% | -0.5% | -9.8% | -6.9% | -2.9% | -10.8% | 3% |
| join_filter | 2.74 | +0.7% | +24.1% | +11.7% | -2.6% | -1.1% | -4.7% | 12% |
| native_calls | 77.67 | -0.4% | +7.3% | -6.0% | -3.9% | -11.6% | -12.9% | 2% |
| table_map_filter_reduce | 179.50 | +0.0% | -3.3% | -3.6% | -5.6% | -5.0% | -11.4% | 1% |
| table_map_filter_reduce_findvalue | 7.38 | +0.5% | +0.0% | +0.0% | +1.9% | -6.1% | -11.2% | 1% |

### Fixed-call series

| workload | base ms | 803099 | 803100 | 804165 | 803363 | 804166 | spread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **documentation suite** | | | | | | | |
| n-bodies | 1503.00 | -0.1% | -0.5% | -0.2% | +0.7% | +0.6% | 1% |
| n-bodies-arrays | 1741.00 | +0.2% | -0.2% | -0.3% | +1.1% | +0.2% | 4% |
| particles-kinematics | 476.00 | -0.4% | +0.6% | -0.2% | +1.3% | +0.4% | 3% |
| exp-loop | 451.00 | +0.9% | +1.1% | +0.2% | +0.7% | +0.2% | 1% |
| dictionary | 734.00 | +0.0% | -0.4% | -0.5% | -0.8% | -0.5% | 2% |
| darg-ui-benchmark | 210.00 | -3.3% | -2.4% | -3.3% | -2.9% | -2.4% | 3% |
| fibonacci-recursive | 194.50 | -1.0% | -1.3% | -0.8% | -1.0% | -1.0% | 3% |
| fibonacci-loop | 90.67 | +0.4% | +1.5% | +0.7% | +0.4% | +0.0% | 3% |
| primes-loop | 205.00 | -2.0% | -1.0% | +0.0% | +0.0% | +0.5% | 9% |
| float2string | 191.00 | -0.5% | -0.8% | -0.5% | -0.3% | -1.0% | 1% |
| string2float | 127.50 | +12.5% | +14.9% | +16.9% | +5.9% | +18.0% | 3% |
| queen | 163.50 | -0.3% | -0.3% | +0.0% | -0.6% | -0.9% | 2% |
| sort | 92.00 | -23.9% | -23.6% | -23.9% | -22.8% | -24.3% | 1% |
| spectral-norm | 992.00 | -3.2% | -4.3% | -4.1% | -3.6% | -5.3% | 2% |
| tree | 1094.00 | -1.6% | -0.9% | -2.3% | -0.3% | -1.2% | 2% |
| **VM suite** | | | | | | | |
| fib | 119.00 | -0.8% | -0.8% | -1.3% | -1.3% | -1.7% | 1% |
| binarytrees | 32.43 | +0.0% | +0.0% | -0.4% | +0.0% | +0.0% | 2% |
| life | 102.00 | -0.5% | +0.0% | -0.5% | +0.0% | -0.5% | 2% |
| mandel | 81.33 | +0.0% | +0.8% | +0.0% | +0.0% | +0.0% | 3% |
| strings | 87.67 | +0.8% | -0.8% | -0.8% | -0.4% | +0.8% | 3% |
| desc_churn | 124.00 | +0.4% | +0.4% | +0.0% | +3.2% | +1.2% | 1% |
| probe_storm | 97.33 | +0.0% | +0.3% | +0.7% | +0.0% | +0.0% | 7% |
| nullable_probe | 102.00 | -1.5% | -0.5% | -1.0% | -1.0% | -1.5% | 2% |
| closure_storm | 123.50 | +0.8% | +0.8% | +1.2% | +0.8% | +0.4% | 1% |
| method_calls | 518.00 | +5.0% | +4.6% | +5.0% | +4.8% | +4.8% | 1% |
| **workloads of --extra-dir** | | | | | | | |
| array8_map_filter_reduce_chain | 130.50 | +2.3% | -12.6% | -1.9% | -16.1% | -10.0% | 2% |
| array_apply | 51.75 | -21.3% | -31.4% | -3.9% | -30.4% | -29.0% | 4% |
| array_each | 70.33 | +1.9% | +1.9% | -9.5% | -7.6% | -17.1% | 2% |
| array_each_findindex | 14.47 | +0.0% | +0.9% | -9.2% | -12.9% | -16.6% | 1% |
| array_filter | 52.25 | -2.4% | +0.0% | +12.0% | -11.0% | -10.5% | 6% |
| array_findindex_last | 64.75 | -3.1% | -2.3% | -8.5% | -22.4% | -21.2% | 1% |
| array_findvalue_middle | 34.00 | -7.4% | -7.4% | -7.4% | -25.5% | -25.0% | 4% |
| array_map | 51.50 | +17.5% | -30.1% | -7.3% | -30.1% | -16.0% | 17% |
| array_map_apply_filter_reduce | 25.88 | -3.4% | -25.1% | -3.9% | -26.1% | -21.7% | 1% |
| array_map_filter_reduce | 129.00 | +2.3% | -17.4% | -0.4% | -22.1% | -14.7% | 2% |
| array_map_filter_reduce_chain | 128.50 | +3.1% | -17.1% | +0.8% | -21.8% | -13.6% | 1% |
| array_map_helper_call | 73.33 | +1.4% | -17.7% | -4.5% | -16.4% | -0.5% | 1% |
| array_map_instance_get | 108.50 | +0.9% | -6.0% | -0.9% | -8.8% | -5.5% | 1% |
| array_reduce | 49.00 | -19.2% | -31.4% | -8.2% | -32.2% | -26.9% | 8% |
| call_typed | 26.62 | -0.5% | -0.5% | -0.5% | -0.9% | -0.5% | 1% |
| join_filter | 2.74 | -1.1% | +1.5% | +12.8% | +0.7% | -21.9% | 7% |
| native_calls | 73.33 | +0.5% | +1.4% | +1.8% | +4.1% | +0.0% | 2% |
| table_map_filter_reduce | 170.50 | +5.0% | +6.7% | +5.0% | -13.2% | -10.0% | 2% |
| table_map_filter_reduce_findvalue | 7.38 | +3.3% | +3.3% | +2.3% | -11.7% | -11.7% | 2% |

### Table stack

| workload | base ms | 803059 | 803060 | 803061 | 797123 | 803062 | 803063 | spread |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **documentation suite** | | | | | | | | |
| n-bodies | 1505.00 | +2.9% | -2.5% | +4.1% | +5.8% | +2.9% | -0.9% | 1% |
| n-bodies-arrays | 1764.00 | +0.7% | -1.8% | +5.0% | +4.4% | +5.8% | -0.7% | 1% |
| particles-kinematics | 467.00 | +6.4% | -1.5% | +9.6% | +3.4% | +2.8% | +2.1% | 2% |
| exp-loop | 457.00 | +7.0% | -1.3% | -2.2% | -3.7% | -3.1% | -2.8% | 3% |
| dictionary | 749.00 | -1.5% | -2.8% | +1.2% | -2.5% | -8.0% | -9.7% | 2% |
| darg-ui-benchmark | 209.00 | +0.5% | -3.8% | -1.0% | -0.5% | -3.8% | -5.7% | 4% |
| fibonacci-recursive | 201.00 | -3.0% | -8.0% | -3.0% | -0.5% | -1.0% | -3.0% | 3% |
| fibonacci-loop | 102.00 | -5.4% | -16.7% | -3.9% | +2.0% | +4.4% | -15.2% | 5% |
| primes-loop | 242.00 | -4.1% | -16.1% | -1.2% | +0.0% | +3.7% | -16.5% | 2% |
| float2string | 192.50 | +3.9% | +4.4% | +5.2% | +7.0% | +6.2% | +8.6% | 1% |
| string2float | 141.50 | +3.5% | -1.1% | +3.2% | +6.0% | +6.7% | +4.9% | 2% |
| queen | 177.00 | -4.5% | -6.8% | -4.0% | -2.5% | -2.3% | -5.9% | 1% |
| sort | 211.00 | -21.3% | -16.1% | -14.2% | -19.9% | -13.3% | -9.5% | 5% |
| spectral-norm | 985.00 | -1.3% | -5.7% | -0.8% | -0.5% | -0.9% | -3.8% | 2% |
| tree | 1134.00 | -1.9% | -4.4% | -3.2% | -3.4% | -4.0% | -2.1% | 7% |
| **VM suite** | | | | | | | | |
| fib | 123.50 | -3.2% | -8.1% | -2.8% | -1.6% | -1.6% | -4.0% | 2% |
| binarytrees | 34.17 | +0.5% | -5.4% | -2.4% | -2.9% | -4.4% | -7.8% | 2% |
| life | 111.50 | -1.8% | -5.8% | -0.9% | -3.6% | -4.0% | -7.2% | 1% |
| mandel | 85.00 | +3.1% | -5.5% | +4.3% | +5.5% | +6.7% | -1.2% | 2% |
| strings | 90.33 | -2.2% | -2.2% | -2.2% | -2.2% | -2.2% | -3.0% | 1% |
| desc_churn | 129.00 | +5.4% | -2.7% | +0.0% | +1.9% | -2.7% | -12.4% | 2% |
| probe_storm | 100.00 | +0.0% | -2.5% | +0.0% | -2.0% | -2.5% | +6.0% | 8% |
| nullable_probe | 111.50 | -2.7% | -21.1% | -18.4% | -19.3% | -18.8% | -22.4% | 5% |
| closure_storm | 128.00 | +3.5% | -8.6% | -1.6% | -2.0% | -0.8% | -10.5% | 3% |
| method_calls | 553.00 | -2.5% | -17.4% | -12.3% | -8.3% | -14.8% | -17.0% | 2% |
| **workloads of --extra-dir** | | | | | | | | |
| array8_map_filter_reduce_chain | 126.50 | +3.6% | -2.8% | +5.5% | +1.6% | +1.6% | +5.1% | 3% |
| array_apply | 42.20 | +4.3% | -0.5% | +9.0% | +8.5% | +6.6% | +1.4% | 6% |
| array_each | 71.67 | -6.0% | -12.6% | -13.0% | +0.0% | -4.2% | -3.7% | 13% |
| array_each_findindex | 14.79 | -5.3% | -9.7% | -10.6% | -6.3% | -5.8% | -7.7% | 16% |
| array_filter | 53.00 | +0.5% | -1.9% | +1.9% | +1.9% | +3.3% | +6.1% | 3% |
| array_findindex_last | 67.00 | -3.5% | -4.0% | -14.4% | -12.4% | -10.0% | -6.0% | 4% |
| array_findvalue_middle | 33.67 | -2.0% | -13.9% | -14.9% | -12.9% | -4.0% | -6.4% | 2% |
| array_map | 45.20 | -2.2% | -5.8% | +14.6% | -0.4% | -2.2% | +12.4% | 5% |
| array_map_apply_filter_reduce | 22.22 | +5.0% | +1.0% | +9.5% | +5.5% | +4.5% | +7.5% | 3% |
| array_map_filter_reduce | 121.50 | +3.3% | -2.9% | +6.6% | +0.4% | +0.8% | +5.8% | 2% |
| array_map_filter_reduce_chain | 122.00 | +3.3% | -2.5% | +6.6% | +1.6% | +1.2% | +5.3% | 3% |
| array_map_helper_call | 70.67 | -7.5% | -7.5% | +5.7% | -0.9% | -3.3% | -1.4% | 3% |
| array_map_instance_get | 101.50 | +1.5% | +0.0% | +6.9% | +4.4% | +4.4% | +8.4% | 1% |
| array_reduce | 49.00 | -12.2% | -15.1% | -13.1% | -10.2% | -12.2% | -17.6% | 4% |
| call_typed | 27.38 | -1.4% | -6.8% | -0.9% | +2.3% | +1.4% | -4.1% | 5% |
| join_filter | 2.72 | +13.2% | +16.9% | +19.9% | +1.5% | +2.9% | -0.7% | 2% |
| native_calls | 76.00 | +4.4% | +5.3% | +20.6% | +11.8% | +9.6% | +21.9% | 2% |
| table_map_filter_reduce | 176.50 | -0.3% | -5.1% | +0.3% | -1.1% | +1.7% | +4.2% | 2% |
| table_map_filter_reduce_findvalue | 7.48 | -2.8% | -4.1% | -3.7% | -5.1% | -0.5% | +0.9% | 1% |

## Quirrel against Lua and Luau

`run_vm_bench.py` (CPU 2 pinned, 3 processes, the median of the best rep of each; the clock steps by 1 ms). The first
column is 4.44.1 of master (`data/vm_bench.json`, a copy of `../../vm_bench.json` of 2026-10-21); the other Quirrel columns are the bases and tips above. A ratio is
Quirrel time over the other VM, so above 1.0 means Quirrel is slower: next to a Quirrel time it is over Luau -O2 of
the same run, in the Lua and Luau columns (the times of the 4.44.1 run) it is of 4.44.1.

| workload | Quirrel 4.44.1 ms | Quirrel call-base ms | Quirrel call-804160 ms | Quirrel fixed-base ms | Quirrel fixed-804166 ms | Quirrel table-base ms | Quirrel table-803063 ms | lua | luau-O0 | luau-O1 | luau-O2 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| fib | 126.0 (2.37x) | 114.0 (2.18x) | 76.0 (1.47x) | 122.0 (2.33x) | 117.0 (2.26x) | 122.0 (2.36x) | 118.0 (2.27x) | 55.0 (2.29x) | 55.5 (2.27x) | 52.7 (2.39x) | 53.2 (2.37x) |
| binarytrees | 34.0 (2.35x) | 34.0 (2.42x) | 28.0 (1.98x) | 33.0 (2.31x) | 32.0 (2.25x) | 34.0 (2.38x) | 31.0 (2.16x) | 49.0 (0.69x) | 18.5 (1.83x) | 14.5 (2.34x) | 14.4 (2.35x) |
| life | 111.0 (2.87x) | 104.0 (2.73x) | 106.0 (2.80x) | 102.0 (2.67x) | 102.0 (2.70x) | 109.0 (2.88x) | 102.0 (2.70x) | 56.0 (1.98x) | 51.3 (2.17x) | 38.6 (2.87x) | 38.7 (2.87x) |
| mandel | 91.0 (3.47x) | 80.0 (3.08x) | 84.0 (3.28x) | 83.0 (3.15x) | 81.0 (3.17x) | 83.0 (3.23x) | 85.0 (3.32x) | 31.0 (2.94x) | 29.4 (3.09x) | 27.3 (3.34x) | 26.2 (3.47x) |
| strings | 91.0 (1.10x) | 91.0 (1.11x) | 88.0 (1.06x) | 91.0 (1.06x) | 89.0 (1.09x) | 89.0 (1.09x) | 88.0 (1.08x) | 96.0 (0.95x) | 90.4 (1.01x) | 86.1 (1.06x) | 82.5 (1.10x) |
| desc_churn | 128.0 (2.35x) | 127.0 (2.35x) | 120.0 (2.22x) | 131.0 (2.36x) | 126.0 (2.32x) | 128.0 (2.38x) | 112.0 (2.06x) | 228.0 (0.56x) | 82.0 (1.56x) | 55.8 (2.29x) | 54.5 (2.35x) |
| probe_storm | 100.0 (1.58x) | 98.0 (1.55x) | 97.0 (1.55x) | 101.0 (1.59x) | 97.0 (1.55x) | 100.0 (1.60x) | 105.0 (1.68x) | 98.0 (1.02x) | 63.3 (1.58x) | 63.2 (1.58x) | 63.4 (1.58x) |
| nullable_probe | 112.0 (4.54x) | 105.0 (4.25x) | 106.0 (4.31x) | 104.0 (4.05x) | 100.0 (4.07x) | 111.0 (4.53x) | 87.0 (3.54x) | 43.0 (2.60x) | 24.7 (4.54x) | 24.7 (4.54x) | 24.6 (4.54x) |
| closure_storm | 123.0 (3.29x) | 117.0 (3.12x) | 102.0 (2.73x) | 132.0 (3.51x) | 125.0 (3.35x) | 126.0 (3.37x) | 115.0 (3.09x) | 262.0 (0.47x) | 68.5 (1.80x) | 55.7 (2.21x) | 37.4 (3.29x) |
| method_calls | 516.0 (2.15x) | 512.0 (2.14x) | 458.0 (1.91x) | 513.0 (2.15x) | 539.0 (2.26x) | 545.0 (2.29x) | 466.0 (1.95x) | 305.0 (1.69x) | 241.9 (2.13x) | 241.3 (2.14x) | 239.9 (2.15x) |

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
