#!/usr/bin/env python3
# DEP_DOMAIN outerSpace/prog
import os
import sys
import argparse
import subprocess
import shutil
from pathlib import Path

DAGOR_ROOT_FOLDER = Path(__file__).resolve().parents[2]

def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument("--no-cvs-update", action="store_true", help="Skip CVS update, only copy files")
  return parser.parse_args()

def update_and_copy(src, dst, skip_cvs_update=False):
  if not skip_cvs_update:
    prev_cwd = os.getcwd()
    os.chdir(str(src.parent))
    print(f"cvs -q update -C {src.name}")
    subprocess.run(["cvs", "update", "-C", src.name], check=True)
    os.chdir(prev_cwd)

  # copy file
  try:
    shutil.copy2(src, dst / src.name)
    print(f"Copied: {src.name} -> {dst / src.name}")
  except OSError as e:
    print(f"ERROR: Failed to copy {src}: {e}")

def update_dng_snapshot(project_root, game_folder, vromfs_list, skip_cvs_update=False):
  dst = Path(project_root) / "tools" / "snapshot"
  shaders = Path(project_root) / game_folder / "compiledShaders" / "game.ps50.shdump.bin"
  tool_scripts_dir = Path(project_root) / "prog" / "tools"
  game_folder_abs = DAGOR_ROOT_FOLDER / project_root / game_folder

  print(f'\nUpdating DNG snapshot for "{project_root}"...')
  os.chdir(str(DAGOR_ROOT_FOLDER))
  dst.mkdir(parents=True, exist_ok=True)

  # shaders
  update_and_copy(shaders, dst, skip_cvs_update)

  # vfsroms
  for src in vromfs_list:
    src = Path(src)
    if src.name.startswith(".#"):
      continue
    subdir_dst = dst / src.parent.relative_to(game_folder_abs)
    subdir_dst.mkdir(parents=True, exist_ok=True)
    update_and_copy(src, subdir_dst, skip_cvs_update)

  # scripts
  for f in tool_scripts_dir.glob("*.das"):
    update_and_copy(f, dst, True)

if __name__ == "__main__":
  args = parse_args()

  project_list = [
    "outerSpace",
    "samples/dngSceneViewer"
  ]
  for project in project_list:
    script = DAGOR_ROOT_FOLDER / project / "prog" / "tools" / "update_snapshot.py"
    subprocess.run([sys.executable, str(script)] + sys.argv[1:], check=True)
