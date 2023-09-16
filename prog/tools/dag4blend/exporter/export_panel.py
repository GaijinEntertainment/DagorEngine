import bpy
import os
from bpy.props          import StringProperty, BoolProperty, PointerProperty, EnumProperty
from ..helpers.popup import show_popup

#helpers
def fix_exp_path(self,context):
    if self.dag_export_path.find('"')>-1:
        self.dag_export_path=self.dag_export_path.replace('"','')
    if not self.dag_export_path.endswith(os.sep):
        self.dag_export_path+=os.sep

def create_directory(path):
    try:
        os.makedirs(path)
        show_popup(message=f'{path}',title='Created directory:',icon='INFO')
        return True
    except:
        show_popup(message=f'{path}',title="Can't create directory:",icon='ERROR')
        return False

class DAGOR_OT_FastExport(bpy.types.Operator):
    bl_idname = "dt.fast_export"
    bl_label = "Fast export"
    bl_description = "For now works only with visible and selectable objects"
    def execute(self, context):
        S=context.scene
        if os.path.exists(S.dag_export_path):
            if S.dag_limit in ['Visible','Sel.Joined']:
                path=os.path.join(S.dag_export_path,S.dag_name)
            else:
                path=S.dag_export_path
            bpy.ops.export_scene.dag(filepath=path, normals=S.dag_vnorm,modifiers=S.dag_mods,mopt=S.dag_mopt,orphans=S.dag_orphans,limits=S.dag_limit)
        else:
            create_directory(S.dag_export_path)
        return {'FINISHED'}

class DAGOR_PT_Export(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Batch export"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    def draw(self,context):
        S=bpy.context.scene
        l = self.layout
        l.prop(S, 'dag_vnorm')
        l.prop(S, 'dag_mods')
        l.prop(S, 'dag_mopt')
        l.prop(S, 'dag_cleanup_names')
        l.prop(S, 'dag_limit')
        btn_text='Create directory'
        path_exists=os.path.exists(S.dag_export_path)

        l.prop(S, 'dag_export_path')
        if S.dag_limit == 'Visible':
            l.prop(S, 'dag_name')
            if path_exists:
                l.operator('dt.fast_export', text='EXPORT '+S.dag_name+'.dag')
                l.operator('wm.path_open', icon = 'FILE_FOLDER', text='open export folder').filepath = bpy.context.scene.dag_export_path
            else:
                l.operator('dt.fast_export', text='Create directory',icon='ERROR')

        elif S.dag_limit == 'Sel.Separated':
            if path_exists:
                l.operator('dt.fast_export', text='EXPORT')
                l.operator('wm.path_open', icon = 'FILE_FOLDER', text='open export folder').filepath = bpy.context.scene.dag_export_path
            else:
                l.operator('dt.fast_export', text='Create directory',icon='ERROR')

        elif S.dag_limit == 'Sel.Joined':
            l.prop(S, 'dag_name')
            if path_exists:
                l.operator('dt.fast_export', text='EXPORT '+S.dag_name+'.dag')
                l.operator('wm.path_open', icon = 'FILE_FOLDER', text='open export folder').filepath = bpy.context.scene.dag_export_path
            else:
                l.operator('dt.fast_export', text='Create directory',icon='ERROR')

        elif S.dag_limit == 'Col.Separated':
            l.prop(S, 'dag_col')
            l.prop(S, 'dag_orphans')
            if path_exists:
                l.operator('dt.fast_export', text='EXPORT')
                l.operator('wm.path_open', icon = 'FILE_FOLDER', text='open export folder').filepath = bpy.context.scene.dag_export_path
            else:
                l.operator('dt.fast_export', text='Create directory',icon='ERROR')

        elif S.dag_limit == 'Col.Joined':
            l.prop(S, 'dag_col')
            if S.dag_col is None:
                l.label(text='Choose collection!',icon='ERROR')
            elif path_exists:
                l.operator('dt.fast_export',text='EXPORT '+S.dag_col.name+'.dag')
            else:
                l.operator('dt.fast_export', text='Create directory',icon='ERROR')

classes = [DAGOR_PT_Export, DAGOR_OT_FastExport]
def register():
    S=bpy.types.Scene
    for cl in classes:
        bpy.utils.register_class(cl)
    #exportPanel param
    S.dag_limit = EnumProperty(
            #(identifier, name, description)
    items = [('Visible','Visible','Export everithing as one dag'),
             ('Sel.Joined','Sel.Joined','Selected objects as one dag'),
             ('Sel.Separated','Sel.Separated','Selected objects as separate dags'),
             ('Col.Separated','Col.Separated','Chosen collection and subcollections as separate .dags'),
             ('Col.Joined','Col.Joined','Objects from chosen collection and subcollections as one .dag')],
    name = "Limit by",
    default = 'Visible')
    S.dag_name=StringProperty(name="Name", default='Filename', description = 'Name for exported .dag file')
    S.dag_export_path=StringProperty(name="Path", default='C:\\tmp\\', subtype = 'DIR_PATH', description = "Where your .dag files should be saved?",update=fix_exp_path)
    S.dag_mods=BoolProperty(name="applyMods", default=True, description = 'Export meshes with effects of modifiers')
    S.dag_mopt=BoolProperty(name="Optimize Materials", default=True, description = 'Remove unused slots; merge similar materials')
    S.dag_vnorm=BoolProperty(name="vNormals", default=True, description = 'Export of custom vertex normals')
    S.dag_orphans=BoolProperty(name="exportOrphans", default=False, description = 'Export of objects not in the bottom of hierarchy as separate .dags')
    S.dag_col=PointerProperty(type = bpy.types.Collection, name = '',description = 'Drag&Drop collection here or select it from list. Keep it empty to use Scene Collection instead')
    S.dag_cleanup_names=BoolProperty(name="Cleanup Names", default=False, description = "Remove indices and node types from names. Use only for cmp palettes!")

def unregister():
    S=bpy.types.Scene
    for cl in classes[::-1]:
        bpy.utils.unregister_class(cl)
    del S.dag_name
    del S.dag_export_path
    del S.dag_mods
    del S.dag_mopt
    del S.dag_vnorm
    del S.dag_orphans
    del S.dag_col
    del S.dag_cleanup_names