import os
import sys
import pathlib
import shutil
import zipfile

cwd = os.getcwd()

shutil.copy2('../pythonCommon/pyparsing.py', 'pyparsing.py')
shutil.copy2('../pythonCommon/datablock.py', 'datablock.py')

# Files to exclude from compression
exclude_files = [
  '__build_pack.py',
  'pack.bat',
]

exclude_dirs = [
  'additional',
]

exclude_extentions = [
  '.bak',  # winmerge produced backup
  '.blend1',  # blender produced backup
  '.zip'  # previous archive
]

pack_dirs = [dir for dir in os.listdir(cwd)
            if os.path.isdir(os.path.join(cwd, dir))
            and dir not in exclude_dirs]

pack_files = [f for f in os.listdir(cwd)
             if os.path.isfile(os.path.join(cwd, f))
             and f not in exclude_files]

for file in list(pack_files):
  for ext in exclude_extentions:
    if file.endswith(ext):
      pack_files.remove(file)

for subdir in pack_dirs:
  for _root, _directories, _files in os.walk(subdir):
    for filename in _files:
      try:
        ext = os.path.splitext(filename)[1]
        if ext in exclude_extentions:
          continue
      except:  # file had no extention at all
        pass
      filepath = os.path.join(subdir, filename)
      pack_files.append(filepath)

os.chdir(os.path.normpath(cwd+'/..'))  # dag4blend directory itself should be archived

# Path to save the zip file
dest_zip_fn = r"dag4blend/dag4blend.zip"
if len(sys.argv) > 1:
  if sys.argv[1] == 'FINAL':
    dest_zip_fn = r"../../tools/dagor_cdk/universal/dag4blend.zip"
  else:
    print('echo usage: {0} [FINAL]'.format(sys.argv[0]))
    exit(1)

os.makedirs(dest_zip_fn[:dest_zip_fn.rindex('/')], exist_ok=True)

print('building {0} of {1} files'.format(dest_zip_fn, len(pack_files)))
with zipfile.ZipFile(dest_zip_fn, 'w', compression= zipfile.ZIP_LZMA) as zip:
  for file in pack_files:
    zip.write(os.path.join('dag4blend',file))
    zipfile
print('done!')

os.remove(os.path.normpath('dag4blend/pyparsing.py'))
os.remove(os.path.normpath('dag4blend/datablock.py'))

os.chdir(cwd)
