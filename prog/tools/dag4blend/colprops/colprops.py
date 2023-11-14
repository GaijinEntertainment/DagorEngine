import bpy
from bpy.types          import Operator, Panel
from bpy.utils          import register_class, unregister_class
from bpy.props          import IntProperty, StringProperty
from ..helpers.basename import basename
from ..helpers.popup    import show_popup

#FUNCTIONS
def get_lod_collections():
    o=bpy.context.object
    act_col=o.instance_collection
    cols=[]#All,lod00,lod01,etc
    if re.search('.*lod0[0-9]',act_col.name) is not None:
        basename=act_col.name[:-6]
    else:
        basename=act_col.name
    for i in range(9):
        name=f"{basename}.lod0{i}"
        lod=bpy.data.collections.get(name)
        if lod is not None:
            cols.append(lod.name)
    all=bpy.data.collections.get(basename)
    if all is not None:
        cols.append(basename)
    return cols

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
        pref = bpy.context.preferences.addons[basename(__package__)].preferences
        col = C.collection
        if col is not None:
            colprops = l.box()
            header = colprops.row()
            header.prop(pref, 'colprops_maximized',icon = 'DOWNARROW_HLT'if pref.colprops_maximized else 'RIGHTARROW_THIN',
                emboss=False,text='Active Collection',expand=True)
            header.label(text='', icon='OUTLINER_COLLECTION')
            if pref.colprops_maximized:
                props = col.keys()
                renamed = "name" in props
                override = colprops.row()
                override.operator('dt.set_col_name',text = "", icon = 'CHECKBOX_HLT' if renamed else 'CHECKBOX_DEHLT').col = col.name
                override.label(text = "Override name:")
                if renamed:
                    colprops.prop(col,'["name"]',text = '')
                else:
                    namebox = colprops.row()
                    namebox.enabled = False
                    namebox.prop(col, "name", text = "")

            cols = [col for col in bpy.data.collections if "name" in col.keys()]
            if cols.__len__()>0:
                all_col = l.box()
                header = all_col.row()
                header.prop(pref, 'colprops_all_maximized',icon = 'DOWNARROW_HLT'if pref.colprops_all_maximized else 'RIGHTARROW_THIN',
                    emboss=False,text='Overridden:',expand=True)
                header.label(text='', icon='OUTLINER_COLLECTION')
                if pref.colprops_all_maximized:
                    cols = list(col for col in bpy.data.collections if 'name' in col.keys() and col.children.__len__()==0)
                    for col in cols:
                        box = all_col.box()
                        row = box.row()
                        row.operator('dt.set_col_name',text='',icon='REMOVE').col = col.name
                        row.label(text=f'{col.name}')
                        box.prop(col, '["name"]',text='')
            if pref.guess_dag_type or pref.use_cmp_editor:
                typebox = l.box()
                boxname = typebox.row()
                boxname.prop(pref, 'type_maximized', text = "", icon = 'DOWNARROW_HLT'if pref.type_maximized else 'RIGHTARROW_THIN',
                emboss=False,expand=True)
                boxname.label(text = 'Type:' if pref.type_maximized else "Type")
                if not pref.type_maximized:
                    return
                if not "type" in col.keys():
                    current_type = "undefined"
                else:
                    current_type = col["type"]

                undefined = typebox.row()
                undefined.operator('dt.set_col_type', text = "", emboss = False, icon = 'RADIOBUT_ON' if current_type == "undefined" else 'RADIOBUT_OFF').type = 0
                undefined.label(text = "Undefined")

                prefab = typebox.row()
                prefab.operator('dt.set_col_type', text = "", emboss = False, icon = 'RADIOBUT_ON' if current_type == "prefab" else 'RADIOBUT_OFF').type = 1
                prefab.label(text = "Prefab")

                rendinst = typebox.row()
                rendinst.operator('dt.set_col_type', text = "", emboss = False, icon = 'RADIOBUT_ON' if current_type == "rendinst" else 'RADIOBUT_OFF').type = 2
                rendinst.label(text = "RendInst")

                dynmodel = typebox.row()
                dynmodel.operator('dt.set_col_type', text = "", emboss = False, icon = 'RADIOBUT_ON' if current_type == "dynmodel" else 'RADIOBUT_OFF').type = 3
                dynmodel.label(text = "DynModel")

                composit = typebox.row()
                composit.operator('dt.set_col_type', text = "", emboss = False, icon = 'RADIOBUT_ON' if current_type == "composit" else 'RADIOBUT_OFF').type = 4
                composit.label(text = "Composit")

                gameobj = typebox.row()
                gameobj.operator('dt.set_col_type', text = "", emboss = False, icon = 'RADIOBUT_ON' if current_type == "gameobj" else 'RADIOBUT_OFF').type = 5
                gameobj.label(text = "Gameobj")
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