import os
import subprocess
import sys, platform

class pushd:
    def __init__(self, path):
        self.olddir = os.getcwd()
        if path != '':
            os.chdir(os.path.normpath(path))
    def __enter__(self):
        pass
    def __exit__(self, type, value, traceback):
        os.chdir(self.olddir)

if platform.system() == "Windows" :
  csq = r"..\..\tools\dagor3_cdk\util\csq-dev"
elif platform.system() == "Linux" :
  csq = "../../tools/dagor3_cdk/util-linux64/csq-dev"

for root, dirs, files in os.walk("."):
  files[:] = [f for f in files if f.endswith(".nut")]
  cmd = os.path.relpath(csq, root)
  print("changing cwd to dir", root)
  with pushd(root):
    for file in files:
      print(cmd, file)
      p = subprocess.run([cmd, file], shell=False, stderr=subprocess.STDOUT)
      if p.returncode!=0:
        sys.exit(p.returncode)

