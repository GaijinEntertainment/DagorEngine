import bpy, os
from bpy.props          import StringProperty, BoolProperty, PointerProperty, EnumProperty
from ..helpers.popup    import show_popup
from ..helpers.texts    import *
from time               import time
#helpers
def create_directory(path):
    try:
        os.makedirs(path)
        show_popup(message=f'{path}',title='Created directory:',icon='INFO')
        return True
    except:
        show_popup(message=f'{path}',title="Can't create directory:",icon='ERROR')
        return False

class DAGOR_OT_BatchExport(bpy.types.Operator):
    bl_idname = "dt.batch_export"
    bl_label = "Batch export"
    bl_description = "For now works only with visible and selectable objects"
    def execute(self, context):
        start = time()
        P = bpy.data.scenes[0].dag4blend.exporter
        if os.path.exists(P.dirpath):
            if P.limit_by in ['Visible','Sel.Joined']:
                path=os.path.join(P.dirpath,P.filename)
            else:
                path=P.dirpath
            bpy.ops.export_scene.dag(filepath=path, normals=P.vnorm, modifiers=P.modifiers, mopt=P.mopt, orphans=P.orphans,limits=P.limit_by)
            time_spent=round(time() - start,5)
            show_popup(message=f'Batch export finished in {time_spent} sec')
        else:
            create_directory(P.dirpath)
        return {'FINISHED'}

class DAGOR_PT_Export(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Batch export"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    def draw(self,context):
        P = bpy.data.scenes[0].dag4blend.exporter
        l = self.layout
        l.prop(P, 'vnorm')
        l.prop(P, 'modifiers')
        l.prop(P, 'mopt')
        l.prop(P, 'cleanup_names')
        l.prop(P, 'limit_by')
        btn_text='Create directory'
        path_exists=os.path.exists(P.dirpath)

        l.prop(P, 'dirpath')
        if P.limit_by == 'Visible':
            l.prop(P, 'filename')
            if path_exists:
                l.operator('dt.batch_export', text='EXPORT '+P.filename+'.dag')
                l.operator('wm.path_open', icon = 'FILE_FOLDER', text='open export folder').filepath = P.dirpath
            else:
                l.operator('dt.batch_export', text='Create directory',icon='ERROR')

        elif P.limit_by == 'Sel.Separated':
            if path_exists:
                l.operator('dt.batch_export', text='EXPORT')
                l.operator('wm.path_open', icon = 'FILE_FOLDER', text='open export folder').filepath = P.dirpath
            else:
                l.operator('dt.batch_export', text='Create directory',icon='ERROR')

        elif P.limit_by == 'Sel.Joined':
            l.prop(P, 'filename')
            if path_exists:
                l.operator('dt.batch_export', text='EXPORT '+P.filename+'.dag')
                l.operator('wm.path_open', icon = 'FILE_FOLDER', text='open export folder').filepath = P.dirpath
            else:
                l.operator('dt.batch_export', text='Create directory',icon='ERROR')

        elif P.limit_by == 'Col.Separated':
            l.prop(P, 'collection')
            l.prop(P, 'orphans')
            if path_exists:
                l.operator('dt.batch_export', text='EXPORT')
                l.operator('wm.path_open', icon = 'FILE_FOLDER', text='open export folder').filepath = P.dirpath
            else:
                l.operator('dt.batch_export', text='Create directory',icon='ERROR')

        elif P.limit_by == 'Col.Joined':
            l.prop(P, 'collection')
            if P.collection is None:
                l.label(text='Choose collection!',icon='ERROR')
            elif path_exists:
                l.operator('dt.batch_export',text='EXPORT '+P.collection.name+'.dag')
            else:
                l.operator('dt.batch_export', text='Create directory',icon='ERROR')

classes = [DAGOR_PT_Export, DAGOR_OT_BatchExport]
def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    return

def unregister():
    for cl in classes[::-1]:
        bpy.utils.unregister_class(cl)
    return