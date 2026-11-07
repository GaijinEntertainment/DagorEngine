from multiprocessing import Pool, cpu_count
import os
import argparse

# Convert the existing avif tiles of a tiled map folder to ddsx, for maps that have no source to regenerate them.
# Source tiles stay in place: gen_one_big_image.py reads them.

from gen_tiled_map import convertTifToDdsx, runTool, nconvert, ddsConverter, ddsxCvt

threads = max(cpu_count() - 2, 1)

def convertTile(tuple):
  avifPath, ddsConverterPath, ddsxCvtPath = tuple
  outPath = os.path.dirname(avifPath)
  name = os.path.splitext(os.path.basename(avifPath))[0]
  tifPath = os.path.join(outPath, f"{name}_from_avif.tif")
  print(f"[INFO] Converting {avifPath}")
  ok = runTool([nconvert, "-quiet", "-overwrite", "-out", "tiff", "-o", tifPath, avifPath], outputPath=tifPath)
  ok = ok and convertTifToDdsx(tifPath, outPath, name, ddsConverterPath, ddsxCvtPath)
  if os.path.exists(tifPath):
    os.remove(tifPath)
  return ok

def main():
  parser = argparse.ArgumentParser(description='Convert avif tiles of tiled map folders to ddsx tiles next to them.')
  parser.add_argument("folders", type=str, nargs="+", help="Tiled map folders with <quadkey>.avif and combined.avif tiles")
  parser.add_argument("--ddsConverter", type=str, help="Path to ddsConverter-dev.exe", default=ddsConverter)
  parser.add_argument("--ddsxCvt", type=str, help="Path to ddsxCvt.exe", default=ddsxCvt)
  args = parser.parse_args()

  for tool in [nconvert, args.ddsConverter, args.ddsxCvt]:
    if not os.path.exists(tool):
      print(f"[ERROR] Tool not found: {tool}")
      exit(1)

  convertArgs = []
  # ddsConverter runs in the tile folder, so relative tile paths do not resolve there
  for folder in map(os.path.abspath, args.folders):
    if not os.path.exists(os.path.join(folder, "combined.avif")):
      print(f"[ERROR] combined.avif not found in {folder}")
      exit(1)
    convertArgs += [(os.path.join(folder, f), args.ddsConverter, args.ddsxCvt)
      for f in os.listdir(folder) if os.path.isfile(os.path.join(folder, f)) and f.endswith(".avif")]
  print(f"[INFO] Found {len(convertArgs)} tiles")

  pool = Pool(threads)
  try:
    result = pool.map_async(convertTile, convertArgs)
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
    print(f"[ERROR] {failedCount} tiles failed to convert")
    exit(1)

if __name__ == "__main__":
  main()
