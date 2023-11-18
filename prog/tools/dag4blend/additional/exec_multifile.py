# copy to c:\Program Files\Blender Foundation\Blender 2.93\2.93\scripts\templates_py\
import tempfile
import os
import shutil
import glob
import bpy
import sys

addon_folder = 'bl_dag_exporter'

source_path = 'D:/dagor2/prog/tools'

files_mask = '*.py'
add_files_names = ['dagorShaders.cfg']

addon_path = os.path.join(source_path, addon_folder)

files = glob.glob(os.path.join(addon_path, files_mask)) + [os.path.join(addon_path, file) for file in add_files_names]

with tempfile.TemporaryDirectory() as temp_dir:

    addon_folder_to_files = os.path.join(temp_dir, addon_folder, addon_folder)

    os.makedirs(addon_folder_to_files)

    for file in files:
        shutil.copy(file, addon_folder_to_files)

    addon_folder_to_zip = os.path.join(temp_dir, addon_folder)

    shutil.make_archive(addon_folder_to_zip, 'zip', addon_folder_to_zip)

    addon_zip_path = addon_folder_to_zip + '.zip'

    bpy.ops.preferences.addon_disable(module=addon_folder)
    bpy.ops.preferences.addon_remove(module=addon_folder)
    bpy.ops.preferences.addon_refresh()

    for module in list(sys.modules.keys()):
        if hasattr(sys.modules[module], '__package__'):
            if(sys.modules[module].__package__ == addon_folder):
                del sys.modules[module]

    bpy.ops.preferences.addon_install(filepath=addon_zip_path, overwrite=True)
    bpy.ops.preferences.addon_enable(module=addon_folder)
