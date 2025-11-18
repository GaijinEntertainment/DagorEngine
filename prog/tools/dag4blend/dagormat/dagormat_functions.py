import bpy

from os.path import join, exists, abspath
from os     import walk

from ..helpers.names    import texture_basename, asset_basename
from .rw_dagormat_text  import (dagormat_from_text,
                                dagormat_to_text,
                                dagormat_prop_to_string,
                                dagormat_prop_from_string,)
from .build_node_tree    import build_dagormat_node_tree
from ..helpers.texts    import get_text_clear
from ..helpers.version  import get_blender_version
from ..helpers.getters  import get_preferences

supported_extensions = ['.dds','.tga','.tif']

# MISC #################################################################################################################

def clear_tex_paths(material):
    T = material.dagormat.textures
    for tex in T.keys():
        if T[tex]!='':
            T[tex] = asset_basename(T[tex])
    return

def remove_prop_group(dagormat, group_name):
    props = dagormat.optional
    existing_props = list(props.keys())
    if group_name == 'all':
        for name in existing_props:
            del props[name]
        return
    props_to_remove = []
    shader_class = dagormat.shader_class
    prefs = get_preferences()
    config = prefs.shaders[shader_class]
    if config:
        known_props = config['props']
    else:
        known_props = {}
    if group_name == 'Unknown':
        for name in existing_props:
            if name not in known_props.keys():
                del props[name]
        return
    elif group_name == 'Uncategorized':
        for name in known_props.keys():
            if known_props[name].get('property_group') is None:
                if name in existing_props:
                    del props[name]
    else:
        for name in known_props.keys():
            if known_props[name].get('property_group') == group_name:
                if name in existing_props:
                    del props[name]
    return

#functions/search
def get_missing_tex(materials):
    missing_tex=[]
    for material in materials:
        textures = material.dagormat.textures
        for key in textures.keys():
            if textures[key].replace(' ', '') == '':
                continue
            tex_name = texture_basename(textures[key])
            if tex_name not in missing_tex:
                if not exists(bpy.data.images[tex_name].filepath):
                    if tex_name not in missing_tex:
                        missing_tex.append(tex_name)
    return missing_tex

def find_textures(materials):
    cleanup_textures()
    missing_tex = get_missing_tex(materials)
    if missing_tex.__len__()==0:
        print('nothing to search')
        return
    pref = get_preferences()
    i=int(pref.project_active)
    search_path = pref.projects[i].path

    for subdir,dirs,files in walk(search_path):
        for file in files:
            for name in missing_tex:
                for extension in supported_extensions:
                    if file.lower()==(name+extension).lower():
                        print ('Found: '+name)
                        bpy.data.images[name].source = 'FILE'
                        bpy.data.images[name].filepath = subdir + '/' + file
    return

def find_proxymats(materials):
    pref = get_preferences()
    i=int(pref.project_active)
    search_path = pref.projects[i].path
    proxymats=[]
    for material in materials:
        DM = material.dagormat
        if DM.is_proxy:
            if not exists(join(DM.proxy_path,(material.name+'.proxymat.blk'))):
                if material.name not in proxymats:
                    proxymats.append(material.name)
    if proxymats.__len__()>0:
        for subdir,dirs,files in walk(search_path):
            for file in files:
                for proxy in proxymats:
                    if file.endswith('.proxymat.blk') and asset_basename(file)==asset_basename(proxy):
                        bpy.data.materials[proxy].dagormat.proxy_path=subdir+'/'
                        read_proxy_blk(bpy.data.materials[proxy])
    return

def get_nodegroups_names():
    nodegroup_names = ["default", "default_uv"]
    preferences = get_preferences()
    shader_names = preferences.shaders.keys()
    for shader_name in shader_names:
        nodegroup_names.append(shader_name)
        nodegroup_names.append(shader_name + "_uv")
    return nodegroup_names

def loop_clear_shader_nodegroups(shader_nodegroups, preserved_groups):
    was_updated = False
    version = get_blender_version()
    for gr in list(shader_nodegroups):
        if gr is None:
            continue
        if gr in preserved_groups:
            continue

        if version >= 4.2:  # only Blender 4.2+ has "use_extra_user" property
            if gr.users == 2 and gr.use_fake_user and gr.use_extra_user:
                gr.use_fake_user = False
                gr.use_extra_user = False
            if gr.users == 1 and gr.use_extra_user:
                gr.use_extra_user = False

        if gr.users == 1 and gr.use_fake_user:
            gr.use_fake_user = False
        if gr.users == 0:
            print("removed", gr.name)
            shader_nodegroups.remove(gr)
            bpy.data.node_groups.remove(gr)
            was_updated = True
    if was_updated:
        loop_clear_shader_nodegroups(shader_nodegroups, preserved_groups)
    return

def force_clear_materials():
    nodegroup_names = get_nodegroups_names()

    shader_nodegroups = [gr for gr in bpy.data.node_groups if gr.type == 'SHADER']
    shader_nodegroups = list(shader_nodegroups)

    preserved_groups = [group for group in shader_nodegroups  # this could be preserved by user for a reason
        if group.use_fake_user and group.users == 1 and group.name not in nodegroup_names]

    groups_to_replace = [group for group in shader_nodegroups if group.name in nodegroup_names]

    for group in groups_to_replace:
        group.name = group.name + '_old'
#removing old nodes
    for material in bpy.data.materials:
        if material.is_grease_pencil:
            continue
        if not material.use_nodes:
            continue
        if material.dagormat.shader_class == "":
            continue  # update was disabled manually
        nodes = material.node_tree.nodes
        for node in nodes:
            if node.name.startswith("tex"):
                continue
            nodes.remove(node)
