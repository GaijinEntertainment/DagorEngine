import bpy, os, bmesh, math
from   bpy.types                    import Operator, Panel
from   bpy.props                    import StringProperty,IntProperty
from   mathutils                    import Vector

from ..helpers.basename             import basename

def remove_service_vcol_layers(obj, vertexColorIndexName):
    mesh = obj.data
    for vcol in mesh.vertex_colors:
        if vcol.name != vertexColorIndexName:
            mesh.vertex_colors.remove(vcol)

def set_inside_faces(obj):
    mesh = obj.data
    clothSide = mesh.attributes.get('clothSide')
    if clothSide is None:
        clothSide=mesh.attributes.new('clothSide','INT','FACE')
    elif clothSide.domain!='FACE' or clothSide.data_type!='INT':
        clothSide.name=clothSide.name+'.wrong_type'
        clothSide=mesh.attributes.new('clothSide','INT','FACE')

    for i in range (clothSide.data.__len__()):
        clothSide.data[i].value = 0

    for p in mesh.polygons:
        if p.select:
            clothSide.data[p.index].value = 1

def invert_inside_faces_normals(obj):
    mesh = obj.data
    vertexColorIndexName = "windDir"
    color_layer = mesh.vertex_colors[vertexColorIndexName]

    mesh = bpy.context.active_object.data
    clothSide = mesh.attributes.get('clothSide')
    if clothSide is None:
        clothSide=mesh.attributes.new('clothSide','INT','FACE')

    for p in mesh.polygons:
        isInside = mesh.polygon_layers_int["clothSide"].data[p.index].value
        if not isInside:
            continue
        for li in p.loop_indices:
            normal = -(Vector(color_layer.data[li].color).xyz * 2 - Vector([1, 1, 1]))
            normal = normal * 0.5 + Vector([0.5, 0.5, 0.5])
            color_layer.data[li].color = Vector([normal.x, normal.y, normal.z, 1])


def bake_normals_to_vertex_color(obj, vertexColorIndexName):
    mesh = obj.data
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.faces.ensure_lookup_table()
    bm.edges.ensure_lookup_table()

    if not "_service_layer" in mesh.vertex_colors:
        mesh.vertex_colors.new(name="_service_layer")

    if not vertexColorIndexName in mesh.vertex_colors:
        mesh.vertex_colors.new(name=vertexColorIndexName)

    color_layer = mesh.vertex_colors[vertexColorIndexName]
    color_layer.active = True
    color_layer.active_render = True
    mesh.calc_normals_split()

    for loop in mesh.loops:
        edge = bm.edges[loop.edge_index]
        normal = (loop.normal + Vector([1, 1, 1])) * 0.5
        color_layer.data[loop.index].color = (normal.x, normal.y, normal.z, 1.0)

def fix_vertex_color_normals_smooth_groups(obj, vertexColorIndexName, defaultSG):
    mesh = obj.data
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.faces.ensure_lookup_table()
    bm.edges.ensure_lookup_table()

    if not "_service_layer" in mesh.vertex_colors:
        mesh.vertex_colors.new(name="_service_layer")

    if not vertexColorIndexName in mesh.vertex_colors:
        mesh.vertex_colors.new(name=vertexColorIndexName)

    color_layer = mesh.vertex_colors[vertexColorIndexName]
    color_layer.active = True
    color_layer.active_render = True
    mesh.calc_normals_split()

    color_layer = mesh.vertex_colors[vertexColorIndexName]
    SG = bm.faces.layers.int['SG']
    clothSide = bm.faces.layers.int['clothSide']
    mesh.calc_normals_split()

    updateAllVertexLoops = True
    hardEdges = [e for e in bm.edges if not e.smooth]
    for e in hardEdges:
        fs_loops = set()
        for face in e.link_faces:
            fs_loops |= set(face.loops)
        for face in e.link_faces:
            if face[SG] == defaultSG:
                v1 = e.verts[0]
                v2 = e.verts[1]
                v1_loops = set(v1.link_loops)
                v2_loops = set(v2.link_loops)
                f_loops = set(face.loops)
                v1_normal = mesh.loops[(v1_loops & f_loops).pop().index].normal
                v2_normal = mesh.loops[(v2_loops & f_loops).pop().index].normal
                if face[clothSide] == 1:
                    v1_normal = -v1_normal
                    v2_normal = -v2_normal
                v1_normal =  (v1_normal + \
                    Vector([1, 1, 1])) * 0.5
                v2_normal =  (v2_normal + \
                    Vector([1, 1, 1])) * 0.5
                s1 = v1_loops if updateAllVertexLoops else v1_loops & fs_loops
                s2 = v2_loops if updateAllVertexLoops else v2_loops & fs_loops
                for l in s1:
                    color_layer.data[l.index].color = (
                        v1_normal.x, v1_normal.y, v1_normal.z, 1.0)
                for l in s2:
                    color_layer.data[l.index].color = (
                        v2_normal.x, v2_normal.y, v2_normal.z, 1.0)

