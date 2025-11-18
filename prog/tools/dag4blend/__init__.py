import bpy

from .                  import settings
from .popup             import popup_operators
from .projects          import project_manager_panel
from .exporter          import exporter
from .exporter          import export_panel
from .tools             import tools_panel, bake_panel, mesh_tools_panel
from .object_properties import object_properties
from .dagormat          import dagormat_operators, dagormat
from .colprops          import colprops
from .importer          import importer
from .importer          import import_panel

from .cmp               import cmp_panels,cmp_import,cmp_export, node_properties
from .smooth_groups     import smooth_groups

modules=[settings,
        popup_operators,
        project_manager_panel,
        smooth_groups,
        exporter,
        object_properties,
        export_panel,
        tools_panel,
        bake_panel,
        mesh_tools_panel,
        dagormat_operators,
        dagormat,
        node_properties,
        cmp_import,
        cmp_export,
        cmp_panels,
        colprops,
        importer,
        import_panel]

bl_info = {"name": "dag4blend",
           "description": "Tools for editing dag files",
           "author": "Gaijin Entertainment",
           "version": (2, 10, 1),#2025.10.24
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
