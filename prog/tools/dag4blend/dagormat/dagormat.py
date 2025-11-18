import bpy, os

from time   import time

from bpy.props import (BoolProperty,
                        IntProperty,
                        FloatProperty,
                        FloatVectorProperty,
                        StringProperty,
                        PointerProperty,
                        EnumProperty,
                        )
from os.path    import exists, join
from bpy.types import  PropertyGroup, Panel

from ..constants            import DAGTEXNUM
from .build_node_tree       import build_dagormat_node_tree
from ..helpers.texts        import *
from ..helpers.names        import basename
from .rw_dagormat_text      import dagormat_from_text, dagormat_to_text
from .dagormat_functions    import cleanup_textures
from ..helpers.props        import (fix_type,
                                    get_default_shader_prop_value,
                                    get_property_type,
                                    prop_value_to_string,
                                    )
from ..helpers.getters      import get_preferences
from ..ui.draw_elements     import draw_custom_header, draw_custom_toggle

from ..popup.popup_functions    import show_popup

classes = []

#functions

def prop_range_warning(value, parameters, actual_type):
    warn = ""
    type = parameters['type']
    if type in ['t', 'b']:
        return warn  # no warnings, text and bool have no soft_min/soft_max
    if actual_type in ['t', 'b']:
        return warn  # will have type mismatch warning instead
    min = parameters.get('soft_min')
    max = parameters.get('soft_max')
    if type in ['i', 'r']:
        value = [value]  # to use same comparing as vector types
    out_of_range = False
    if min is not None:
        for number in value:
            if number < min:
                out_of_range = True
    else:
        min = "-inf"  # for proper display in warning
    if max is not None:
        for number in value:
            if number > max:
                out_of_range = True
    else:
        max = "inf"
    if out_of_range:
        warn = f"Value is out of range! Expected [{min} .. {max}]!"
    return warn

#  shaders  getters  ###################################################################################################
def get_shaders_enum(self,context):
    start = time()
    pref = get_preferences()
    all_shader_names = pref.shaders.keys()
    single_category = self.filter_categories
    if not self.shader_class in all_shader_names:
        single_category = False  # can't find a category for unknown shader, showing all shaders instead
    if single_category:
        shader_names = pref.shader_categories[pref.shaders[self.shader_class]['category']]['shaders']
    else:
        shader_names = all_shader_names
    current_category = "None"
    items = []
    for name in shader_names:
        if current_category != pref.shaders[name]['category']:
            current_category = pref.shaders[name]['category']
            if not single_category:
                description = pref.shader_categories[current_category].get('description')
                if description is None:
                    description = "It's a group, not a shader!"
                items.append((current_category + ":", current_category + ":", description, 'GROUP', -1))
        description = pref.shaders[name].get('description')
        if description is None:
            description = current_category
        items.append((name, name, description))
    if self.shader_class not in shader_names:
        if self.shader_class == "":
            items.append(("None", "None", "Can't resolve empty shader class!", 'ERROR', -2))
        else:
            items.append((self.shader_class, self.shader_class, "Not found in shader config!", 'ERROR', -2))
    return items

def get_shader_categories_enum(self, context):
    items = []
    pref = get_preferences()
    for name in pref.shader_categories.keys():
        description = pref.shader_categories[name].get('description')
        if description is None:
            description = name
        items.append((name, name, description))
    if self.shader_class not in pref.shaders.keys():
        items.append(("None", "None", "None"))
    return items

#shaders    updaters####################################################################################################
def update_use_shader_enum(self, context):
    if self.shader_class_enum == "None" and self.shader_class == "":
        return
    if self.shader_class == self.shader_class_enum and self.shader_class != "":
        return
    if self.use_shader_enum:
        self.shader_class_enum = "None" if self.shader_class == "" else self.shader_class
    else:
        self.shader_class = "" if self.shader_class_enum == "None" else self.shader_class_enum
    build_dagormat_node_tree(self.id_data)
    return

