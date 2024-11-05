import sys
import os
import subprocess

_DIR = os.path.dirname(os.path.realpath(__file__))
def shell(cmd):
  subprocess.check_call(cmd, shell=False, stdout=sys.stdout, stderr=subprocess.STDOUT, bufsize=1, universal_newlines=True)

def main():
  prevd = os.getcwd()
  os.chdir(os.path.join(_DIR, ".."))
  shell(["python3", "_docs/qdoc_main.py", "-o", "_docs/source/api-references/quirrel-modules", "-c", ".quirrel_modules.qdox", "-t", "Quirrel Modules Docs"])
  shell(["python3", "_docs/qdoc_main.py", "-o", "_docs/source/api-references/dagor-engine-docs", "-c", ".engine.qdox", "-t", "Engine Libraries Docs"])
  os.chdir(prevd)

if __name__ == "__main__":
  main()