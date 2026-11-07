#!/usr/bin/env python3
"""Fill the prebuilt gradle libs dir (devtools/android_libs) from a gradle cache.

The prebuilt dir is a plain local maven repository (<group as path>/<artifact>/
<version>/<files>). By default jam builds take gradle libraries only from it
(see GradleLibAllowDownload in android/clang-link.jam).

To add or update libraries, build the apk once with
  -sGradleLibAllowDownload=yes -sGradleLibExport=yes
jam then runs gradle with a separate gradle user home (<user profile>/
.gradle-dagor-export, next to the default ~/.gradle), so its cache holds only
what export builds downloaded
(libraries, plugins, parent POMs, BOMs, gradle module metadata), and calls this
script to copy the whole cache to the prebuilt dir (jam GradleLibExportDir, the
same dir that builds read). The home is kept between builds and shared by all
targets, so the export also copies the libraries of earlier export builds;
delete that home to export only the current build. Build on each host OS that
builds android, because some artifacts (aapt2) have a classifier per OS.

Each copied file gets a <file>.sha1 next to it; --verify checks them and
reports files without a .sha1.
The gradle cache keeps a file under its 'name' from the gradle module metadata
(.module), but a repo must serve it under its 'url' (for example
room-runtime-release.aar -> room-runtime-android-2.7.0.aar); export uses the url.
The gradle cache keeps no maven-metadata.xml, but gradle needs it in a local
repo to resolve a version range (like [9.7.0,10.0.0) in a POM); export writes
it for each artifact (--update-metadata only does this step, and also renames
files of older exports to their url). It lists only the complete versions,
the same ones as COMPLETE_LIST, so a version range never selects a version
whose library files are not in the repo.

COMPLETE_LIST holds the module versions that have all their library files in
the repo: a group:artifact:version line, then an indented line for each of
these files. The file names are on their own lines, because older init scripts
read every line with three ':' fields as a version and skip the other lines.
With GradleLibAllowDownload=yes the init script takes these versions only from
the repo, so a remote repository that fails (5xx, timeout) does not break them.
The init script skips (with a warning) a listed version whose files are not in
the repo any more, or that has no file lines (a list of an older export), so it
comes from the remote repositories again; --verify reports the versions whose
files are not in the repo as errors.
A module version with a .pom only is normal: gradle gets only the metadata
of the versions that lose the version conflict resolution.

--include limits the export to some groups or artifacts. Use it to take a
library from the main gradle cache (~/.gradle) when its repository is not
available any more, for example: --gradle-home ~/.gradle --include com.vk.id

Usage:
  export_gradle_libs.py --prepare-home <gradle_home>
  export_gradle_libs.py --gradle-home <gradle_home> --dest <repo_dir> [--include <group[:artifact]>]...
  export_gradle_libs.py --update-metadata <repo_dir>
  export_gradle_libs.py --verify <repo_dir>
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import sys
import time


def log(msg):
  print('export_gradle_libs: %s' % msg, flush=True)


def sha1_of(path):
  h = hashlib.sha1()
  with open(path, 'rb') as f:
    for chunk in iter(lambda: f.read(1 << 20), b''):
      h.update(chunk)
  return h.hexdigest()


def same_hash(a, b):
  try:
    return int(a, 16) == int(b, 16)
  except ValueError:
    return False


def prepare_home(home):
  os.makedirs(home, exist_ok=True)
  user_home = os.environ.get('GRADLE_USER_HOME') or os.path.join(os.path.expanduser('~'), '.gradle')
  src = os.path.join(user_home, 'gradle.properties')
  if os.path.isfile(src) and os.path.abspath(user_home) != os.path.abspath(home):
    shutil.copyfile(src, os.path.join(home, 'gradle.properties'))
  return 0


def iter_cache(cache):
  # layout: files-2.1/<group>/<module>/<version>/<sha1>/<file>
  for group in sorted(os.listdir(cache)):
    gdir = os.path.join(cache, group)
    if not os.path.isdir(gdir):
      continue
    for module in sorted(os.listdir(gdir)):
      mdir = os.path.join(gdir, module)
      if not os.path.isdir(mdir):
        continue
      for version in sorted(os.listdir(mdir)):
        vdir = os.path.join(mdir, version)
        if not os.path.isdir(vdir):
          continue
        for hdir in sorted(os.listdir(vdir)):
          fdir = os.path.join(vdir, hdir)
          if not os.path.isdir(fdir):
            continue
          for fn in sorted(os.listdir(fdir)):
            if os.path.isfile(os.path.join(fdir, fn)):
              yield group, module, version, hdir, fn, os.path.join(fdir, fn)


def included(group, module, includes):
  if not includes:
    return True
  for inc in includes:
    if ':' in inc:
      if '%s:%s' % (group, module) == inc:
        return True
    elif group == inc or group.startswith(inc + '.'):
      return True
  return False


def is_plain_name(name):
  return bool(name) and name not in ('.', '..') and '/' not in name and '\\' not in name


def module_file_urls(module_path):
  try:
    with open(module_path, encoding='utf-8') as f:
      data = json.load(f)
  except (OSError, ValueError):
    return {}
  urls = {}
  for v in data.get('variants', []):
    for fl in v.get('files', []):
      name, url = fl.get('name'), fl.get('url')
      if is_plain_name(name) and is_plain_name(url) and name != url:
        urls[name] = url
  return urls


def cache_module_urls(cache, group, module, version, memo):
  key = (group, module, version)
  if key not in memo:
    memo[key] = {}
    vdir = os.path.join(cache, group, module, version)
    for hdir in os.listdir(vdir):
      path = os.path.join(vdir, hdir, '%s-%s.module' % (module, version))
      if os.path.isfile(path):
        memo[key] = module_file_urls(path)
        break
  return memo[key]


def rename_to_urls(repo):
  for root, _dirs, files in os.walk(repo):
    for fn in files:
      if not fn.endswith('.module'):
        continue
      for name, url in module_file_urls(os.path.join(root, fn)).items():
        src, dst = os.path.join(root, name), os.path.join(root, url)
        if os.path.isfile(src) and not os.path.exists(dst):
          os.rename(src, dst)
          if os.path.isfile(src + '.sha1'):
            os.replace(src + '.sha1', dst + '.sha1')
          log('renamed %s -> %s' % (os.path.relpath(src, repo), url))


COMPLETE_LIST = 'dagor_complete_modules.txt'


def write_text(path, text):
  tmp = path + '.tmp'
  with open(tmp, 'w') as f:
    f.write(text)
  os.replace(tmp, path)


def complete_files(vdir, artifact, version, files):
  """The files that make a module version complete, or None when some of them are missing."""
  base = '%s-%s' % (artifact, version)
  need = {f for f in (base + '.pom', base + '.module') if f in files}
  if base + '.pom' in files:
    with open(os.path.join(vdir, base + '.pom'), errors='replace') as f:
      m = re.search(r'<packaging>\s*([^<\s]+)\s*</packaging>', f.read())
    packaging = m.group(1) if m else 'jar'
    if packaging != 'pom':
      lib = base + ('.aar' if packaging == 'aar' else '.jar')
      if lib not in files:
        return None
      need.add(lib)
  if base + '.module' in files:
    try:
      with open(os.path.join(vdir, base + '.module'), encoding='utf-8') as f:
        data = json.load(f)
    except (OSError, ValueError):
      return None
    variants = data.get('variants', [])
    if not any('available-at' in v for v in variants):
      urls = {fl.get('url') for v in variants for fl in v.get('files', [])}
      urls = {u for u in urls if u and not u.endswith(('-sources.jar', '-javadoc.jar', '-versionMetadata.json'))}
      if not urls <= files:
        return None
      need |= urls
  return sorted(need)


def version_key(v):
  return [(0, int(p), '') if p.isdigit() else (1, 0, p) for p in re.split(r'[.\-]', v)]


def read_complete_list(path):
  """[(group:artifact:version, [file names])] of COMPLETE_LIST; the list is empty for lines without files."""
  entries = []
  with open(path) as f:
    for line in f:
      if not line.strip():
        continue
      if line[0].isspace():
        if entries:
          entries[-1][1].append(line.strip())
      elif len(line.strip().split(':')) == 3:
        entries.append((line.strip(), []))
  return entries


def update_metadata(repo):
  if not os.path.isdir(repo) or not os.listdir(repo):
    log('ERROR: %s is empty or not found' % repo)
    return 1
  rename_to_urls(repo)
  count = 0
  complete = []
  stamp = time.strftime('%Y%m%d%H%M%S', time.gmtime())
  for root, dirs, _files in os.walk(repo):
    artifact = os.path.basename(root)
    with_meta = [v for v in dirs
                 if os.path.isfile(os.path.join(root, v, '%s-%s.pom' % (artifact, v)))
                 or os.path.isfile(os.path.join(root, v, '%s-%s.module' % (artifact, v)))]
    if not with_meta:
      continue
    need = {v: complete_files(os.path.join(root, v), artifact, v, set(os.listdir(os.path.join(root, v))))
            for v in with_meta}
    versions = sorted((v for v in with_meta if need[v]), key=version_key)
    group = os.path.relpath(os.path.dirname(root), repo).replace(os.sep, '.')
    complete += ['%s:%s:%s' % (group, artifact, v) + ''.join('\n  ' + fn for fn in need[v]) for v in versions]
    xml = ['<?xml version="1.0" encoding="UTF-8"?>', '<metadata>',
           '  <groupId>%s</groupId>' % group, '  <artifactId>%s</artifactId>' % artifact,
           '  <versioning>']
    if versions:
      xml += ['    <latest>%s</latest>' % versions[-1]]
    xml += ['    <versions>']
    xml += ['      <version>%s</version>' % v for v in versions]
    xml += ['    </versions>', '    <lastUpdated>%s</lastUpdated>' % stamp, '  </versioning>', '</metadata>', '']
    path = os.path.join(root, 'maven-metadata.xml')
    try:
      with open(path) as f:
        old = re.sub(r'<lastUpdated>\d+</lastUpdated>', '', f.read())
    except OSError:
      old = None
    new = '\n'.join(xml)
    if old == re.sub(r'<lastUpdated>\d+</lastUpdated>', '', new):
      continue
    write_text(path, new)
    count += 1
  log('updated maven-metadata.xml of %d artifact(s) in %s' % (count, repo))
  write_text(os.path.join(repo, COMPLETE_LIST), ''.join(c + '\n' for c in sorted(complete)))
  log('%d complete module version(s) listed in %s' % (len(complete), COMPLETE_LIST))
  return 0


def export(home, dest, includes=None):
  cache = os.path.join(home, 'caches', 'modules-2', 'files-2.1')
  if not os.path.isdir(cache):
    log('no gradle cache in %s, nothing to export' % home)
    return 0

  if os.path.isdir(dest):
    rename_to_urls(dest)

  added, present, errors = 0, 0, []
  url_memo = {}
  for group, module, version, hdir, fn, src in iter_cache(cache):
    if not included(group, module, includes):
      continue
    gav = '%s:%s:%s' % (group, module, version)
    if version.endswith('-SNAPSHOT'):
      log('skipped snapshot %s %s' % (gav, fn))
      continue
    digest = sha1_of(src)
    if not same_hash(hdir, digest):
      errors.append('%s: sha1 %s does not match the cache dir name' % (src, digest))
      continue

    fn = cache_module_urls(cache, group, module, version, url_memo).get(fn, fn)
    dst_dir = os.path.join(dest, *(group.split('.') + [module, version]))
    dst = os.path.join(dst_dir, fn)
    if os.path.isfile(dst):
      if sha1_of(dst) == digest:
        present += 1
        if not os.path.isfile(dst + '.sha1'):
          with open(dst + '.sha1', 'w') as f:
            f.write(digest)
      else:
        errors.append('%s: differs from the downloaded file of %s' % (dst, gav))
      continue

    log('add %s %s' % (gav, fn))
    added += 1
    os.makedirs(dst_dir, exist_ok=True)
    with open(dst + '.sha1', 'w') as f:
      f.write(digest)
    tmp = dst + '.tmp'
    shutil.copyfile(src, tmp)
    os.replace(tmp, dst)

  log('added %d file(s) to %s, %d already present' % (added, dest, present))
  if os.path.isdir(dest) and os.listdir(dest):
    update_metadata(dest)
  for e in errors:
    log('ERROR: %s' % e)
  return 1 if errors else 0


def verify(repo):
  checked, errors = 0, []
  for root, _dirs, files in os.walk(repo):
    for fn in files:
      if not fn.endswith('.sha1'):
        if fn not in ('maven-metadata.xml', COMPLETE_LIST) and fn + '.sha1' not in files:
          errors.append('no .sha1, not checked: %s' % os.path.join(root, fn))
        continue
      path = os.path.join(root, fn[:-len('.sha1')])
      checked += 1
      if not os.path.isfile(path):
        errors.append('file is missing: %s' % path)
        continue
      with open(os.path.join(root, fn)) as f:
        words = f.read().split()
      expected = words[0] if words else ''
      if not same_hash(expected, sha1_of(path)):
        errors.append('sha1 mismatch: %s' % path)
  if not checked:
    errors.append('no .sha1 files in %s, nothing is checked' % repo)
  list_path = os.path.join(repo, COMPLETE_LIST)
  if os.path.isfile(list_path):
    for gav, files in read_complete_list(list_path):
      group, artifact, version = gav.split(':')
      vdir = os.path.join(repo, *group.split('.'), artifact, version)
      missing = [fn for fn in files if not os.path.isfile(os.path.join(vdir, fn))]
      if not files and not (os.path.isdir(vdir) and complete_files(vdir, artifact, version, set(os.listdir(vdir)))):
        missing = ['some of its files']
      if missing:
        errors.append('%s lists %s, but %s not found (run --update-metadata)'
                      % (COMPLETE_LIST, gav, ', '.join(missing)))
  log('checked %d file(s) in %s' % (checked, repo))
  for e in errors:
    log('ERROR: %s' % e)
  return 1 if errors else 0


def main():
  p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
  mode = p.add_mutually_exclusive_group(required=True)
  mode.add_argument('--prepare-home', metavar='GRADLE_HOME', help='prepare a separate gradle user home before the build')
  mode.add_argument('--gradle-home', help='gradle user home to export from (needs --dest)')
  mode.add_argument('--update-metadata', metavar='REPO_DIR', help='write maven-metadata.xml for each artifact of a local maven repo')
  mode.add_argument('--verify', metavar='REPO_DIR', help='check the .sha1 files of a local maven repo')
  p.add_argument('--dest', help='local maven repo to export to')
  p.add_argument('--include', action='append', metavar='GROUP[:ARTIFACT]', help='export only these (repeatable)')
  args = p.parse_args()

  if args.gradle_home:
    if not args.dest:
      p.error('--gradle-home needs --dest')
    return export(args.gradle_home, args.dest, args.include)
  if args.dest or args.include:
    p.error('--dest and --include are only for --gradle-home')
  if args.prepare_home:
    return prepare_home(args.prepare_home)
  if args.update_metadata:
    return update_metadata(args.update_metadata)
  return verify(args.verify)


if __name__ == '__main__':
  sys.exit(main())
