import sys
import typing
import bl_ui.space_toolsystem_common
import bpy_types


class IMAGE_PT_tools_active(
        bl_ui.space_toolsystem_common.ToolSelectPanelHelper, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    keymap_prefix = None
    ''' '''

    tool_fallback_id = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_active_tool_fallback(self, context, layout, tool,
                                  is_horizontal_layout):
        ''' 

        '''
        pass

    def draw_active_tool_header(self, context, layout, show_tool_name,
                                tool_key):
        ''' 

        '''
        pass

    def draw_cls(self, layout, context, detect_layout, scale_y):
        ''' 

        '''
        pass

    def draw_fallback_tool_items(self, layout, context):
        ''' 

        '''
        pass

    def draw_fallback_tool_items_for_pie_menu(self, layout, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keymap_ui_hierarchy(self, context_mode):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def register(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def tool_active_from_context(self, context):
        ''' 

        '''
        pass

    def tools_all(self):
        ''' 

        '''
        pass

    def tools_from_context(self, context, mode):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class NODE_PT_tools_active(bl_ui.space_toolsystem_common.ToolSelectPanelHelper,
                           bpy_types.Panel, bpy_types._GenericUI):
    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    keymap_prefix = None
    ''' '''

    tool_fallback_id = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_active_tool_fallback(self, context, layout, tool,
                                  is_horizontal_layout):
        ''' 

        '''
        pass

    def draw_active_tool_header(self, context, layout, show_tool_name,
                                tool_key):
        ''' 

        '''
        pass

    def draw_cls(self, layout, context, detect_layout, scale_y):
        ''' 

        '''
        pass

    def draw_fallback_tool_items(self, layout, context):
        ''' 

        '''
        pass

    def draw_fallback_tool_items_for_pie_menu(self, layout, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keymap_ui_hierarchy(self, context_mode):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def register(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def tool_active_from_context(self, context):
        ''' 

        '''
        pass

    def tools_all(self):
        ''' 

        '''
        pass

    def tools_from_context(self, context, mode):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class SEQUENCER_PT_tools_active(
        bl_ui.space_toolsystem_common.ToolSelectPanelHelper, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    keymap_prefix = None
    ''' '''

    tool_fallback_id = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_active_tool_fallback(self, context, layout, tool,
                                  is_horizontal_layout):
        ''' 

        '''
        pass

    def draw_active_tool_header(self, context, layout, show_tool_name,
                                tool_key):
        ''' 

        '''
        pass

    def draw_cls(self, layout, context, detect_layout, scale_y):
        ''' 

        '''
        pass

    def draw_fallback_tool_items(self, layout, context):
        ''' 

        '''
        pass

    def draw_fallback_tool_items_for_pie_menu(self, layout, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keymap_ui_hierarchy(self, context_mode):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def register(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def tool_active_from_context(self, context):
        ''' 

        '''
        pass

    def tools_all(self):
        ''' 

        '''
        pass

    def tools_from_context(self, context, mode):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class VIEW3D_PT_tools_active(
        bl_ui.space_toolsystem_common.ToolSelectPanelHelper, bpy_types.Panel,
        bpy_types._GenericUI):
    bl_label = None
    ''' '''

    bl_options = None
    ''' '''

    bl_region_type = None
    ''' '''

    bl_rna = None
    ''' '''

    bl_space_type = None
    ''' '''

    id_data = None
    ''' '''

    keymap_prefix = None
    ''' '''

    tool_fallback_id = None
    ''' '''

    def append(self, draw_func):
        ''' 

        '''
        pass

    def as_pointer(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass(self):
        ''' 

        '''
        pass

    def bl_rna_get_subclass_py(self):
        ''' 

        '''
        pass

    def draw(self, context):
        ''' 

        '''
        pass

    def draw_active_tool_fallback(self, context, layout, tool,
                                  is_horizontal_layout):
        ''' 

        '''
        pass

    def draw_active_tool_header(self, context, layout, show_tool_name,
                                tool_key):
        ''' 

        '''
        pass

    def draw_cls(self, layout, context, detect_layout, scale_y):
        ''' 

        '''
        pass

    def draw_fallback_tool_items(self, layout, context):
        ''' 

        '''
        pass

    def draw_fallback_tool_items_for_pie_menu(self, layout, context):
        ''' 

        '''
        pass

    def driver_add(self):
        ''' 

        '''
        pass

    def driver_remove(self):
        ''' 

        '''
        pass

    def get(self):
        ''' 

        '''
        pass

    def is_extended(self):
        ''' 

        '''
        pass

    def is_property_hidden(self):
        ''' 

        '''
        pass

    def is_property_overridable_library(self):
        ''' 

        '''
        pass

    def is_property_readonly(self):
        ''' 

        '''
        pass

    def is_property_set(self):
        ''' 

        '''
        pass

    def items(self):
        ''' 

        '''
        pass

    def keyframe_delete(self):
        ''' 

        '''
        pass

    def keyframe_insert(self):
        ''' 

        '''
        pass

    def keymap_ui_hierarchy(self, context_mode):
        ''' 

        '''
        pass

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_resolve(self):
        ''' 

        '''
        pass

    def pop(self):
        ''' 

        '''
        pass

    def prepend(self, draw_func):
        ''' 

        '''
        pass

    def property_overridable_library_set(self):
        ''' 

        '''
        pass

    def property_unset(self):
        ''' 

        '''
        pass

    def register(self):
        ''' 

        '''
        pass

    def remove(self, draw_func):
        ''' 

        '''
        pass

    def tool_active_from_context(self, context):
        ''' 

        '''
        pass

    def tools_all(self):
        ''' 

        '''
        pass

    def tools_from_context(self, context, mode):
        ''' 

        '''
        pass

    def type_recast(self):
        ''' 

        '''
        pass

    def values(self):
        ''' 

        '''
        pass


class _defs_annotate:
    eraser = None
    ''' '''

    line = None
    ''' '''

    poly = None
    ''' '''

    scribble = None
    ''' '''

    def draw_settings_common(self, context, layout, tool):
        ''' 

        '''
        pass


class _defs_edit_armature:
    bone_envelope = None
    ''' '''

    bone_size = None
    ''' '''

    extrude = None
    ''' '''

    extrude_cursor = None
    ''' '''

    roll = None
    ''' '''


class _defs_edit_curve:
    curve_radius = None
    ''' '''

    curve_vertex_randomize = None
    ''' '''

    draw = None
    ''' '''

    extrude = None
    ''' '''

    extrude_cursor = None
    ''' '''

    tilt = None
    ''' '''


class _defs_edit_mesh:
    bevel = None
    ''' '''

    bisect = None
    ''' '''

    edge_slide = None
    ''' '''

    extrude = None
    ''' '''

    extrude_cursor = None
    ''' '''

    extrude_individual = None
    ''' '''

    extrude_manifold = None
    ''' '''

    extrude_normals = None
    ''' '''

    inset = None
    ''' '''

    knife = None
    ''' '''

    loopcut_slide = None
    ''' '''

    offset_edge_loops_slide = None
    ''' '''

    poly_build = None
    ''' '''

    push_pull = None
    ''' '''

    rip_edge = None
    ''' '''

    rip_region = None
    ''' '''

    shrink_fatten = None
    ''' '''

    spin = None
    ''' '''

    spin_duplicate = None
    ''' '''

    tosphere = None
    ''' '''

    vert_slide = None
    ''' '''

    vertex_randomize = None
    ''' '''

    vertex_smooth = None
    ''' '''


class _defs_gpencil_edit:
    bend = None
    ''' '''

    box_select = None
    ''' '''

    circle_select = None
    ''' '''

    extrude = None
    ''' '''

    lasso_select = None
    ''' '''

    radius = None
    ''' '''

    select = None
    ''' '''

    shear = None
    ''' '''

    tosphere = None
    ''' '''

    transform_fill = None
    ''' '''

    def is_segment(self, context):
        ''' 

        '''
        pass


class _defs_gpencil_paint:
    arc = None
    ''' '''

    box = None
    ''' '''

    circle = None
    ''' '''

    curve = None
    ''' '''

    cutter = None
    ''' '''

    eyedropper = None
    ''' '''

    line = None
    ''' '''

    polyline = None
    ''' '''

    def generate_from_brushes(self, context):
        ''' 

        '''
        pass

    def gpencil_primitive_toolbar(self, context, layout, tool, props):
        ''' 

        '''
        pass


class _defs_gpencil_sculpt:
    def generate_from_brushes(self, context):
        ''' 

        '''
        pass

    def poll_select_mask(self, context):
        ''' 

        '''
        pass


class _defs_gpencil_vertex:
    def generate_from_brushes(self, context):
        ''' 

        '''
        pass

    def poll_select_mask(self, context):
        ''' 

        '''
        pass


class _defs_gpencil_weight:
    def generate_from_brushes(self, context):
        ''' 

        '''
        pass


class _defs_image_generic:
    cursor = None
    ''' '''

    sample = None
    ''' '''

    def poll_uvedit(self, context):
        ''' 

        '''
        pass


class _defs_image_uv_edit:
    rip_region = None
    ''' '''


class _defs_image_uv_sculpt:
    def generate_from_brushes(self, context):
        ''' 

        '''
        pass


class _defs_image_uv_select:
    box = None
    ''' '''

    circle = None
    ''' '''

    lasso = None
    ''' '''

    select = None
    ''' '''


class _defs_image_uv_transform:
    rotate = None
    ''' '''

    scale = None
    ''' '''

    transform = None
    ''' '''

    translate = None
    ''' '''


class _defs_node_edit:
    links_cut = None
    ''' '''


class _defs_node_select:
    box = None
    ''' '''

    circle = None
    ''' '''

    lasso = None
    ''' '''

    select = None
    ''' '''


class _defs_particle:
    def generate_from_brushes(self, context):
        ''' 

        '''
        pass


class _defs_pose:
    breakdown = None
    ''' '''

    push = None
    ''' '''

    relax = None
    ''' '''


class _defs_sculpt:
    cloth_filter = None
    ''' '''

    color_filter = None
    ''' '''

    face_set_box = None
    ''' '''

    face_set_edit = None
    ''' '''

    face_set_lasso = None
    ''' '''

    hide_border = None
    ''' '''

    mask_border = None
    ''' '''

    mask_by_color = None
    ''' '''

    mask_lasso = None
    ''' '''

    mask_line = None
    ''' '''

    mesh_filter = None
    ''' '''

    project_line = None
    ''' '''

    trim_box = None
    ''' '''

    trim_lasso = None
    ''' '''

    def generate_from_brushes(self, context):
        ''' 

        '''
        pass


class _defs_sequencer_generic:
    blade = None
    ''' '''

    sample = None
    ''' '''


class _defs_sequencer_select:
    box = None
    ''' '''

    select = None
    ''' '''


class _defs_texture_paint:
    def generate_from_brushes(self, context):
        ''' 

        '''
        pass

    def poll_select_mask(self, context):
        ''' 

        '''
        pass


class _defs_transform:
    rotate = None
    ''' '''

    scale = None
    ''' '''

    scale_cage = None
    ''' '''

    shear = None
    ''' '''

    transform = None
    ''' '''

    translate = None
    ''' '''


class _defs_vertex_paint:
    def generate_from_brushes(self, context):
        ''' 

        '''
        pass

    def poll_select_mask(self, context):
        ''' 

        '''
        pass


class _defs_view3d_add:
    cone_add = None
    ''' '''

    cube_add = None
    ''' '''

    cylinder_add = None
    ''' '''

    ico_sphere_add = None
    ''' '''

    uv_sphere_add = None
    ''' '''

    def description_interactive_add(self, context, _item, _km, prefix):
        ''' 

        '''
        pass

    def draw_settings_interactive_add(self, layout, tool, extra):
        ''' 

        '''
        pass


class _defs_view3d_generic:
    cursor = None
    ''' '''

    cursor_click = None
    ''' '''

    ruler = None
    ''' '''


class _defs_view3d_select:
    box = None
    ''' '''

    circle = None
    ''' '''

    lasso = None
    ''' '''

    select = None
    ''' '''


class _defs_weight_paint:
    gradient = None
    ''' '''

    sample_weight = None
    ''' '''

    sample_weight_group = None
    ''' '''

    def generate_from_brushes(self, context):
        ''' 

        '''
        pass

    def poll_select_mask(self, context):
        ''' 

        '''
        pass


class _template_widget:
    def VIEW3D_GGT_xform_extrude(self):
        ''' 

        '''
        pass

    def VIEW3D_GGT_xform_gizmo(self):
        ''' 

        '''
        pass


def generate_from_enum_ex(_context, idname_prefix, icon_prefix, type, attr,
                          cursor, tooldef_keywords, exclude_filter):
    ''' 

    '''

    pass


def kmi_to_string_or_none(kmi):
    ''' 

    '''

    pass
