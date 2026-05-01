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
  "-add_alpha", "255",
  "-out", "tiff",
]

tiff_compression_out_format = [ # Renamed from tiff_resample_out_format
  "-add_alpha", "255",
  "-out", "tiff",
  "-c", "3",           # LZW
  "-colors", "256"   # Reduce to 256 colors
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
    # ["-resize", "50%", "50%"] +
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

def mergeTilesByIdx(tuple):
  x, y, z, outPath = tuple
  a = tileXYToQuadKey(x, y, z + 1)
  b = tileXYToQuadKey(x + 1, y, z + 1)
  c = tileXYToQuadKey(x, y + 1, z + 1)
  d = tileXYToQuadKey(x + 1, y + 1, z + 1)
  if z == 0:
    result = "big_combined.tif"
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

def cleanup_temp_files(outPath, final_image_basename):
  """Remove all .tif files except the final preserved one."""
  if not final_image_basename:
    print(f"[WARNING] Final image path not found or invalid. Skipping cleanup of .tif files in '{outPath}'.")
    return

  print(f"[INFO] Cleaning up intermediate files. Preserving '{final_image_basename}'.")
  filesToRemove = [f for f in os.listdir(outPath) if os.path.isfile(os.path.join(outPath, f)) and f.endswith(".tif") and f != final_image_basename]
  for f_to_remove in filesToRemove:
    try:
      os.remove(os.path.join(outPath, f_to_remove))
    except OSError as e:
      print(f"[WARNING] Could not remove intermediate file {f_to_remove}: {e}")

def rename_image_file(source_path, target_path):
  """Rename/move an image file from source to target path. Returns the final path or None if failed."""
  if not os.path.exists(source_path):
    print(f"[WARNING] Source file '{source_path}' not found for renaming.")
    return None

  print(f"[INFO] Renaming image from '{source_path}' to '{target_path}'")
  try:
    os.replace(source_path, target_path)
    print(f"[INFO] Image successfully renamed to {target_path}")
    return target_path
  except OSError as e:
    print(f"[WARNING] Could not rename '{source_path}' to '{target_path}': {e}. The image remains as '{source_path}'.")
    return source_path

def handle_final_image_placement(initial_merged_image_path, final_target_full_path):
  """Handle final image placement logic. Returns the final path or None if failed."""
  # No action needed if paths are the same
  if final_target_full_path == initial_merged_image_path:
    print(f"[INFO] No renaming needed. Final image is {initial_merged_image_path}")
    return initial_merged_image_path

  # Delegate the actual renaming to specialized function
  return rename_image_file(initial_merged_image_path, final_target_full_path)

def perform_resize(initial_merged_image_path, final_target_full_path, final_target_basename, args_resolution, outPath):
  """Perform image resizing and return the path of the resulting image or None if failed."""
  if args_resolution <= 0:
    print(f"[ERROR] Invalid resolution specified: {args_resolution}. Must be a positive integer. Skipping resize.")
    return None

  print(f"[INFO] Attempting to resize image to {args_resolution}x{args_resolution} for final output '{os.path.basename(final_target_full_path)}'")
  temp_resized_path = os.path.normpath(os.path.join(outPath, f"{final_target_basename}_temp_resized.tif"))

  # Clean up any existing temp file
  if os.path.exists(temp_resized_path):
    try:
      os.remove(temp_resized_path)
    except OSError as e_rem:
      print(f"[WARNING] Could not remove pre-existing temp file {temp_resized_path}: {e_rem}")

  # Check prerequisites
  if not os.access(outPath, os.W_OK):
    print(f"[ERROR] Output directory '{outPath}' is not writable. Skipping nconvert resize operation.")
    return None

  if not (os.path.exists(initial_merged_image_path) and os.path.isfile(initial_merged_image_path) and os.access(initial_merged_image_path, os.R_OK)):
    print(f"[ERROR] Input file '{initial_merged_image_path}' for resize is missing, not a file, or not readable. Skipping resize.")
    return None

  # Prepare resize command
  temp_output_filename_for_nconvert = os.path.basename(temp_resized_path)
  input_filename_for_nconvert = os.path.basename(initial_merged_image_path)

  resize_command = [
      nconvert,
      "-overwrite",
      "-resize", str(args_resolution), str(args_resolution)
  ] + tiff_compression_out_format + [
      "-o", temp_output_filename_for_nconvert,
      input_filename_for_nconvert
  ]

  # Execute resize
  try:
    p_resize = subprocess.Popen(
        resize_command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=outPath
    )
    out, err = p_resize.communicate(timeout=300)

    if p_resize.returncode != 0:
      error_message_stderr = err.decode(errors='ignore').strip() if err else "No stderr output."
      error_message_stdout = out.decode(errors='ignore').strip() if out else "No stdout output."
      print(f"[ERROR] nconvert resize failed (code {p_resize.returncode}):")
      if error_message_stdout:
          print(f"  NConvert STDOUT:\\n{error_message_stdout}")
      if error_message_stderr:
          print(f"  NConvert STDERR:\\n{error_message_stderr}")
      return None

    # Validate output
    if not os.path.exists(temp_resized_path):
      print(f"[ERROR] nconvert reported success (code 0) but the output file '{temp_resized_path}' was not found. Resize considered failed.")
      return None

    if os.path.getsize(temp_resized_path) == 0:
      print(f"[ERROR] nconvert reported success (code 0) but the output file '{temp_resized_path}' is empty. Resize considered failed.")
      try:
          os.remove(temp_resized_path)
      except OSError as e_rem_empty:
          print(f"[WARNING] Could not remove empty temporary file {temp_resized_path}: {e_rem_empty}")
      return None

    # Move temp file to final location
    os.replace(temp_resized_path, final_target_full_path)
    print(f"[INFO] Image successfully resized and saved as {final_target_full_path}")
    return final_target_full_path

  except KeyboardInterrupt:
      print("[INFO] Terminating resize process...")
      if p_resize and p_resize.poll() is None:
          p_resize.send_signal(signal.SIGINT)
          try:
              p_resize.wait(timeout=5)
          except subprocess.TimeoutExpired:
              p_resize.kill()
      if os.path.exists(temp_resized_path):
          try: os.remove(temp_resized_path)
          except OSError as e: print(f"[WARNING] Could not remove temp file {temp_resized_path} during interrupt: {e}")
      exit(1)
  except subprocess.TimeoutExpired:
      print(f"[ERROR] nconvert resize command timed out after 300 seconds for resolution {args_resolution}x{args_resolution}.")
      if p_resize:
         p_resize.kill()
         p_resize.wait()
      if os.path.exists(temp_resized_path):
          try: os.remove(temp_resized_path)
          except OSError as e: print(f"[WARNING] Could not remove temp file {temp_resized_path} after timeout: {e}")
      return None
  except Exception as e:
      print(f"[ERROR] An unexpected error occurred during resize: {e}")
      if os.path.exists(temp_resized_path):
          try: os.remove(temp_resized_path)
          except OSError as e_rem: print(f"[WARNING] Could not remove temp file {temp_resized_path} after exception: {e_rem}")
      return None

def main():
  global nconvert
  global outPath
  global inputPath

  if not os.path.exists(DAGOR_DIR):
    print(f"[ERROR] DAGOR_DIR not found: in following path {DAGOR_DIR}. Consider to set Environment variable DAGOR_DIR to the correct path.")
    exit(1)

  parser = argparse.ArgumentParser(description='this script generates one big image from tiled map, needed for lvl designers')
  parser.add_argument("game", type=str, help="Which game to use (should work with any dng game)")
  parser.add_argument("scene", type=str, help="Path to the scene *.blk file (as it passed to the game)")
  parser.add_argument("--nconvert", type=str, help="Path to nconvert.exe", default=nconvert)
  parser.add_argument("--resolution", type=int, help="Optional final image resolution. If not specified, the original size after assembly is used.")
  parser.add_argument("--output_name", type=str, help="Optional base name for the final output. Defaults to 'big_combined'")

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
  outPath = os.path.normpath(os.path.join(DAGOR_DIR, args.game, "develop", "gui", "maps", outputPrefix))
  inputPath = os.path.normpath(os.path.join(DAGOR_DIR, args.game, "develop", "gui", "maps", outputPrefix))
  print(f"[INFO] Input path: {inputPath}")

  filesToMerge = [f for f in os.listdir(inputPath) if os.path.isfile(os.path.join(inputPath, f)) and len(f.split(".")[0]) == zlevels and f.endswith(".avif")]

  preprocessArgs = [(f, inputPath, outPath) for f in filesToMerge if os.path.isfile(os.path.join(inputPath, f))]
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

  initial_merged_image_path = os.path.normpath(os.path.join(outPath, "big_combined.tif"))
  path_of_image_to_preserve = initial_merged_image_path # Default to original merged image
  if not os.path.exists(initial_merged_image_path):
    print(f"[ERROR] Initial merged image {initial_merged_image_path} not found. Cannot apply resolution/name changes.")
    cleanup_temp_files(outPath, None)
    return

  # Process final image naming and resizing
  final_target_basename = args.output_name if args.output_name else "big_combined"
  final_target_filename_with_ext = f"{final_target_basename}.tif"
  final_target_full_path = os.path.normpath(os.path.join(outPath, final_target_filename_with_ext))

  # Handle resizing if requested
  if args.resolution:
    print(f"[INFO] Attempting to resize the final image to {args.resolution}x{args.resolution}...")
    resized_image_path = perform_resize(initial_merged_image_path, final_target_full_path, final_target_basename, args.resolution, outPath)

    if resized_image_path:
      final_image_path = resized_image_path
      cleanup_temp_files(outPath, os.path.basename(final_image_path))
    else:
      print(f"[WARNING] Resize failed. Using original merged image.")
      final_image_path = handle_final_image_placement(initial_merged_image_path, final_target_full_path)
      if final_image_path:
        cleanup_temp_files(outPath, os.path.basename(final_image_path))
  else:
    final_image_path = handle_final_image_placement(initial_merged_image_path, final_target_full_path)
    if final_image_path:
      cleanup_temp_files(outPath, os.path.basename(final_image_path))

  # Final validation
  if final_image_path:
    print(f"[INFO] Image generation completed successfully: {final_image_path}")
  else:
    print(f"[ERROR] Failed to generate final image.")
    cleanup_temp_files(outPath, None)
    return 1

if __name__ == "__main__":
  main()