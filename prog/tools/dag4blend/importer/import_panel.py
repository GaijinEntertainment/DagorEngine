import bpy, os, re
from time               import time
from bpy.types          import Operator, Panel
from bpy.props          import StringProperty, BoolProperty, PointerProperty, EnumProperty
from ..helpers.popup    import show_popup
from ..helpers.texts    import log

#helpers
def string_to_re(mask):
    re_mask='^'+mask
    re_mask=re_mask.replace('.','\.')
    re_mask=re_mask.replace('*','.*')
    re_mask+='$'
    return(re_mask)

def check_import_name(name,excludes,masks):
    for e in excludes:
        if e!='':
            if re.search(string_to_re(e),name) is not None:
                return False
    for m in masks:
        if m!='':
            if re.search(string_to_re(m),name) is not None:
                return True
    return False

#classes
class DAGOR_PT_Import(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Batch import"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    def draw(self,context):
        P = bpy.data.scenes[0].dag4blend.importer
        layout = self.layout
        toggles = layout.column(align = True)

        row = toggles.row()
        row.prop(P, 'with_subfolders', toggle = True,
            icon = 'CHECKBOX_HLT' if P.with_subfolders else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(P, 'mopt', toggle = True,
            icon = 'CHECKBOX_HLT' if P.mopt else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(P, 'preserve_sg', toggle = True,
            icon = 'CHECKBOX_HLT' if P.preserve_sg else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(P, 'replace_existing', toggle = True,
            icon = 'CHECKBOX_HLT' if P.replace_existing else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(P, 'preserve_path', toggle = True,
            icon = 'CHECKBOX_HLT' if P.preserve_path else 'CHECKBOX_DEHLT')

        layout.prop(P, 'masks')
        layout.prop(P, 'excludes')
        layout.prop(P, 'dirpath')
        if os.path.exists(P.dirpath):
            row = layout.row()
            row.scale_y = 2
            row.operator('dt.batch_import', text='IMPORT', icon = 'IMPORT')
            layout.operator('wm.path_open', icon = 'FILE_FOLDER', text='open import folder').filepath = P.dirpath

class DAGOR_OT_BatchImport(Operator):
    bl_idname = "dt.batch_import"
    bl_label = "Batch import"
    bl_description = "Import dags from folder by mask"
    bl_options = {'REGISTER', 'UNDO'}
    def execute(self, context):
        P = bpy.data.scenes[0].dag4blend.importer
        start=time()
        context.view_layer.objects.active = None
        dirpath=P.dirpath
        dir = os.listdir(dirpath)
        masks    = P.masks.lower()
        masks    = masks.replace(' ','')
        masks    = masks.split(';')
        excludes = P.excludes.lower()
        excludes = excludes.replace(' ','')
        excludes = excludes.split(';')
#collecting correct .dags
        dags=[]
        if P.with_subfolders:
            for subdir,dirs,files in os.walk(dirpath):
                for file in files:
                    name=file.lower()
                    if not name.endswith('.dag'):
                        pass
                    elif check_import_name(name,excludes,masks):
                        dags.append(os.path.join(subdir,file))
        else:
            for file in dir:
                name=file.lower()
                if not name.endswith('.dag'):
                    pass
                elif check_import_name(name,excludes,masks):
                    dags.append(os.path.join(dirpath,file))
#import
        amount=dags.__len__()
        if amount>0:
            msg = f'found {amount} dag(s) for import:\n'
            for d in dags:
                msg+=f'{d}\n'
            msg+='\n'
            log(msg,type = 'INFO')
            for dag in dags:
                try:
                    bpy.ops.import_scene.dag(filepath = dag,
                                            preserve_sg = P.preserve_sg,
                                            replace_existing = P.replace_existing,
                                            mopt = P.mopt,
                                            preserve_path = P.preserve_path)
                except:
                    msg =f"\nCan not import {dag}\n"
                    log(msg, type = 'ERROR')
                amount-=1
        else:
            msg = 'Nothing to import\n'
            log(msg,type = 'INFO')
        show_popup(f'finished in {round(time()-start,4)} sec')
        return {'FINISHED'}

classes = [DAGOR_PT_Import, DAGOR_OT_BatchImport]
def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    return

def unregister():
    for cl in classes[::-1]:
        bpy.utils.unregister_class(cl)
    return
