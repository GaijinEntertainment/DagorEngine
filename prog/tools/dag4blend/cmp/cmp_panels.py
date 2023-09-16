import bpy

from bpy.types              import Panel
from bpy.utils              import register_class, unregister_class
from bpy.props              import StringProperty, BoolProperty, PointerProperty

from ..helpers.basename     import basename

from .cmp_import            import DAGOR_OP_CmpImport
from .cmp_export            import DAGOR_OP_CmpExport

#functions
def fix_cmp_i_path(self,context):
    if self.cmp_i_path.startswith('"') or self.cmp_i_path.endswith('"'):
        self.cmp_i_path=self.cmp_i_path.replace('"','')

#CLASSES

class DAGOR_PT_cmp_import(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "CMP Import"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    @classmethod
    def poll(self, context):
        pref = bpy.context.preferences.addons[basename(__package__)].preferences
        return pref.use_cmp_editor

    def draw(self,context):
        S=context.scene
        l = self.layout
        l.label(text='CMP import path:')
        l.prop(S,'cmp_i_path',text='')
        l.prop(S,'cmp_i_recursive')
        if S.cmp_i_recursive:
            l.prop(S,'cmp_i_dags')
            if S.cmp_i_dags:
                l.prop(S,'cmp_i_lods')
        l.operator('dt.cmp_import')
        return

class DAGOR_PT_cmp_export(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "CMP Export"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    @classmethod
    def poll(self, context):
        pref = bpy.context.preferences.addons[basename(__package__)].preferences
        return pref.use_cmp_editor

    def draw(self,context):
        S=context.scene
        l = self.layout
        l.prop(S,'cmp_col')
        l.label(text='CMP export path:')
        l.prop(S,'cmp_e_path',text='')
        l.operator('dt.cmp_export')
        return

classes=[DAGOR_PT_cmp_import,DAGOR_PT_cmp_export]

def register():
    S=bpy.types.Scene
    for cl in classes:
        register_class(cl)

    S.cmp_i_path        = StringProperty(name="cmp import path", default='C:\\tmp\\', subtype = 'FILE_PATH', description = "Path to composit",update=fix_cmp_i_path)
    S.cmp_i_recursive   = BoolProperty  (name="recursive", default=False, description = 'Search for sub-composits as well')
    S.cmp_i_dags        = BoolProperty  (name="with dags", default=False, description = 'Import dags (!MIGHT BE REALLY SLOW!)')
    S.cmp_i_lods        = BoolProperty  (name="with lods", default=False, description = 'Import dags (!MIGHT BE EVEN SLOWER!)')

    S.cmp_e_path=StringProperty(name="cmp export path", default='C:\\tmp\\', subtype = 'DIR_PATH', description = "Path to save your composit")
    S.cmp_col=PointerProperty(type = bpy.types.Collection, name = '',description = 'Drag&Drop collection here or select it from list')


def unregister():
    S=bpy.types.Scene
    for cl in classes:
        unregister_class(cl)
    del S.cmp_i_path
    del S.cmp_i_recursive
    del S.cmp_i_dags

    del S.cmp_col
    del S.cmp_e_path