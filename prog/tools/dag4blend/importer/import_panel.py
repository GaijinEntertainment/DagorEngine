import bpy, os, re
from time               import time
from ..helpers.texts    import get_text
from bpy.props          import StringProperty, BoolProperty, PointerProperty, EnumProperty
from ..helpers.popup    import show_popup

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

def fix_imp_path(self,context):
    S=context.scene
    path=S.dag_import_path.replace('"','')
    if path.endswith('.dag'):
        folder = os.path
        S.dag_import_masks = os.path.basename(path) #dag name turned to mask for fast import
        path = os.path.dirname(path) # dag name removed from path
    if not path.endswith('\\'):
        path=(path+'\\')
    S.dag_import_path=path
#classes
class DAGOR_PT_Import(bpy.types.Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Batch import"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    def draw(self,context):
        S=bpy.context.scene
        layout = self.layout
        layout.prop(S, 'dag_import_subfolders')
        layout.prop(S, 'dag_import_mopt')
        layout.prop(S, 'dag_import_refresh')
        layout.prop(S, 'dag_preserve_path')
        layout.prop(S, 'dag_import_masks')
        layout.prop(S, 'dag_import_exclude')
        layout.prop(S, 'dag_import_path')
        if os.path.exists(bpy.context.scene.dag_import_path):
            layout.operator('dt.fast_import', text=('IMPORT'))
            layout.operator('wm.path_open', icon = 'FILE_FOLDER', text='open import folder').filepath = bpy.context.scene.dag_import_path

class DAGOR_OT_FastImport(bpy.types.Operator):
    bl_idname = "dt.fast_import"
    bl_label = "Fast import"
    bl_description = "Import dags from folder by mask"
    bl_options = {'REGISTER', 'UNDO'}
    def execute(self, context):
        S = context.scene
        start=time()
        context.view_layer.objects.active = None
        path=context.scene.dag_import_path
        dir = os.listdir(path)
        masks    = context.scene.dag_import_masks.lower()
        masks    = masks.replace(' ','')
        masks    = masks.split(';')
        excludes = context.scene.dag_import_exclude.lower()
        excludes = excludes.replace(' ','')
        excludes = excludes.split(';')
#collecting correct .dags
        dags=[]
        if context.scene.dag_import_subfolders:
            for subdir,dirs,files in os.walk(path):
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
                    dags.append(os.path.join(path,file))
#import
        amount=dags.__len__()
        if amount>0:
            log=get_text('log')
            log.write(f'found {amount} dag(s) for import:\n')
            for d in dags:
                log.write(f'{d}\n')
            log.write('\n')
            print(str(amount)+' dag files found, importing...')
            for dag in dags:
                try:
                    bpy.ops.import_scene.dag(filepath = dag,
                                            import_refresh = S.dag_import_refresh,
                                            import_mopt = S.dag_import_mopt,
                                            import_preserve_path = S.dag_preserve_path)
                except:
                    print ("ERROR!\nCan't import "+dag)
                amount-=1
                print('just '+str(amount)+' dags to go')
        else:
            print('Nothing to import')
        show_popup(f'finished in {round(time()-start,4)} sec')
        return {'FINISHED'}

classes = [DAGOR_PT_Import, DAGOR_OT_FastImport]
def register():
    S=bpy.types.Scene
    for cl in classes:
        bpy.utils.register_class(cl)
    #import panel param
    S.dag_import_subfolders =BoolProperty  (name="Search in subfolders", default=False, description = 'Search for .dags in subfolders as well')
    S.dag_import_mopt       =BoolProperty  (name="Optimize material slots", default=True, description = 'Remove unnecessary material slots')
    S.dag_import_refresh    =BoolProperty  (name="Refresh existing", default=False, description = 'Replace dags that already exist in scene by imported versions')
    S.dag_preserve_path     =BoolProperty  (name="Preserve paths", default=False, description = 'Override export path for each imported dag')
    S.dag_import_path       =StringProperty(name="Path",    default='C:\\tmp\\', subtype = 'DIR_PATH', description = "Where search for .dag files?",update=fix_imp_path)
    S.dag_import_masks      =StringProperty(name="Masks",   default='', description = 'name should contain at least one to be imported. Split by";"')
    S.dag_import_exclude    =StringProperty(name="Excludes",default='', description = 'name should not contain any to be imported. Split by";"')

def unregister():
    S=bpy.types.Scene
    for cl in classes[::-1]:
        bpy.utils.unregister_class(cl)
    del S.dag_import_mopt
    del S.dag_import_masks
    del S.dag_import_refresh
    del S.dag_import_exclude
    del S.dag_import_subfolders
    del S.dag_import_path
    del S.dag_preserve_path