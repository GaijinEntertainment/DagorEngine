from os.path    import exists, join, abspath
from time       import time


import bpy
from bpy.props import (BoolProperty,
                        StringProperty,
                        )
from bpy.types import Operator

from .dagormat_functions    import (_dagormat_operator_generic_description,
                                    _proxymat_operator_generic_description,
                                    _dagormat_operator_generic_poll,
                                    _proxymat_operator_generic_poll,
                                    force_clear_materials,
                                    read_proxy_blk,
                                    write_proxy_blk,
                                    get_dagormats,
                                    update_dagormat_parameters,
                                    find_textures,
                                    find_proxymats,
                                    remove_prop_group,
                                    clear_tex_paths,
                                    )
from .dagormat              import (get_props_enum,
                                    update_tex_paths,
                                    )
from .read_shader_config    import read_shader_config
from .build_node_tree       import build_dagormat_node_tree
from ..helpers.texts        import *
from .rw_dagormat_text      import dagormat_from_text, dagormat_to_text
from ..helpers.props        import (
                                    dagormat_prop_add,
                                    try_remove_custom_prop,
                                    )
from ..helpers.getters      import get_preferences

from ..popup.popup_functions    import show_popup

classes = []

# UPDATERS #############################################################################################################

class DAGOR_OT_dagormats_update(Operator):
    bl_idname = "dt.dagormats_update"
    bl_label = "Rebuild"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        return _dagormat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = True)
    @classmethod
    def description(cls, context, properties):
        description = "Update UI and node tree of active dagormat"
        description_all = "Upadte UI and node tree of every dagormat in blend file"
        return _dagormat_operator_generic_description(context,
                                                      description = description,
                                                      description_all = description_all)
    def execute(self,context):
        materials = get_dagormats()
        for material in materials:
            update_dagormat_parameters(material)
            build_dagormat_node_tree(material)
        show_popup(message = 'Done!')
        return{'FINISHED'}
classes.append(DAGOR_OT_dagormats_update)


class DAGOR_OT_dagormats_force_update(Operator):
    bl_idname = "dt.dagormats_force_update"
    bl_label = "FORCE REBUILD"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        return _dagormat_operator_generic_poll(context,
                                               has_single_mode = False,
                                               has_global_mode = True)
    @classmethod
    def description(cls, context, properties):
        description_all = "Rebuild all dagormats with replacement of existing nodegroups"
        return _dagormat_operator_generic_description(context,
                                                      description = "",
                                                      description_all = description_all)
    def execute(self, context):
        materials_to_update = get_dagormats(force_mode = 'ALL')
        force_clear_materials()
        for material in materials_to_update:
            build_dagormat_node_tree(material)
        show_popup(message = 'Done!')
        return{'FINISHED'}
classes.append(DAGOR_OT_dagormats_force_update)


# PROXYMATS ############################################################################################################
class DAGOR_OT_read_proxy(Operator):
    bl_idname = "dt.dagormat_read_proxy"
    bl_label = "read proxymat"
    bl_options = {'UNDO'}
    @classmethod
    def poll(self, context):
        return _proxymat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = False,
                                               check_existence = True)
    @classmethod
    def description(cls, context, properties):
        description = 'Read proxymat parameters from blk file'
        return _proxymat_operator_generic_description(context,
                                                      description = description,
                                                      description_all = "",
                                                      check_existence = True)
    def execute(self,context):
        material = context.object.active_material
        read_proxy_blk(material)
        show_popup(message = 'Done!')
        return{'FINISHED'}
classes.append(DAGOR_OT_read_proxy)


class DAGOR_OT_write_proxy(Operator):
    bl_idname = "dt.dagormat_write_proxy"
    bl_label = "write proxymat"
    bl_options = {'UNDO'}
    @classmethod
    def poll(self, context):
        return _proxymat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = False,
                                               check_existence = False)
    @classmethod
    def description(cls, context, properties):
        description = 'Write proxymat parameters to blk file'
        return _proxymat_operator_generic_description(context,
                                                      description = description,
                                                      description_all = "")
    def execute(self,context):
        material = context.object.active_material
        write_proxy_blk(material)
        show_popup(message = 'Done!')
        return{'FINISHED'}
classes.append(DAGOR_OT_write_proxy)


class DAGOR_OT_FindProxymats(Operator):
    bl_idname = "dt.find_proxymats"
    bl_label = "find proxymats"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        return _proxymat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = True,
                                               check_existence = False)
    @classmethod
    def description(cls, context, properties):
        description = "Find *.proxymat.blk for active material"
        description_all = "Find *.proxymat.blk for every proxymat in blend file"
        return _proxymat_operator_generic_description(context,
                                                      description = description,
                                                      description_all = description_all,
                                                      check_existence = False)
    def execute(self,context):
        materials = get_dagormats()
        find_proxymats(materials)
        show_popup(message='Done!')
        return{'FINISHED'}
