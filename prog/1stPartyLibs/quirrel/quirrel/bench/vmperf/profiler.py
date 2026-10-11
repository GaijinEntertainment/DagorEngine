"""Where the time of a build goes: sampled shares per function and per line of SQVM::Execute.

sprof.cpp suspends the main thread of csq at a fixed interval and records its instruction
pointer. It needs no admin rights and no ETW; the sampler runs on another P-core.
"""
import json
import os
import re
import shutil
import subprocess
from collections import Counter

import build
import image
import timing

PROF_DIR = os.path.join(build.WORK, "prof")


def sprof_exe():
    src, exe = os.path.join(build.HERE, "sprof.cpp"), os.path.join(build.WORK, "sprof.exe")
    if os.path.exists(exe) and os.path.getmtime(exe) > os.path.getmtime(src):
        return exe
    compiles, _, link, _ = build.jam_commands()
    cmd = next(iter(compiles.values()))
    includes = [a for i, a in enumerate(cmd[1:], 1) if a == "-imsvc" or cmd[i - 1] == "-imsvc"]
    os.makedirs(build.WORK, exist_ok=True)
    subprocess.run([os.path.join(build.llvm_bin(), "clang-cl.exe"), "/nologo", "/O2", "/EHsc", "/std:c++17",
                    "-Wno-deprecated-declarations", *includes, src, "/Fe" + exe,
                    "/Fo" + os.path.join(build.WORK, "sprof.obj"), "/link",
                    *[a for a in link if a.startswith("-libpath:")]], check=True)
    return exe


def prof_dir(name):
    return os.path.join(PROF_DIR, re.sub(r"[^\w.-]+", "_", name))


def collect(builds, benches, runs, interval_us, cpu, sampler_cpu):
    sprof = sprof_exe()
    timing.prepare(benches)
    for name in builds:
        shutil.rmtree(prof_dir(name), ignore_errors=True)
        os.makedirs(prof_dir(name))
    for i in range(runs):
        for b in benches:
            for name, info in builds.items():
                out = os.path.join(prof_dir(name), f"{b}_{i}.txt")
                proc = subprocess.run([sprof, out, str(interval_us), str(cpu), str(sampler_cpu), info["exe"],
                                       "--opt-closure-hoisting", b + ".nut"], cwd=timing.RUN_DIR,
                                      capture_output=True, text=True)
                if proc.returncode or "VMPERF" not in proc.stdout:
                    raise SystemExit(f"{name} {b}.nut failed\n{proc.stdout}\n{proc.stderr}")
        print(f"run {i + 1}/{runs} done", flush=True)


def histogram(name, bench):
    hist = Counter()
    for file in os.listdir(prof_dir(name)):
        if file.rsplit("_", 1)[0] == bench:
            with open(os.path.join(prof_dir(name), file)) as f:
                for line in f:
                    if not line.startswith("#"):
                        rva, n = line.split()
                        hist[int(rva, 16)] += int(n)
    return hist


def symbolize(img, rvas):
    """{rva: (function name, source line)}"""
    out = subprocess.run([os.path.join(build.llvm_bin(), "llvm-symbolizer.exe"), "--obj=" + img.exe, "--no-inlines",
                          "--output-style=JSON"], input="\n".join(f"{img.base + r:#x}" for r in rvas),
                         capture_output=True, text=True).stdout
    res = {}
    for rva, line in zip(rvas, out.splitlines()):
        sym = (json.loads(line).get("Symbol") or [{}])[0]
        res[rva] = (sym.get("FunctionName", "?").split("(")[0] or "?", sym.get("Line", 0))
    return res


def report(builds, bench, top_functions=15, top_lines=30):
    """Shares per function and per line of Execute<false>. A line is shown by its text, as
    line numbers differ between revisions; the number is the one in the first build."""
    per_func, per_line, totals, line_no = {}, {}, {}, {}
    for name, info in builds.items():
        hist = histogram(name, bench)
        totals[name] = sum(hist.values()) or 1
        img = image.Image(info["exe"])
        exec_rva, exec_end = img.function(image.EXECUTE)
        source = build.sqvm_source(info["label"])
        text = open(source, encoding="utf-8").read().splitlines() if os.path.exists(source) else None
        funcs, lines = Counter(), Counter()
        for rva, (func, line) in symbolize(img, sorted(hist)).items():
            funcs[func] += hist[rva]
            if exec_rva <= rva < exec_end:
                key = ("(no line: code shared by several lines)" if line == 0
                       else text[line - 1].strip() if text else f"sqvm.cpp:{line}")
                lines[key] += hist[rva]
                line_no.setdefault(key, line or "")
        per_func[name], per_line[name] = funcs, lines
    names = list(builds)

    def table(title, data, top, label):
        keys = sorted({k for n in names for k in data[n]}, key=lambda k: -max(data[n][k] / totals[n] for n in names))
        print(f"\n{title:66}" + "".join(f"{chr(ord('A') + i):>7}" for i in range(len(names))))
        for k in keys[:top]:
            if max(data[n][k] / totals[n] for n in names) >= 0.002:
                print(f"{label(k)[:66]:66}" + "".join(f"{data[n][k] / totals[n] * 100:6.1f}%" for n in names))

    print(f"{bench}: " + ", ".join(f"{chr(ord('A') + i)} = {n} ({totals[n]} samples)" for i, n in enumerate(names)))
    table("function", per_func, top_functions, lambda k: k)
    table("line of SQVM::Execute<false>", per_line, top_lines, lambda k: f"{line_no[k]:>5} {k}")
