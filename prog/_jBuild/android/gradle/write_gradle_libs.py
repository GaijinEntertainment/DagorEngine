import sys

# Writes maven coordinates (from a module's AndroidLibs jam variable) into a
# per-module .libs file in the gradle staging dir, one coordinate per line.
# These files are later merged into dagor.dependencies.gradle by make_gradle_deps.py.
#
# Usage: write_gradle_libs.py <out_file> <coordinate> [<coordinate> ...]


def main():
  if len(sys.argv) < 3:
    print('Usage: python write_gradle_libs.py <out_file> <coordinate> [<coordinate> ...]')
    sys.exit(1)

  out, coords = sys.argv[1], sys.argv[2:]

  for c in coords:
    if c.count(':') < 2:
      print("ERROR: '{}' is not a valid maven coordinate (expected group:artifact:version)".format(c))
      sys.exit(1)

  with open(out, 'w') as f:
    for c in coords:
      f.write(c + '\n')
  print('* android libs ({}): {}'.format(len(coords), out))


if __name__ == '__main__':
  main()
