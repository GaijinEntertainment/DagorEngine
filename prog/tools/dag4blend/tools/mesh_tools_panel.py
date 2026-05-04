import bpy, bmesh
from   bpy.types    import Operator, Panel
from   bpy.props    import BoolProperty, FloatProperty

from ..ui.draw_elements     import draw_custom_header

from ..popup.popup_functions    import show_popup
from ..helpers.texts        import log
from ..helpers.getters      import get_preferences
from  .bmesh_functions      import *
from  .mesh_tools_functions import *


classes = []

def draw_vert_cleanup(context, layout):
    box = layout.box()
    pref = get_preferences()
    props = bpy.data.scenes[0].dag4blend.tools
    draw_custom_header(box, "Vertices", pref, 'vert_cleanup_maximized', icon = 'VERTEXSEL')
    if not pref.vert_cleanup_maximized:
        return
    mode = box.row(align = True)
    mode.prop(props, 'vert_cleanup_mode', expand = True)
    box.prop(props, 'vert_threshold', text = "Angle threshold, °")
    col = box.column()
    col.scale_y = 2
    btn = col.operator('dt.find_unnecessary_verts',
        text = "Select" if props.vert_cleanup_mode == 'SELECT' else 'Dissolve')
    btn.threshold = props.vert_threshold
    btn.dissolve = props.vert_cleanup_mode == 'DISSOLVE'
    return

def draw_degenerate_removal(context, layout):
    box = layout.box()
    pref = get_preferences()
    props = bpy.data.scenes[0].dag4blend.tools
    draw_custom_header(box, "Degenerates", pref, 'tris_cleanup_maximized', icon = 'FACESEL')
    if not pref.tris_cleanup_maximized:
        return
    prop_col = box.column(align = True)
    row = prop_col.row()
    row.prop(props, 'tri_destructive', text = 'Destructive', toggle = True,
        icon = 'CHECKBOX_HLT' if props.tri_destructive else 'CHECKBOX_DEHLT')
    row = prop_col.row()
    row.prop(props, 'tri_legacy_mode', toggle = True,
        icon = 'CHECKBOX_HLT' if props.tri_legacy_mode else 'CHECKBOX_DEHLT')
    if not props.tri_legacy_mode:
        prop_col.separator()
        row = prop_col.row()
        row.label(text = 'Thresholds:')
        row = prop_col.row()
        row.prop(props, 'tri_area_threshold')
        row = prop_col.row()
        row.prop(props, 'tri_ratio_threshold')
    col = box.column()
    col.scale_y = 2
    btn = col.operator('dt.find_degenerates')
    btn.legacy_mode = props.tri_legacy_mode
    btn.destructive = props.tri_destructive
    btn.area_threshold = props.tri_area_threshold
    btn.ratio_threshold = props.tri_ratio_threshold
    return

class DAGOR_OT_Find_Unnecessary_Verts(Operator):
    '''Finds vertices with no linked faces;
verices with two linked edges almost on a same line
    '''
    bl_idname = 'dt.find_unnecessary_verts'
    bl_label = 'Find'
    bl_options = {'REGISTER', 'UNDO'}

    threshold:  FloatProperty(default = 1, min = 0)
    dissolve:   BoolProperty(default = False)

    def execute(self, context):
        bpy.ops.mesh.select_mode(use_extend = False, use_expand = False, type = 'VERT')
        for object in context.objects_in_mode:
            mesh = object.data
            bm = mesh_to_bmesh(mesh)
            if self.dissolve:
                bm_dissolve_unnecessary_verts(bm, self.threshold)
            else:
                bm_select_unnecessary_verts(bm, self.threshold)
            bmesh_to_mesh(bm, mesh)
        return {'FINISHED'}
classes.append(DAGOR_OT_Find_Unnecessary_Verts)

class DAGOR_OT_Find_Degenerated_Tris(Operator):
    '''Finds too small and stretched tris'''
    bl_idname = 'dt.find_degenerates'
    bl_label = "Find"
    bl_options = {'REGISTER', 'UNDO'}

    legacy_mode:    BoolProperty(default = True,
        description = "Use the same search algorithm as dabuild")
    destructive:    BoolProperty(default = True,
        description = "Triangulate polygons that produce degenerates on export")
    area_threshold: FloatProperty(default = 1e-3, min = 1e-4)
    ratio_threshold:FloatProperty(default = 1e-3, min = 1e-4)

    def execute (self, context):
        full_count = 0
        for obj in context.objects_in_mode:
            count = find_degenerates(obj,
                                    self.legacy_mode,
                                    self.destructive,
                                    self.area_threshold,
                                    self.ratio_threshold)
            if count == 0 :
                continue
            if self.destructive:
                log(f'Object "{obj.name}" have {count} degenerated triangles\n', type = 'WARNING')
            else:
                log(f'Object "{obj.name}" have {count} faces with degenerated triangles\n', type = 'WARNING')
            full_count += count
        bpy.ops.mesh.select_mode(type='FACE')
        if full_count == 0:
            full_count = "No"
        if self.destructive:
            msg = f"{full_count} degenerated triangles found\n"
        else:
            msg = f"{full_count} faces with degenerated triangles found\n"
        msg_type = 'INFO' if full_count == 'No' else 'WARNING'
        show_popup(message = msg)
        log(msg, type = msg_type, show = True)
        return{'FINISHED'}
classes.append(DAGOR_OT_Find_Degenerated_Tris)


# This panel conteins tools specific for edit mode

class DAGOR_PT_Mesh_Tools(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = 'mesh_edit'
    bl_label = "Tools"
    bl_category = "Dagor"

    def draw(self, context):
        layout = self.layout
        cleanup = layout.box()
        cleanup.label(text = "Cleanup")
        draw_vert_cleanup(context, cleanup)
        draw_degenerate_removal(context, cleanup)
        return
classes.append(DAGOR_PT_Mesh_Tools)


def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    return


def unregister():
    for cl in classes[::-1]:
        bpy.utils.unregister_class(cl)
    return