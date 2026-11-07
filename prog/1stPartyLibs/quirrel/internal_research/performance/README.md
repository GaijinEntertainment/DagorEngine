# Quirrel performance research (internal)

This folder is for internal research. It is not a part of the Quirrel sources or of the public documentation, so
do not link to it from `quirrel/doc` or `quirrel/bench`.

## Quirrel against Lua and Luau

`run_vm_bench.py` runs Quirrel against Lua 5.5 and Luau (`-O0`, `-O1`, `-O2`) on paired workloads with the same
algorithms: general interpreter loads (fib, binarytrees, life, mandel, strings) and daRg-UI-shaped loads
(desc_churn, probe_storm, nullable_probe, closure_storm, method_calls).

| Part | Where |
| --- | --- |
| Quirrel ports | `../../quirrel/bench/quirrel/<name>.nut` (vmperf runs them too) |
| Lua and Luau ports | `lua/<name>.lua`, `luau/<name>.luau`; timing helpers `lua/vm_profile.lua` (`lua.exe` finds a `profile.lua` next to it first) and `luau/profile.luau` |
| interpreters | the release csq of this tree; `lua.exe` and `luau.exe` of `../../quirrel/bench` |
| last results | `vm_bench.json` |

Each workload prints five in-process reps as `BENCH <name> <ms> ms (r=<result>)` lines. The harness takes the best
rep of each process and the median over `--runs` processes, checks that each VM computed the same result, and
prints a ratio table (Quirrel over the other VM, so above 1.0 means Quirrel is slower). It pins to logical CPU 2
(`--cpu N` another CPU, `--cpu -1` no pin) and writes the CPU into `cpu` of the result file.

    python run_vm_bench.py                        # measures the release csq
    python run_vm_bench.py --quirrel <other.exe>  # or another build
    python run_vm_bench.py --out try.json fib     # a subset, vm_bench.json stays as it is

Code layout alone moves these numbers by up to 17% on some CPUs, so one run before and one after a VM change do
not show what the change does. Measure a change with `../../quirrel/bench/vmperf/vmperf.py`.

## Timing of several builds

`measure_builds.py` times csq builds against the first one on the Quirrel workloads of both suites, and on the
workloads of an extra folder, in interleaved rounds on one pinned CPU. It uses the timing helper of vmperf, so a
rep of the first build takes 200 ms or more against the 1 ms step of the clock. `stacks_report.py` makes `report.md` and `report.html`
from a template and the result files.

    python measure_builds.py --build base=a.exe --build tip=b.exe --out r.json [--extra-dir <dir>]
    python stacks_report.py <template.md> <results dir> <out dir>   # <name>.json for each {{table:<name>}};
                                                                     # vm_bench.json, vm_bench-*.json for {{luau}}

## Reports

| Folder | What |
| --- | --- |
| `reports/2026-10-21-stacks` | the fast call chain, the fixed-call series and the table stack, each against its base |
