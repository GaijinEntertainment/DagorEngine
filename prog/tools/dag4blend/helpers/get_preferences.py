import bpy
from .basename import basename

def get_preferences():
    addon_name = basename(__package__)
    return bpy.context.preferences.addons[addon_name].preferences

def get_local_props():
    return bpy.data.scenes[0].dag4blend