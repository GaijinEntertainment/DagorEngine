#!/usr/bin/env python3
"""Times several csq builds on the Quirrel workloads, in interleaved rounds.

The workloads run with the timing helper of vmperf (../../quirrel/bench/vmperf/profile.nut):
a rep calls the workload VMPERF_INNER times, calibrated on the first build, which keeps a rep of
that build at 200 ms or more against the 1 ms step of clock(), and a process gives the fastest
of its 5 reps. A round runs each
workload once in each build, in an order that turns from round to round, so that heat and
background work fall on all builds alike. The summary keeps the fastest round of each build,
because noise only adds time, and shows the spread of the rounds next to it.

Usage:
  python measure_builds.py --build base=<csq.exe> --build tip=<csq.exe> --out result.json
                           [--rounds 3] [--cpu 2] [--suite doc,vm] [--extra-dir <dir>] [workload ...]
"""

import argparse
import json
import math
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
BENCH = os.path.normpath(os.path.join(HERE, "..", "..", "quirrel", "bench"))
sys.path[:0] = [BENCH, os.path.join(BENCH, "vmperf")]

import timing  # noqa: E402
from benchmarks import PINNED_CPU, WORKLOADS, machine_info, pin_self_and_children  # noqa: E402
from run_vm_bench import BENCHES  # noqa: E402
from stacks_report import md_table, stack_table  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("workloads", nargs="*")
    ap.add_argument("--build", action="append", required=True, help="name=path of a csq.exe; the first is the base")
    ap.add_argument("--rounds", type=int, default=3)
    ap.add_argument("--cpu", type=int, default=PINNED_CPU)
    ap.add_argument("--suite", default="doc,vm", help="doc (benchmarks.py), vm (run_vm_bench.py), none")
    ap.add_argument("--extra-dir", help="also each .nut of this folder but profile.nut")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    builds = dict(b.split("=", 1) for b in args.build)
    names = list(builds)
    suites = args.suite.split(",")
    quirrel_dir = os.path.join(BENCH, "quirrel")
    sources = []
    if "doc" in suites:
        sources += [(row, os.path.join(quirrel_dir, stem + ".nut")) for row, stem in WORKLOADS]
    if "vm" in suites:
        sources += [(stem, os.path.join(quirrel_dir, stem + ".nut")) for stem in BENCHES]
    if args.extra_dir:
        extra = os.path.abspath(args.extra_dir)
        sources += [(os.path.splitext(f)[0], os.path.join(extra, f)) for f in sorted(os.listdir(extra))
                    if f.endswith(".nut") and f != "profile.nut"]
    if args.workloads:
        sources = [s for s in sources if s[0] in args.workloads or os.path.basename(s[1])[:-4] in args.workloads]

    assert len({os.path.basename(p) for _, p in sources}) == len({n for n, _ in sources}) == len(sources), \
        f"two workloads share a name or a file name: {sources}"
    timing.prepare([])
    for _, path in sources:
        shutil.copy(path, timing.RUN_DIR)
    jobs = [(name, os.path.basename(path)[:-4]) for name, path in sources]

    if args.cpu >= 0:
        pin_self_and_children(args.cpu)
    first = builds[names[0]]
    inner = {name: max(1, math.ceil(timing.TARGET_MS / max(timing.run_once(first, stem, 1)[0], 0.01)))
             for name, stem in jobs}
    data = {"machine": machine_info(), "cpu": args.cpu, "rounds": args.rounds, "builds": builds, "inner": inner,
            "results": {j[0]: {n: [] for n in names} for j in jobs},
            "outputs": {j[0]: {} for j in jobs}, "mismatches": []}
    for r in range(args.rounds):
        order = names[r % len(names):] + names[:r % len(names)]
        for name, stem in jobs:
            for n in order:
                ms, result = timing.run_once(builds[n], stem, inner[name])
                data["results"][name][n].append(ms)
                seen = data["outputs"][name].setdefault(n, result)
                if seen != result:
                    data["mismatches"].append(f"{name}: {n} computed {result} and {seen}")
            print(f"round {r + 1} {name:>34} " + " ".join(f"{n}={data['results'][name][n][-1]:.3f}" for n in names),
                  flush=True)
        with open(args.out, "w", encoding="utf-8", newline="\n") as f:
            json.dump(data, f, indent=1)
    for name, outs in data["outputs"].items():
        if len(set(outs.values())) > 1:
            data["mismatches"].append(f"{name}: the builds computed {outs}")
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, indent=1)
        f.write("\n")
    print()
    print(md_table(*stack_table(data)))
    if data["mismatches"]:
        raise SystemExit("\nresults differ:\n  " + "\n  ".join(data["mismatches"]))


if __name__ == "__main__":
    main()
