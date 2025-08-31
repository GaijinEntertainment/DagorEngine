import sys
import os
import subprocess
import time

_DIR = os.path.dirname(os.path.realpath(__file__))
def shell(cmd):
  subprocess.check_call(cmd, shell=False, stdout=sys.stdout, stderr=subprocess.STDOUT, bufsize=1, universal_newlines=True)

def main():
  prevd = os.getcwd()
  shell(["python3", "qdoc_main.py", "-o", "source/api-references/quirrel-modules", "-c", ".quirrel_modules.qdox", "-t", "Dagor Quirrel Modules Docs"])
  shell(["python3", "qdoc_main.py", "-o", "source/_internal/api-references/projects-quirrel-modules", "-c", ".internal_quirrel_modules.qdox", "-t", "Internal Projects Quirrel Modules Docs", "--quiet"])
  shell(["python3", "qdoc_main.py", "-o", "source/api-references/dagor-engine-docs", "-c", ".engine.qdox", "-t", "Engine Libraries Docs"])
  os.chdir(prevd)

if __name__ == "__main__":
    start = time.time()
    main()
    duration = time.time() - start
    print(f"\nbuild_all_docs.py finished in {duration:.2f} seconds.")