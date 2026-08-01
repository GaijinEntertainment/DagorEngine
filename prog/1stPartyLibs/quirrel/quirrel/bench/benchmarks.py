import os
import sys
import subprocess
import json
import platform
import time
import argparse

HERE = os.path.dirname(os.path.abspath(__file__))
# committed results rendered into the documentation live next to the perf docs
RESULTS_DIR = os.path.normpath(os.path.join(HERE, "..", "doc", "source", "performance"))

lua = ["Lua-5.4.6", "lua", ["lua.exe"]]
luajit_joff = ["LuaJIT2.1-joff", "lua", ["luajit.exe", "-joff"]]
quickjs = ["QuickJS", "js", ["qjs.exe"]]
# the Quirrel row runs the release csq built from this repo (jam -sConfig=rel
# in the consoleSq tool) - the shipped runtime configuration (mimalloc).
# A plain cmake sq.exe sits on the CRT heap and is up to 2x slower on
# table-sweep workloads, misrepresenting shipped performance.
CSQ_REL = os.path.normpath(os.path.join(HERE, "..", "..", "..", "..", "..",
                                        "tools", "dagor_cdk", "windows-x86_64", "csq.exe"))
quirrel = ["Quirrel-4.35.1", "quirrel", [CSQ_REL]]
squirrel = ["Squirrel-3.1", "quirrel", ["sq3-64.exe"]]
daslang_int =  ["Daslang (interperter)", None, None]
luau = ["Luau", "luau", ["luau.exe"]]

featured_lang = quirrel[0]
baseline_lang = None

benchmarks = [
  ("n-bodies", [
    [lua, "nbodies.lua"],
    [luau, "nbodies.luau"],
    [luajit_joff, "nbodies.lua"],
    [quickjs, "nbodies.js"],
    [quirrel, "nbodies.nut"],
    [squirrel, "nbodies.nut"],
  ]),
  ("particles-kinematics", [
    [lua, "particles.lua"],
    [luau, "particles.luau"],
    [luajit_joff, "particles.lua"],
    [quickjs, "particles.js"],
    [quirrel, "particles.nut"],
    [squirrel, "particles.nut"],
  ]),
  ("exp-loop", [
    [lua, "exp.lua"],
    [luau, "exp.luau"],
    [luajit_joff, "exp.lua"],
    [quickjs, "exp.js"],
    [quirrel, "exp.nut"],
    [squirrel, "exp.nut"],
  ]),
  ("dictionary", [
    [lua, "dict.lua"],
    [luau, "dict.luau"],
    [luajit_joff, "dict.lua"],
    [quickjs, "dict.js"],
    [quirrel, "dict.nut"],
    [squirrel, "dict.nut"],
  ]),
  ("darg-ui-benchmark", [
    [lua, "darg.lua"],
    [luajit_joff, "darg.lua"],
    [luau, "darg.luau"],
    [quickjs, "darg.js"],
    [quirrel, "darg.nut"],
    [squirrel, "darg.nut"],
  ]),
  ("fibonacci-recursive", [
    [lua, "fib_recursive.lua"],
    [luau, "fib_recursive.luau"],
    [luajit_joff, "fib_recursive.lua"],
    [quickjs, "fib_recursive.js"],
    [quirrel, "fib_recursive.nut"],
    [squirrel, "fib_recursive.nut"],
  ]),
  ("fibonacci-loop", [
    [lua, "fib_loop.lua"],
    [luau, "fib_loop.luau"],
    [luajit_joff, "fib_loop.lua"],
    [quickjs, "fib_loop.js"],
    [quirrel, "fib_loop.nut"],
    [squirrel, "fib_loop.nut"],
  ]),
  ("primes-loop", [
    [lua, "primes.lua"],
    [luau, "primes.luau"],
    [luajit_joff, "primes.lua"],
    [quickjs, "primes.js"],
    [quirrel, "primes.nut"],
    [squirrel, "primes.nut"],
  ]),
  ("float2string", [
    [quirrel, "f2s.nut"],
    [squirrel, "f2s.nut"],
    [lua,"f2s.lua"],
    [luajit_joff,"f2s.lua"],
    [luau,"f2s.luau"],
    [quickjs,"f2s.js"],
  ]),
  ("queen", [
    [quirrel, "queen.nut"],
    [squirrel, "queen.nut"],
    [lua,"queen.lua"],
    [luajit_joff,"queen.lua"],
    [luau,"queen.luau"],
  ]),
  ("sort", [
    [quirrel, "table-sort.nut"],
    [squirrel, "table-sort.nut"],
    [lua,"table-sort.lua"],
    [luajit_joff,"table-sort.lua"],
    [luau,"table-sort.luau"],
  ]),
  ("spectral-norm", [
    [quirrel, "spectral-norm.nut"],
    [squirrel, "spectral-norm.nut"],
    [lua,"spectral-norm.lua"],
    [luajit_joff,"spectral-norm.lua"],
    [luau,"spectral-norm.luau"],
    [quickjs,"spectral-norm.js"],
  ]),
  ("string2float", [
    [quirrel, "f2i.nut"],
    [squirrel, "f2i.nut"],
    [lua,"f2i.lua"],
    [luajit_joff,"f2i.lua"],
    [luau,"f2i.luau"],
    [quickjs,"f2i.js"],
  ])
]


