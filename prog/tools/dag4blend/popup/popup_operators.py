from bpy.types import Operator, UILayout
from bpy.props import StringProperty
from bpy.utils import register_class, unregister_class

from .popup_functions   import show_popup

classes = []

class DAGOR_OT_show_popup(Operator):
    bl_idname = "dt.show_popup"
    bl_label = ""

    message:    StringProperty(default = "")
    title:      StringProperty(default = "")
    icon:       StringProperty(default = 'INFO')

    @classmethod
    def description(cls, context, properties):
        return f'{properties.title} {properties.message}'

    def execute(self,context):
        all_icons = UILayout.bl_rna.functions['prop'].parameters['icon'].enum_items.keys()
        show_popup(message = self.message,
                   title = self.title,
                   icon = self.icon if self.icon in all_icons else 'NONE')
        return {'FINISHED'}
classes.append(DAGOR_OT_show_popup)


def register():
    for cl in classes:
        register_class(cl)
    return


def unregister():
    for cl in classes[::-1]:  # backwards to avoid broken dependencies
        unregister_class(cl)
    return