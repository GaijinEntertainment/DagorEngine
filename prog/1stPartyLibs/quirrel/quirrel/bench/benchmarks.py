import os
import sys
import subprocess
import json
import platform
import time
import argparse
from collections import namedtuple

HERE = os.path.dirname(os.path.abspath(__file__))
# The doc site reads this side-car and renders the bars; see the {{benchmarks}}
# directive in ../doc/gen/render.py.
RESULTS = os.path.normpath(os.path.join(HERE, "..", "doc", "content", "_bench.json"))

PINNED_CPU = 2
RUNS_PER_CELL = 3
SUSPECT_SPREAD_RATIO = 0.05
SUSPECT_SPREAD_SECONDS = 0.005
# relative, for float results of float32 rows
FLOAT32_TOLERANCE = 1e-3

# release csq: jam -sConfig=rel in prog/tools/consoleSq
CSQ_REL = os.path.normpath(os.path.join(HERE, "..", "..", "..", "..", "..",
                                        "tools", "dagor_cdk", "windows-x86_64", "csq.exe"))

Lang = namedtuple("Lang", "label folder ext cmd float32")

# games enable closure hoisting; csq does not
quirrel = Lang("Quirrel-4.43.1", "quirrel", ".nut", [CSQ_REL, "--opt-closure-hoisting"], True)
squirrel = Lang("Squirrel-3.2", "squirrel", ".nut", ["sq3-64.exe"], True)
lua = Lang("Lua-5.5.1", "lua", ".lua", ["lua.exe"], False)
luajit_joff = Lang("LuaJIT-2.1-joff", "luajit", ".lua", ["luajit.exe", "-joff"], False)
luau_o0 = Lang("Luau-0.735-O0", "luau", ".luau", ["luau.exe", "-O0"], False)
luau_o1 = Lang("Luau-0.735-O1", "luau", ".luau", ["luau.exe", "-O1"], False)
luau_o2 = Lang("Luau-0.735-O2", "luau", ".luau", ["luau.exe", "-O2"], False)
quickjs = Lang("QuickJS-ng-0.16.2", "js", ".js", ["qjs.exe"], False)

# Every interpreter the suite measures. ../doc/gen/check.py reads this to catch a
# row whose label was bumped here but left stale in the committed side-car.
ALL_LANGS = [quirrel, squirrel, lua, luajit_joff, luau_o0, luau_o1, luau_o2, quickjs]

featured_lang = quirrel.label

# (page row, file stem in every folder)
WORKLOADS = [
  ("n-bodies", "nbodies"),
  ("n-bodies-arrays", "nbodies_arrays"),
  ("particles-kinematics", "particles"),
  ("exp-loop", "exp"),
  ("dictionary", "dict"),
  ("darg-ui-benchmark", "darg"),
  ("fibonacci-recursive", "fib_recursive"),
  ("fibonacci-loop", "fib_loop"),
  ("primes-loop", "primes"),
  ("float2string", "f2s"),
  ("string2float", "s2f"),
  ("queen", "queen"),
  ("sort", "table-sort"),
  ("spectral-norm", "spectral-norm"),
  ("tree", "tree"),
]


def pin_self_and_children(cpu):
  if sys.platform == "win32":
    from ctypes import WinDLL, WinError, c_size_t, get_last_error, wintypes
    k32 = WinDLL("kernel32", use_last_error=True)
    k32.GetCurrentProcess.restype = wintypes.HANDLE
    k32.SetProcessAffinityMask.argtypes = [wintypes.HANDLE, c_size_t]
    if not k32.SetProcessAffinityMask(k32.GetCurrentProcess(), 1 << cpu):
      raise WinError(get_last_error())
  else:
    os.sched_setaffinity(0, {cpu})


def suspect_spread(times):
  fastest, slowest = min(times), max(times)
  ratio = slowest / fastest - 1
  if slowest - fastest < SUSPECT_SPREAD_SECONDS or ratio < SUSPECT_SPREAD_RATIO:
    return 0
  return ratio


def results_agree(ref, other, loose):
  if ref == other:
    return True
  if not loose or None in (ref, other) or "." not in ref + other:
    return False
  try:
    a, b = float(ref), float(other)
  except ValueError:
    return False
  return abs(a - b) <= FLOAT32_TOLERANCE * max(abs(a), abs(b))


def check_results(test_name, results):
  ref = next((row for row in results if not row[0].float32), results[0] if results else None)
  if ref is None:
    return []
  bad = []
  for lang, r in results:
    if not results_agree(ref[1], r, lang.float32 or ref[0].float32):
      bad.append(f"{test_name}: {lang.label} computed {r}, {ref[0].label} computed {ref[1]}")
  return bad


def header_mismatch(kept, runs):
  now = machine_info()
  claimed = (("runs", kept.get("runs"), runs),
             ("cpu", kept.get("info", {}).get("cpu"), now["cpu"]),
             ("platform", kept.get("info", {}).get("platform"), now["platform"]))
  return [name for name, before, after in claimed if before != after]


def measure_once(cmds, folder):
  try:
    proc = subprocess.run(cmds, cwd=folder, capture_output=True, text=True, check=True)
  except (subprocess.CalledProcessError, OSError) as e:
    print("Error", e, f"\nin {folder} running {cmds}")
    return None
  lines = proc.stdout.splitlines()
  fields = [f.strip() for f in lines[0].split(",")] if lines else []
  try:
    seconds = float(fields[1])
  except (IndexError, ValueError):
    print(f"Error in {cmds}: expected <name>, <seconds>, <iterations>[, <result>], got {proc.stdout!r}")
    return None
  return seconds, (fields[3] if len(fields) > 3 else None)