class pushd:
    def __init__(self, path):
        self.olddir = os.getcwd()
        if path != '':
            os.chdir(os.path.normpath(path))
    def __enter__(self):
        pass
    def __exit__(self, type, value, traceback):
        os.chdir(self.olddir)

def isnumeric(f):
  try:
    float(f)
  except ValueError:
    return False
  return True

def run_tests(benchmarks, results_file_name=None, perform_tests=None, perform_tests_by_name=None, update=False):
  src = None
  if update:
    with open(results_file_name, "rt", encoding="utf-8") as f:
      src = json.load(f)
  res = {} if src is None else src
  for test_name, benchs in benchmarks:
    if perform_tests_by_name is not None and test_name not in perform_tests_by_name:
      continue
    rs = res.get(test_name, {})
    res[test_name] = rs
    for data, file in benchs:
      if perform_tests is not None and data not in perform_tests:
        continue
      lang_name, folder, cmds = data
      cmds = cmds + [file]
      folder = os.path.join(HERE, folder)
      # newer Windows Python does not resolve bare exe names against the
      # child cwd; the interpreters live next to their scripts
      exe = os.path.join(folder, cmds[0])
      if os.path.exists(exe):
        cmds = [exe] + cmds[1:]
      print(cmds, folder)
      try:
        with pushd(folder):
          proc = subprocess.run(cmds, capture_output=True,text=True, check=True)
          out = proc.stdout
      except subprocess.CalledProcessError as e:
        print("Error", e, f"\nin {folder} performing {cmds}")
        continue
      fullout = out
      out = out.splitlines()
      if len(out)==0:
        print(f"Error in {cmds}, no correct output, got {fullout}, expected <test name>, <testres in float seconds>, <number of tests>")
        continue
      out = out[0].split(",")
      out = [str(o).strip() for o in out]
      possible_vals = [o for o in out if isnumeric(o)]
      if len(possible_vals) > 0:
        possible_val = possible_vals[0]
      elif len(out) >1:
        possible_val = out[0]
      else:
        possible_val = out[0]
      val = float(possible_val) if isnumeric(possible_val) else possible_val
      rs[lang_name] = val
  with open(results_file_name, "w") as f:
    json.dump(res, f, indent=2)
  return res

def read_results(results_file_name=None):
  data = None
  if not os.path.exists(results_file_name) or results_file_name is None:
    print(results_file_name, "doesnt exists!")
    return data
  with open(results_file_name, "r") as f:
    data = json.load(f)
  return data

def convert_results_to_rst(data, fname, test_info):
  out = []
  out.append("\n.. raw:: html\n")
  for testname, results in data.items():
    out.append(f"  <br><br>\n  <h3>{testname}</h3>")
    out.append(f'  <table class="chart" style="width: 600px;">')
    results = list(results.items())
    results.sort(key=lambda x: 1/x[1], reverse=True)
    if len(results)==0:
      continue
    maxvalue = results[-1][1]
    for lang, res in results:
      relative = int(res/maxvalue*100)
      res = round(res, 3)
      chart = 'chart-bar featured' if lang == featured_lang else 'chart-bar'
      s = f'    <tr><th>{lang}</th><td><div class="{chart}"  style="width: {relative}%;">{res}s&nbsp;</div></td></tr>'
      out.append(s)
    out.append("\n  </table>\n\n")
  out.append("| ")
  for line in test_info:
    out.append(f"| *{line}*")
  with open(fname, "wt") as f:
    f.writelines(line + '\n' for line in out)

def get_current_test_info():
  return [
    "Platform: " + platform.platform() +"(" +platform.release() +")",
    "Architecture: " + platform.machine(),
    "Processor: " + platform.processor(),
    time.ctime()
  ]
if __name__ == "__main__":
  results_file_name = os.path.join(RESULTS_DIR, "results.json")
  result_rst = os.path.join(RESULTS_DIR, "results.rst")
  def_perform_tests_for_langs = [lua, luajit_joff, quickjs, quirrel, squirrel, luau]
  def_langs = [l[0] for l in def_perform_tests_for_langs]
  def_tests = [b[0] for b in benchmarks]
  parser = argparse.ArgumentParser(description='Script to do benchmarks.')
  parser.add_argument('-l','--lang', type=str, nargs='+', default = def_langs, help='langs to perform tests. default are:' + ",".join(def_langs))
  parser.add_argument('--update', '-u', default=False, help='Update results', action='store_true')
  parser.add_argument('--only_rst', '-o', default=False, help='Only Build RST', action='store_true')
  parser.add_argument('-t','--test', type=str, nargs='+', default = def_tests, help='test to do. default are all, Possible options:' + ",".join(def_tests))
  parser.add_argument('-r','--result', type=str, default = results_file_name, help='json file for results')
  args = parser.parse_args()
  langs = args.lang
  update = args.update
  tests = args.test
  test_info = get_current_test_info()
  perform_tests_for_langs = [l for l in def_perform_tests_for_langs if l[0] in langs]
  if not args.only_rst:
    run_tests(benchmarks, results_file_name, perform_tests_for_langs, tests, update=update)
  results = read_results(results_file_name)
  convert_results_to_rst(results, result_rst, test_info)