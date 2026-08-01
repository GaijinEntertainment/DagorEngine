#!/usr/bin/env python3
"""Quirrel VM acceptance benchmark: csq vs Lua 5.4 / Luau interpreters.

Runs the workloads in quirrel/ (*.nut) and lua/ (same algorithms) on each
available VM, parses the self-reported per-rep timings ("BENCH <name>
<ms> ..." lines), and reports the median-of-min per workload.

Each workload runs 5 reps inside one process (warmup amortized); the harness
additionally repeats whole processes (--runs) and takes the best rep per run,
median across runs - a standard interpreter-benchmark protocol that filters
both cold-start and machine noise.

This is the acceptance harness for interpreter work: run it before and after
a VM change. Use a release build of csq (jam -sConfig=rel in
the consoleSq tool). --lua/--luau default to the prebuilt interpreters
committed next to the workloads (see README.md).

Usage:
  python run_vm_bench.py --csq <csq.exe> [--lua <lua.exe>] [--luau <luau.exe>]
                         [--runs 3] [--out vm_bench_results.json] [bench ...]
"""

import argparse
import json
import os
import re
import statistics
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

BENCHES = ["fib", "binarytrees", "life", "mandel", "strings",
           "desc_churn", "probe_storm", "nullable_probe", "closure_storm",
           "method_calls"]

BENCH_RE = re.compile(r"BENCH (\S+) ([0-9.]+) ms")


def run_one(cmd, cwd):
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=cwd, timeout=300)
    times = [float(m.group(2)) for m in BENCH_RE.finditer(proc.stdout)]
    if not times:
        raise RuntimeError(f"no BENCH output from {' '.join(cmd)} (rc={proc.returncode})\n{proc.stdout}\n{proc.stderr}")
    return min(times)  # best rep of this process


def bench_vm(vm_name, cmd_template, bench, runs):
    script_dir = "quirrel" if vm_name == "quirrel" else "lua"
    ext = ".nut" if vm_name == "quirrel" else ".lua"
    script = os.path.join(HERE, script_dir, bench + ext)
    results = []
    for _ in range(runs):
        results.append(run_one(cmd_template + [script], HERE))
    return statistics.median(results)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("benches", nargs="*")
    ap.add_argument("--csq", required=True, help="path to csq.exe (release build!)")
    ap.add_argument("--lua", help="path to lua 5.4 interpreter (default: committed lua/lua.exe)")
    ap.add_argument("--luau", help="path to luau CLI (default: committed luau/luau.exe)")
    ap.add_argument("--luau-codegen", action="store_true", help="add a luau --codegen column")
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--out", default="vm_bench_results.json")
    args = ap.parse_args()

    if not args.lua:
        default_lua = os.path.join(HERE, "lua", "lua.exe")
        args.lua = default_lua if os.path.exists(default_lua) else None
    if not args.luau:
        default_luau = os.path.join(HERE, "luau", "luau.exe")
        args.luau = default_luau if os.path.exists(default_luau) else None

    vms = [("quirrel", [args.csq])]
    if args.lua:
        vms.append(("lua54", [args.lua]))
    if args.luau:
        vms.append(("luau", [args.luau, "-O2"]))
        if args.luau_codegen:
            vms.append(("luau-codegen", [args.luau, "-O2", "--codegen"]))

    benches = args.benches if args.benches else BENCHES
    table = {}
    for bench in benches:
        table[bench] = {}
        for vm_name, cmd in vms:
            try:
                ms = bench_vm(vm_name, cmd, bench, args.runs)
                table[bench][vm_name] = ms
                print(f"{bench:>16} {vm_name:>14} {ms:10.2f} ms", flush=True)
            except Exception as e:
                table[bench][vm_name] = None
                print(f"{bench:>16} {vm_name:>14}      FAILED: {e}", flush=True)

    with open(os.path.join(HERE, args.out), "w") as f:
        json.dump(table, f, indent=1)

    print("\n=== summary (ms, median of best-rep; ratio vs quirrel) ===")
    vm_names = [v[0] for v in vms]
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


if __name__ == "__main__":
    main()
