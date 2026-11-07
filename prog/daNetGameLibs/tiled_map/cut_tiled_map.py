from multiprocessing import Pool, cpu_count
import os
import sys
import signal
import subprocess
import argparse
import time
from pathlib import Path

# Generate z levels for tiled map. Files are named as quadkeys.
# See more: https://learn.microsoft.com/en-us/azure/azure-maps/zoom-levels-and-tile-grid?tabs=csharp#quadkey-indices

_SCRIPT_DIR = Path(__file__).resolve().parent  # .../prog/daNetGameLibs/tiled_map
_REPO_DIR = _SCRIPT_DIR.parents[2]             # .../dagor  (daNetGameLibs -> prog -> dagor)
sys.path.insert(0, str(_REPO_DIR / '_buildtools' / 'tools'))

import datablock as datablock
from gen_tiled_map import convertTifToDdsx, runTool, ddsConverter, ddsxCvt

DAGOR_DIR = str(_REPO_DIR)

nconvert = os.path.abspath(f"{DAGOR_DIR}/tools/converters/nconvert/nconvert.exe")
inputPath = ""
outPath = ""
zlevels = -1

threads = max(cpu_count() - 2, 1)

tiff_out_format = [
  "-add_alpha", "0",
  "-out","tiff",
]

avif_out_format = [
  "-out","avif",
  "-avif_format", "1:422",
  "-avif_speed", "2", # 0-10 (slow-fast)
  "-avif_quant_color", "0", "0", "24",
]

def resizeBaseImage(filename, tileWidth):
  print(f"[INFO] Resizing base image {filename} to {tileWidth}x{tileWidth}")
  p = subprocess.Popen([nconvert, "-quiet", "-overwrite"] +
    tiff_out_format +
    ["-resize", f"{tileWidth}", f"{tileWidth}"] +
    ["-o", os.path.join(outPath, "combined.tif"), filename],
    stdout=subprocess.DEVNULL)
  try:
    p.wait()
  except KeyboardInterrupt:
    print("[INFO] Terminating...")
    p.send_signal(signal.SIGINT)
    exit(1)

# splits one tile into 4
# a | b
# c | d
def splitTile(tuple):
  tile, z = tuple
  name = os.path.basename(tile).split(".")[0]
  names = [name, name, name, name]

  if z == 1:
    names = ["0", "1", "2", "3"]
  else:
    names = list(map(lambda x, y: f"{x}{y}", names, range(0, 4)))

  paths = [os.path.join(outPath, f"{name}.tif") for name in names]

  print(f"[INFO] Splitting {tile} into 4 tiles")

  variants = [
    ["-canvas", "50%", "50%", "top-left"], # a
    ["-canvas", "50%", "50%", "top-right"], # b
    ["-canvas", "50%", "50%", "bottom-left"], # c
    ["-canvas", "50%", "50%", "bottom-right"], # d
  ]

  for i in range(0, 4):
    p = subprocess.Popen([nconvert, "-quiet", "-overwrite"] +
      ["-rtype", "lanczos2"] +
      variants[i] +
      tiff_out_format +
      ["-o", paths[i], tile],
      stdout=subprocess.DEVNULL)

    try:
      p.wait()
    except KeyboardInterrupt:
      print("[INFO] Terminating...")
      p.send_signal(signal.SIGINT)
      exit(1)

  return paths


def postProcessAllLevels(tuple):
  file, outPath, tileWidth, formats, ddsConverterPath, ddsxCvtPath = tuple
  name = os.path.splitext(file)[0]
  tifPath = os.path.join(outPath, file)
  print(f"[INFO] Postprocessing {file}")

  resize = ["-resize", f"{tileWidth}", f"{tileWidth}"]
  ok = True
  if "avif" in formats:
    ok = runTool([nconvert, "-quiet", "-overwrite"] + avif_out_format + resize + ["-o", os.path.join(outPath, name), tifPath])
  if "ddsx" in formats:
    resizedTifPath = os.path.join(outPath, f"{name}_resized.tif")
    ddsxOk = runTool([nconvert, "-quiet", "-overwrite"] + tiff_out_format + resize + ["-o", resizedTifPath, tifPath],
      outputPath=resizedTifPath)
    ddsxOk = ddsxOk and convertTifToDdsx(resizedTifPath, outPath, name, ddsConverterPath, ddsxCvtPath)
    if os.path.exists(resizedTifPath):
      os.remove(resizedTifPath)
    ok = ddsxOk and ok
  if ok:
    os.remove(tifPath)
  return ok


