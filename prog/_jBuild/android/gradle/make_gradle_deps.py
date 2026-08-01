import os
import sys

# Collects maven coordinates from all *.libs files in the gradle staging dir
# (produced from AndroidLibs by write_gradle_libs.py) and generates
# dagor.dependencies.gradle with a dependencies {} block; the file is applied
# from build.gradle. Replaces the old inject_gradle_libs.py which edited
# project.gradle in place.
#
# Usage: make_gradle_deps.py <stage_dir>

DEPS_FILE = 'dagor.dependencies.gradle'


def main():
  if len(sys.argv) != 2:
    print('Usage: python make_gradle_deps.py <stage_dir>')
    sys.exit(1)

  folder = sys.argv[1]
  coords = set()
  for f in sorted(os.listdir(folder)):
    if not f.endswith('.libs'):
      continue
    with open(os.path.join(folder, f)) as fh:
      for line in fh:
        line = line.strip()
        if line:
          coords.add(line)

  # warn about same artifact requested with different versions (gradle will pick one)
  by_artifact = {}
  for c in sorted(coords):
    ga = c.rsplit(':', 1)[0]
    by_artifact.setdefault(ga, []).append(c)
  for ga, versions in by_artifact.items():
    if len(versions) > 1:
      print('WARNING: multiple versions requested for {}: {}'.format(ga, ', '.join(versions)))

  out_path = os.path.join(folder, DEPS_FILE)
  with open(out_path, 'w') as f:
    f.write('// Auto-generated from AndroidLibs by dagor build - do not edit\n')
    f.write('dependencies {\n')
    for c in sorted(coords):
      f.write("    implementation '{}'\n".format(c))
    f.write('}\n')
  print('generated {} ({} dependencies)'.format(out_path, len(coords)))


if __name__ == '__main__':
  main()
