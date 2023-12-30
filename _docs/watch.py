import time
import os
from confiture import shell, pushd
import argparse
from cfg import read_commented_json, validated_qdox_cfg
import re

def getFiles(paths, config, initial=True):
  changed = False
  res = {}
  files = [] + paths
  root = os.path.dirname(config)
  try:
    cfg = validated_qdox_cfg(read_commented_json(config))
  except Exception as e:
    print("exception during loading config")
    print(e)
    return res

  for e in cfg:
    paths = e['paths']
    exd = re.compile("|".join(e["exclude_dirs_re"]))
    exf = re.compile("|".join(e["exclude_files_re"]))
    for p in paths:
      if isinstance(p, str) and os.path.isfile(p):
        files.append(p)
        if initial:
          print(f"add '{p}' to watch changes")
        continue
      for root, dirs, files_ in os.walk(p):
        if initial:
          print(f"looking in '{root}\({e['extensions']})' to watch files")
        if not e["recursive"]:
          dirs[:] = []
        else:
          dirs[:] = [d for d in dirs if not exd.fullmatch(d)]
        for f in files_:
          if exf.fullmatch(f):
            continue
          for i in e["extensions"]:
            if f.endswith(i):
              files.append(os.path.join(root, f))
              break
  [res.update({f:os.path.getmtime(f)}) for f in files]
  return res

def checkChanged(old, new):
  changed = False
  if len(new)!=len(old):
    changed = True
  for i, mtime in old.items():
    if i not in new or new[i] != mtime:
      changed = True
      break
  return changed


def main():
  p = argparse.ArgumentParser(description='qdox')
  p.add_argument('-c', '--config', action='store', type=str, default=".qdox")
  p.add_argument('-i', '--interval', action='store', type=int, default=-1, help="if below zero - rebuild in timeout")
  args = p.parse_args()
  _my_dir = os.path.dirname(os.path.realpath(__file__))
  _me = os.path.basename(os.path.realpath(__file__))
  watch_paths = [os.path.join(_my_dir, f) for f in os.listdir(_my_dir) if (f.endswith(".py") and f != _me)]+[args.config]
  config_dir = os.path.dirname(args.config)

  print(f"starting watch for '{args.config}' config change to rebuild docs")
  initial = True
  old = {}
  timePassed = 0.0
  while 1:
    new = getFiles(watch_paths, args.config, initial)
    initial=False
    changed = checkChanged(old, new)
    hasTimePassed = args.interval > 0 and timePassed >= args.interval
    if hasTimePassed or changed:
      if not initial:
        print("\nfiles have been changed, reloading\n...")
      old = new
      try:
        if hasTimePassed or changed:
          with pushd(config_dir):
            shell(f"python {os.path.abspath(_my_dir)}/qdoc_main.py")
      except RuntimeError as e:
        print("error rebuilding, waiting 2 second")
        time.sleep(2)
        timePassed+=2.0

    time.sleep(0.5)
    timePassed += 0.5

main()