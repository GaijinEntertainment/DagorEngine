"""Run ducible, retrying transient rename failures.

ducible patches a binary by writing '<file>.tmp' and renaming it over the
original. On Windows an on-access scanner can still hold a handle without
FILE_SHARE_DELETE on a binary the linker has just written, so that rename
fails with 'Access is denied'. The window is short (seconds) and scales with
file size, so large PDBs are hit most; retry instead of failing the build.
"""

import os
import subprocess
import sys
import time

RETRY_DELAYS = (0.5, 1.0, 2.0, 3.0, 4.0, 6.0)
TRANSIENT_MARKERS = ('failed to rename', 'access is denied')

def run(cmd):
  proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
  out = proc.communicate()[0].decode('utf-8', 'replace')
  return proc.returncode, out

def remove_stale_tmp(paths):
  for path in paths:
    try:
      os.remove(path + '.tmp')
    except OSError:
      pass

def main(argv):
  if len(argv) < 2:
    sys.stderr.write('usage: run_ducible.py <ducible> <image> [<pdb>]\n')
    return 2

  attempts = len(RETRY_DELAYS) + 1
  for attempt in range(attempts):
    code, out = run(argv)
    if code == 0:
      sys.stdout.write(out)
      return 0

    transient = any(marker in out.lower() for marker in TRANSIENT_MARKERS)
    if not transient or attempt + 1 == attempts:
      sys.stdout.write(out)
      remove_stale_tmp(argv[1:])
      return code

    sys.stdout.write(f'ducible: transient failure, retrying ({attempt + 1} of {attempts - 1})\n')
    sys.stdout.flush()
    time.sleep(RETRY_DELAYS[attempt])

  return 1

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
