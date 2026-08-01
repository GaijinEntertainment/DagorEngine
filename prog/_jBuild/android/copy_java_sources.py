import os
import re
import sys
import shutil

# Copies .java/.kt source file into <out_src_dir>/<package path>/<filename>,
# deriving the destination from the file's own 'package' declaration.
# Touches <stamp_file> on success (used by jam for dependency tracking).
#
# Usage: copy_java_sources.py <out_src_dir> <stamp_file> <source_file>

PACKAGE_RE = re.compile(r'^\s*package\s+([A-Za-z_][\w]*(?:\s*\.\s*[A-Za-z_][\w]*)*)\s*;?\s*$')


def get_package(filename):
  in_block_comment = False
  with open(filename, 'r', encoding='utf-8-sig', errors='replace') as f:
    for line in f:
      if in_block_comment:
        if '*/' in line:
          line = line.split('*/', 1)[1]
          in_block_comment = False
        else:
          continue
      line = line.split('//', 1)[0]
      while '/*' in line:
        before, after = line.split('/*', 1)
        if '*/' in after:
          line = before + after.split('*/', 1)[1]
        else:
          line = before
          in_block_comment = True
      m = PACKAGE_RE.match(line)
      if m:
        return re.sub(r'\s+', '', m.group(1))
      if line.strip() and not line.strip().startswith('@'):
        # first meaningful non-package statement reached - no package declared
        return None
  return None


def main():
  if len(sys.argv) != 4:
    print('Usage: python copy_java_sources.py <out_src_dir> <stamp_file> <source_file>')
    sys.exit(1)

  out_src_dir, stamp_file, src = sys.argv[1], sys.argv[2], sys.argv[3]

  if not os.path.isfile(src):
    print('ERROR: source file not found: {}'.format(src))
    sys.exit(1)

  package = get_package(src)
  if package is None:
    print('ERROR: no package declaration found in {}'.format(src))
    sys.exit(1)

  dest_dir = os.path.join(out_src_dir, *package.split('.'))
  dest = os.path.join(dest_dir, os.path.basename(src))
  os.makedirs(dest_dir, exist_ok=True)
  shutil.copy2(src, dest)
  print('* copy java/kt src to: {}'.format(dest))

  os.makedirs(os.path.dirname(stamp_file) or '.', exist_ok=True)
  with open(stamp_file, 'w') as f:
    f.write(dest + '\n')


if __name__ == '__main__':
  main()
