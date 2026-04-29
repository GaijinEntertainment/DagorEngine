#!/usr/bin/env python3
import sys
from pathlib import Path

DAGOR_ROOT_FOLDER = Path(__file__).resolve().parents[4]
sys.path.append(str(DAGOR_ROOT_FOLDER / "prog" / "tools"))
from update_dng_snapshots import update_dng_snapshot, parse_args
sys.path.pop()

project_root = "samples/dngSceneViewer"
game_folder = "viewer"
game = DAGOR_ROOT_FOLDER / project_root / game_folder
vromfs_list = list(game.glob("*.vromfs.bin"))

update_dng_snapshot(project_root, game_folder, vromfs_list, parse_args().no_cvs_update)
