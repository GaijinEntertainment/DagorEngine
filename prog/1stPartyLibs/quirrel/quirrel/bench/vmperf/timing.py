"""Pinned and interleaved timing of csq builds on the workloads of bench/quirrel."""
import ctypes
import glob
import json
import math
import os
import re
import shutil
import struct
import subprocess
import sys

import build

BENCH = os.path.join(build.REPO, build.QUIRREL, "bench")
RUN_DIR = os.path.join(build.WORK, "run")
DEFAULT_BENCHES = ["primes", "life", "fib", "binarytrees", "mandel", "closure_storm", "method_calls", "nbodies",
                   "dict", "strings", "table-sort", "s2f", "darg"]
# a timed rep of 200 ms keeps the 1 ms clock step below 0.5%
TARGET_MS = 200

sys.path.insert(0, BENCH)
from benchmarks import machine_info  # noqa: E402


def all_benches():
    res = []
    for path in sorted(glob.glob(os.path.join(BENCH, "quirrel", "*.nut"))):
        with open(path, encoding="utf-8") as f:
            if '"profile.nut"' in f.read():
                res.append(os.path.splitext(os.path.basename(path))[0])
    return res


def cores():
    """[(efficiency class, [logical CPUs])] for each physical core; P-cores have the higher class."""
    k32 = ctypes.WinDLL("kernel32", use_last_error=True)
    size = ctypes.c_ulong(0)
    k32.GetLogicalProcessorInformationEx(0, None, ctypes.byref(size))
    buf = ctypes.create_string_buffer(size.value)
    if not k32.GetLogicalProcessorInformationEx(0, buf, ctypes.byref(size)):
        raise ctypes.WinError(ctypes.get_last_error())
    raw, res, off = buf.raw, [], 0
    while off < size.value:
        # PROCESSOR_RELATIONSHIP: Flags, EfficiencyClass, Reserved[20], GroupCount, GroupMask[0].Mask
        entry_size = struct.unpack_from("<I", raw, off + 4)[0]
        mask = struct.unpack_from("<Q", raw, off + 32)[0]
        res.append((raw[off + 9], [i for i in range(64) if mask >> i & 1]))
        off += entry_size
    return res


def p_cores():
    all_cores = cores()
    top = max(e for e, _ in all_cores)
    return [cpus for e, cpus in all_cores if e == top]


def default_cpu():
    """The first logical CPU of the second P-core: the first core takes more interrupts."""
    pc = p_cores()
    return pc[min(1, len(pc) - 1)][0]


def pin(cpu):
    """Pin this process, and so the children it starts, to one logical CPU."""
    for cpus in p_cores():
        if cpu in cpus:
            siblings = [c for c in cpus if c != cpu]
            print(f"pinned to CPU {cpu}, a P-core" + (f"; keep CPU {siblings} idle, it shares the core" if siblings else ""))
            break
    else:
        print(f"WARNING: CPU {cpu} is not a P-core, see `vmperf.py cores`")
    k32 = ctypes.WinDLL("kernel32", use_last_error=True)
    k32.GetCurrentProcess.restype = ctypes.c_void_p
    k32.SetProcessAffinityMask.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    if not k32.SetProcessAffinityMask(k32.GetCurrentProcess(), 1 << cpu):
        raise ctypes.WinError(ctypes.get_last_error())


def prepare(benches):
    """Copy the workloads and the vmperf timing helper into RUN_DIR."""
    os.makedirs(RUN_DIR, exist_ok=True)
    shutil.copy(os.path.join(build.HERE, "profile.nut"), RUN_DIR)
    for b in benches:
        shutil.copy(os.path.join(BENCH, "quirrel", b + ".nut"), RUN_DIR)


def run_once(exe, bench, inner):
    """(ms for one call of the workload, its result): the fastest of the reps of one process."""
    proc = subprocess.run([exe, "--opt-closure-hoisting", bench + ".nut"], cwd=RUN_DIR, capture_output=True, text=True,
                          env=dict(os.environ, VMPERF_INNER=str(inner)), timeout=900)
    m = re.search(r"^VMPERF ([0-9.]+) (.*)$", proc.stdout, re.M)
    if proc.returncode or not m:
        raise SystemExit(f"{exe} {bench}.nut failed (exit code {proc.returncode})\n{proc.stdout}\n{proc.stderr}")
    return float(m.group(1)), m.group(2).strip()


def measure(builds, benches, rounds, cpu, out):
    """Time every build on every workload, `rounds` processes each, interleaved: a round runs
    every build once, in an order that turns from round to round. Writes `out` after each round."""
    prepare(benches)
    pin(cpu)
    first = next(iter(builds.values()))["exe"]
    inner = {b: max(1, math.ceil(TARGET_MS / max(run_once(first, b, 1)[0], 0.01))) for b in benches}
    data = {"machine": machine_info(), "cpu": cpu, "builds": builds, "inner": inner,
            "results": {b: {n: [] for n in builds} for b in benches}, "outputs": {b: {} for b in benches}}
    names = list(builds)
    for r in range(rounds):
        for b in benches:
            for n in names[r % len(names):] + names[:r % len(names)]:
                ms, result = run_once(builds[n]["exe"], b, inner[b])
                data["results"][b][n].append(ms)
                data["outputs"][b][n] = result
        with open(out, "w") as f:
            json.dump(data, f, indent=1)
        print(f"round {r + 1}/{rounds} done", flush=True)
    return data
