import argparse
import os
import sys

# Collects maven coordinates from all *.libs files in the gradle staging dir
# (produced from AndroidLibs by write_gradle_libs.py) and generates
# dagor.dependencies.gradle with a dependencies {} block; the file is applied
# from build.gradle. Replaces the old inject_gradle_libs.py which edited
# project.gradle in place.
#
# Usage: make_gradle_deps.py <stage_dir> [--prebuilt-dir <dir> [--no-download]]
# With --no-download it fails when a coordinate is not in the prebuilt dir
# (transitive dependencies are checked by gradle itself). An empty or missing
# prebuilt dir is reported by android/clang-link.jam before the build starts.

DEPS_FILE = 'dagor.dependencies.gradle'


def missing_in_prebuilt(coords, prebuilt_dir):
  missing = []
  for c in sorted(coords):
    parts = c.split('@', 1)[0].split(':')
    group, artifact, version = parts[0], parts[1], parts[2]
    if any(ch in version for ch in '+[]()') or version.startswith('latest.'):
      continue  # dynamic version, only gradle can resolve it
    vdir = os.path.join(prebuilt_dir, *(group.split('.') + [artifact, version]))
    base = os.path.join(vdir, '%s-%s' % (artifact, version))
    if not os.path.isfile(base + '.pom') and not os.path.isfile(base + '.module'):
      missing.append(c)
  return missing


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('stage_dir')
  parser.add_argument('--prebuilt-dir', help='local maven repo with prebuilt gradle libs')
  parser.add_argument('--no-download', action='store_true', help='gradle may use only the prebuilt dir')
  args = parser.parse_args()
  if args.no_download and not args.prebuilt_dir:
    parser.error('--no-download needs --prebuilt-dir')

  folder = args.stage_dir
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

  if args.no_download:
    missing = missing_in_prebuilt(coords, args.prebuilt_dir)
    if missing:
      print('ERROR: gradle downloads are disabled (GradleLibAllowDownload=no) and these AndroidLibs are not in {}:'.format(args.prebuilt_dir))
      for c in missing:
        print('  ' + c)
      print('Add them with -sGradleLibAllowDownload=yes -sGradleLibExport=yes (see export_gradle_libs.py),')
      print('or build with -sGradleLibAllowDownload=yes')
      sys.exit(1)

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