def update_shader_class(self, context):
    build_dagormat_node_tree(self.id_data)
    if self.shader_class == self.shader_class_enum:
        return
    if self.shader_class == "":
        self.shader_class_enum = "None"
    else:
        self.shader_class_enum = self.shader_class
    return

def update_shader_class_enum(self,context):
    if self.shader_class_enum == "None":
        if self.shader_class != "":
            self.shader_class = ""
        if self.shader_category_enum != "None":
            self.shader_category_enum = "None"
        return

    current_shaders = get_shaders_enum(self,context)
    for element in current_shaders:
        if element[0] != self.shader_class_enum:
            continue
        if element.__len__() != 5:
            break  # has no specified icon and index, not a category
        if element[4] == -1:
            show_popup(message = "Can't assign category name as shader!")
            self.shader_class_enum = self.shader_class if self.shader_class != "" else "None"
            return

    if self.shader_class != self.shader_class_enum:
        self.shader_class = self.shader_class_enum

    pref = get_preferences()
    if self.shader_class_enum not in pref.shaders.keys():
        if self.shader_category_enum != "None":
            self.shader_category_enum = "None"
        return

    category_to_set = pref.shaders[self.shader_class_enum]['category']
    if self.shader_category_enum != category_to_set:
        self.shader_category_enum = category_to_set
    return

def update_shader_category_enum(self,context):
    if self.shader_category_enum == "None":
        return
    pref = get_preferences()
    if self.shader_class in pref.shaders.keys():
        previous_category = pref.shaders[self.shader_class]['category']
    else:
        previous_category = "None"
    if previous_category != self.shader_category_enum:
        new_category_shaders = pref.shader_categories[self.shader_category_enum]['shaders']
        self.shader_class = self.shader_class_enum = new_category_shaders[0]
    return

def upd_category_filter(self,context):
    self.shader_class_enum = "None" if self.shader_class == "" else self.shader_class
    return

#props##################################################################################################################

def get_props_enum(self,context):
    pref = get_preferences()
    current_shader_config = pref.shaders.get(self.shader_class)
    if current_shader_config is None:
        prop_names = []
    else:
        prop_names = current_shader_config['props'].keys()
    exclude_prop_names = list(self.optional.keys())
    exclude_prop_names.append('real_two_sided')#already exists in ui
    items = []
    for key in prop_names:
        if key in exclude_prop_names:
            continue
        items.append((key,key,key))
    if self.prop_name not in prop_names and self.prop_name not in exclude_prop_names + ["","None"]:
        items.append((self.prop_name, self.prop_name,"Not found in config!", 'ERROR', -2))
    if items.__len__() == 0:
        items.append(("None", "None", "No props available!", 'ERROR', -1))  # preventing bug on displaying first element of list with zero elements
    return items

def update_prop_mode(self, context):
    if self.prop_name != '' and self.prop_name_enum != 'None':
        if self.prop_name == self.prop_name_enum:
            return
    if self.use_prop_name_enum:  # just switched to dropdown
        available_props = get_props_enum(self, context)
        available_prop_names = [prop[0] for prop in available_props]
        if self.prop_name in available_prop_names:
            self.prop_name_enum = self.prop_name
        else:
            self.prop_name_enum = available_prop_names[0]
    else:  # just switched to text
        self.prop_name = self.prop_name_enum if self.prop_name_enum != "None" else ""
    return

def update_prop_name_enum(self, context):
    if not self.use_prop_name_enum:
        return
    if self.prop_name_enum == self.prop_name:
        return
    self.prop_name = self.prop_name_enum
    update_prop_value(self, context)
    return

def update_prop_name(self, context):
    if self.use_prop_name_enum:
        return
    if self.prop_name_enum == self.prop_name:
        return
    self.prop_name_enum = elf.prop_name
    update_prop_value(self, context)
    return