#removing stuff, used only by old nodes
    loop_clear_shader_nodegroups(shader_nodegroups, preserved_groups)
    return

def get_dagormats(force_mode = None):
    if force_mode == 'ALL':
        all_materials = True
    elif force_mode == 'ACTIVE':
        all_materials = False
    else:
        pref = get_preferences()
        all_materials = pref.process_all_materials
    if not all_materials:
        active_object = bpy.context.object
        if active_object is None:
            return []
        active_material = active_object.active_material
        if active_material is None:
            return []
        return [active_material]
    valid_materials = []
    for material in bpy.data.materials:
        if material.is_grease_pencil:
            continue
        if material.dagormat.shader_class in ['', 'None']:
            continue
        valid_materials.append(material)
    return valid_materials

def read_proxy_blk(material):
    proxymat = get_text_clear('dagormat_temp')
    DM = material.dagormat
    file = open(join(DM.proxy_path,material.name+'.proxymat.blk'),'r')
    lines=file.readlines()
    for line in lines:
        proxymat.write(line.replace(' ',''))
    file.close()
    dagormat_from_text(material, proxymat)
    build_dagormat_node_tree(material)
    return

def write_proxy_blk(mat, custom_dirpath = ""):
    text = get_text_clear('dagormat_temp')
    dagormat_to_text(mat, text)
    DM = mat.dagormat
    dirpath = DM.proxy_path if custom_dirpath == "" else custom_dirpath
    filename = mat.name + ".proxymat.blk"
    if DM.is_proxy:
        file = open(join(dirpath, filename),'w')
        for line in text.lines:
            if line.body.__len__() > 0:
                file.write(line.body+'\n')
        file.close()
    return


#removes duplicates with the same filepath; cleanes up the .001+ indices
def cleanup_textures():
    images = bpy.data.images
    for image in list(images):
        name = image.name
        if name[-4] == "." and name[-3:].isnumeric():
            original_image = images.get(name[:-4])
            if original_image is None:
                image.name = image.name[:-4]
            else:
                image.user_remap(original_image)
                bpy.data.images.remove(image)
    return

# OPERATOR HELPERS #####################################################################################################

# all thenecessary checks to avoid copypaste
def _dagormat_operator_generic_description(context, description = "", description_all = ""):
    if description_all != "":
        pref = get_preferences()
        if pref.process_all_materials:
            return description_all
        if description == "":
            return description_all  # No description for single-material mode, must be global only
    #definitely active material mode ->
    if context.object is None:
        return f'{description}. DISABLED: No object to process!'
    material = context.object.active_material
    if material is None:
        return f'{description}. DISABLED: No active material to process!'
    if material.is_grease_pencil:
        return f'{description}. DISABLED: Grease Pencil materials are not supported!'
    return description

def _dagormat_operator_generic_poll(context, has_single_mode = True, has_global_mode = False):
    if has_global_mode:
        if not has_single_mode:  # always global mode, does not depends on active dagormat
            return True  # TODO: add check if blend file has at least one dagormat to process
        pref = get_preferences()
        if pref.process_all_materials:
            return True
    # definitely active material mode ->
    if context.object is None:
        return False
    material = context.object.active_material
    if material is None:
        return False
    if material.is_grease_pencil:
        return False
    return True

# same but for proxymat
def _proxymat_operator_generic_description(context, description = "", description_all = "", check_existence = False):
    generic_description = _dagormat_operator_generic_description(context,
                                                                description = description,
                                                                description_all = description_all)
    if generic_description.find('DISABLED: ') > -1:  # disabled already, no beed for extra checks
        return generic_description
    if generic_description == description_all:  # global processing does not need checks of active material
        return generic_description
    # definitely in single-material mode, and dagormat exists:
    material = context.object.active_material
    if not material.dagormat.is_proxy:
        return f'{description}. DISABLED: Current material is not a proxymat!'
    if check_existence:
        if not _active_proxymat_blk_exists(context):
            proxy_filepath = get_proxy_filepath(material)
            return f'{description}. DISABLED: file "{proxy_filepath}" is not found!'
    return description

def _proxymat_operator_generic_poll(context, has_single_mode = True, has_global_mode = False, check_existence = False):
    if has_single_mode == False:
        return True
    generic_poll = _dagormat_operator_generic_poll( context,
                                                    has_single_mode = has_single_mode,
                                                    has_global_mode = has_global_mode)
    if generic_poll is False:
        return False
    if has_global_mode:
        pref = get_preferences()
        if pref.process_all_materials:
            return True
    active_material = context.object.active_material
    if not active_material.dagormat.is_proxy:
        return False
    if check_existence:
        if not _active_proxymat_blk_exists(context):
            return False
    return True

def get_proxy_filepath(material):
    proxy_dirpath = abspath(material.dagormat.proxy_path)
    if material.name.find('.') > -1:
        proxy_name = material.name[:material.name.find('.')]
    else:
        proxy_name = material.name
    proxy_name = proxy_name + ".proxymat.blk"
    return join(proxy_dirpath, proxy_name)

# checks filepath of active proxymaterial
def _active_proxymat_blk_exists(context):
    material = context.object.active_material
    proxy_filepath = get_proxy_filepath(material)
    return exists(proxy_filepath)

# UPDATERS #############################################################################################################

#convertion to string and back fixes the type when possible, and sets limits/description/etc. from shaderConfig
def update_dagormat_parameters(material):
    dagormat = material.dagormat
    prop_names = list(dagormat.optional.keys())
    for prop_name in prop_names:
        prop_string = dagormat_prop_to_string(dagormat.optional, prop_name)
        dagormat_prop_from_string(dagormat, prop_string)
    return