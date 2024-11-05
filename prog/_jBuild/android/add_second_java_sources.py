import os, argparse
import re
import fnmatch

def find_files(directory, pattern):
    for root, dirs, files in os.walk(directory):
        for basename in files:
            if fnmatch.fnmatch(basename, pattern):
                filename = os.path.join(root, basename)
                yield filename

parser = argparse.ArgumentParser()
parser.add_argument('JAVA_SRC_ZIP')#1
parser.add_argument('JAVA_SRC_DIR')#1

args = parser.parse_args()

def fix_imports(filename):
  # print(f'Process: {os.path.relpath(filename, args.JAVA_SRC_DIR)}')

  with open(filename, 'r') as origin:
    lines = origin.readlines()

  importRe = re.compile(r"import\s+(?P<class>[\d\w\.\*]+)")
  packageRe = re.compile(r"package\s+(?P<package>[\d\w\.\*]+)")

  with open(filename, 'w+') as output:
    for line in lines:
      match = packageRe.match(line)
      if match:
        packagePath = match.group("package").replace('.', '/')
        if os.path.exists(os.path.join(args.JAVA_SRC_DIR, packagePath)):
          line = 'package ' + ('second/' + packagePath).replace('/', '.') + ';\n\nimport com.gaijingames.wtm.R;\n\n'
          # print(f'  Fixed: {line}')

      match = importRe.match(line)
      if match:
        classPath = match.group("class").replace('.', '/')
        if '*.' not in classPath and os.path.exists(os.path.join(args.JAVA_SRC_DIR, classPath + '.java')):
          line = 'import ' + ('second/' + classPath).replace('/', '.') + ';\n'
          # print(f'  Fixed: {line}')

      output.write(line)


def unzip_to_folder(zip_path, output_path):
  import zipfile
  with zipfile.ZipFile(zip_path, 'r') as zipf:
    zipf.extractall(output_path)

print(f'Add secound Java sources: {args.JAVA_SRC_DIR}')

if os.path.exists(args.JAVA_SRC_DIR):
  import shutil
  shutil.rmtree(args.JAVA_SRC_DIR)

unzip_to_folder(args.JAVA_SRC_ZIP, args.JAVA_SRC_DIR)
for filename in find_files(args.JAVA_SRC_DIR, '*.java'):
  fix_imports(os.path.normpath(filename))