def update_prop_value(self, context):  # not direct update, called only from other name updaters
    pref = get_preferences()
    current_shader_config = pref.shaders.get(self.shader_class)
    if current_shader_config is None:
        return
    current_prop = current_shader_config['props'].get(self.prop_name)
    if current_prop is None:  # every prop is added already, dropdown is empty`
        current_prop = {}
    default_value = current_prop.get('default')
    current_type = current_prop.get('type')
    if current_prop is None:
        return
    if default_value is not None:
        self.prop_value = prop_value_to_string(default_value, current_type)
        return
    if current_type is not None:
        default_value = get_default_shader_prop_value(self.shader_class, self.prop_name)
        self.prop_value = prop_value_to_string(default_value, current_type)
    return


#other##################################################################################################################
def update_tex_paths(material):
    T = material.dagormat.textures
    for tex in T.keys():
        tex_name=basename(T[tex])#no extention needed
        if tex_name.find("*?") > -1:
            tex_name = tex_name[:tex_name.find('*?')]
        if T[tex]=='':
            continue
        elif exists(T[tex]):
            continue
        img=bpy.data.images.get(tex_name)
        if img is None:
            continue
        if exists(img.filepath):
            T[tex]=img.filepath
    return

def update_backface_culling(self, context):
    #self is dagormat
    material = self.id_data#material, owner of current dagormat PropertyGroup
    material.use_backface_culling = self.sides == 0
    return

def update_material(self,context):
    material = self.id_data
    build_dagormat_node_tree(material)
    return


def update_proxy_path(self,context):
    if context.active_object is None or context.active_object.active_material is None:
        return
    DM=context.object.active_material.dagormat
    if DM.proxy_path=='':
        return
    if DM.proxy_path.startswith('"') or DM.proxy_path.endswith('"'):
        DM.proxy_path=DM.proxy_path.replace('"','')
    basename=os.path.basename(DM.proxy_path)
    if basename!='' and basename.endswith('.proxymat.blk'):
        context.material.name=os.path.basename(DM.proxy_path).replace('.proxymat.blk','')
        DM.proxy_path=os.path.dirname(DM.proxy_path)
    if DM.proxy_path[-1]!=os.sep:
        DM.proxy_path=DM.proxy_path+os.sep
    return

#properties
class dagor_optional(PropertyGroup):
    pass
classes.append(dagor_optional)


class dagor_textures(PropertyGroup):
    tex0:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex1:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex2:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex3:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex4:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex5:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex6:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex7:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex8:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex9:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex10: StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex11: StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex12: StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex13: StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex14: StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex15: StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
classes.append(dagor_textures)


class dagormat(PropertyGroup):
#legacy properties
    ambient:        FloatVectorProperty(default=(0.5,0.5,0.5),max=1.0,min=0.0,subtype='COLOR')
    specular:       FloatVectorProperty(default=(0.0,0.0,0.0),max=1.0,min=0.0,subtype='COLOR')
    diffuse:        FloatVectorProperty(default=(1.0,1.0,1.0),max=1.0,min=0.0,subtype='COLOR')
    emissive:       FloatVectorProperty(default=(0.0,0.0,0.0),max=1.0,min=0.0,subtype='COLOR')
    power:          FloatProperty(default=0.0)

# 0 - single_sided, 1 - two_sided, 2 - real_two_sided
    sides:          IntProperty(default = 0, min = 0, max = 2, update = update_backface_culling)
#shader_class related stuff
    shader_class:           StringProperty(default = '', update = update_shader_class)
    filter_categories:      BoolProperty(default = False, name = "Filter by category", description = "Show all shaders or just current category?",
        update = upd_category_filter)
    use_shader_enum:        BoolProperty(default = False, description = 'use dropdowns for shaders and categories',
                                            update = update_use_shader_enum)
    shader_category_enum:   EnumProperty(items = get_shader_categories_enum, update = update_shader_category_enum)
    shader_class_enum:      EnumProperty(items = get_shaders_enum, update = update_shader_class_enum)
#property groups
    textures:   PointerProperty(type=dagor_textures)
    optional:   PointerProperty(type=dagor_optional)
#temporary props for UI
    prop_name:          StringProperty (default='atest', update = update_prop_name, name = "Name")
    prop_value:         StringProperty (default='127', name = "Value")
    prop_name_enum:        EnumProperty   (items = get_props_enum, update = update_prop_name_enum)
    use_prop_name_enum:  BoolProperty(default = False, description = 'show as dropdown list',
        update = update_prop_mode)
