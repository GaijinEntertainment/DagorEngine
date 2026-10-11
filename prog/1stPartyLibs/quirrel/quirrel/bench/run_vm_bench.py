#!/usr/bin/env python3
"""Quirrel VM benchmark: Quirrel vs Lua and Luau interpreters.

Runs the paired workloads (quirrel/<name>.nut, lua/<name>.lua, luau/<name>.luau,
same algorithms) on each VM, parses the self-reported per-rep timings
("BENCH <name> <ms> ms (r=<result>)" lines), and reports the median-of-min per
workload. Luau runs at -O0, -O1 and -O2.

Each workload runs 5 reps inside one process (warmup amortized); the harness
additionally repeats whole processes (--runs) and takes the best rep per run,
median across runs - a standard interpreter-benchmark protocol that filters
both cold-start and machine noise. A VM that reports another result fails the
run and leaves the side-car alone; one that reports none is printed FAILED.

It shows where the VM stands against Lua and Luau. To measure what a VM change
does, use vmperf/vmperf.py: one build pair mixes the change with code layout.
It measures the release csq (jam -sConfig=rel in the consoleSq tool); a dev
build measures its asserts.
--lua/--luau default to the prebuilt interpreters committed next to the
workloads (see README.md).

Usage:
  python run_vm_bench.py [--quirrel <csq.exe>] [--lua <lua.exe>] [--luau <luau.exe>]
                         [--runs 3] [--out vm_bench_results.json] [bench ...]
"""

import argparse
import json
import os
import re
import statistics
import subprocess

# One definition of the machine-info envelope and result check, shared with the documentation suite.
from benchmarks import CSQ_REL, Lang, check_results, machine_info

HERE = os.path.dirname(os.path.abspath(__file__))

BENCHES = ["fib", "binarytrees", "life", "mandel", "strings",
           "desc_churn", "probe_storm", "nullable_probe", "closure_storm",
           "method_calls"]

BENCH_RE = re.compile(r"BENCH (\S+) ([0-9.]+) ms \(r=([^)]*)\)")

# The doc site reads this side-car and renders the ratio table; see the
# {{vm_benchmarks}} directive in ../doc/gen/render.py.
RESULTS = os.path.normpath(os.path.join(HERE, "..", "doc", "content", "_vm_bench.json"))


def run_one(cmd, cwd):
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=cwd, timeout=300)
    reps = BENCH_RE.findall(proc.stdout)
    if not reps:
        raise RuntimeError(f"no BENCH output from {' '.join(cmd)} (rc={proc.returncode})\n{proc.stdout}\n{proc.stderr}")
    return min(float(ms) for _, ms, _ in reps), reps[-1][2]


def bench_vm(vm, bench, runs):
    best, result = [], None
    for _ in range(runs):
        ms, result = run_one(vm.cmd + [bench + vm.ext], os.path.join(HERE, vm.folder))
        best.append(ms)
    return statistics.median(best), result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("benches", nargs="*")
    ap.add_argument("--quirrel", "--sq", default=CSQ_REL,
                    help="path to the release csq (default: tools/dagor_cdk/windows-x86_64/csq.exe)")
    ap.add_argument("--lua", help="path to the lua interpreter (default: committed lua/lua.exe)")
    ap.add_argument("--luau", help="path to luau CLI (default: committed luau/luau.exe)")
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--out", default=RESULTS,
                    help="side-car to write; a path of your own leaves the committed one alone")
    args = ap.parse_args()

    if not os.path.exists(args.quirrel):
        raise SystemExit(f"{args.quirrel} is not there; build it with "
                         "jam -sConfig=rel in the consoleSq tool (see README.md)")

    lua = os.path.abspath(args.lua) if args.lua else os.path.join(HERE, "lua", "lua.exe")
    luau = os.path.abspath(args.luau) if args.luau else os.path.join(HERE, "luau", "luau.exe")

    vms = [Lang("quirrel", "quirrel", ".nut", [os.path.abspath(args.quirrel), "--opt-closure-hoisting"], True),
           Lang("lua", "lua", ".lua", [lua], False)]
    vms += [Lang(f"luau{level}", "luau", ".luau", [luau, level], False) for level in ("-O0", "-O1", "-O2")]

    benches = args.benches if args.benches else BENCHES
    table = {}
    mismatches = []
    for bench in benches:
        table[bench] = {}
        computed = []
        for vm in vms:
            try:
                ms, result = bench_vm(vm, bench, args.runs)
                table[bench][vm.label] = ms
                computed.append((vm, result))
                print(f"{bench:>16} {vm.label:>14} {ms:10.2f} ms  r={result}", flush=True)
            except Exception as e:
                table[bench][vm.label] = None
                print(f"{bench:>16} {vm.label:>14}      FAILED: {e}", flush=True)
        mismatches.extend(check_results(bench, computed))

    print("\n=== summary (ms, median of best-rep; ratio vs quirrel) ===")
    vm_names = [v.label for v in vms]
    print(f"{'bench':>16}" + "".join(f"{n:>22}" for n in vm_names))
    for bench in benches:
        row = f"{bench:>16}"
        q = table[bench].get("quirrel")
        for n in vm_names:
            v = table[bench].get(n)
            if v is None:
                row += f"{'-':>22}"
            elif q and n != "quirrel":
                row += f"{v:>12.2f} ({q / v:>5.2f}x)"
            else:
                row += f"{v:>22.2f}"
        print(row)
    if mismatches:
        raise SystemExit("\nThe VMs do not compute the same results:\n  " + "\n  ".join(mismatches))

    doc = {"info": machine_info(), "runs": args.runs, "results": table}
    with open(os.path.join(HERE, args.out), "w", encoding="utf-8", newline="\n") as f:
        json.dump(doc, f, indent=1)
        f.write("\n")


if __name__ == "__main__":
    main()
