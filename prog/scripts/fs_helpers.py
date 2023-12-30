import os
import sys
import subprocess

PY3K = sys.version_info >= (3, 0)

class pushd:
    def __init__(self, path):
        self.olddir = os.getcwd()
        if path != '':
            os.chdir(os.path.normpath(path))
    def __enter__(self):
        pass
    def __exit__(self, type, value, traceback):
        os.chdir(self.olddir)


def decode_fname(f):
    if PY3K:
      try:
          return f.decode(encoding="utf-8")
      except UnicodeDecodeError:
          try:
              return f.decode('cp1251') #sq3analyzer use such strange codepage
          except:
              return f
    return f

