#!/usr/bin/env python3
"""Measure how a Quirrel VM or compiler change affects speed, net of code layout. See README.md.

  vmperf.py compare [build ...]     build, time and compare (default: HEAD against the working tree)
  vmperf.py show <result.json>      print a result again; --all adds every build
  vmperf.py build <rev ...>         build only
  vmperf.py dispatch <build ...>    where the opcode dispatch block of SQVM::Execute lies
  vmperf.py profile <build ...>     sampled time per function and per line of SQVM::Execute
  vmperf.py cores                   the P- and E-cores of this machine

A build is a git revision (all five variants: nat k0 k16 k32 k48), `wt` for the working
tree, <rev>/<variant> for one variant, or the path of a csq.exe.
"""
import argparse
import json
import os
import statistics
import time

import build
import image
import profiler
import timing


def builds_of(specs, rebuild=False, variants=build.VARIANTS):
    """{name: {exe, rev, label, variant, dispatch}}"""
    res = {}
    for spec in specs:
        if os.path.isfile(spec):
            exe = os.path.abspath(spec)
            res[spec] = {"exe": exe, "rev": spec, "label": spec, "variant": "nat", "dispatch": dispatch_of(exe)}
            continue
        rev, _, variant = spec.rpartition("/")
        if variant not in build.VARIANTS:
            rev, variant = spec, None
        label = build.build(rev, rebuild)
        for v in [variant] if variant else variants:
            exe = build.exe_path(label, v)
            res[f"{label}/{v}"] = {"exe": exe, "rev": rev, "label": label, "variant": v, "dispatch": dispatch_of(exe)}
    return res


def dispatch_of(exe):
    if not os.path.exists(os.path.splitext(exe)[0] + ".pdb"):
        return "unknown, no pdb"
    try:
        return image.describe_dispatch(image.Image(exe).dispatch())
    except SystemExit as e:
        return str(e)


