from multiprocessing import Pool, cpu_count
import os
import signal
import subprocess
import datablock as datablock
import argparse
import time

# Generate z levels for tiled map. Files are named as quadkeys.
# See more: https://learn.microsoft.com/en-us/azure/azure-maps/zoom-levels-and-tile-grid?tabs=csharp#quadkey-indices

DAGOR_DIR = os.environ.get('DAGOR_DIR',r'd:/dagor')

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

def tileXYToQuadKey(tileX, tileY, zoom):
  quadkey = ""
  for i in range(zoom, 0, -1):
    digit = '0'
    mask = 1 << (i - 1)
    if (tileX & mask) != 0:
      digit = chr(ord(digit) + 1)
    if (tileY & mask) != 0:
      digit = chr(ord(digit) + 2)
    quadkey += digit
  return quadkey

# merges 4 tiles into one
# a | b
# c | d
def mergeTiles(a, b, c, d, output):
  p = subprocess.Popen([nconvert, "-quiet", "-overwrite"] +
    ["-bgcolor2", "0", "0", "0", "0"] +
    ["-canvas", "200%", "200%", "top-left"] +
    ["-wmflag", "top-right", "-wmfile"] + [b] +
    ["-wmflag", "bottom-left", "-wmfile"] + [c] +
    ["-wmflag", "bottom-right", "-wmfile"] + [d] +
    ["-resize", "50%", "50%"] +
    tiff_out_format +
    ["-o", os.path.join(outPath, output), a],
    stdout=subprocess.DEVNULL)
  try:
    p.wait()
  except KeyboardInterrupt:
    print("[INFO] Terminating...")
    p.send_signal(signal.SIGINT)
    exit(1)

def preProcessBaseLevel(tuple):
  file, inputPath, outPath = tuple
  name = os.path.splitext(file)[0]
  print(f"[INFO] Preprocessing base level {name}")

  p = subprocess.Popen([nconvert, "-quiet", "-overwrite"] +
    tiff_out_format +
    ["-o", os.path.join(outPath, name), os.path.join(inputPath, file)],
    stdout=subprocess.DEVNULL)
  try:
    p.wait()
  except KeyboardInterrupt:
    print("[INFO] Terminating...")
    p.send_signal(signal.SIGINT)
    exit(1)

def postProcessAllLevels(tuple):
  file, outPath = tuple
  name = os.path.splitext(file)[0]
  print(f"[INFO] Postprocessing {name}")

  p = subprocess.Popen([nconvert, "-quiet", "-overwrite"] +
    avif_out_format +
    ["-D"] + # delete original file
    ["-o", os.path.join(outPath, name), os.path.join(outPath, file)],
    stdout=subprocess.DEVNULL)
  try:
    p.wait()
  except KeyboardInterrupt:
    print("[INFO] Terminating...")
    p.send_signal(signal.SIGINT)
    exit(1)

