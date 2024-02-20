import os
import sys
import pathlib
import shutil
import zipfile

prev_cwd = os.getcwd()
os.chdir(os.path.normpath(prev_cwd+'/..'))

shutil.copy2('pythonCommon/pyparsing.py', 'dag4blend/pyparsing.py')
shutil.copy2('pythonCommon/datablock.py', 'dag4blend/datablock.py')

# Files to compress
files = [
  'dag4blend/__init__.py', 'dag4blend/dagorShaders.cfg', 'dag4blend/settings.py',
  'dag4blend/constants.py', 'dag4blend/face.py', 'dag4blend/mesh.py', 'dag4blend/node.py',
  'dag4blend/material.py', 'dag4blend/nodeMaterial.py', 'dag4blend/dagMath.py',
  'dag4blend/read_config.py',
  'dag4blend/dagormat/build_node_tree.py',
  'dag4blend/dagormat/compare_dagormats.py', 'dag4blend/dagormat/dagormat.py', 'dag4blend/dagormat/rw_dagormat_text.py',
  'dag4blend/exporter/export_panel.py', 'dag4blend/exporter/exporter.py', 'dag4blend/exporter/writer.py',
  'dag4blend/importer/import_panel.py', 'dag4blend/importer/importer.py', 'dag4blend/importer/reader.py',
  'dag4blend/object_properties/object_properties.py', 'dag4blend/smooth_groups/smooth_groups.py',
  'dag4blend/smooth_groups/mesh_calc_smooth_groups.py',
  'dag4blend/colprops/colprops.py',
  'dag4blend/pyparsing.py', 'dag4blend/datablock.py',
]
scan_folders = [
  'dag4blend/helpers',
  'dag4blend/tools',
  'dag4blend/cmp',
  'dag4blend/extras',
]

for f in scan_folders:
  for _root, _directories, _files in os.walk(f):
    for filename in _files:
      # join the two strings in order to form the full filepath.
      filepath = os.path.join(f, filename)
      files.append(filepath)

# Path to save the zip file
dest_zip_fn = r"dag4blend/dag4blend.zip"
if len(sys.argv) > 1:
  if sys.argv[1] == 'FINAL':
    dest_zip_fn = r"../../tools/dagor3_cdk/pluginBlender/dag4blend.zip"
  else:
    print('echo usage: {0} [FINAL]'.format(sys.argv[0]))
    exit(1)

os.makedirs(dest_zip_fn[:dest_zip_fn.rindex('/')], exist_ok=True)

print('building {0} of {1} files'.format(dest_zip_fn, len(files)))
with zipfile.ZipFile(dest_zip_fn, 'w', compression= zipfile.ZIP_LZMA) as zip:
  for file in files:
    zip.write(file)
    zipfile
print('done!')

os.remove(os.path.normpath('dag4blend/pyparsing.py'))
os.remove(os.path.normpath('dag4blend/datablock.py'))

os.chdir(prev_cwd)
