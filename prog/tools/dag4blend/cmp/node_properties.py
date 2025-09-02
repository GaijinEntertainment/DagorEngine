import bpy

from bpy.types  import Operator
from bpy.utils  import register_class, unregister_class
from bpy.props  import IntProperty

classes = []

place_types = [
                "default",
                "pivot",
                "pivot and use normal",
                "3-point (bbox)",
                "foundation (bbox)",
                "on water (floatable)",
                "pivot with RI collision",
                ]

#
#operator functions
#

def rotation_convert(node):
    wm = node.matrix_world
    node.rotation_mode = 'XYZ'
    node.matrix_world = wm  # rotation now looks the same as before, despite difference in axis order
    return

def matatrix_to_offsets(node):
    node.dagorprops['use_tm:b'] = 'no'
    return

def offsets_to_matrix(node):
    del node.dagorprops['use_tm:b']
    return

#
#extra functions
#
def uses_matrix(node):
    if node is None:
        return False
    DP = node.dagorprops
    if 'use_tm:b' not in DP.keys():
        return True
    if f"{DP['use_tm:b']}".lower() in ['yes', 'true', '1']:
        return True
    return False


#
#draw-functions
#
def draw_matrix(layout, node):
    tm_column = layout.column(align = True)
    tm_column.operator('dt.toggle_tm',
        icon = 'CHECKBOX_HLT' if uses_matrix(node) else 'CHECKBOX_DEHLT',
        depress = uses_matrix(node))
    if not uses_matrix(node):
        return
    location_box = tm_column.box()
    column = location_box.column(align = True)
    column.label(text = 'Location:')
    row = column.row(align = True)
    row.prop(node, 'location', index = 0, text = "")
    row.prop(node, 'location', index = 2, text = "")
    row.prop(node, 'location', index = 1, text = "")

    rotation_box = tm_column.box()
    column = rotation_box.column(align = True)
    column.label(text = 'Rotation:')
    row = column.row(align = True)
    if node.rotation_mode == 'XYZ':
        row.prop(node, 'rotation_euler', index = 0, text = "")
        row.prop(node, 'rotation_euler', index = 2, text = "")
        row.prop(node, 'rotation_euler', index = 1, text = "")
    else:
        row.operator('dt.rotation_convert', icon = 'ERROR')

    scale_box = tm_column.box()
    column = scale_box.column(align = True)
    column.label(text = 'Scale:')
    row = column.row(align = True)
    row.prop(node, 'scale', index = 0, text = "")
    row.prop(node, 'scale', index = 2, text = "")
    row.prop(node, 'scale', index = 1, text = "")
    return

def draw_place_type(layout, node):
    if node is None:
        layout.label(text = 'Select a node', icon = 'INFO')
        return
    elif node.type!='EMPTY':
        layout.label(text = 'Only Empty supported', icon = 'INFO')
        return
    column = layout.column(align = True)
    current_index = -1
    if node.dagorprops.get('place_type:i') is not None:
        current_index = node.dagorprops['place_type:i']
    index = -1
    for type in place_types:
        set = column.operator('dt.node_place_type_set', text = type, depress = index == current_index,
            icon = 'RADIOBUT_ON' if index == current_index else 'RADIOBUT_OFF')
        set.value = index
        index += 1
    return

#
#classes
#

class DAGOR_OT_rotation_convert(Operator):
    bl_idname = 'dt.rotation_convert'
    bl_label = 'Convert to XYZ'
    bl_description = 'Only XYZ Euler rotation will be displayed correctly'
    bl_options = {'UNDO'}

    @classmethod
    def poll(self, context):
        if context.object is None:
            return False
        if context.object.type != 'EMPTY':
            return False
        if context.object.rotation_mode == 'XYZ':
            return False
        return True

    def execute(self, context):
        rotation_convert(context.object)
        return {'FINISHED'}
classes.append(DAGOR_OT_rotation_convert)

class DAGOR_OT_node_place_type_set(Operator):
    bl_idname = 'dt.node_place_type_set'
    bl_label = 'Set place type'
    bl_description = 'Set place_type to this value'

    value:  IntProperty(default = -1)

    bl_options = {'UNDO'}
    @classmethod
    def poll(self, context):
        if context.object is None:
            return False
        if context.object.type != 'EMPTY':
            return False
        return True

    def execute(self, context):
        DP = context.object.dagorprops
        if not 0 <= self.value <= 5:
            if DP.get('place_type:i') is None:
                return {'FINISHED'}
            del DP['place_type:i']
            return {'FINISHED'}
        DP['place_type:i'] = self.value
        return {'FINISHED'}
classes.append(DAGOR_OT_node_place_type_set)


class DAGOR_OT_node_toggle_tm(Operator):
    bl_idname = 'dt.toggle_tm'
    bl_label = 'Use TM'
    bl_description = 'Switch between simple matrix and randomized offsets'
    bl_options = {'UNDO'}

    @classmethod
    def poll(self, context):
        if context.object is None:
            return False
        if context.object.type != 'EMPTY':
            return False
        return True

    def execute(self, context):
        active_node = context.object
        DP = active_node.dagorprops
        if ('use_tm:b' in DP.keys()
            and f"{DP['use_tm:b']}".lower() in ['0','no', 'false']):
            offsets_to_matrix(active_node)
        else:
            matatrix_to_offsets(active_node)
        return {'FINISHED'}
classes.append(DAGOR_OT_node_toggle_tm)


def register():
    for cl in classes:
        register_class(cl)
    return

def unregister():
    for cl in classes[::-1]:
        unregister_class(cl)
    return