def mergeTilesByIdx(tuple):
  x, y, z, outPath = tuple
  a = tileXYToQuadKey(x, y, z + 1)
  b = tileXYToQuadKey(x + 1, y, z + 1)
  c = tileXYToQuadKey(x, y + 1, z + 1)
  d = tileXYToQuadKey(x + 1, y + 1, z + 1)
  if z == 0:
    result = "combined.tif"
  else:
    result = tileXYToQuadKey(x // 2, y // 2, z)
  print(f"[INFO] Merging {a} {b} {c} {d} into {result}")
  mergeTiles(
    os.path.join(outPath, f"{a}.tif"),
    os.path.join(outPath, f"{b}.tif"),
    os.path.join(outPath, f"{c}.tif"),
    os.path.join(outPath, f"{d}.tif"),
    os.path.join(outPath, result)
  )


def mergeZoomLevel(zoomlevel):
  argsList = [(x, y, zoomlevel, outPath) for y in range(0, 2**(zoomlevel+1), 2) for x in range(0, 2**(zoomlevel+1), 2)]
  pool = Pool(threads)
  try:
    result = pool.map_async(mergeTilesByIdx, argsList)
    while not result.ready():
      pass
  except KeyboardInterrupt:
    print("[INFO] KeyboardInterrupt")
    pool.terminate()
    pool.join()
    exit(1)
  else:
    print(f"[INFO] Finished zoom level {zoomlevel}")
    pool.close()
    pool.join()

def rmDirRecusive(path):
  for root, dirs, files in os.walk(path, topdown=False):
    for name in files:
        os.remove(os.path.join(root, name))
    for name in dirs:
        os.rmdir(os.path.join(root, name))

def main():
  global nconvert

  if not os.path.exists(DAGOR_DIR):
    print(f"[ERROR] DAGOR_DIR not found: in following path {DAGOR_DIR}. Consider to set Environment variable DAGOR_DIR to the correct path.")
    exit(1)

  parser = argparse.ArgumentParser(description='Generate tiled map. The base level tiles are collected in game ' +
    'and saved in the `Screenshots/genTiledMap` directory. ' +
    'The scene should have a ' +
    '`tiled_map` entity and static camera. The hero spawn should be disabled. ' +
    'NOTE: The script will remove files in the `Screenshots/genTiledMap` directory.'
    '\n' +
    'The files processing is done in intermediate tiff format. As there is a bug with alpha channel in avif merging.' +
    'The 4 tiles of the previous level are merged into one tile of the current level. And then scaled down by 50%.' +
    'The post processing converts tiff to avif format for better compression.'
    )
  parser.add_argument("game", type=str, help="Which game to use (should work with any dng game)")
  parser.add_argument("scene", type=str, help="Path to the scene *.blk file (as it passed to the game)")
  parser.add_argument("--nconvert", type=str, help="Path to nconvert.exe", default=nconvert)

  args = parser.parse_args()
  nconvert = args.nconvert

  gamePWD = os.path.join(DAGOR_DIR, args.game, "game")
  gameEXE = os.path.join(gamePWD, "win64", f"{args.game}-dev.exe")
  scenePath = os.path.join(DAGOR_DIR, args.game, "prog", "gameBase", args.scene)

  if not os.path.exists(scenePath):
    scenePath = os.path.join(DAGOR_DIR, args.game, "prog", "gamebase_dev", args.scene)

  tilesPath = ""
  blocks = datablock.DataBlock(scenePath, include_includes = True, preserve_formating = False)
  for block in blocks.getBlocks():
    if block.name != "entity":
      continue
    param = block.getParam("tiled_map__tilesPath")
    if param:
      tilesPath = param
      break

  if tilesPath == "":
    print(f"[ERROR] tiled_map__tilesPath not found in {scenePath}")
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
    print(f"[ERROR] zlevels not found in {scenePath}")
    exit(1)

  outputPrefix = os.path.basename(tilesPath).split(".")[0]
  global outPath
  outPath = os.path.join(DAGOR_DIR, args.game, "develop", "gui", "maps", outputPrefix)
  global inputPath
  inputPath = os.path.join(gamePWD, "Screenshots", "genTiledMap")
  print(f"[INFO] Input path: {inputPath}")

  # clean up old files
  try:
    rmDirRecusive(inputPath)
  except OSError as error:
    print(f"[ERROR]: {error}")
    pass

  try:
    os.mkdir(inputPath)
  except OSError as error:
    pass

  p = subprocess.Popen([gameEXE] +
    ["-config:circuit:t=internal"] +
    ["-config:debug/useAddonVromSrc:b=yes"] +
    ["-config:disableMenu:b=yes"] +
    ["-config:generate_tiled_map:b=yes"] +
    ["-config:graphics/aoQuality:t=high"] +
    ["-config:graphics/dynamicShadowsQuality:t=high"] +
    ["-config:graphics/groundDeformations:t=high"] +
    ["-config:screenshots/dir:t=Screenshots/genTiledMap"] +
    ["-config:video/driver:t=dx11"] +
    ["-config:video/mode:t=windowed"] +
    ["-config:video/position:ip2=69,69"] +
    ["-config:video/resolution:t=864x864"] +
    ["-config:screenshots/writeDepthToPng:b=yes"] +
    ["-fatals_to_stderr"] +
    ["-logerr_to_stderr"] +
    ["-quiet"] +
    ["-scene:" + args.scene],
    cwd=gamePWD)

  try:
    p.wait()
  except KeyboardInterrupt:
    print("[INFO] Terminating...")
    p.send_signal(signal.SIGINT)
    exit(1)

  try:
    os.mkdir(outPath)
  except OSError as error:
    print(f"[INFO] Directory already exists: {outPath}")

  preprocessArgs = [(f, inputPath, outPath) for f in os.listdir(inputPath) if os.path.isfile(os.path.join(inputPath, f))]
  print(f"[INFO] Found {len(preprocessArgs)} files")

  pool = Pool(threads)
  try:
    result = pool.map_async(preProcessBaseLevel, preprocessArgs)
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

  for i in range(zlevels-1, 0, -1):
    mergeZoomLevel(i)
  mergeZoomLevel(0)

  postProcessAllLevelsArgs = [(f, outPath) for f in os.listdir(outPath) if os.path.isfile(os.path.join(outPath, f)) and f.endswith(".tif")]
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

if __name__ == "__main__":
  main()
