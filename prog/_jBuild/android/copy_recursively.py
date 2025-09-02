import os
import sys
import shutil
import fnmatch

skip_dirs = [".DS_Store", "CVS", ".git"]

def copy_files(src, dst):
  if not os.path.exists(dst):
      os.makedirs(dst)

  for root, dirs, files in os.walk(src):
    dirs[:] = [d for d in dirs if not any(fnmatch.fnmatch(d, pattern) for pattern in skip_dirs)]
    rel_path = os.path.relpath(root, src)
    target_dir = os.path.join(dst, rel_path)

    if not os.path.exists(target_dir):
      os.makedirs(target_dir)

    for file_name in files:
      src_file = os.path.join(root, file_name)
      dst_file = os.path.join(target_dir, file_name)
      shutil.copy2(src_file, dst_file)

def main():
  # Expect an even number of arguments beyond the script name
  # e.g. copy_recursively.py src1 dst1 src2 dst2 ...
  if (len(sys.argv) - 1) < 2 or (len(sys.argv) - 1) % 2 != 0:
    print("Usage: python copy_recursively.py <source1> <destination1> [<source2> <destination2> ...]")
    sys.exit(1)

  args = sys.argv[1:]
  for i in range(0, len(args), 2):
    source_folder = args[i]
    destination_folder = args[i + 1]
    print(f"Copying from {source_folder} to {destination_folder} ...")
    copy_files(source_folder, destination_folder)

if __name__ == '__main__':
    main()
