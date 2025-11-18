import bpy
from bpy.types  import Operator, Panel
from bpy.utils  import register_class, unregister_class

from ..popup.popup_functions    import show_popup
from ..ui.draw_elements     import draw_custom_header
from .draw_project_manager  import *

classes = []

class DAG_OT_SetSearchFolder(Operator):
    bl_idname = "dt.set_search_path"
    bl_label = "Set search folder"
    bl_description = "Use project directory as a path for Batch Import (including subfolders)"
    def execute(self, context):
        pref = get_preferences()
        i=int(pref.project_active)
        bpy.data.scenes[0].dag4blend.importer.dirpath=pref.projects[i].path
        bpy.data.scenes[0].dag4blend.importer.with_subfolders=True
        show_popup(message='Import without full name of an asset as mask will be extremely slow!',
            title='WARNING!', icon='INFO')
        return {'FINISHED'}
classes.append(DAG_OT_SetSearchFolder)


class DAG_PT_project(Panel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "scene"
    bl_label = "Project (dagor)"

    def draw(self,context):
        layout = self.layout
        pref = get_preferences()
        if pref.projects.__len__() == 0:
            box = layout.box()
            row = box.row(align = True)
            row.label(icon = 'ERROR')
            row.label(text = 'NO ACTIVE PROJECT FOUND!')
            row.label(icon = 'BLANK1')  # this one aligns text to the center
            big_button = box.row()
            big_button.scale_y = 2
            big_button.operator('dt.add_project', text = "ADD", icon = 'ADD')
            return
        layout.prop(pref,'project_active',text='')
        index = int(pref.project_active)
        project = pref.projects[index]
        draw_project_settings(layout, project, index)
        layout.operator('dt.set_search_path',text='Apply as search path')
        return
classes.append(DAG_PT_project)

def register():
    for cl in classes:
        register_class(cl)
    return


def unregister():
    for cl in classes[::-1]:
        unregister_class(cl)
    return