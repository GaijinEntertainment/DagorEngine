import bpy
from os.path            import exists

from .                  import settings
from .exporter          import exporter
from .exporter          import export_panel
from .tools             import tools_panel, bake_panel
from .object_properties import object_properties
from .dagormat          import dagormat
from .colprops          import colprops


if exists(bpy.utils.user_resource('SCRIPTS',path=f'addons\\{__package__}\\importer')):
    from .importer          import importer
    from .importer          import import_panel
    import_cl = [importer, import_panel]

from .cmp               import cmp_panels,cmp_import,cmp_export
from .smooth_groups     import smooth_groups

main_cl      = [settings, smooth_groups, exporter, object_properties, export_panel, tools_panel, bake_panel, dagormat,cmp_import,cmp_export,cmp_panels,colprops]

bl_info = {"name": "dag4blend",
           "description": "Tools for editing dag files",
           "author": "Gaijin Entertainment",
           "version": (2, 1, 2),#2023.11.14
           "blender": (3, 5, 1),
           "location": "File > Export",
           "wiki_url": "",
           "tracker_url": "",
           "category": "Import-Export"}

def register():
    for c in main_cl:
        c.register()
    if exists(bpy.utils.user_resource('SCRIPTS',path=f'addons\\{__package__}\\importer')):
        for c in import_cl:
            c.register()

def unregister():
    for c in main_cl:
        c.unregister()
    if exists(bpy.utils.user_resource('SCRIPTS',path=f'addons\\{__package__}\\importer')):
        for c in import_cl:
            c.unregister()


if __name__ == "__main__":
    register()