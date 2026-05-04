import bpy
from bpy.types          import Operator, Panel
from bpy.utils          import register_class, unregister_class
from bpy.props          import IntProperty, StringProperty
from ..helpers.getters  import get_preferences
from ..ui.draw_elements import draw_custom_header

from ..popup.popup_functions    import show_popup

#OPERATORS

#(re)sets custom name for collection
class DT_OT_SetColName(Operator):
    bl_idname = "dt.set_col_name"
    bl_label = "Set custom collection name"
    bl_description = "Set custom collection name for dagor tools"
    bl_options = {'UNDO'}

    col: StringProperty(default = "")

    def execute(self, context):
        col = bpy.data.collections.get(self.col)
        if self.col == "Scene Collection":
            show_popup('"Scene Collection" name/path can not be overridden')
            return {'CANCELLED'}
        elif col is None:
            show_popup('No collection selected!')
            return {'CANCELLED'}
        elif col.children.__len__()>0 and "name" not in col.keys():
            show_popup(f"{col.name} have sub-collections, name/path can not be overridden")
            return {'CANCELLED'}
        if "name" not in col.keys():
            col["name"] = col.name
        else:
            del col["name"]
        return {'FINISHED'}

#(re)sets custom type for collection
class DT_OT_SetColType(Operator):
    bl_idname = "dt.set_col_type"
    bl_label = "Set custom collection type"
    bl_description = "Set custom collection type for dagor tools"
    bl_options = {'UNDO'}

    type:       IntProperty(default = 0)

    @classmethod
    def poll(self, context):
        return context.collection is not None and context.collection!=context.scene.collection


    def execute(self,context):
        type = self.type
        col = bpy.context.collection
        if "type" not in col.keys() and type == 0:
            return {'CANCELLED'}
        if type == 0:
            del col["type"]
        elif type == 1:
            col["type"] = 'prefab'
        elif type == 2:
            col["type"] = 'rendinst'
        elif type == 3:
            col["type"] = 'dynmodel'
        elif type == 4:
            col["type"] = 'composit'
        elif type == 5:
            col["type"] = 'gameobj'
        return {'FINISHED'}

#PANELS
class DAGOR_PT_CollectionProperties(Panel):
    bl_idname = 'dt.col_props'
    bl_label = 'Collecton Properties'
    bl_space_type = 'VIEW_3D'
    bl_context = "objectmode"
    bl_region_type = 'UI'
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        C=bpy.context
        l=self.layout
        column = l.column(align = True)
        pref = get_preferences()
        col = C.collection
        if col is not None:
            colprops = column.box()
            draw_custom_header(colprops, 'Active Collection', pref, 'colprops_maximized', icon = 'OUTLINER_COLLECTION')
            if pref.colprops_maximized:
                props = col.keys()
                renamed = "name" in props
                override = colprops.row()
                toggle_operator = override.operator('dt.set_col_name', text = "Override name/path:",
                    icon = 'CHECKBOX_HLT' if renamed else 'CHECKBOX_DEHLT',
                    depress = renamed)
                toggle_operator.col = col.name
                if renamed:
                    colprops.prop(col,'["name"]',text = '')
                else:
                    namebox = colprops.row()
                    namebox.enabled = False
                    namebox.prop(col, "name", text = "")

            cols = [col for col in bpy.data.collections if "name" in col.keys()]
            if cols.__len__()>0:
                all_col = column.box()
                draw_custom_header(all_col, 'Overridden:', pref, 'colprops_all_maximized', icon = 'OUTLINER_COLLECTION')
                if pref.colprops_all_maximized:
                    cols = list(col for col in bpy.data.collections if 'name' in col.keys() and col.children.__len__()==0)
                    for col in cols:
                        box = all_col.box()
                        row = box.row()
                        row.operator('dt.set_col_name',text='',icon='REMOVE').col = col.name
                        row.label(text=f'{col.name}')
                        box.prop(col, '["name"]',text='')
            if pref.guess_dag_type or pref.use_cmp_editor:
                typebox = column.box()
                draw_custom_header(typebox, 'Type:', pref, 'type_maximized')
                if not pref.type_maximized:
                    return
                if not "type" in col.keys():
                    current_type = "undefined"
                else:
                    current_type = col["type"]

                types = typebox.column(align = True)

                types.operator('dt.set_col_type', text = "Undefined",
                    depress = True if current_type == "undefined" else False,
                    icon = 'RADIOBUT_ON' if current_type == "undefined" else 'RADIOBUT_OFF').type = 0



                types.operator('dt.set_col_type', text = "Prefab",
                    depress = True if current_type == "prefab" else False,
                    icon = 'RADIOBUT_ON' if current_type == "prefab" else 'RADIOBUT_OFF').type = 1


                types.operator('dt.set_col_type', text = "RendInst",
                    depress = True if current_type == "rendinst" else False,
                    icon = 'RADIOBUT_ON' if current_type == "rendinst" else 'RADIOBUT_OFF').type = 2



                types.operator('dt.set_col_type', text = "DynModel",
                    depress = True if current_type == "dynmodel" else False,
                    icon = 'RADIOBUT_ON' if current_type == "dynmodel" else 'RADIOBUT_OFF').type = 3



                types.operator('dt.set_col_type', text = "Composit",
                    depress = True if current_type == "composit" else False,
                    icon = 'RADIOBUT_ON' if current_type == "composit" else 'RADIOBUT_OFF').type = 4


                types.operator('dt.set_col_type', text = "Gameobj",
                    depress = True if current_type == "gameobj" else False,
                    icon = 'RADIOBUT_ON' if current_type == "gameobj" else 'RADIOBUT_OFF').type = 5
        return

classes =  [DT_OT_SetColName,
            DT_OT_SetColType,
            DAGOR_PT_CollectionProperties
            ]

def register():
    for cl in classes:
        register_class(cl)
    return

def unregister():
    for cl in classes[::-1]:
        unregister_class(cl)
    return