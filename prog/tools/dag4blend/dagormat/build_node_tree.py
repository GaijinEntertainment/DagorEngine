from os.path    import exists

import bpy
from ..helpers.getters  import get_addon_directory
from ..helpers.texts    import get_text
from ..helpers.names    import texture_basename
from ..settings         import upd_project
from ..helpers.version  import get_blender_version


def check_remap(name):
    addon_dir = get_addon_directory()
    remap_file = addon_dir + '/extras/remap.txt'
    with open(remap_file,'r') as t:
        lines=t.readlines()
        t.close()
    for l in lines:
        if l.find(':')!=-1:
            pair = l.split(':')
            if pair[0] == name:
                name = pair[1].replace('\n','')
    return name

# Clear all nodes in a mat
def _clear_material(material):
    if material.node_tree:
        material.node_tree.links.clear()
        material.node_tree.nodes.clear()

def _rgb_to_4array(c):
    return [c.r, c.g, c.b, 1.0]

def get_image(tex):
    noncolor = ['_n','_m']
    tex_name = texture_basename(tex)
    #texture should exist no matter what
    if bpy.data.images.get(tex_name) is None:
        # easy to see textures with broken paths, still possible to check UVs.
        bpy.data.images.new(name=tex_name, width=128, height=128)
        # path shouldn't be lost, even if texture doesn't exist
        bpy.data.images[tex_name].filepath = tex
    if exists(bpy.data.images[tex_name].filepath):#lost textures should be reversed from purple to generated
        bpy.data.images[tex_name].source = 'FILE'
    elif exists(tex):
        bpy.data.images[tex_name].filepath = tex
        bpy.data.images[tex_name].source = 'FILE'
    else:
        bpy.data.images[tex_name].generated_color = (0.5, 0.5, 0, 0.5)#neutral normal ,non-metallic, medium glossiness
        bpy.data.images[tex_name].source = 'GENERATED'
        if not tex_name.endswith('_n'):
            bpy.data.images[tex_name].generated_type = 'COLOR_GRID'
        else:
            bpy.data.images[tex_name].generated_type = 'BLANK'
    for suffix in noncolor:
        if tex_name.endswith(suffix):
            bpy.data.images[tex_name].colorspace_settings.name = 'Non-Color'
    bpy.data.images[tex_name].alpha_mode = 'CHANNEL_PACKED'
    return bpy.data.images[tex_name]

def get_node_group(group_name):
    node_group = bpy.data.node_groups.get(group_name)
    if node_group is not None:
        return node_group
    start_mode = bpy.context.mode
    if start_mode == 'EDIT_MESH':
        bpy.ops.object.editmode_toggle()
    group_name = check_remap(group_name)
    addon_dir = get_addon_directory()
    lib_path = addon_dir + f'/extras/library.blend/NodeTree'
    file = lib_path+f"/{group_name}"
    bpy.ops.wm.append(filepath = file, directory = lib_path,filename = group_name, do_reuse_local_id = True)
    #if nodegroup not found in library it still be a None
    node_group = bpy.data.node_groups.get(group_name)
    if node_group is None:
        if group_name.endswith('_uv'):
            node_group = bpy.data.node_groups.get('default_uv')
            if node_group is None:
                bpy.ops.wm.append(filepath = lib_path+"/default_uv", directory = lib_path,filename = 'default_uv', do_reuse_local_id = True)
                node_group = bpy.data.node_groups.get('default_uv')
        else:
            node_group = bpy.data.node_groups.get('default')
            if node_group is None:
                bpy.ops.wm.append(filepath = lib_path+"/default", directory = lib_path,filename = 'default', do_reuse_local_id = True)
                node_group = bpy.data.node_groups.get('default')
    if start_mode == 'EDIT_MESH':
        bpy.ops.object.editmode_toggle()
    hide_world_node()
    return node_group

def hide_world_node():
    try:
        world_node = bpy.context.scene.objects.get('world')
        if world_node is None:
            return
        if world_node.type != 'EMPTY':
            return
        bpy.context.scene.collection.objects.unlink(world_node)
        bpy.data.scenes['TECH_STUFF'].collection.objects.link(world_node)
    except:
        pass
    return