classes.append(DAGOR_OT_FindProxymats)


# TEXT CONVERTION ######################################################################################################
class DAGOR_OT_dagormat_to_text(Operator):
    bl_idname = "dt.dagormat_to_text"
    bl_label = "dagormat to text"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        return _dagormat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = False)
    @classmethod
    def description(cls, context, properties):
        description = 'Write dagormat data to "dagormat_temp" text'
        return _dagormat_operator_generic_description(context,
                                                      description = description,
                                                      description_all = "")
    def execute(self,context):
        material = context.object.active_material
        text = get_text_clear('dagormat_temp')
        dagormat_to_text(material,text)
        show_text(text)  # only if it wasn't opened
        show_popup(message = 'Check "dagormat_temp" in the text editor',title = 'Done',icon = 'INFO')
        return{'FINISHED'}
classes.append(DAGOR_OT_dagormat_to_text)


class DAGOR_OT_dagormat_from_text(Operator):
    bl_idname = "dt.dagormat_from_text"
    bl_label = "dagormat from text"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        if bpy.data.texts.get('dagormat_temp') is None:
            return False
        return _dagormat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = False)
    @classmethod
    def description(cls, context, properties):
        description = 'Read dagormat data from "dagormat_temp" text'
        if bpy.data.texts.get('dagormat_temp') is None:
            return f'{description}. DISABLED: "dagormat_temp" text is not found!'
        return _dagormat_operator_generic_description(context,
                                                      description = description,
                                                      description_all = "")
    def execute(self,context):
        material = context.object.active_material
        text = get_text('dagormat_temp')
        dagormat_from_text(material, text)
        dagormat = material.dagormat
        build_dagormat_node_tree(material)
        # making sure that dropdown has valid value for updated list of props
        if dagormat.use_prop_name_enum:
            available_props = get_props_enum(dagormat, context)
            dagormat.prop_name_enum = dagormat.prop_name = available_props[-1][0]
        return {'FINISHED'}
classes.append(DAGOR_OT_dagormat_from_text)


# PARAMETERS ###########################################################################################################
class DAGOR_OT_Dagormat_Prop_Add(Operator):
    bl_idname = "dt.dagormat_prop_add"
    bl_label = "optional dagormat parameter:"
    bl_description = "Add optional parameter"
    bl_options = {'UNDO'}

    def execute(self, context):
        dagormat = context.object.active_material.dagormat
        prop_name = dagormat.prop_name_enum if dagormat.use_prop_name_enum else dagormat.prop_name
        prop_name = prop_name.replace(' ', '')
        if prop_name in ['None', '']:
            show_popup(message = 'Can not create property without name!')
            return {'CANCELLED'}
        prop_value_string = dagormat.prop_value
        dagormat_prop_add(dagormat, prop_name, prop_value_string)
        # making sure that dropdown has valid value for updated list of props
        if dagormat.use_prop_name_enum:
            available_props = get_props_enum(dagormat, context)
            dagormat.prop_name_enum = dagormat.prop_name = available_props[-1][0]
        return {'FINISHED'}
classes.append(DAGOR_OT_Dagormat_Prop_Add)


class DAGOR_OT_Dagormat_Prop_Remove(Operator):
    bl_idname = "dt.dagormat_prop_remove"
    bl_label = ""
    bl_options = {'UNDO'}

    prop_name: StringProperty()
    group_name: StringProperty()

    @classmethod
    def description(cls, context, properties):
        if properties.prop_name != '':
            return f'Remove "{properties.prop_name}" property'
        return f'Remove {properties.group_name} properties'

    def execute(self, context):
        dagormat = context.active_object.active_material.dagormat
        if self.prop_name != "":
            try_remove_custom_prop(dagormat.optional, self.prop_name)
        elif self.group_name != "":
            remove_prop_group(dagormat, self.group_name)
        # making sure that dropdown has valid value for updated list of props
        if dagormat.use_prop_name_enum:
            if dagormat.prop_name_enum not in ['', 'None']:
                dagormat.prop_name_enum = dagormat.prop_name_enum  # triggering update function
            else:
                available_props = get_props_enum(dagormat, context)
                dagormat.prop_name_enum = available_props[-1][0]
        return {'FINISHED'}
classes.append(DAGOR_OT_Dagormat_Prop_Remove)