def report(data, show_all=False):
    builds, results = data["builds"], data["results"]
    rounds = min(len(v) for r in results.values() for v in r.values())
    best = {b: {n: min(v[:rounds]) for n, v in r.items()} for b, r in results.items()}
    groups = {}
    for name, info in builds.items():
        groups.setdefault(info["label"], {})[info["variant"]] = name
    labels = list(groups)
    letter = {label: chr(ord("A") + i) for i, label in enumerate(labels)}
    for label, members in groups.items():
        rev = builds[next(iter(members.values()))]["rev"]
        print(f"{letter[label]} = {rev}" + (f" ({label})" if label != rev else ""))
        print("    dispatch block, bytes of its 64-byte line: "
              + ", ".join(f"{v} {builds[n]['dispatch']}" for v, n in members.items()))

    def delta(v, v0):
        return f"{(v / v0 - 1) * 100:+.1f}%"

    cols = []  # (group title, column title, cell of a bench)
    if all(set(build.LAYOUTS) <= set(m) for m in groups.values()):
        mean = {b: {lb: statistics.mean(best[b][groups[lb][k]] for k in build.LAYOUTS) for lb in labels} for b in best}
        for i, lb in enumerate(labels):
            cols.append(("layout mean" if i == 0 else "", letter[lb], lambda b, lb=lb: f"{mean[b][lb]:.2f}"))
            if i:
                cols.append(("", "delta", lambda b, lb=lb: delta(mean[b][lb], mean[b][labels[0]])))
        for i, lb in enumerate(labels):
            def spread(b, lb=lb):
                v = [best[b][groups[lb][k]] for k in build.LAYOUTS]
                return f"{(max(v) / min(v) - 1) * 100:.0f}%"
            cols.append(("spread" if i == 0 else "", letter[lb], spread))
    if all("nat" in m for m in groups.values()):
        nat = {b: {lb: best[b][groups[lb]["nat"]] for lb in labels} for b in best}
        for i, lb in enumerate(labels):
            cols.append(("plain build" if i == 0 else "", letter[lb], lambda b, lb=lb: f"{nat[b][lb]:.2f}"))
            if i:
                cols.append(("", "delta", lambda b, lb=lb: delta(nat[b][lb], nat[b][labels[0]])))
    if show_all or not cols:
        for name in builds:
            cols.append(("every build" if name == next(iter(builds)) else "",
                         f"{letter[builds[name]['label']]}/{builds[name]['variant']}",
                         lambda b, name=name: f"{best[b][name]:.2f}"))

    print(f"\nms per workload call, the fastest of {rounds} rounds on CPU {data['cpu']}")
    w = 9
    titles = ""
    for i, (g, _, _) in enumerate(cols):
        if g:
            titles = f"{titles:{max(16 + i * w + 3, len(titles) + 2)}}{g}"
    print(titles)
    print(f"{'bench':16}" + "".join(f"{c:>{w}}" for _, c, _ in cols))
    for b in best:
        print(f"{b:16}" + "".join(f"{cell(b):>{w}}" for _, _, cell in cols))
    for b, outputs in data["outputs"].items():
        if len(set(outputs.values())) > 1:
            print(f"WARNING: {b} computes different results: {outputs}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("compare")
    p.add_argument("builds", nargs="*", default=["HEAD", build.WORKTREE])
    p.add_argument("--rounds", type=int, default=4)
    p.add_argument("--bench", default=",".join(timing.DEFAULT_BENCHES), help="names from bench/quirrel, or all")
    p.add_argument("--cpu", type=int, default=None)
    p.add_argument("--rebuild", action="store_true")
    p.add_argument("--out", help="result file, default WORK/results/<time>.json")
    p.add_argument("--all", action="store_true", help="also print every build")
    p = sub.add_parser("show")
    p.add_argument("result")
    p.add_argument("--all", action="store_true")
    p = sub.add_parser("build")
    p.add_argument("revs", nargs="+")
    p.add_argument("--rebuild", action="store_true")
    p = sub.add_parser("dispatch")
    p.add_argument("builds", nargs="+")
    p.add_argument("--listing", action="store_true", help="also write execute.asm next to each csq.exe")
    p = sub.add_parser("profile")
    p.add_argument("builds", nargs="+", help="a revision means its nat variant here")
    p.add_argument("--bench", default="primes,life,darg")
    p.add_argument("--runs", type=int, default=3)
    p.add_argument("--interval", type=int, default=250, help="microseconds between samples")
    p.add_argument("--cpu", type=int, default=None)
    sub.add_parser("cores")
    args = ap.parse_args()

    if args.cmd == "compare":
        builds = builds_of(args.builds, args.rebuild)
        benches = timing.all_benches() if args.bench == "all" else args.bench.split(",")
        out = args.out or os.path.join(build.WORK, "results", time.strftime("%Y%m%d-%H%M%S") + ".json")
        os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
        data = timing.measure(builds, benches, args.rounds, timing.default_cpu() if args.cpu is None else args.cpu, out)
        report(data, args.all)
        print("result:", out)
    elif args.cmd == "show":
        with open(args.result) as f:
            report(json.load(f), args.all)
    elif args.cmd == "build":
        for rev in args.revs:
            label = build.build(rev, args.rebuild)
            print(rev, "->", os.path.join(build.WORK, label))
    elif args.cmd == "dispatch":
        for name, info in builds_of(args.builds).items():
            print(f"{name}: dispatch block, bytes of its 64-byte line: {info['dispatch']}")
            if args.listing:
                path = os.path.join(os.path.dirname(info["exe"]), "execute.asm")
                image.Image(info["exe"]).listing(image.EXECUTE, path)
                print("   ", path)
    elif args.cmd == "profile":
        builds = builds_of(args.builds, variants=["nat"])
        benches = args.bench.split(",")
        cpu = timing.default_cpu() if args.cpu is None else args.cpu
        sampler = next(c[0] for c in timing.p_cores() if cpu not in c)
        profiler.collect(builds, benches, args.runs, args.interval, cpu, sampler)
        for b in benches:
            profiler.report(builds, b)
            print()
    elif args.cmd == "cores":
        for i, (efficiency, cpus) in enumerate(timing.cores()):
            print(f"core {i:2}: efficiency class {efficiency}, logical CPUs {cpus}")
        print("P-cores have the highest class; default CPU for timing:", timing.default_cpu())


if __name__ == "__main__":
    main()
