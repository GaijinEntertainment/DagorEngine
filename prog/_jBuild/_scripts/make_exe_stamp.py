import sys
import time
import os
import subprocess

def get_git_revision_hash():
    return subprocess.check_output(['git', 'rev-parse', 'HEAD']).decode('ascii').strip()

if len(sys.argv) < 3:
  print("ERROR: too few args, usage: make_exe_stamp.py <dest_fpath> <exe_fpath>")
  sys.exit(13)

f = open(sys.argv[1], 'wt')
f.write('{0} ({1}, rev: {2})\n'.format(
  time.strftime("%b %d %Y  %H:%M:%S", time.localtime(os.path.getmtime(sys.argv[2]))),
  os.path.basename(sys.argv[2]),
  get_git_revision_hash()))
f.close()
