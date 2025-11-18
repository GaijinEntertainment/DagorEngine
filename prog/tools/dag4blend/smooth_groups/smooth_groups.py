import bpy
import bmesh
from math               import pi
from bpy.app.handlers   import persistent
from bpy.types          import Operator
from bpy.props          import IntProperty

from ..ui.draw_elements         import draw_custom_header
from ..helpers.version          import get_blender_version
from ..helpers.getters          import get_preferences
from .mesh_calc_smooth_groups   import *

dic={}
# update method
def update_sg(self, context):
    eo = context.edit_object
    bm = dic.setdefault(eo.name, bmesh.from_edit_mesh(eo.data))
    face = bm.faces.active
    SG = bm.faces.layers.int.get("SG")
    if face and SG:
        face[SG] = self.sg_current_value
    return None
#sg_current_value is used to set the value

#functions

def sg_to_sharp_edges(mesh):
    version = get_blender_version()
    if version < 4.1:
        mesh.use_auto_smooth=True
        mesh.auto_smooth_angle=pi
    bm = mesh_to_bmesh(mesh)
    for face in bm.faces:
        face.smooth = True
    SG=bm.faces.layers.int['SG']
    for edge in bm.edges:
        link_faces = edge.link_faces
        if link_faces.__len__()!=2:
            edge.smooth = False
            continue
        share_sg = link_faces[0][SG] & link_faces[1][SG] > 0
        edge.smooth = share_sg
    bmesh_to_mesh(bm, mesh)
    return

#keep wm.sg_current_value updated to match selection
def update_sg_current(bm):
    face = bm.faces.active
    if face is not None:
        SG = bm.faces.layers.int.get("SG")
        if SG:
            bpy.context.window_manager.sg_current_value = face[SG]
    return None
#scene update handler
@persistent
def edit_object_change_handler(self,context):
    obj = bpy.context.view_layer.objects.active
    if obj is None:
        return None
    # add one instance of edit bmesh to global dic
    if context.mode == 'EDIT_MESH':
        bm = dic.setdefault(obj.name, bmesh.from_edit_mesh(obj.data))
        update_sg_current(bm)
        return None
    dic.clear()
    return None

#operators
class DAGOR_OT_SmoothGroupInit(bpy.types.Operator):
    bl_label = 'Init SmoothGroup'
    bl_idname = 'dt.init_smooth_group'
    bl_description = 'Initialize smoothing groups'
    bl_options = {'UNDO'}
    def execute(self, context):
        objects_calc_smooth_groups([context.object])
        return {'FINISHED'}

class DAGOR_OT_SmoothGroupsRemove(bpy.types.Operator):
    bl_label = 'Remove SmoothGroups'
    bl_idname = 'dt.remove_smooth_groups'
    bl_description = 'Remove SG data'
    bl_options = {'UNDO'}
    def execute(self, context):
        mesh = context.object.data
        SG = mesh.attributes.get('SG')
        if SG is not None:
            mesh.attributes.remove(SG)
        return {'FINISHED'}

class DAGOR_OT_SmoothGroupToSharpEdges(bpy.types.Operator):
    bl_label = 'SmoothGroups to sharp edges'
    bl_idname = 'dt.preview_sg'
    bl_description = 'Preview smoothing groups as sharp edges'
    bl_options = {'UNDO'}
    def execute(self, context):
        if context.mode=='EDIT_MESH':
            sg_to_sharp_edges(context.edit_object.data)
        return {'FINISHED'}

class DAGOR_OT_SmoothGroupSet(bpy.types.Operator):
    bl_label = 'Set SmoothGroup'
    bl_idname = 'dt.update_sg_current'
    bl_description = 'Set smoothing group'
    bl_options = {'UNDO'}
    index:   bpy.props.IntProperty(default=-1)
    pressed: bpy.props.IntProperty(default=0)
    def execute(self, context):
        i=self.index
        pressed=self.pressed
        pref = get_preferences()
        obj=context.edit_object
        bm = dic.setdefault(obj.name, bmesh.from_edit_mesh(obj.data))
        SG = bm.faces.layers.int.get("SG")
        sel=[f for f in bm.faces if f.select]
        if i<0:
            for face in sel:
                face[SG] = 0
            return{'FINISHED'}
        active_bit=2**i
        if pressed&active_bit!=0:
            for face in sel:
                face[SG]=uint_to_int((face[SG])&int_to_uint(~active_bit))
        else:
            for face in sel:
                face[SG]=uint_to_int(face[SG]|active_bit)
        if pref.sg_live_refresh:
            bpy.ops.dt.preview_sg()
        return {'FINISHED'}

