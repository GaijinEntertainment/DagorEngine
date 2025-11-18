from os.path    import exists, isfile, join
from os         import listdir

import bpy

from   bpy.props    import StringProperty, PointerProperty, FloatProperty
from   bpy.utils    import register_class, unregister_class
from   bpy.types    import Operator, Panel, PropertyGroup

from ..helpers.texts    import *
from ..ui.draw_elements import draw_custom_header
from ..popup.popup_functions    import show_popup
from ..helpers.props    import fix_type
from ..helpers.getters  import get_preferences

#functions
def clear_props(obj):
    props=[]
    for key in obj.dagorprops.keys():
        props.append(key)
    for key in props:
        del obj.dagorprops[key]

def update_uniform_scale(self, context):
    self.scale = [self.uniform_scale]*3
    return

#func/text
def text_to_props(obj):
    clear_props(obj)
    DP=obj.dagorprops
    txt=get_text('props_temp')
    broken=''
    for line in txt.lines:
        line=line.body
        line=line.replace(' ','')
        if line!='':
            prop=line.split('=')
            if prop.__len__()!=2:
                broken+=line+';'
            elif prop[0].__len__()==0 or prop[1].__len__()==0:
                broken+=line+';'
            elif prop[0].endswith(':b') and prop[1] not in ['yes','no']:
                broken+=line+';'
            elif prop[0]=='broken_properties:t':
                broken+=line[1:-1]+';'
            else:
                DP[prop[0]]=fix_type(prop[1])
    if broken.__len__()>0:
        DP['broken_properties:t']=f'"{broken}"'

def props_to_text(obj):
    txt=get_text_clear('props_temp')
    DP=obj.dagorprops
    for key in obj.dagorprops.keys():
        txt.write(key+'=')
        try:
            value=str(DP[key].to_list())
            value=value.replace('[','')
            value=value.replace(']','')
        except:
            value=str(DP[key])
        txt.write(value+'\n')

def get_presets_list():
    pref = get_preferences()
    path = pref.props_presets_path
    files = [f for f in listdir(path) if isfile(join(path,f))]
    return files

#operators
class DAGOR_OT_text_to_props(Operator):
    bl_idname = "dt.text_to_props"
    bl_label = "Set object properties from text"
    bl_description = "Apply properties from text to active object"
    bl_options = {'REGISTER', 'UNDO'}
    def execute(self, context):
        obj = bpy.context.object
        if obj is None:
            show_popup(message='No active object',title='Error',icon='ERROR')
            return {'CANCELLED'}
        txt=bpy.data.texts.get('props_temp')
        if txt is None:
            show_popup(message='Text [props_temp] not found',title='Error',icon='ERROR')
            return {'CANCELLED'}
        text_to_props(obj)
        return {'FINISHED'}


class DAGOR_OT_props_to_text(Operator):
    bl_idname = "dt.props_to_text"
    bl_label = "Write object properties to text"
    bl_description = "Write properties from active object to text"
    bl_options = {'REGISTER', 'UNDO'}
    def execute(self, context):
        obj = bpy.context.object
        if obj is None:
            show_popup(message='No active object',title='Error',icon='ERROR')
            return {'CANCELLED'}
        props_to_text(obj)
        show_text(get_text('props_temp'))
        show_popup(message='Check "props_temp" in text editor',title='Done',icon='INFO')
        return {'FINISHED'}


class DAGOR_OT_apply_op_preset(Operator):
    bl_idname = "dt.apply_op_preset"
    bl_label = "Apply properties from preset"
    bl_description = "Apply selected prop preset to active object"
    bl_options = {'UNDO'}
    def execute(self, context):
        obj = bpy.context.object
        if obj is None:
            show_popup(message='No active object found',title='Error',icon='ERROR')
            return {'CANCELLED'}
        temp=get_text_clear('props_temp')
        pref = get_preferences()
        preset = pref.prop_preset
        path = pref.props_presets_path + f"/{preset}.txt"
        if exists(path):
            with open(path,'r') as t:
                temp.write(t.read())
                t.close()
            text_to_props(obj)
        else:
            show_popup(message = f'"{preset}.txt" not found in {pref.props_presets_path}', title='Error', icon='ERROR')
        return {'FINISHED'}