#proxymat related properties
    is_proxy:   BoolProperty   (name = "Is proxymat", default=False, description='is it a proxymat?')
    proxy_path: StringProperty (default='', subtype = 'FILE_PATH', description='/<Material.name>.proxymat.blk',
                                update=update_proxy_path)
classes.append(dagormat)


#panel
class DAGOR_PT_dagormat(Panel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "material"
    bl_label = "dagormat"
    bl_options = {'DEFAULT_CLOSED'}

    @classmethod
    def poll(self, context):
        if context.active_object.active_material is None:
            return False
        if context.active_object.active_material.is_grease_pencil:
            return False
        return True

    def draw_sides_switcher(self,context, layout):
        DM = context.object.active_material.dagormat
        column = layout.column(align = True)
        row = column.row()
        row.label(text = "Backfacing mode:")
        row = column.row(align = True)
        row.context_pointer_set(name = "prop_owner", data = DM)#called in operator

        button = row.operator('dt.set_value', text = "1sided", depress = DM.sides == 0)
        button.prop_value = "0"
        button.prop_name = 'sides'
        button.description = 'Transparent backfaces'

        button = row.operator('dt.set_value', text = "2sided", depress = DM.sides == 1)
        button.prop_name = "sides"
        button.prop_value = "1"
        button.description = 'Backfaces are displayed via shader'

        button = row.operator('dt.set_value', text = "real2sided", depress = DM.sides == 2)
        button.prop_name = 'sides'
        button.prop_value = "2"
        button.description = 'Backfaces made by copying and flipping all the triangles with this shader (Dagor only)'
        return

    def draw_legacy_properties(self, context, layout):
        pref = get_preferences()
        if pref.hide_dagormat_legacy:
            return
        DM = context.object.active_material.dagormat
        legacy_props = layout.box()
        draw_custom_header(legacy_props, "Legacy properties", pref, 'legacy_maximized', icon = 'GHOST_DISABLED')
        if not pref.legacy_maximized:
            return
        colors=legacy_props.row()
        colors.prop(DM, 'ambient', text='')
        colors.prop(DM, 'specular', text='')
        colors.prop(DM, 'diffuse', text='')
        colors.prop(DM, 'emissive', text='')
        legacy_props.prop   (DM, 'power')
        return

    def draw_shader_selector(self, context, layout):
        pref = get_preferences()
        DM = context.object.active_material.dagormat
        if DM.use_shader_enum:
            row = layout.row()
            row.prop(DM, 'shader_class_enum',text = 'shader')
            buttons = row.row(align = True)
            buttons.prop(DM, 'filter_categories', text = '', icon = 'FILTER', emboss = True)
            buttons.prop(DM, 'use_shader_enum', text = "", icon = 'MENU_PANEL')

            if DM.filter_categories:
                row = layout.row()
                row.prop(DM,'shader_category_enum', text='category')
                offset_row = row.row(align = True)
                offset_row.label(icon = 'BLANK1')  # for alignment, placed under buttuns
                offset_row.label(icon = 'BLANK1')
        else:
            row = layout.row()
            row.prop(DM, 'shader_class', text = 'shader')
            row.prop(DM, 'use_shader_enum', text = "", icon='MENU_PANEL')
        return

    def draw_main(self, context, layout):
        DM = context.object.active_material.dagormat
        pref = get_preferences()
        box = layout.box()
        header = draw_custom_header(box, "Main", pref, 'dagormat_main_maximized', icon = 'MATERIAL', label_offset = 1)
        button_layout = header.row()
        button_layout.operator('dt.refresh_shaders_config', text = "", icon = 'FILE_REFRESH')
        if not pref.dagormat_main_maximized:
            return
        content = box.column()
        content.enabled = not DM.is_proxy
        self.draw_legacy_properties(context, content)
        self.draw_sides_switcher(context, content.box())
        self.draw_shader_selector(context, content.box())
        return

    def draw_textures(self, context, layout):
        DM = context.object.active_material.dagormat
        pref = get_preferences()
        textures=layout.box()
        draw_custom_header(textures, "Textures", pref, 'tex_maximized', icon = 'IMAGE')
        if not pref.tex_maximized:
            return
        content = textures.column(align = True)
        content.enabled = not DM.is_proxy
        T = DM.textures
        table = content.split(factor = 0.06, align = True)
        ids = table.column()
        paths = table.column()
        for i in range(DAGTEXNUM):
            ids.label(text = f'{i}')
            paths.prop(T, f'tex{i}', text='')
        return

    def draw_dagormat_prop(self, context, layout, owner, name, expected_type = None, warning = ""):
        actual_type = get_property_type(owner, name)
        row = layout.row()  # adds round corners instead of gluing boxes together
        box = row.box()
    #checking if label can fit in the same row as value
        ui_scale = context.preferences.view.ui_scale
        region_width = context.region.width
        approx_char_width = 8 * ui_scale
        approx_max_name_len = int(((region_width - 50 * ui_scale)/3)/approx_char_width)
    #name on a separate row
        if name.__len__() > approx_max_name_len or actual_type == 'm':
            param_row = box.row()
            name_row = param_row.row()
            buttons_row = param_row.row()
            name_row.label(text = name)
            value_row = box.row()
    # name on a row with the value
        else:
            split = box.split(factor = 0.3)
            split.label(text = name)
            param_row = split.row()
            value_row = param_row.row()
            buttons_row = param_row.row()
    # buttons
        type_mismatch = expected_type != actual_type and expected_type is not None
        show_warning = warning != "" or type_mismatch
        if show_warning:
            message = ""
            if type_mismatch:
                message += f'Type mismatch! Expected "{expected_type}", got "{actual_type}"'
            if warning != "":
                if type_mismatch:
                    message += "; "
                message += warning
            error_button = buttons_row.operator('dt.show_popup', icon = 'ERROR', emboss = False)
            error_button.message = message
            error_button.title = 'WARNING:'
            error_button.icon = 'ERROR'
        remove = buttons_row.operator('dt.dagormat_prop_remove',text='',icon='TRASH')
        remove.prop_name = name
        remove.group_name = ""
    # matrix
        if actual_type == 'm':
            column = value_row.column(align = True)
            row = column.row()
            row_a = row.row(align = True)
            row_a.prop(owner, f'["{name}"]', index = 0, text = "")
            row_a.prop(owner, f'["{name}"]', index = 1, text = "")
            row_a.prop(owner, f'["{name}"]', index = 2, text = "")
            row = column.row()
            row_b = row.row(align = True)
            row_b.prop(owner, f'["{name}"]', index = 3, text = "")
            row_b.prop(owner, f'["{name}"]', index = 4, text = "")
            row_b.prop(owner, f'["{name}"]', index = 5, text = "")
            row = column.row()
            row_c = row.row(align = True)
            row_c.prop(owner, f'["{name}"]', index = 6, text = "")
            row_c.prop(owner, f'["{name}"]', index = 7, text = "")
            row_c.prop(owner, f'["{name}"]', index = 8, text = "")
            row = column.row()
            row_d = row.row(align = True)
            row_d.prop(owner, f'["{name}"]', index = 9, text = "")
            row_d.prop(owner, f'["{name}"]', index = 10, text = "")
            row_d.prop(owner, f'["{name}"]', index = 11, text = "")
            return
    #bool
        if actual_type == 'b':
            value_row.prop(owner, f'["{name}"]', toggle = True,
                text = 'yes' if owner[name] else 'no',
                icon = 'CHECKBOX_HLT' if owner[name] else 'CHECKBOX_DEHLT')
            return
        else:
    #color
            ui_props = owner.id_properties_ui(name).as_dict()
            if ui_props.get('subtype') == 'COLOR':
                color_column = value_row.column(align = True)
                numbers = color_column.row(align = True)
                numbers.prop(owner, f'["{name}"]', text = "", index = 0)
                numbers.prop(owner, f'["{name}"]', text = "", index = 1)
                numbers.prop(owner, f'["{name}"]', text = "", index = 2)
                numbers.prop(owner, f'["{name}"]', text = "", index = 3)
                color_column.prop(owner, f'["{name}"]', text = "")
    #other
            else:
                value_row.prop(owner, f'["{name}"]', text = "")
        return

    def draw_optional(self, context, layout):
        DM = context.object.active_material.dagormat
        pref = get_preferences()
        box = layout.box()
        header = draw_custom_header(box, "Optional", pref, 'optional_maximized', icon = 'NONE', label_offset = 2)
        button_layout = header.row(align = True)
        button_layout.operator('dt.refresh_dagormat_ui', icon = 'FILE_REFRESH', text = "")
        remove = button_layout.operator('dt.dagormat_prop_remove', icon = 'TRASH')
        remove.prop_name = ""
        remove.group_name = "all"

        if not pref.optional_maximized:
            return
        content = box.column()
        content.enabled = not DM.is_proxy

        addbox = content.box()
        addbox.operator("dt.dagormat_prop_add", text = "ADD", icon = "ADD")

        prew = addbox.row(align = True)
        if DM.use_prop_name_enum:
            prew.prop(DM,'prop_name_enum', text = '')
        else:
            prew.prop(DM,'prop_name', text = '')
        prew.label(text = '', icon = 'FORWARD')
        prew.prop(DM, 'prop_value', text = '')
        prew.prop(DM, 'use_prop_name_enum', icon = 'MENU_PANEL', text = '')
        props_box = content.column(align = True)
        current_shader_config = pref.shaders.get(DM.shader_class)
        if current_shader_config is not None:
            known_props = current_shader_config['props']
            known_props_names = known_props.keys()
        else:
            known_props_names = []
        props_to_draw = list(DM.optional.keys())
        current_group = None
        current_layout = props_box
        uncategorized_layout = None
        for name in known_props_names:
            if not name in props_to_draw:
                continue
            props_to_draw.remove(name)
            new_group = known_props[name].get('property_group')
            if new_group is not None:
                if new_group != current_group:
                    props_box.separator()
                    current_group = new_group
                    current_layout = props_box.box()
                    current_header = current_layout.row()
                    category_description = current_shader_config['prop_groups'][current_group].get('description')
                    if category_description is None:
                        current_header.label(text = current_group, icon = 'GROUP')
                    else:
                        description_button = current_header.operator('dt.show_popup', icon = 'INFO', emboss = False)
                        description_button.message = category_description
                        description_button.title = current_group + ":"
                        description_button.icon = 'INFO'
                        current_header.label(text = current_group)
                    remove = current_header.operator('dt.dagormat_prop_remove', icon = 'TRASH')
                    remove.prop_name = ""
                    remove.group_name = current_group
                    current_layout = current_layout.column(align = True)
            else:
                if uncategorized_layout is None:
                    uncategorized_layout = props_box.box()
                    uncategorized_header = uncategorized_layout.row()
                    uncategorized_header.label(icon = 'GROUP', text = "Uncategorized")
                    remove = uncategorized_header.operator('dt.dagormat_prop_remove', icon = 'TRASH')
                    remove.prop_name = ""
                    remove.group_name = "Uncategorized"
                    uncategorized_layout = uncategorized_layout.column(align = True)
                current_layout = uncategorized_layout

            actual_type = get_property_type(DM.optional, name)
            warn = prop_range_warning(DM.optional[name], known_props[name], actual_type)

            self.draw_dagormat_prop(context, current_layout, DM.optional, name,
                                    expected_type = known_props[name]['type'], warning = warn)
        if props_to_draw.__len__() > 0:
            props_to_draw.sort()  # easier to manage in alphabetic order
            props_box.separator()
            current_layout = props_box.box()
            header_row = current_layout.row()
            error_button = header_row.operator('dt.show_popup', icon = 'ERROR', emboss = False)
            error_button.message = f'These parameters are not found in the "{DM.shader_class}" config'
            error_button.icon = 'ERROR'
            error_button.title = "WARNING:"
            header_row.label(text = "Unknown")
            remove = header_row.operator('dt.dagormat_prop_remove', icon = 'TRASH')
            remove.prop_name = ""
            remove.group_name = "Unknown"
            props_column = current_layout.column(align = True)
            for name in props_to_draw:
                self.draw_dagormat_prop(context, props_column, DM.optional, name)
        return

    def draw_proxymat(self, context, layout):
        active_material = context.object.active_material
        DM = active_material.dagormat
        pref = get_preferences()
        box = layout.box()
        draw_custom_header(box, "Proxy", pref, 'proxy_maximized', icon = 'FILE')
        if not pref.proxy_maximized:
            return
        draw_custom_toggle(box, DM, 'is_proxy')
        column = box.column()
        column.label(text='Proxymat folder:')
        column.prop(DM,'proxy_path',text='')
        column.operator('dt.dagormat_read_proxy', text = '(Re)load from file',icon = 'RECOVER_LAST')
        column.operator('dt.dagormat_write_proxy', text = 'Save proxymat',icon = 'FILE_TICK')
        column.enabled = DM.is_proxy
        return

    def draw_tools(self, context, layout):
        active_material = context.object.active_material
        DM = active_material.dagormat
        pref = get_preferences()
        tools = layout.box()
        draw_custom_header(tools, "Tools", pref, 'tools_maximized', icon = 'TOOL_SETTINGS')
        if not pref.tools_maximized:
            return
        text=tools.box()
        text.label(icon = 'TEXT', text = "Text editing")
#Text Editing
        text_ed=text.row()
        text_ed.operator('dt.dagormat_to_text',text='Open as text', icon = 'TEXT')
        text_ed.operator('dt.dagormat_from_text',text='Apply from text', icon = 'TEXT')
#Search
        shared=tools.box()
        mode=shared.row(align = True)
        mode.context_pointer_set(name = "prop_owner", data = pref)

        single = mode.operator('dt.set_value', text = 'Current material', depress = not pref.process_all_materials)
        single.prop_value = ('0')
        single.prop_name = 'process_all_materials'
        single.description = 'Process only active material'

        all = mode.operator('dt.set_value', text = "All materials", depress = pref.process_all_materials)
        all.prop_value = '1'
        all.prop_name = 'process_all_materials'
        all.description = "Process every dagormat in the blend file"

        shared.label(text='Search', icon='VIEWZOOM')
        search = shared.column(align = True)

        row = search.row()
        find_textures = row.operator('dt.find_textures', text = 'Find missing textures',icon = 'TEXTURE')
        if pref.process_all_materials or DM.is_proxy:
            row = search.row()
            row.operator('dt.find_proxymats',
                         text = 'Find missing proxymats' if pref.process_all_materials else 'Find missing proxymat',
                         icon = 'MATERIAL')
#Operators
        shared.label(text='Process', icon = 'TOOL_SETTINGS')
        process = shared.column(align = True)
        row = process.row()
        row.operator('dt.dagormats_update', icon = 'MATERIAL')
        if pref.process_all_materials:
            row = process.row()
            row.operator('dt.dagormats_force_update', icon = 'MATERIAL')
        row = process.row()
        row.operator('dt.update_tex_paths', text = 'Update texture paths', icon = 'TEXTURE')
        row = process.row()
        row.operator('dt.clear_tex_paths', text = 'Clear texture paths',  icon = 'TEXTURE')

        return

    def draw(self,context):
        layout = self.layout
        self.draw_proxymat(context, layout)
        self.draw_main(context, layout)
        self.draw_textures(context, layout)
        self.draw_optional(context, layout)
        self.draw_tools(context, layout)
        return
classes.append(DAGOR_PT_dagormat)


def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    bpy.types.Material.dagormat = PointerProperty(type = dagormat)
    return


def unregister():
    for cl in classes[::-1]:  # backwards to avoid broken dependencies
        bpy.utils.unregister_class(cl)
    del bpy.types.Material.dagormat
    return