class DAGOR_OT_SmoothGroupSelect(Operator):
    bl_label = 'Select SmoothGroup'
    bl_idname = 'dt.select_smooth_group'
    bl_description = 'Select by smoothing group'
    bl_options = {'UNDO'}
    index:   bpy.props.IntProperty(default=-1)
    def execute(self,context):
        i=self.index
        active_bit=2**i
        obj=context.edit_object
        mesh = bmesh.from_edit_mesh(obj.data)
        bm = dic.setdefault(obj.name, mesh)
        SG = bm.faces.layers.int.get("SG")
        if i<0:
            for face in bm.faces:
                if face[SG]==0:
                    face.select = True
        else:
            for face in bm.faces:
                if face[SG]&active_bit!=0:
                    face.select = True
        bmesh.update_edit_mesh(obj.data)
        return {'FINISHED'}

class DAGOR_PT_SmoothGroupPanel(bpy.types.Panel):
    bl_label = "Smoothing Groups"
    bl_region_type = "UI"
    bl_space_type = "VIEW_3D"
    bl_category = 'Dagor'

    @classmethod
    def poll(cls, context):
        # Only allow in edit mode for a selected mesh.
        return context.mode == "EDIT_MESH"

    def draw(self, context):
        pref = get_preferences()
        obj = context.object
        mesh = obj.data
        bm = dic.setdefault(obj.name, bmesh.from_edit_mesh(mesh))
        smooth_groups=bm.faces.layers.int.get('SG')
        sm=context.tool_settings.mesh_select_mode
        l=self.layout
        if smooth_groups is None:
            l.label(text='No smoothing groups detected')
            l.operator('dt.init_smooth_group',text='Init')
            return
        elif list(sm)!=[False,False,True]:
            l.label(text='Switch to FACE select mode')
            return
        face = bm.faces.active
        sel=[f for f in bm.faces if f.select]
        smooth_group_layer = bm.faces.layers.int.get("SG")
        if smooth_group_layer is None:
            l.label(text="No SG attribute detected")
        ops = l.row()
        ops.operator('dt.init_smooth_group',text='Recalculate')
        ops.operator('dt.remove_smooth_groups',text='Remove', icon = 'TRASH')
        l.operator('dt.preview_sg',text='Convert to sharp edges')
        #buttons state cheaper calc
        if sel.__len__()>0:
            all_pressed=uint_to_int(SG_MAX_UINT-1)
            ones=sel[0][smooth_groups]
            zeros=~sel[0][smooth_groups]
            for face in sel:
                all_pressed&=face[smooth_groups]
                ones&=face[smooth_groups]#similar 1 bits
                zeros&=~face[smooth_groups]#similar 0 bits
            all_active=ones|zeros
        else:
            all_pressed=0
            all_active=SG_MAX_UINT-1
        #buttons state end
        SG_set=l.box()
        draw_custom_header(SG_set, pref, 'sg_set_maximized')
        if pref.sg_set_maximized:
            SG_set.prop(pref, 'sg_live_refresh', text = "Live Update", toggle = True,
                icon = 'CHECKBOX_HLT' if pref.sg_live_refresh else 'CHECKBOX_DEHLT')
            SG_set=SG_set.column(align = True)
            reset = SG_set.row(align = True)
            reset.operator('dt.update_sg_current', text = "Clear Selected").index = -1
            text=0
            row = SG_set.row(align = True)
            columns = [row.column(align = True) for i in range(4)]
            for line in range(8):
                for column in range(4):
                    pressed=all_pressed&2**text!=0
                    active=all_active&2**text!=0
                    btn_container = columns[column].row()
                    btn = btn_container.operator('dt.update_sg_current',text=f'{text+1}',depress=pressed)
                    btn_container.active = active
                    btn.index=text
                    text+=1
                    btn.pressed=all_pressed
        SG_select = l.box()
        draw_custom_header(SG_select, 'Select by SG', pref, sg_select_maximized)
        if pref.sg_select_maximized:
            SG_select=SG_select.column(align = True)
            SG_select.operator('dt.select_smooth_group',text='0').index = -1
            text = 0
            row = SG_select.row(align = True)
            columns = [row.column(align = True) for i in range(4)]
            for line in range(8):
                for column in range(4):
                    btn_container = columns[column].row()
                    btn=btn_container.operator('dt.select_smooth_group',text=f'{text+1}')
                    btn.index=text
                    text+=1
        return

classes = [DAGOR_OT_SmoothGroupSet,
            DAGOR_OT_SmoothGroupInit,
            DAGOR_OT_SmoothGroupsRemove,
            DAGOR_OT_SmoothGroupToSharpEdges,
            DAGOR_OT_SmoothGroupSelect,
            DAGOR_PT_SmoothGroupPanel,
            ]

def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    bpy.types.WindowManager.sg_current_value = IntProperty(name="DEBUG", update=update_sg)
    bpy.app.handlers.depsgraph_update_post.append(edit_object_change_handler)

def unregister():
    for cl in classes:
        bpy.utils.unregister_class(cl)

if __name__ == "__main__":
    register()