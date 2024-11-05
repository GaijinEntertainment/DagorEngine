import os
import subprocess
import sys, platform
from fs_helpers import pushd

if platform.system() == "Windows" :
  csq = r"..\..\tools\dagor_cdk\windows-x86_64\csq-dev"
elif platform.system() == "Linux" :
  csq = "../../tools/dagor_cdk/linux-x86_64/csq-dev"

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

