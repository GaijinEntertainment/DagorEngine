import bpy
from bpy.utils  import script_paths
from os.path    import join, exists
from .names     import basename

def get_addon_name():
    addon_name = basename(__package__)
    return addon_name

def get_preferences():
    addon_name = get_addon_name()
    return bpy.context.preferences.addons[addon_name].preferences

def get_local_props():
    return bpy.data.scenes[0].dag4blend

def get_addon_directory():
    addon_name = get_addon_name()
    addon_relpath = join("addons", addon_name)
    for path in script_paths():
        current_path = join(path, addon_relpath)
        current_path = current_path.replace('\\', '/')
        if exists(current_path):
            return current_path
    return None
