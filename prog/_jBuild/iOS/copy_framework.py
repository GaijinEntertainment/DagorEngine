import os
import sys
import shutil

if len(sys.argv) < 3:
    sys.exit(1)

src = sys.argv[1]
dst = sys.argv[2]

exclude = ['.DS_Store', 'CVS', '.svn', '.git', '.hg', 'Headers', 'PrivateHeaders', 'Modules']

def get_framework_files(root):
  res = []
  for f in os.listdir(root):
    if f in exclude or f.endswith('.tbd'):
      continue
    fullpath = os.path.join(root, f)
    if os.path.isfile(fullpath):
      res.append(fullpath)
    else:
      res += get_framework_files(fullpath)
  return res

for s in get_framework_files(src):
  d = os.path.join(dst, os.path.relpath(s, src))
  os.makedirs(os.path.dirname(d), exist_ok=True)
  shutil.copy2(s, d)