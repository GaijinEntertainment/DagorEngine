import bpy, os, re
from time                       import time
from bpy.types                  import Operator, Panel
from bpy.props                  import StringProperty, BoolProperty, PointerProperty, EnumProperty
from ..helpers.basename         import basename
from ..helpers.popup            import show_popup
from ..helpers.texts            import log
from ..helpers.get_preferences  import *

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
        props = get_local_props()
        props_import = props.importer
        layout = self.layout
        toggles = layout.column(align = True)

        row = toggles.row()
        row.prop(props_import, 'with_subfolders', toggle = True,
            icon = 'CHECKBOX_HLT' if props_import.with_subfolders else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(props_import, 'mopt', toggle = True,
            icon = 'CHECKBOX_HLT' if props_import.mopt else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(props_import, 'preserve_sg', toggle = True,
            icon = 'CHECKBOX_HLT' if props_import.preserve_sg else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(props_import, 'replace_existing', toggle = True,
            icon = 'CHECKBOX_HLT' if props_import.replace_existing else 'CHECKBOX_DEHLT')

        row = toggles.row()
        row.prop(props_import, 'preserve_path', toggle = True,
            icon = 'CHECKBOX_HLT' if props_import.preserve_path else 'CHECKBOX_DEHLT')

        layout.prop(props_import, 'masks')
        layout.prop(props_import, 'excludes')
        layout.prop(props_import, 'dirpath')
        if os.path.exists(props_import.dirpath):
            row = layout.row()
            row.scale_y = 2
            row.operator('dt.batch_import', text='IMPORT', icon = 'IMPORT')
            layout.operator('wm.path_open', icon = 'FILE_FOLDER', text='open import folder').filepath = props_import.dirpath

class DAGOR_OT_BatchImport(Operator):
    bl_idname = "dt.batch_import"
    bl_label = "Batch import"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(self, context):
        pref = get_preferences()
        if pref.projects.__len__() == 0:
            return False
        return True

    @classmethod
    def description(cls, context, properties):
        pref = get_preferences()
        if pref.projects.__len__() == 0:
            return "No active project found. Please, configure at least one project"
        return "Import dags from folder by mask"

    def execute(self, context):
        props = get_local_props()
        props_import = props.importer
        start=time()
        context.view_layer.objects.active = None
        dirpath=props_import.dirpath
        dir = os.listdir(dirpath)
        masks    = props_import.masks.lower()
        masks    = masks.replace(' ','')
        masks    = masks.split(';')
        excludes = props_import.excludes.lower()
        excludes = excludes.replace(' ','')
        excludes = excludes.split(';')
#collecting correct .dags
        dags=[]
        if props_import.with_subfolders:
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
                                            preserve_sg = props_import.preserve_sg,
                                            replace_existing = props_import.replace_existing,
                                            mopt = props_import.mopt,
                                            preserve_path = props_import.preserve_path)
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
