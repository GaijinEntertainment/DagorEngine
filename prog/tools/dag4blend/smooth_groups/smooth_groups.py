import bpy
import bmesh
from math             import pi
from bpy.app.handlers import persistent
from bpy.props        import IntProperty


#constants
SG_MAX_UINT = 4294967295#2**0..2**31
SG_CONVERT  = 2147483648#greater than signed int32 max

def uint_to_int(int):
    if int>=SG_CONVERT:
        return int-4294967296#int-2**32, which is = SG_MAX_UINT+1
    else:
        return int
def int_to_uint(int):
    if int<0:
        return 4294967296+int#2**32+int
    else:
        return int

dic={}
# update method
def updage_sg(self, context):
    eo = context.edit_object
    bm = dic.setdefault(eo.name, bmesh.from_edit_mesh(eo.data))
    face = bm.faces.active
    SG = bm.faces.layers.int.get("SG")
    if face and SG:
        face[SG] = self.layer_int_value
    return None
#layer_int_value is used to set the value

#functions
def precalc_sg(mesh):
    SG=mesh.attributes.get('SG')
    if SG is None:
        SG=mesh.attributes.new('SG','INT','FACE')
    elif SG.domain!='FACE' or SG.data_type!='INT':
        SG.name=SG.name+'.wrong_type'
        SG=mesh.attributes.new('SG','INT','FACE')
    precalc=mesh.calc_smooth_groups(use_bitflags=True)[0]
    for i in range (SG.data.__len__()):
        SG.data[i].value=precalc[i] if mesh.polygons[i].use_smooth else 0
    return
#edit mode only
def sg_to_sharp_edges(mesh):
    mesh.use_auto_smooth=True
    mesh.auto_smooth_angle=pi
    if bpy.context.mode=='EDIT_MESH':
        bm=bmesh.from_edit_mesh(mesh)
    else:
        bm=bmesh.new()
        bm.from_mesh(mesh)
    bm.faces.ensure_lookup_table()
    bm.edges.ensure_lookup_table()
    edge_group=[0 for index in range(bm.edges.__len__())]
    smoothed=[edge_group.copy() for i in range(32)]
    SG=bm.faces.layers.int['SG']
    for face in bm.faces:
        face.smooth=True
        for i in range (32):
            if (face[SG])&(2**i)!=0:
                for edge in face.edges:
                    smoothed[i][edge.index]+=1
    for edge in bm.edges:
        edge.smooth=False
    for smooth_group in smoothed:
        i=0
        for edge_count in smooth_group:
            if edge_count>1:
                bm.edges[i].smooth=True
            i+=1
    if bpy.context.mode=='EDIT_MESH':
        bmesh.update_edit_mesh(mesh)
    else:
        bm.to_mesh(mesh)
        bm.free()
        del bm
    return

#keep wm.layer_int_value updated to active face
def set_smooth_group(bm):
    face = bm.faces.active
    if face is not None:
        SG = bm.faces.layers.int.get("SG")
        if SG:
            bpy.context.window_manager.layer_int_value = face[SG]
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
        set_smooth_group(bm)
        return None
    dic.clear()
    return None

#operators
class SmoothGroupInit(bpy.types.Operator):
    bl_label = 'Init SmoothGroup'
    bl_idname = 'dt.init_smooth_group'
    bl_description = 'Initialize smoothing groups'
    bl_options = {'UNDO'}
    object: bpy.props.StringProperty(default='')
    def execute(self, context):
        if context.mode=='EDIT_MESH':
            bpy.ops.object.editmode_toggle()
            precalc_sg(context.object.data)
            bpy.ops.object.editmode_toggle()
        return {'FINISHED'}

class SmoothGroupsRemove(bpy.types.Operator):
    bl_label = 'Remove SmoothGroups'
    bl_idname = 'dt.remove_smooth_groups'
    bl_description = 'Remove SG data'
    bl_options = {'UNDO'}
    object: bpy.props.StringProperty(default='')
    def execute(self, context):
        if context.mode=='EDIT_MESH':
            bpy.ops.object.editmode_toggle()
            mesh = context.object.data
            SG = mesh.attributes.get('SG')
            if SG is not None:
                mesh.attributes.remove(SG)
            bpy.ops.object.editmode_toggle()
        return {'FINISHED'}

class SmoothGroupToSharpEdges(bpy.types.Operator):
    bl_label = 'SmoothGroups to sharp edges'
    bl_idname = 'dt.preview_sg'
    bl_description = 'Preview smoothing groups as sharp edges'
    bl_options = {'UNDO'}
    def execute(self, context):
        if context.mode=='EDIT_MESH':
            sg_to_sharp_edges(context.edit_object.data)
        return {'FINISHED'}

class SmoothGroupSet(bpy.types.Operator):
    bl_label = 'Set SmoothGroup'
    bl_idname = 'dt.set_smooth_group'
    bl_description = 'Set smoothing group'
    bl_options = {'UNDO'}
    index:   bpy.props.IntProperty(default=-1)
    pressed: bpy.props.IntProperty(default=0)
    def execute(self, context):
        i=self.index
        pressed=self.pressed
        if i<0:
            return{'CANCELLED'}
        obj=context.edit_object
        bm = dic.setdefault(obj.name, bmesh.from_edit_mesh(obj.data))
        SG = bm.faces.layers.int.get("SG")
        sel=[f for f in bm.faces if f.select]
        active_bit=2**i
        if pressed&active_bit!=0:
            for face in sel:
                face[SG]=uint_to_int((face[SG])&int_to_uint(~active_bit))
        else:
            for face in sel:
                face[SG]=uint_to_int(face[SG]|active_bit)
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
        SG=l.box()
        i=0
        #buttons state cheaper calc
        if sel.__len__()>0:
            all_pressed=uint_to_int(SG_MAX_UINT)
            tex_ones=sel[0][smooth_groups]
            tex_zeros=~sel[0][smooth_groups]
            for face in sel:
                all_pressed&=face[smooth_groups]
                tex_ones&=face[smooth_groups]#similar 1 bits
                tex_zeros&=~face[smooth_groups]#similar 0 bits
            all_w_text=tex_ones|tex_zeros
        else:
            all_pressed=0
            all_w_text=SG_MAX_UINT
        #buttons state end
        for line in range(8):
            row=SG.row()
            for column in range (4):
                pressed=all_pressed&2**i!=0
                w_text=all_w_text&2**i!=0
                i+=1
                btn=row.operator('dt.set_smooth_group',text=str(i) if w_text else '',depress=pressed)
                btn.index=i-1
                btn.pressed=all_pressed

classes = [SmoothGroupSet,
            SmoothGroupInit,
            SmoothGroupsRemove,
            SmoothGroupToSharpEdges,
            DAGOR_PT_SmoothGroupPanel,
            ]

def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    bpy.types.WindowManager.layer_int_value = IntProperty(name="DEBUG", update=updage_sg)
    bpy.app.handlers.depsgraph_update_post.append(edit_object_change_handler)

def unregister():
    for cl in classes:
        bpy.utils.unregister_class(cl)

if __name__ == "__main__":
    register()