# TODO: move to separate "Parameters" directory, it's not limited to dagormats
class DAGOR_OT_Set_Value(Operator):
    bl_idname = "dt.set_value"
    bl_label = ""
    bl_options = {'UNDO'}
    prop_value: StringProperty(default = "")
    prop_name:  StringProperty(default = "")
    description:StringProperty(default = "")
    @classmethod
    def description(cls, context, properties):
        if properties.description == "":
            return f"Set value to {properties.prop_value}"
        else:
            return properties.description
    @classmethod
    def poll(self, context):
        if context.prop_owner is None:
            return False
        if context.active_object.active_material is None:
            return False
        return True

    def execute(self, context):
        value = self.prop_value
        type = context.prop_owner.bl_rna.properties[self.prop_name].type
        if type == "BOOLEAN":
            value = False if value.lower() in ['no', 'false', '0'] else True
        elif type == "INT":
            value = int(value)
        elif type == "FLOAT":
            value = float(value)
        setattr(context.prop_owner, self.prop_name, value)
        return {'FINISHED'}
classes.append(DAGOR_OT_Set_Value)

# TEXTURES #############################################################################################################
class DAGOR_OT_FindTextures(Operator):
    bl_idname = "dt.find_textures"
    bl_label = "find textures"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        return _dagormat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = True)
    @classmethod
    def description(cls, context, properties):
        description = "Serch for missing textures of active material in the current project directory"
        description_all = "Serch for missing textures of dagormats in the current project directory"
        return _dagormat_operator_generic_description(context,
                                                      description = description,
                                                      description_all = description_all)
    def execute(self,context):
        materials = get_dagormats()
        find_textures(materials)
        show_popup(message = 'Done!')
        return{'FINISHED'}
classes.append(DAGOR_OT_FindTextures)

class DAGOR_OT_clear_tex_paths(Operator):
    bl_idname = "dt.clear_tex_paths"
    bl_label = "clear textures"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        return _dagormat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = True)
    @classmethod
    def description(cls, context, properties):
        description = "Remove dirpaths and extentions from texture slots of active material, keep only base names"
        description_all = "Remove dirpaths and extentions from texture slots of every dagormat, keep only base names"
        return _dagormat_operator_generic_description(context,
                                                      description = description,
                                                      description_all = description_all)
    def execute(self,context):
        materials = get_dagormats()
        for material in materials:
            clear_tex_paths(material)
        show_popup(message = f'Done!')
        return {'FINISHED'}
classes.append(DAGOR_OT_clear_tex_paths)


class DAGOR_OT_update_tex_paths(Operator):
    bl_idname = "dt.update_tex_paths"
    bl_label = "update texture paths"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        return _dagormat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = True)
    @classmethod
    def description(cls, context, properties):
        description = "Write real filepaths to texture slots of active material"
        description_all = "Write real filepaths to texture slots of every dagormat"
        return _dagormat_operator_generic_description(context,
                                                      description = description,
                                                      description_all = description_all)
    def execute(self,context):
        materials = get_dagormats()
        for material in materials:
            update_tex_paths(material)
        show_popup(message = f'Done!')
        return {'FINISHED'}
classes.append(DAGOR_OT_update_tex_paths)


class DAGOR_OT_refresh_dagormat_ui(Operator):
    bl_idname = "dt.refresh_dagormat_ui"
    bl_label = "Refresh ui"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        return _dagormat_operator_generic_poll(context,
                                               has_single_mode = True,
                                               has_global_mode = False)
    @classmethod
    def description(cls, context, properties):
        description = 'Updates optional parameters of each optional property, if current shader is found in config'
        return _dagormat_operator_generic_description(context,
                                                      description = description)
    def execute(self,context):
        update_dagormat_parameters(context.material)
        show_popup(message = f'Done!')
        return {'FINISHED'}
classes.append(DAGOR_OT_refresh_dagormat_ui)


class DAGOR_OT_refresh_shaders_config(Operator):
    bl_idname = "dt.refresh_shaders_config"
    bl_label = "Reload shader config"
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        return _dagormat_operator_generic_poll(context,
                                               has_single_mode = False,
                                               has_global_mode = True)
    @classmethod
    def description(cls, context, properties):
        description = 'Updates list of shaders from "dagorShaders.blk" without restarting the Blender'
        return _dagormat_operator_generic_description(context,
                                                      description = description)
    def execute(self,context):
        read_shader_config()
        show_popup(message = f'Done!')
        return {'FINISHED'}
classes.append(DAGOR_OT_refresh_shaders_config)



def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    return


def unregister():
    for cl in classes[::-1]:  # backwards to avoid broken dependencies
        bpy.utils.unregister_class(cl)
    return