class DAGOR_OT_save_op_preset(Operator):
    bl_idname = "dt.save_op_preset"
    bl_label = "Save props preset"
    bl_description = "Save object properties as preset"
    bl_options = {'UNDO'}
    def execute(self, context):
        obj = bpy.context.object
        if obj is None:
            show_popup(message='No active object found',title='Error',icon='ERROR')
            return {'CANCELLED'}
        props_to_text(obj)
        pref = get_preferences()
        name = pref.prop_preset_name
        if name == '':
            name = 'unnamed'
        dirpath = pref.props_presets_path
        if not exists(dirpath):
            os.makedirs(dirpath)
        path = f"{dirpath}/{name}.txt"
        with open (path,'w') as preset:
            for line in bpy.data.texts["props_temp"].lines:
                if line.body!='':
                    preset.write(f'{line.body}\n')
        preset.close()
        return {'FINISHED'}


class DAGOR_OT_remove_op_preset(Operator):
    bl_idname = "dt.remove_op_preset"
    bl_label = "Remove preset"
    bl_description = "Remove active object props preset"
    bl_options = {'UNDO'}

    @classmethod
    def poll(self, context):
        return get_presets_list().__len__()>0

    def execute(self, context):
        pref = get_preferences()
        name = pref.prop_preset
        path = pref.props_presets_path + f"/{name}.txt"
        os.remove(path)
        list = get_presets_list()
        if list.__len__()>0:
            pref.props_preset = 0
        return {'FINISHED'}


class DAGOR_invert_dagbool(Operator):
    bl_idname = 'dt.invert_dagbool'
    bl_label = 'Switch value'
    bl_description = 'invert dag_bool value'
    prop: StringProperty(default='yes')
    def execute(self,context):
        DP=bpy.context.object.dagorprops
        prop=self.prop
        if DP[prop]=='no':
            DP[prop]='yes'
        else:
            DP[prop]='no'
        return {'FINISHED'}

class DAGOR_OT_remove_prop(Operator):
    bl_idname = "dt.remove_prop"
    bl_label = "remove"
    bl_description = "Remove this property"
    bl_options = {'UNDO'}
    prop: StringProperty(default = 'prop')
    def execute(self, context):
        obj = bpy.context.object
        if obj is None:
            show_popup(message='No active object found',title='Error',icon='ERROR')
            return {'CANCELLED'}
        prop=self.prop
        del obj.dagorprops[prop]
        return {'FINISHED'}

class DAGOR_OT_add_prop(Operator):
    bl_idname = "dt.add_prop"
    bl_label = "add"
    bl_description = "Add property"
    bl_options = {'UNDO'}
    def execute(self, context):
        obj = bpy.context.object
        if obj is None:
            show_popup(message='No active object found',title='Error',icon='ERROR')
            return {'CANCELLED'}
        pref = get_preferences()
        if pref.prop_name.replace(' ','')=='':
            show_popup(message='Enter prop name',title='Error',icon='ERROR')
            return {'CANCELLED'}
        elif pref.prop_name.find(':')==-1:
            show_popup(message='Specify prop type',title='Error',icon='ERROR')
            return {'CANCELLED'}
        if pref.prop_value.replace(' ','')=='':
            show_popup(message='Enter prop value',title='Error',icon='ERROR')
            return {'CANCELLED'}
        obj.dagorprops[pref.prop_name]=fix_type(pref.prop_value)
        return {'FINISHED'}

class DAGOR_OT_transfer_props(Operator):
    bl_idname = "dt.properties_transfer"
    bl_label = "Transfer Dag Properties"
    bl_description = "Apply attributes from active object to each selected"
    def execute(self, context):
        obj = bpy.context.object
        if obj is None:
            show_popup(message='No active object',title='Error',icon='ERROR')
            return {'CANCELLED'}
        props_to_text(obj)
        for obj in context.selected_objects:
            text_to_props(obj)
        show_popup(message='Finished',icon='INFO')
        return {'FINISHED'}

#Empty PropertyGroup to store optional props in
class dagorprops(PropertyGroup):
    pass

