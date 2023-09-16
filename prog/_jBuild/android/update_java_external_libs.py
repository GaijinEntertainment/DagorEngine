import os, argparse

import fnmatch

def find_files(directory, pattern):
    for root, dirs, files in os.walk(directory):
        for basename in files:
            if fnmatch.fnmatch(basename, pattern):
                filename = os.path.join(root, basename)
                yield filename

parser = argparse.ArgumentParser()
parser.add_argument('ARG_JAVA_CLASSES_PATH')#1
parser.add_argument('ARG_LIBRARIES_LIST', nargs='+')#2

args = parser.parse_args()

count = len(args.ARG_LIBRARIES_LIST)

print ('Checking EXTERNAL_LIBS in java files')

LOAD_LIBS = ''
LOAD_LIBS += '\n  try { \n'
for i in range(count):
  LOAD_LIBS += '    System.loadLibrary("{}");\n'.format(args.ARG_LIBRARIES_LIST[i])
LOAD_LIBS += '  } catch(java.lang.UnsatisfiedLinkError e) { }\n'


for filename in find_files(args.ARG_JAVA_CLASSES_PATH, '*.java'):

  with open(filename, 'r') as origin:
    content = origin.readlines()

  for i in range(len(content)):
    new_line = content[i].replace('{EXTERNAL_LIBS}',LOAD_LIBS)
    if new_line!=content[i]:
      print('Found EXTERNAL_LIBS in FILE {}'.format(filename))
    content[i] = new_line

  with open(filename, 'w+') as output:
    for i in range(len(content)):
      output.write(content[i])