def build_dagormat_node_tree(mat):
    mat.use_nodes = True
    props       = mat.dagormat.optional
    textures    = mat.dagormat.textures
    nodes       = mat.node_tree.nodes
    links       = mat.node_tree.links
    shader_class= mat.dagormat.shader_class
    if shader_class in ['', 'None'] or shader_class.endswith(':proxymat'):
        return

    nodes.clear()

    output = nodes.new(type = "ShaderNodeOutputMaterial")
    shader_node = nodes.new(type="ShaderNodeGroup")
    shader_node.name = "Shader"
    shader_node.node_tree = get_node_group(shader_class)
    shader_node.location = (-180,0)
    links.new(shader_node.outputs['shader'],output.inputs['Surface'])

    uvs = nodes.new(type="ShaderNodeGroup")
    uvs.name = "UVs"
    node_tree = get_node_group(f'{shader_class}_uv')
    uvs.node_tree = node_tree
    uvs.location = (-680,0)

    ctrl_node = nodes.new(type="ShaderNodeGroup")
    ctrl_node.node_tree = get_node_group('global_settings')
    ctrl_node.location = (-920,-680)
    for out in ctrl_node.outputs.keys():
        if out in shader_node.inputs:
            links.new(ctrl_node.outputs[out],shader_node.inputs[out])
        if out in uvs.inputs:
            links.new(ctrl_node.outputs[out],uvs.inputs[out])

#TEXTURES
    active_found = False
    for tex in textures.keys():
        if textures[tex]!='':
            i = 10 if tex.endswith('10') else int(tex[-1])
            textures[tex]=textures[tex].replace('"','')
        else:
            continue
        img = nodes.new(type = "ShaderNodeTexImage")
        img.name = f"tex{i}"
        img.hide = True
        img.location = (-480,-40*i)
        img.image = get_image(textures[tex])
        links.new(uvs.outputs[f'tex{i}'],img.inputs["Vector"])
        links.new(img.outputs['Color'],shader_node.inputs[f"tex{i}"])
        links.new(img.outputs['Alpha'],shader_node.inputs[f"tex{i}_alpha"])

        #active node will be used in UV editor by default
        #otherwise non-square palette will tak it's place, distorting uv preview
        if not active_found:
            nodes.active = img
            active_found = True

#PROPERTIES
    for prop in props.keys():
        try:
            values = props[prop].to_list()#works only with float4 props
            name1 = f'{prop}.xyz'
            name2 = f'{prop}.w'
            if name1 in uvs.inputs:
                uvs.inputs[name1].default_value = [values[0],values[1],values[2]]
            elif name1 in shader_node.inputs:
                shader_node.inputs[name1].default_value = [values[0],values[1],values[2]]
            if name2 in uvs.inputs:
                try:
                    uvs.inputs[name2].default_value = values[3]
                except:
                    uvs.inputs[name2].default_value = int(values[3])
            elif name2 in shader_node.inputs:
                try:
                    shader_node.inputs[name2].default_value = values[3]
                except:
                    shader_node.inputs[name2].default_value = int(values[3])
        except:
            if prop in uvs.inputs:
                try:
                    uvs.inputs[prop].default_value = props[prop]#doesn't work with int input and float prop
                except:
                    uvs.inputs[prop].default_value = int(props[prop])
            elif prop in shader_node.inputs:
                try:
                    shader_node.inputs[prop].default_value = props[prop]#doesn't work with int input and float prop
                except:
                    shader_node.inputs[prop].default_value = int(props[prop])

#TRANSPARENSY
    mode = shader_node.inputs['blend'].default_value.upper()
    if get_blender_version() <= 4.1:
        try:
            mat.blend_method = mode
            if mode not in ['OPAQUE', 'CLIP']:
                mat.shadow_method = 'NONE'
            else:
                mat.shadow_method = mode
                #shaders with atest is always 'CLIP', even when it turned off (100% white alpha)
        except:
            pass
        backface = shader_node.inputs.get('show_backface')
        if backface is not None:
            mat.show_transparent_back = backface.default_value

    else:  # Blender 4.2+
        mat.surface_render_method = 'DITHERED' if mode in ['OPAQUE', 'CLIP'] else 'BLENDED'
        mat.use_transparency_overlap = True if mode == 'BLENDED' else False


#BACKFACE_CULLING
    mat.use_backface_culling = (mat.dagormat.sides == 0)#'0' is "single_sided"


#DESELECT
    for node in nodes:
        node.select = False
#REFRESHING PALETTES
    upd_project(None, bpy.context)#making sure that palette and project are correct
    return