#!/usr/bin/env python3
"""daRg benchmark suite driver (Phase 0 WS2, see _ui_research/09_phase0_plan.md).

Runs each bench_*.ui.nut scene in dargbox, collects the BENCH_RESULT json line
that bench_stats.nut writes to the debug log, and emits a summary table plus a
machine-readable json file.

Usage (from prog/tools/dargbox):
  python run_bench.py [--stub] [--runs N] [--out results.json] [scene ...]

--stub renders through the null video driver (headless, CI-friendly; render
metrics are meaningless in this mode but update/rebuild/layout metrics hold).
"""

import argparse
import json
import os
import statistics
import subprocess
import sys

SCENES = [
    "bench_static",
    "bench_list_update",
    "bench_hud_watch",
    "bench_hud_rtprop",
    "bench_deep_flex",
    "bench_children_churn",
    "bench_text_heavy",
    "bench_field_watch",
    "bench_bound_props",
    "bench_bound_text",
]

# limit_updates counts RENDER frames while the runner counts acts; this is
# only a hard backstop so a broken scene cannot hang the suite - the normal
# exit is bench_stats.nut calling exit(0) after its capture window
LIMIT_UPDATES = 200000

# saturate the act loop: acts run back-to-back instead of the 30 Hz default,
# so wall time per frame reflects actual update cost, not pacing
ACT_RATE = 1000

DARGBOX_DIR = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../../tools/dargbox"))


# exe lives in a per-arch subdir of tools/dargbox (see prog/tools/dargbox/jamfile)
def default_dargbox_exe():
    arch = (os.environ.get("PROCESSOR_ARCHITEW6432") or os.environ.get("PROCESSOR_ARCHITECTURE", "")).upper()
    # arm64 hosts can run the x86_64 exe through emulation; the reverse cannot work
    arch_dirs = ["windows-arm64", "windows-x86_64"] if arch == "ARM64" else ["windows-x86_64"]
    for d in arch_dirs:
        exe = os.path.join(d, "dargbox-dev.exe")
        if os.path.exists(os.path.join(DARGBOX_DIR, exe)):
            return exe
    return os.path.join(arch_dirs[0], "dargbox-dev.exe")


def run_scene(scene, stub, exe):
    cmd = [
        os.path.join(DARGBOX_DIR, exe),
        "-quiet",
        "-silent",
        f"-config:script:t=samples_prog/benchmarks/{scene}.ui.nut",
        f"-config:debug/limit_updates:i={LIMIT_UPDATES}",
        f"-config:workcycle/act_rate:i={ACT_RATE}",
        # daRg act frames trip the engine spike profiler at saturated act rate
        # and every spike writes a .dap dump, dwarfing the benchmark itself
        "-config:debug/profiler:t=off",
        "-config:debug/useAddonVromSrc:b=yes",
        "-config:debug/fatalOnLogerrOnExit:b=no",
        "-config:video/vsync:b=no",
    ]
    if stub:
        cmd.append('-config:video/driver:t=stub')

    proc = subprocess.run(cmd, cwd=DARGBOX_DIR, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=600)

    log_ptr = os.path.join(DARGBOX_DIR, ".log", "last_debug")
    if not os.path.exists(log_ptr):
        raise RuntimeError(f"{scene}: no .log/last_debug after run (rc={proc.returncode})\n{proc.stdout}\n{proc.stderr}")
    with open(log_ptr) as f:
        debug_dir = f.read().strip()
    debug_file = os.path.join(DARGBOX_DIR, debug_dir, "debug")
    if not os.path.exists(debug_file):
        debug_file = os.path.join(debug_dir, "debug")

    result = None
    with open(debug_file, encoding="utf-8", errors="replace") as f:
        for line in f:
            pos = line.find("BENCH_RESULT ")
            if pos >= 0:
                result = json.loads(line[pos + len("BENCH_RESULT "):].strip())
    if result is None:
        raise RuntimeError(f"{scene}: no BENCH_RESULT in {debug_file} (rc={proc.returncode}); scene crashed or never reached capture end")
    return result


def aggregate(runs):
    """Median across runs for numeric fields; nested metrics tables get per-key medians of avg."""
    agg = dict(runs[0])
    if len(runs) > 1:
        for k in runs[0]:
            if isinstance(runs[0][k], (int, float)):
                agg[k] = statistics.median(r[k] for r in runs if k in r)
    agg["runsCount"] = len(runs)
    return agg


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("scenes", nargs="*", default=None)
    ap.add_argument("--stub", action="store_true", help="use null video driver (headless)")
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--out", default="bench_results.json")
    ap.add_argument("--exe", default=default_dargbox_exe(), help="dargbox binary, relative to tools/dargbox (use <arch>/dargbox.exe for release timings)")
    args = ap.parse_args()

    scenes = args.scenes if args.scenes else SCENES
    all_results = {}
    for scene in scenes:
        runs = []
        for i in range(args.runs):
            print(f"[{scene}] run {i + 1}/{args.runs} ...", flush=True)
            r = run_scene(scene, args.stub, args.exe)
            print(f"[{scene}]   {r['frames']} frames in {r['elapsedMs']} ms "
                  f"(builderEvals={r.get('builderEvals')}, elemsSetupRebuild={r.get('elemsSetupRebuild')})", flush=True)
            runs.append(r)
        all_results[scene] = {"agg": aggregate(runs), "runs": runs}

    with open(args.out, "w") as f:
        json.dump({"stub": args.stub, "results": all_results}, f, indent=1)
    print(f"\nresults written to {args.out}\n")

    hdr = ["scene", "ms/frame", "builderEvals/fr", "setupRebuild/fr", "setupRt/fr", "created/fr", "layoutRoots/fr", "elems", "descSlots"]
    print("  ".join(f"{h:>16}" for h in hdr))
    for scene, data in all_results.items():
        a = data["agg"]
        fr = max(a.get("frames", 1), 1)
        layout_roots = a.get("layoutFixedSizeRoots", 0) + a.get("layoutSizeRoots", 0) + a.get("layoutFlowRoots", 0)
        row = [
            scene,
            f"{a.get('elapsedMs', 0) / fr:.3f}",
            f"{a.get('builderEvals', 0) / fr:.1f}",
            f"{a.get('elemsSetupRebuild', 0) / fr:.1f}",
            f"{a.get('elemsSetupRealtime', 0) / fr:.1f}",
            f"{a.get('elemsCreated', 0) / fr:.1f}",
            f"{layout_roots / fr:.1f}",
            str(a.get("elemCount", "-")),
            str(a.get("descTableSlots", "-")),
        ]
        print("  ".join(f"{c:>16}" for c in row))


if __name__ == "__main__":
    main()