class DAGOR_OT_BakeNormalsToVertexColor(Operator):
    bl_idname = "dt.bake_normals_to_vertex_color"
    bl_label = "Bake normals to vertex colors"
    bl_description = "Used for cloth wind effect"
    bl_options = {'UNDO'}

    vertexColorIndexName: StringProperty(default='windDir', description = "Normals vertex color index name")

    def execute(self,context):
        sel=list(obj for obj in context.view_layer.objects.selected if obj.type=='MESH')
        if sel.__len__()==0:
            show_popup(
                message='No selected meshes to process!',
                title='Error!',
                icon='ERROR')
            return {'CANCELLED'}
        for obj in sel:
            bake_normals_to_vertex_color(obj, self.vertexColorIndexName)
        return{'FINISHED'}

class DAGOR_OT_FixVertexColorSmoothGroupsNormals(Operator):
    bl_idname = "dt.fix_vertex_color_smooth_groups_normals"
    bl_label = "Fix vertex color smooth groups normals "
    bl_description = "Used for clothes wind effect"
    bl_options = {'UNDO'}

    defaultSG:           IntProperty(default = 1)
    vertexColorIndexName: StringProperty(default = 'windDir', description = "Normals vertex color index name")

    def execute(self,context):
        sel=list(obj for obj in context.view_layer.objects.selected if obj.type=='MESH')
        if sel.__len__()==0:
            show_popup(
                message='No selected meshes to process!',
                title='Error!',
                icon='ERROR')
            return {'CANCELLED'}
        for obj in sel:
            fix_vertex_color_normals_smooth_groups(obj, self.vertexColorIndexName, self.defaultSG)
        return{'FINISHED'}

class DAGOR_OT_SetInsideFaces(Operator):
    bl_idname = "dt.set_inside_faces"
    bl_label = "Set inside faces"
    bl_description = "Used for cloth wind effect"
    bl_options = {'UNDO'}

    def execute(self,context):
        sel=list(obj for obj in context.view_layer.objects.selected if obj.type=='MESH')
        if sel.__len__()==0:
            show_popup(
                message='No selected meshes to process!',
                title='Error!',
                icon='ERROR')
            return {'CANCELLED'}
        for obj in sel:
            set_inside_faces(obj)
        return{'FINISHED'}

class DAGOR_OT_InvertInsideFacesVcolNormals(Operator):
    bl_idname = "dt.invert_inside_faces_vcol_normals"
    bl_label = "Inverts inside faces vertex color normals"
    bl_description = "Used for cloth wind effect"
    bl_options = {'UNDO'}

    def execute(self,context):
        sel=list(obj for obj in context.view_layer.objects.selected if obj.type=='MESH')
        if sel.__len__()==0:
            show_popup(
                message='No selected meshes to process!',
                title='Error!',
                icon='ERROR')
            return {'CANCELLED'}
        for obj in sel:
            invert_inside_faces_normals(obj)
        return{'FINISHED'}

class DAGOR_OT_RemoveServiseVcolLayer(Operator):
    bl_idname = "dt.remove_service_vcol_layers"
    bl_label = "Removes service vertex color layers"
    bl_description = "Used for cloth wind effect"
    bl_options = {'UNDO'}

    vertexColorIndexName: StringProperty(default='windDir', description = "Normals vertex color index name")

    def execute(self,context):
        sel=list(obj for obj in context.view_layer.objects.selected if obj.type=='MESH')
        if sel.__len__()==0:
            show_popup(
                message='No selected meshes to process!',
                title='Error!',
                icon='ERROR')
            return {'CANCELLED'}
        for obj in sel:
            remove_service_vcol_layers(obj, self.vertexColorIndexName)
        return{'FINISHED'}

class DAGOR_PT_Tools(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "vertexpaint"
    bl_label = "Tools"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    def draw(self,context):
        pref=context.preferences.addons[basename(__package__)].preferences
        S=context.scene
        layout = self.layout
        windbox = layout.box()
        header = windbox.row()
        header.prop(pref, 'windbox_maximized',icon = 'DOWNARROW_HLT'if pref.windbox_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='WindTools',expand=True)
        header.label(text="", icon = 'MOD_PHYSICS')
        if pref.windbox_maximized:

            vcolName = windbox.row()
            vcolName.label(text="Vertex Color Index Name:")
            vcolName.prop(pref,'wind_vertexColorIndexName',text='')
            bake = windbox.operator('dt.bake_normals_to_vertex_color')
            bake.vertexColorIndexName = pref.wind_vertexColorIndexName

            op = windbox.operator('dt.set_inside_faces')
            op = windbox.operator('dt.invert_inside_faces_vcol_normals')

            defaultSG=windbox.row()
            defaultSG.label(text="Default smooth group:")
            defaultSG.prop(pref, 'wind_defaultSG',text='')
            fix = windbox.operator('dt.fix_vertex_color_smooth_groups_normals')
            fix.vertexColorIndexName = pref.wind_vertexColorIndexName
            fix.defaultSG = pref.wind_defaultSG

            op = windbox.operator('dt.remove_service_vcol_layers')
            op.vertexColorIndexName = pref.wind_vertexColorIndexName

classes = [
           DAGOR_OT_BakeNormalsToVertexColor,
           DAGOR_OT_FixVertexColorSmoothGroupsNormals,
           DAGOR_OT_InvertInsideFacesVcolNormals,
           DAGOR_OT_RemoveServiseVcolLayer,
           DAGOR_OT_SetInsideFaces,
           DAGOR_PT_Tools
           ]

def register():
    for cl in classes:
        bpy.utils.register_class(cl)


def unregister():
    for cl in classes[::-1]:
        bpy.utils.unregister_class(cl)