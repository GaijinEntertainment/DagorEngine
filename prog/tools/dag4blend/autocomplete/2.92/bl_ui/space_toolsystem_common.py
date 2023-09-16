import sys
import typing
import bpy_types


class ToolActivePanelHelper:
    bl_label = None
    ''' '''

    def draw(self, context):
        ''' 

        '''
        pass


class ToolDef:
    cursor = None
    ''' '''

    data_block = None
    ''' '''

    description = None
    ''' '''

    draw_cursor = None
    ''' '''

    draw_settings = None
    ''' '''

    icon = None
    ''' '''

    idname = None
    ''' '''

    keymap = None
    ''' '''

    label = None
    ''' '''

    operator = None
    ''' '''

    widget = None
    ''' '''

    def count(self, value):
        ''' 

        '''
        pass

    def from_dict(self, kw_args):
        ''' 

        '''
        pass

    def from_fn(self, fn):
        ''' 

        '''
        pass

    def index(self, value, start, stop):
        ''' 

        '''
        pass


class ToolSelectPanelHelper:
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

    def keymap_ui_hierarchy(self, context_mode):
        ''' 

        '''
        pass

    def register(self):
        ''' 

        '''
        pass

    def tool_active_from_context(self, context):
        ''' 

        '''
        pass


class WM_MT_toolsystem_submenu(bpy_types.Menu, bpy_types._GenericUI):
    bl_label = None
    ''' '''

    bl_rna = None
    ''' '''

    id_data = None
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

    def draw_collapsible(self, context, layout):
        ''' 

        '''
        pass

    def draw_preset(self, _context):
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

    def keys(self):
        ''' 

        '''
        pass

    def path_from_id(self):
        ''' 

        '''
        pass

    def path_menu(self, searchpaths, operator, props_default, prop_filepath,
                  filter_ext, filter_path, display_name, add_operator):
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

    def remove(self, draw_func):
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


def activate_by_id(context, space_type, idname, as_fallback):
    ''' 

    '''

    pass


def activate_by_id_or_cycle(context, space_type, idname, offset, as_fallback):
    ''' 

    '''

    pass


def description_from_id(context, space_type, idname, use_operator):
    ''' 

    '''

    pass


def item_from_flat_index(context, space_type, index):
    ''' 

    '''

    pass


def item_from_id(context, space_type, idname):
    ''' 

    '''

    pass


def item_from_id_active(context, space_type, idname):
    ''' 

    '''

    pass


def item_from_id_active_with_group(context, space_type, idname):
    ''' 

    '''

    pass


def item_from_index_active(context, space_type, index):
    ''' 

    '''

    pass


def item_group_from_id(context, space_type, idname, coerce):
    ''' 

    '''

    pass


def keymap_from_id(context, space_type, idname):
    ''' 

    '''

    pass