class DAGOR_PT_Properties(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Object Properties"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}

    def draw(self, context):
        C=context
        l = self.layout
        l = l.column(align = True)
        if C.object is None:
            info = l.box()
            info.label(text = "No active object!", icon = 'ERROR')
            return
        pref = get_preferences()
        DP = bpy.context.active_object.dagorprops
        props=l.box()
        draw_custom_header(props, 'Properties', pref, 'props_maximized', icon = 'THREE_DOTS')
        if pref.props_maximized:
            add = props.box()
            add.operator('dt.add_prop',text='',icon='ADD')
            new_p = add.row(align = True)
            new_p.prop(pref, 'prop_name',text='')
            new_p.label(text='',icon='FORWARD')
            new_p.prop(pref, 'prop_value',text='')
            props_column = props.column(align = True)
            for key in DP.keys():
                prop=props_column.box()
                prop = prop.column(align = True)
                rem=prop.row()
                rem.label(text=key)
                if DP[key] in['yes','no']:
                    rem.operator('dt.invert_dagbool',text='',
                        icon='CHECKBOX_HLT' if DP[key]=='yes' else 'CHECKBOX_DEHLT').prop=key
                else:
                    try:
                        len=(key+str(DP[key].to_list())).__len__()
                    except:
                        len=(key+str(DP[key])).__len__()
                    if len>14:
                        value=prop.row()
                        value.prop(C.object.dagorprops,'["'+key+'"]',text='')
                    else:
                        rem.prop(C.object.dagorprops,'["'+key+'"]',text='')
                rem.operator('dt.remove_prop',text='',icon='TRASH').prop=key

        presets = l.box()
        draw_custom_header(presets, 'Presets', pref, 'props_preset_maximized', icon = 'THREE_DOTS')

        if pref.props_preset_maximized:

            path = pref.props_presets_path
            path_exists = exists(path)
            if path_exists:

                save = presets.column(align = True)
                save.operator('dt.save_op_preset',text = "Save preset as:")
                save.prop(pref, 'prop_preset_name', text = '')

                apply = presets.column(align = True)
                apply.operator('dt.apply_op_preset', text = 'Apply preset:')
                act = apply.row(align = True)
                act.prop(pref, 'prop_preset', text = "")
                act.operator('dt.remove_op_preset', text = "", icon = "TRASH")
                props_dir = presets.row(align = True)
                if pref.props_path_editing:
                    props_dir.prop(pref, 'props_presets_path', text = "")
                else:
                    props_dir.operator('wm.path_open', icon = 'FILE_FOLDER', text = "open presets folder").filepath = path
                props_dir.prop(pref, 'props_path_editing', text = "", icon = 'SETTINGS', toggle = True)
            else:
                col = presets.column(align = True)
                col.label(icon = 'ERROR')
                col.label(text = 'Presets folder not found, Please, set existing one')
                col.separator()
                col.prop(pref, 'props_presets_path', text = "")

        tools = l.box()
        draw_custom_header(tools, 'Tools', pref, 'props_tools_maximized', icon = 'TOOL_SETTINGS')
        if pref.props_tools_maximized:
            row = tools.row(align = True)
            row.operator('dt.props_to_text', text='Open as text', icon = 'TEXT')
            row.operator('dt.text_to_props', text='Apply from text', icon = 'TEXT')
            tools.operator('dt.properties_transfer', text = 'Transfer Attr')

classes=[dagorprops,
        DAGOR_OT_text_to_props,
        DAGOR_OT_props_to_text,
        DAGOR_OT_transfer_props,
        DAGOR_OT_apply_op_preset,
        DAGOR_OT_save_op_preset,
        DAGOR_OT_remove_op_preset,
        DAGOR_OT_add_prop,
        DAGOR_OT_remove_prop,
        DAGOR_invert_dagbool,
        DAGOR_PT_Properties,
        ]

def register():
    for cl in classes:
        register_class(cl)
    bpy.types.Object.dagorprops = PointerProperty(type=dagorprops)
    bpy.types.Object.uniform_scale = FloatProperty(update = update_uniform_scale)

def unregister():
    for cl in classes:
        unregister_class(cl)
    del bpy.types.Object.dagorprops
    del bpy.types.Object.uniform_scale