def main():
  global nconvert

  parser = argparse.ArgumentParser(description='Cut tiled map in top down manner. Mostly intended for fog of war generation.' +
    ' Works with dng games.')
  parser.add_argument("game", type=str, help="Which game to use (should work with any dng game)")
  parser.add_argument("scene", type=str, help="Path to the scene *.blk file (as it passed to the game)")
  parser.add_argument("image", type=str, help="Path to the input image file")
  parser.add_argument("--fog", action="store_true", help="Save to the fog folder")
  parser.add_argument("--nconvert", type=str, help="Path to nconvert.exe", default=nconvert)
  parser.add_argument("--format", choices=["avif", "ddsx", "both"], default="avif",
    help="Tile output format. ddsx tiles are BC1 (BC3 with alpha) for the PC target, the game prefers them when combined.ddsx exists")
  parser.add_argument("--ddsConverter", type=str, help="Path to ddsConverter-dev.exe", default=ddsConverter)
  parser.add_argument("--ddsxCvt", type=str, help="Path to ddsxCvt.exe", default=ddsxCvt)

  args = parser.parse_args()
  nconvert = args.nconvert
  formats = ["avif", "ddsx"] if args.format == "both" else [args.format]
  for tool in ([args.ddsConverter, args.ddsxCvt] if "ddsx" in formats else []):
    if not os.path.exists(tool):
      print(f"[ERROR] Tool not found: {tool}")
      exit(1)

  scenePath = os.path.join(DAGOR_DIR, args.game, "prog", "gameBase", args.scene)
  if not os.path.exists(scenePath):
    scenePath = os.path.join(DAGOR_DIR, args.game, "prog", "gamebase_dev", args.scene)

  tilesPath = ""
  tileWidth = 512
  blocks = datablock.DataBlock(scenePath, include_includes = True, preserve_formating = False)
  for block in blocks.getBlocks():
    if block.name != "entity":
      continue
    path = block.getParam("tiled_map__tilesPath")
    width = block.getParam("tiled_map__tileWidth")
    if path and width:
      tilesPath = path
      tileWidth = int(width)

  if tilesPath == "":
    print(f"[ERROR] tiled_map__tilesPath not found in {levelPath}")
    exit(1)


  for block in blocks.getBlocks():
    if block.name != "entity":
      continue
    param = block.getParam("tiled_map__zlevels")
    if param:
      global zlevels
      zlevels = int(param)
      break

  if zlevels == -1:
    print(f"[ERROR] zlevels not found in {levelPath}")
    exit(1)

  outputPrefix = os.path.basename(tilesPath).split(".")[0]
  global outPath
  outPath = os.path.join(DAGOR_DIR, args.game, "develop", "gui", "maps", outputPrefix)
  if args.fog:
    outPath = os.path.join(outPath, "fog")

  try:
    files = [f for f in os.listdir(outPath) if os.path.isfile(os.path.join(outPath, f))]
    for f in files:
      os.remove(os.path.join(outPath, f))
  except OSError as error:
    print(f"[INFO] Previous results not found: {outPath}")

  try:
    os.mkdir(outPath)
  except OSError as error:
    print(f"[INFO] Directory already exists: {outPath}")

  resizeBaseImage(args.image, tileWidth)
  splittedTiles = []
  for i in range(0, zlevels):
    result = []
    print(f"[INFO] Processing zoom level {i+1}")
    if i == 0:
      result = splitTile((args.image, i+1))
    else:
      for tile in splittedTiles:
        result.extend(splitTile((tile, i+1)))
    splittedTiles = result

  postProcessAllLevelsArgs = [(f, outPath, tileWidth, formats, args.ddsConverter, args.ddsxCvt)
    for f in os.listdir(outPath) if os.path.isfile(os.path.join(outPath, f)) and f.endswith(".tif")]
  print(f"[INFO] Found {len(postProcessAllLevelsArgs)} files for post processing")

  pool = Pool(threads)
  try:
    result = pool.map_async(postProcessAllLevels, postProcessAllLevelsArgs)
    while not result.ready():
      pass
  except KeyboardInterrupt:
    print("[INFO] KeyboardInterrupt")
    pool.terminate()
    pool.join()
    exit(1)
  else:
    print("[INFO] Finished")
    pool.close()
    pool.join()

  failedCount = result.get().count(False)
  if failedCount > 0:
    print(f"[ERROR] {failedCount} tiles failed to convert, their .tif files are kept in {outPath}")
    exit(1)

if __name__ == "__main__":
  main()
