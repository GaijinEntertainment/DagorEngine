import bpy

from .                  import settings
from .projects          import project_manager_panel
from .exporter          import exporter
from .exporter          import export_panel
from .tools             import tools_panel, bake_panel, mesh_tools_panel
from .object_properties import object_properties
from .dagormat          import dagormat
from .colprops          import colprops
from .importer          import importer
from .importer          import import_panel

from .cmp               import cmp_panels,cmp_import,cmp_export
from .smooth_groups     import smooth_groups

modules=[settings,
        project_manager_panel,
        smooth_groups,
        exporter,
        object_properties,
        export_panel,
        tools_panel,
        bake_panel,
        mesh_tools_panel,
        dagormat,
        cmp_import,
        cmp_export,
        cmp_panels,
        colprops,
        importer,
        import_panel]

bl_info = {"name": "dag4blend",
           "description": "Tools for editing dag files",
           "author": "Gaijin Entertainment",
           "version": (2, 7, 0),#2025.05.15
           "blender": (4, 4, 0),
           "location": "File > Export",
           "wiki_url": "",
           "tracker_url": "",
           "category": "Import-Export"}

def register():
    for module in modules:
        module.register()
    return

def unregister():
    for module in modules[::-1]:
        module.unregister()
    return


if __name__ == "__main__":
    register()