def check_quirrel_label():
  try:
    version = subprocess.run([quirrel.cmd[0], "--sqversion"], capture_output=True, text=True).stdout.strip()
  except OSError:
    return
  if version and version not in quirrel.label:
    print(f"WARNING: {quirrel.cmd[0]} is Quirrel {version}, but the row is labelled {quirrel.label}; "
          "bump the label in benchmarks.py")


def run_tests(langs, workloads, results_file_name=None, update=False, runs=RUNS_PER_CELL):
  res = {}
  if update:
    # -u keeps the rows this run does not measure. A row whose label changed since
    # is kept too, so refresh a renamed interpreter with a full run instead.
    with open(results_file_name, "rt", encoding="utf-8") as f:
      kept = json.load(f)
    res = kept.get("results", {})
    stale = header_mismatch(kept, runs)
    if stale:
      print("WARNING: the kept rows were measured with a different", ", ".join(stale),
            "and the header written now speaks for them too, so refresh them as well")
  mismatches = []
  for test_name, stem in workloads:
    rs = res.setdefault(test_name, {})
    computed = []
    for lang in langs:
      folder = os.path.join(HERE, lang.folder)
      # newer Windows Python does not resolve bare exe names against the
      # child cwd; the interpreters live next to their scripts
      cmds = [os.path.join(folder, lang.cmd[0])] + lang.cmd[1:] + [stem + lang.ext]
      print(cmds, folder)
      measured = [measure_once(cmds, folder) for _ in range(runs)]
      if None in measured:
        continue
      times = [m[0] for m in measured]
      computed.append((lang, measured[-1][1]))
      timed = min(times) > 0
      val = min(times)
      if runs > 1:
        print("  ", times, "->", val, "result:", measured[-1][1])
        spread = suspect_spread(times) if timed else 0
        if spread:
          print(f"     suspect: the {runs} runs spread {spread * 100:.0f}%, re-measure this row")
      # noise only adds time: keep the lowest
      kept_val = rs.get(lang.label)
      if update and isinstance(kept_val, (int, float)) and kept_val < val:
        print(f"     kept {kept_val}, lower than this run")
        val = kept_val
      rs[lang.label] = val
    for line in check_results(test_name, computed):
      print("  MISMATCH", line)
      mismatches.append(line)
  if mismatches:
    raise SystemExit("\nThe ports do not compute the same results:\n  " + "\n  ".join(mismatches))
  write_results(results_file_name, res, runs)
  return res


def write_results(results_file_name, res, runs=RUNS_PER_CELL):
  """The side-car the doc site renders: the numbers, the row to pick out, and what
  they were measured on."""
  doc = {"info": machine_info(), "featured": featured_lang, "runs": runs, "results": res}
  with open(results_file_name, "w", encoding="utf-8", newline="\n") as f:
    json.dump(doc, f, indent=2)
    f.write("\n")


def machine_info():
  """What the numbers were measured on. run_vm_bench.py imports this too, so the
  two side-cars describe their machine the same way."""
  return {
    "platform": f"{platform.platform()} ({platform.release()})",
    "arch": platform.machine(),
    "cpu": platform.processor(),
    "when": time.strftime("%Y-%m-%d"),
  }


if __name__ == "__main__":
  def_langs = [l.label for l in ALL_LANGS]
  def_tests = [w[0] for w in WORKLOADS]
  parser = argparse.ArgumentParser(description='Script to do benchmarks.')
  parser.add_argument('-l','--lang', type=str, nargs='+', default = def_langs, help='langs to perform tests. default are:' + ",".join(def_langs))
  parser.add_argument('--update', '-u', default=False, help='keep the rows this run does not measure, and the lower time of each re-measured one', action='store_true')
  parser.add_argument('-t','--test', type=str, nargs='+', default = def_tests, help='test to do. default are all, Possible options:' + ",".join(def_tests))
  parser.add_argument('-r','--result', type=str, default = RESULTS, help='json file for results; a path of your own leaves the committed side-car alone')
  parser.add_argument('--quirrel', type=str, default = CSQ_REL, help=f'the Quirrel interpreter, default {CSQ_REL}')
  parser.add_argument('--cpu', type=int, default = PINNED_CPU, help=f'logical CPU every interpreter is pinned to, default {PINNED_CPU}')
  parser.add_argument('--no-pin', dest='pin', default=True, action='store_false', help='do not pin, which lets a row drift by tens of percent')
  parser.add_argument('--runs', type=int, default = RUNS_PER_CELL, help=f'process runs per cell, the fastest is published, default {RUNS_PER_CELL}')
  args = parser.parse_args()
  quirrel.cmd[0] = os.path.abspath(args.quirrel)
  langs = [l for l in ALL_LANGS if l.label in args.lang]
  if quirrel in langs:
    if not os.path.exists(quirrel.cmd[0]):
      raise SystemExit(f"{quirrel.cmd[0]} is not there; build it with jam -sConfig=rel in "
                       "the consoleSq tool or pass --quirrel (see README.md)")
    check_quirrel_label()
  if args.pin:
    pin_self_and_children(args.cpu)
    print("pinned to cpu", args.cpu)
  workloads = [w for w in WORKLOADS if w[0] in args.test]
  run_tests(langs, workloads, args.result, update=args.update, runs=args.runs)
  print("results ->", args.result)
