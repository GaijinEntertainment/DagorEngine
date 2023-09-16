import os
import sys

_dagor2 = os.environ.get('DAGOR2_DIR', os.path.join('D:\\', 'dagor2'))
_flatc = os.path.join(_dagor2, 'tools', 'util', 'flatc')
if sys.platform.startswith('win'):
  _flatc += '.exe'

_args = ['--cpp', '--scoped-enums']

def is_schema(f):
  return os.path.isfile(f) and f.endswith('.fbs')

schemas = [f for f in os.listdir(os.curdir) if is_schema(f)]

cmd = ' '.join([_flatc] + _args + schemas)
print(cmd)
os.system(cmd)
