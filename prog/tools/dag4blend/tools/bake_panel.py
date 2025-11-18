import bpy
import bmesh
from bpy.props  import StringProperty
from bpy.types  import Operator, Panel
from bpy.utils  import register_class, unregister_class, user_resource
from os.path    import join, exists
from os         import rename, remove, makedirs
from time       import time

from ..helpers.texts    import log
from ..helpers.getters  import get_local_props

classes = []

bakeable_shader_outputs = [ '_tex_d_rgb',
                            '_tex_d_alpha',
                            '_tex_met_gloss',
                            '_tex_mask',
                            ]

#FUNCTIONS
def popup(msg):
    def draw(self, context):
        self.layout.label(text = msg)
    bpy.context.window_manager.popup_menu(draw)
    return

#adds datablock property and inits UI
def add_datablock_property(prop_owner, prop_name, data = None, type = 'OBJECT'):
    if prop_name not in prop_owner:
        prop_owner[prop_name] = data
    ui = prop_owner.id_properties_ui(prop_name)
    ui.update(id_type = type)
    return

def get_supported_materials():
    supported_materials = []
    for material in bpy.data.materials:
        if material.node_tree is None:
            continue
        if material.is_grease_pencil:
            continue  # grease pencil can't be baked
        shader_node = material.node_tree.nodes.get('Shader')
        if shader_node is None:
            continue
        else:
            supported_materials.append(material)
        for output_name in bakeable_shader_outputs:
            if shader_node.outputs.get(output_name) is None:
                supported_materials.remove(material)
                break
    return supported_materials

# returns objects, contained in collection and all sub-collections
def collection_get_mesh_objects(col):
    props = get_local_props()
    recursive = props.baker.recursive
    objects = list(col.objects)
    if recursive:
        for subcol in col.children_recursive:
            objects += subcol.objects
        # same object can be linked to multiple collections, we need them once
        objects = set(objects)
    mesh_objects = [obj for obj in objects if obj.type == 'MESH']
    return mesh_objects

# returns list of materials from set of mesh objects
def get_materials(objects):
    materials = []
    for obj in objects:
        mesh = obj.data
        if mesh.materials.__len__()==0:
            return None  # error signal for validator
        for mat in obj.data.materials:
            if mat not in materials:
                materials.append(mat)
    return materials

# makes sure that baking is possible
def bake_validate():
    props = get_local_props()
    baker_props = props.baker
    lp_collection = baker_props.lp_collection
    hp_collection = baker_props.hp_collection
    supported_materials = get_supported_materials()
# lp_collection
    if lp_collection is None:
        return "No lowpoly collection selected"
    lp_objects = collection_get_mesh_objects(lp_collection)
    if lp_objects.__len__() == 0:
        return f'No mesh objects in "{lp_collection.name}"'
    visible = list(bpy.context.view_layer.objects)
    for obj in lp_objects:
        if not obj in visible:
            return(f'Object "{obj.name}" is not visible!')
    lp_mats = get_materials(lp_objects)
    if lp_mats is None:
        return f'No material on some mesh in "{lp_collection.name}"'
    for obj in lp_objects:
        if obj.data.uv_layers.get('Bake') is None:
            return f'"{obj.name}": no"Bake" uv layer'
    init_materials(lp_mats)
    if baker_props.mode == 'UV_TO_UV':
        for mat in lp_mats:
            if mat not in supported_materials:
                return f'"{mat.name}" material can not be baked'
        return
# hp_collection. Make sence only for hp to lp mode
    if baker_props.hp_collection is None:
        return 'No highpoly collection selected'
    hp_objects = collection_get_mesh_objects(hp_collection)
    if hp_objects.__len__()==0:
        return f'No mesh objects in "{hp_collection.name}"'
    for obj in hp_objects:
        if not obj in visible:
            return(f'Object "{obj.name}" is not visible!')
    hp_mats = get_materials(hp_objects)
    if hp_mats is None:
        return f'No material on some mesh in "{hp_collection.name}"'
    for mat in hp_mats:
            if mat not in supported_materials:
                return f'"{mat.name}" material can not be baked'
    return

def cleanup_materials(materials):
    for material in materials:
        nodes = material.node_tree.nodes
        for node in nodes:
            if node.name in ['bake_uv'] + bakeable_shader_outputs:
                nodes.remove(node)
    return

def init_image(suffix, is_sRGB = False):
    props = get_local_props()
    baker_props = props.baker
    bake_height = int(baker_props.bake_height)
    bake_width = int(baker_props.bake_width)
    images = bpy.data.images
    combined_name = suffix
    image = images.get(combined_name)
    if image is None:
        image = images.new(name = combined_name, height = bake_height, width = bake_width)
    else:
        image.generated_width = bake_width
        image.generated_height = bake_height
    image.source = 'GENERATED'  # must rebake incase something is changed
    image.generated_color = (0,0,0,1)
    image.colorspace_settings.name = 'sRGB' if is_sRGB else 'Non-Color'
    return image

def material_get_uv_node(material, uv_name = "Bake"):  # TODO: expose uv name
    nodes = material.node_tree.nodes
    uv_node = nodes.get('bake_uv')
    if uv_node is None:
        uv_node = nodes.new(type = "ShaderNodeAttribute")
        uv_node.name = 'bake_uv'
    uv_node.select = False
    uv_node.attribute_name = "Bake"
    uv_node.location = (-360, 220)
    uv_node.select = False
    uv_node.outputs['Color'].hide = True
    uv_node.outputs['Fac'].hide = True
    uv_node.outputs['Alpha'].hide = True
    return uv_node

def material_init_texture_node(suffix, material, image, location):
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    node = nodes.get(suffix)
    if node is None:
        node = nodes.new(type = 'ShaderNodeTexImage')
        node.name = suffix
        node.hide = True
        node.select = False
        node.outputs['Alpha'].hide = True  # Blender can't bake to alpha propperly, separate textures used instead
        node.image = image # was initialized already, always exists
        node.location = location
        node.width = 140
        uv_node = material_get_uv_node(material)
        links.new(uv_node.outputs['Vector'], node.inputs['Vector'])
    return node

# prepares materials to baking
def init_materials(materials):
    props = get_local_props()
    baker_props = props.baker
# images
    if baker_props.tex_d or baker_props.tex_n:  # mask is necessary for any bake
        tex_mask = init_image("_tex_mask")
    else:
        return  # No reason to init zero textures for baking

    if baker_props.tex_d:
        tex_d_rgb = init_image("_tex_d_rgb", is_sRGB = True)
        tex_d_alpha = init_image("_tex_d_alpha")

    if baker_props.tex_n:
        tex_normal = init_image("_tex_normal")
        tex_met_gloss = init_image("_tex_met_gloss")

# node_trees of materials
    for material in materials:
        material_init_texture_node('_tex_mask', material, tex_mask, (-180, 220))
        if baker_props.tex_d:
            material_init_texture_node('_tex_d_rgb', material, tex_d_rgb, (-180, 160))
            material_init_texture_node('_tex_d_alpha', material, tex_d_alpha, (-180, 120))
        if baker_props.tex_n:
            material_init_texture_node('_tex_normal', material, tex_normal, (-180, 80))
            material_init_texture_node('_tex_met_gloss', material, tex_met_gloss, (-180, 40))
    return


def select_tex_nodes(materials, name):
    for material in materials:
        nodes = material.node_tree.nodes
        nodes.active = nodes.get(name)
    return

def show_tex_output(materials, name):
    for material in materials:
        nodes = material.node_tree.nodes
        material.node_tree.links.new(nodes['Shader'].outputs[name], nodes['Material Output'].inputs['Surface'])
    return

def reset_materials(materials):
    for material in materials:
        nodes = material.node_tree.nodes
        material.node_tree.links.new(nodes['Shader'].outputs['shader'], nodes['Material Output'].inputs['Surface'])
    return

def configure_renderer():
    backup = {}
    R = bpy.context.scene.render
    backup['engine'] = R.engine
    R.engine = 'CYCLES'
    backup['use_clear'] = R.bake.use_clear
    R.bake.use_clear = False
    backup['target'] = R.bake.target
    R.bake.target = 'IMAGE_TEXTURES'
    backup['margin'] = R.bake.margin
    R.bake.margin = 0  # would be added later on compositing stage
    backup['use_selected_to_active'] = R.bake.use_selected_to_active
    backup['normal_g'] = R.bake.normal_g
    R.bake.normal_g = 'NEG_Y'
    return backup

def restore_render(backup):
    R = bpy.context.scene.render
    R.engine = backup['engine']
    R.bake.use_clear = backup['use_clear']
    R.bake.target = backup['target']
    R.bake.margin = backup['margin']
    R.bake.use_selected_to_active = backup['use_selected_to_active']
    R.bake.normal_g = backup['normal_g']
    return

def bake_texture_uv_to_uv(tex_name, bake_type = 'EMIT'):
    props = get_local_props()
    baker_props = props.baker
    R = bpy.context.scene.render
    lp_objects = collection_get_mesh_objects(baker_props.lp_collection)
    bpy.context.view_layer.objects.active = lp_objects[0]
    lp_materials = get_materials(lp_objects)
    select_tex_nodes(lp_materials, tex_name)
    render_settings_backup = configure_renderer()
    R.bake.use_selected_to_active = False
    if bake_type == 'EMIT':
        show_tex_output(lp_materials, tex_name)
    bpy.ops.object.bake(type = bake_type)
    if bake_type == 'EMIT':
        reset_materials(lp_materials)
    restore_render(render_settings_backup)
    return

def bake_texture_hp_to_lp(lp_object, tex_name, bake_type = 'EMIT'):
    S = bpy.context.scene
    bpy.ops.object.select_all(action='DESELECT')
    lp_object.select_set(True)
    bpy.context.view_layer.objects.active = lp_object
    sources = get_highpoly(lp_object)
    print(lp_object, sources)
    hp_materials = get_materials(sources)
    for hp_object in sources:
        hp_object.select_set(True)
    select_tex_nodes(lp_object.data.materials, tex_name)
    if bake_type == 'EMIT':
        show_tex_output(hp_materials, tex_name)
    if S.render.bake.use_cage:
        if "cage" in lp_object.keys() and lp_object['cage'] is not None:
            S.render.bake.cage_object = lp_object['cage']
        else: S.render.bake.cage_object = None
    bpy.ops.object.bake(type = bake_type)
    reset_materials(hp_materials)
    return

def bake_all_textures_hp_to_lp():
    props = get_local_props()
    baker_props = props.baker
    R = bpy.context.scene.render
    lp_objects = collection_get_mesh_objects(baker_props.lp_collection)
    lp_materials = get_materials(lp_objects)
    render_settings_backup = configure_renderer()
    R.bake.use_selected_to_active = True
    for lp_object in lp_objects:
        lp_object.data.uv_layers.active = lp_object.data.uv_layers.get('Bake')
    for lp_object in lp_objects:
        bake_texture_hp_to_lp(lp_object, '_tex_mask')
    if baker_props.tex_d:
        for lp_object in lp_objects:
            bake_texture_hp_to_lp(lp_object, '_tex_d_rgb')
            bake_texture_hp_to_lp(lp_object, '_tex_d_alpha')
    if baker_props.tex_n:
        for lp_object in lp_objects:
            bake_texture_hp_to_lp(lp_object, '_tex_normal', bake_type = 'NORMAL')
            bake_texture_hp_to_lp(lp_object, '_tex_met_gloss')
    restore_render(render_settings_backup)
    cleanup_materials(lp_materials)
    return

def bake_all_textures_uv_to_uv():
    props = get_local_props()
    baker_props = props.baker
    R = bpy.context.scene.render
    lp_objects = collection_get_mesh_objects(baker_props.lp_collection)
    lp_materials = get_materials(lp_objects)
    render_settings_backup = configure_renderer()
    R.bake.use_selected_to_active = False
    bpy.ops.object.select_all(action='DESELECT')
    for lp_object in lp_objects:
        lp_object.select_set(True)
        lp_object.data.uv_layers.active = lp_object.data.uv_layers.get('Bake')
    bake_texture_uv_to_uv('_tex_mask')
    if baker_props.tex_d:
        bake_texture_uv_to_uv('_tex_d_rgb')
        bake_texture_uv_to_uv('_tex_d_alpha')
    if baker_props.tex_n:
        bake_texture_uv_to_uv('_tex_normal', bake_type = 'NORMAL')
        bake_texture_uv_to_uv('_tex_met_gloss')
    restore_render(render_settings_backup)
    bpy.ops.object.select_all(action='DESELECT')
    cleanup_materials(lp_materials)
    return

def get_highpoly(lp_object):
    props = get_local_props()
    baker_props = props.baker
    all_hp_objects = collection_get_mesh_objects(baker_props.hp_collection)
    collected_hp_nodes = []
    for key in lp_object.keys():
        if not key.startswith('hp.'):
            continue
        hp_node = lp_object[key]
        if hp_node is None:
            continue
        try: #incase it's not an object at all
            if hp_node.type != 'MESH':
                continue
            if hp_node not in all_hp_objects:
                continue
            if hp_node in collected_hp_nodes:
                continue
            collected_hp_nodes.append(hp_node)
        except:
            log(f'something wrong with "{lp_object.name}" hp parameters!\n', type = 'ERROR')
    hp_sources = collected_hp_nodes if collected_hp_nodes.__len__() > 0 else all_hp_objects
    return hp_sources


def cleanup_meshes(objects):
    bpy.ops.object.select_all(action='DESELECT')
    supported = get_supported_materials()
    for obj in objects:
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        mesh = obj.data
        indecies = []
        for i in range (mesh.materials.__len__()):
            if mesh.materials[i] not in supported:
                indecies.append(i)
        for v in mesh.vertices:
            v.select = False
        for e in mesh.edges:
            e.select = False
        for p in mesh.polygons:
            p.select = p.material_index in indecies
        bpy.ops.object.editmode_toggle()
        bpy.ops.mesh.delete()
        bpy.ops.object.editmode_toggle()
        bpy.ops.dt.mopt()
        obj.select_set(False)
    return

def get_principled_bsdf_node():
    group_name = 'Principled BSDF'
    node_group = bpy.data.node_groups.get(group_name)
    if node_group is not None:
        return node_group
    lib_path = user_resource('SCRIPTS') + f'/addons/dag4blend/extras/library.blend/NodeTree'
    file = lib_path+f"/{group_name}"
    bpy.ops.wm.append(filepath = file, directory = lib_path,filename = group_name, do_reuse_local_id = True)
    #if nodegroup not found in library it still be a None
    node_group = bpy.data.node_groups.get(group_name)
    return node_group

# temp_textures to dagor textures convertion
def get_converter_node():
    group_name = 'Bake_convert'
    node_group = bpy.data.node_groups.get(group_name)
    if node_group is not None:
        return node_group
    lib_path = user_resource('SCRIPTS') + f'/addons/dag4blend/extras/library.blend/NodeTree'
    file = lib_path+f"/{group_name}"
    bpy.ops.wm.append(filepath = file, directory = lib_path,filename = group_name, do_reuse_local_id = True)
    #if nodegroup not found in library it still be a None
    node_group = bpy.data.node_groups.get(group_name)
    return node_group

def compositor_add_image_node(name, location):
    bpy.context.scene.view_settings.view_transform = 'Standard'
    node_tree = bpy.context.scene.node_tree
    nodes = node_tree.nodes
    links = node_tree.links
    node = nodes.new('CompositorNodeImage')
    node.location = location
    node.name = node.label = name
    node.width = 120
    node.select = False
    return node

def configure_layer_slot(slot, color_mode):
    slot.use_node_format = False
    slot.format.file_format = 'TIFF'
    slot.format.tiff_codec = 'LZW'
    slot.format.color_depth = '8'
    slot.format.color_mode = color_mode
    slot.format.color_management = 'OVERRIDE'
    slot.format.view_settings.look = 'None'  # Color correction will break textures!
    slot.format.view_settings.view_transform = 'Standard'  # Color correction will break textures!
    return

def compose_textures():
    props = get_local_props()
    baker_props = props.baker

    scene_backup = bpy.context.scene

    temp_scene = bpy.data.scenes.new("")  # source for image settings, to preserve C.scene settings and compositor
    temp_scene.render.image_settings.file_format = 'TIFF'
    temp_scene.render.image_settings.tiff_codec = 'LZW'

    bpy.context.window.scene = temp_scene

    dirpath = baker_props.tex_dirpath
    material_name = baker_props.lp_collection.name.split(".")[0]
    bpy.context.scene.use_nodes = True
    node_tree = bpy.context.scene.node_tree
    nodes = node_tree.nodes
    links = node_tree.links
    images = bpy.data.images

    if baker_props.tex_d:
        tex_d_filename = f'#{material_name}_tex_d.tif'
        tex_d_alha_filename = f'#{material_name}_tex_d_alpha.tif'
    if baker_props.tex_n:
        tex_n_filename = f'#{material_name}_tex_n.tif'
        tex_n_alha_filename = f'#{material_name}_tex_n_alpha.tif'

    nodes.clear()

    converter = nodes.new('CompositorNodeGroup')
    converter.node_tree = get_converter_node()
    converter.location = (320, -320)
    converter.select = False

    tex_output = nodes.new('CompositorNodeOutputFile')
    tex_output.select = False
    tex_output.location = (660, -300)
    tex_output.layer_slots.clear()
    tex_output.base_path =dirpath

    if baker_props.tex_d:
        tex_output.layer_slots.new(tex_d_filename)
        configure_layer_slot(tex_output.file_slots[tex_d_filename], 'RGBA')
        links.new(converter.outputs['_tex_d_rgb1'], tex_output.inputs[tex_d_filename])

        tex_output.layer_slots.new(tex_d_alha_filename)
        configure_layer_slot(tex_output.file_slots[tex_d_alha_filename], 'BW')
        links.new(converter.outputs['_tex_d_alpha'], tex_output.inputs[tex_d_alha_filename])

    if baker_props.tex_n:
        tex_output.layer_slots.new(tex_n_filename)
        configure_layer_slot(tex_output.file_slots[tex_n_filename], 'RGBA')
        links.new(converter.outputs['_tex_n_rgb1'], tex_output.inputs[tex_n_filename])

        tex_output.layer_slots.new(tex_n_alha_filename)
        links.new(converter.outputs['_tex_n_alpha'], tex_output.inputs[tex_n_alha_filename])
        configure_layer_slot(tex_output.file_slots[tex_n_alha_filename], 'BW')


    tex_mask = compositor_add_image_node("_tex_mask", (0, -180*4))
    tex_mask.image = images['_tex_mask']
    images['_tex_mask'].pack()
    links.new(tex_mask.outputs[0], converter.inputs[4])

    if baker_props.tex_d:
        tex_d_rgb = compositor_add_image_node("_tex_d_rgb", (0, 0))
        images['_tex_d_rgb'].pack()
        tex_d_rgb.image = images['_tex_d_rgb']
        links.new(tex_d_rgb.outputs[0], converter.inputs[0])

        tex_d_alpha = compositor_add_image_node("_tex_d_alpha", (0, -180))
        tex_d_alpha.image = images['_tex_d_alpha']
        images['_tex_d_alpha'].pack()
        images['_tex_d_alpha'].colorspace_settings.name = 'Non-Color'
        links.new(tex_d_alpha.outputs[0], converter.inputs[1])

    if baker_props.tex_n:
        tex_normal = compositor_add_image_node("_tex_normal", (0, -180*2))
        tex_normal.image = images['_tex_normal']
        images['_tex_normal'].pack()
        images['_tex_normal'].colorspace_settings.name = 'sRGB'
        links.new(tex_normal.outputs[0], converter.inputs[2])

        tex_met_gloss = compositor_add_image_node("_tex_met_gloss", (0, -180*3))
        tex_met_gloss.image = images['_tex_met_gloss']
        images['_tex_met_gloss'].pack()
        images['_tex_met_gloss'].colorspace_settings.name = 'Non-Color'
        links.new(tex_met_gloss.outputs[0], converter.inputs[3])

    nodes.active = tex_output

    bpy.ops.render.render()

    frame_current = str(bpy.context.scene.frame_current)
    cut_len = frame_current.__len__()

    images.remove(images['_tex_mask'])

    if baker_props.tex_d:
        images.remove(images['_tex_d_rgb'])
        old_tex_d_filename = tex_d_filename.replace('#', frame_current)
        new_tex_d_filename = f'{material_name}_tex_d.tif'
        new_tex_d_filepath = join(dirpath, new_tex_d_filename)
        if exists(new_tex_d_filepath):
            remove(new_tex_d_filepath)
        rename(join(dirpath, old_tex_d_filename), new_tex_d_filepath)

        tex_d = images.new("", 1, 1)  # name and resolution do not matter
        tex_d.source = 'FILE'
        tex_d.filepath = join(dirpath, new_tex_d_filepath)
        tex_d.reload()

        tex_d_a = images['_tex_d_alpha']
        tex_d_a.unpack(method = 'REMOVE')
        tex_d_a.filepath = join(dirpath, f'{frame_current}{material_name}_tex_d_alpha.tif')
        tex_d_pixels = list(tex_d.pixels)  # faster than direct access, can be edited
        tex_d_a_pixels = tex_d_a.pixels[:]  # fastest acces, can't be edited
        for index in range(0, tex_d_pixels.__len__(), 4):  # each image stored as RGBA, even BW ones
            tex_d_pixels[index+3] = tex_d_a_pixels[index]
        tex_d.pixels = tex_d_pixels
        tex_d.update()
        tex_d.save_render(tex_d.filepath, scene = temp_scene)  # forced compression settings
        images.remove(tex_d)
        remove(tex_d_a.filepath)
        images.remove(tex_d_a)

    if baker_props.tex_n:
        images.remove(images['_tex_normal'])
        old_tex_n_filename = tex_n_filename.replace('#', frame_current)
        new_tex_n_filename = f'{material_name}_tex_n.tif'
        new_tex_n_filepath = join(dirpath, new_tex_n_filename)
        if exists(new_tex_n_filepath):
            remove(new_tex_n_filepath)
        rename(join(dirpath, old_tex_n_filename), new_tex_n_filepath)

        tex_n = images.new("", 1, 1)  # name and resolution do not matter
        tex_n.source = 'FILE'
        tex_n.filepath = join(dirpath, new_tex_n_filepath)
        tex_n.reload()

        tex_n_a = images['_tex_met_gloss']
        tex_n_a.unpack(method = 'REMOVE')
        tex_n_a.filepath = join(dirpath, f'{frame_current}{material_name}_tex_n_alpha.tif')

        tex_n_pixels = list(tex_n.pixels)  # faster than direct access, can be edited
        tex_n_a_pixels = tex_n_a.pixels[:]  # fastest acces, can't be edited
        for index in range(0, tex_n_pixels.__len__(), 4):  # each image stored as RGBA, even BW ones
            tex_n_pixels[index+3] = tex_n_a_pixels[index]
        tex_n.pixels = tex_n_pixels
        tex_n.update()
        tex_n.save_render(tex_n.filepath, scene = temp_scene)
        images.remove(tex_n)
        remove(tex_n_a.filepath)
        images.remove(tex_n_a)

    bpy.context.window.scene = scene_backup

    bpy.data.scenes.remove(temp_scene)

    if baker_props.reveal_result:
        bpy.ops.wm.path_open(filepath = dirpath)
    if baker_props.apply_proxymat:
        if exists(new_tex_n_filepath) and exists(new_tex_n_filepath):
            apply_proxymat()
        else:
            log("Can't find tex_n and/or tex_n for proxymat!")
        if baker_props.reveal_result:
            bpy.ops.wm.path_open(filepath = baker_props.proxymat_dirpath)
    return

def apply_proxymat():
    props = get_local_props()
    baker_props = props.baker
    proxymat_dirpath = baker_props.proxymat_dirpath
    tex_dirpath = baker_props.tex_dirpath
    lp_nodes = collection_get_mesh_objects(baker_props.lp_collection)
    for node in lp_nodes:
        node.data.materials.clear()
        mat_name = baker_props.lp_collection.name.split('.')[0]
        material = bpy.data.materials.get(mat_name)
        if material is None:
            material = bpy.data.materials.new(mat_name)
        node.data.materials.append(material)
        material.dagormat.shader_class = 'rendinst_simple'
        material.dagormat.is_proxy = True
        material.dagormat.textures.tex0 = join(tex_dirpath, mat_name + '_tex_d.tif')
        material.dagormat.textures.tex2 = join(tex_dirpath, mat_name + '_tex_n.tif')
        material.dagormat.proxy_path = proxymat_dirpath
        bpy.ops.dt.dagormat_write_proxy(material_name = mat_name)
        uv_names = [uv.name for uv in node.data.uv_layers]
        for name in uv_names:
            if name != "Bake":
                node.data.uv_layers.remove(node.data.uv_layers.get(name))
        node.data.uv_layers[0].name = 'UVMap'  # if bake happened, it exists
    return


def build_mesh_plane():
    bm = bmesh.new()
    coords = (  (-0.5,-0.5, 0),
                ( 0.5,-0.5, 0),
                ( 0.5, 0.5, 0),
                (-0.5, 0.5, 0),
                )
    verts = []
    for co in coords:
        vert = bm.verts.new(co)
        verts.append(vert)
    bm.faces.new(verts)
    meshes = bpy.data.meshes
    mesh = meshes.new("")
    bm.to_mesh(mesh)
    return mesh


def build_material_palette(object):
    coords = []
    objects = bpy.data.objects
    S = bpy.context.scene
    SC = S.collection
    x_location = 0
    for material in list(set(object.data.materials)):  # duplicates skipped
        mesh = build_mesh_plane()
        mesh.materials.append(material)
        mesh.uv_layers.new(name = "UVMap", do_init = True)
        mesh.uv_layers.new(name = "Bake", do_init = True)
        object = objects.new(material.name, mesh)
        SC.objects.link(object)
        object.location[0] = x_location
        x_location += 1
    return


#CLASSES

class DAGOR_OT_Add_Datablock_Property(Operator):
    bl_idname = "dt.add_datablock_property"
    bl_label = "Add"
    bl_context = "object"
    bl_space_type = 'VIEW_3D'
    bl_options = {'UNDO'}

    prop_name: StringProperty(default = 'prop')

    @classmethod
    def poll(self, context):
        return context.object is not None

    def execute(self, context):
        add_datablock_property(context.object, self.prop_name)
        return {'FINISHED'}
classes.append(DAGOR_OT_Add_Datablock_Property)


class DAGOR_OT_Remove_Datablock_Property(Operator):
    bl_idname = "dt.remove_datablock_property"
    bl_label = "Remove"
    bl_context = "object"
    bl_space_type = 'VIEW_3D'
    bl_options = {'UNDO'}

    prop_name: StringProperty(default = 'prop')

    @classmethod
    def poll(self, context):
        return context.object is not None

    def execute(self, context):
        if self.prop_name in context.object:
            del context.object[self.prop_name]
            return {'FINISHED'}
        else:
            return {'CANCELLED'}
classes.append(DAGOR_OT_Remove_Datablock_Property)

class DAGOR_OT_Build_Material_Palette(Operator):
    '''
    Creates a plane meshes for each material of active object
    '''
    bl_idname = "dt.build_material_palette"
    bl_label = "To Palette"
    bl_options = {'UNDO'}

    @classmethod
    def poll(self, context):
        return (context.mode == 'OBJECT'
                and context.object is not None
                and context.object.type == 'MESH'
                )

    def execute(self, context):
        build_material_palette(context.object)
        return {'FINISHED'}
classes.append(DAGOR_OT_Build_Material_Palette)

class DAGOR_OT_CleanupMeshes(Operator):
    bl_idname = "dt.prebake_cleanup_meshes"
    bl_label = "Cleanup"
    bl_description = "Removes geometry with unsupported shaders (mostly decals)"
    bl_options = {'UNDO'}

    def execute(self,context):
        if context.mode!='OBJECT':
            popup('go to object mode first!')
            return{'CANCELLED'}
        props = get_local_props()
        baker_props = props.baker
        if baker_props.mode == 'UV_TO_UV':
            col = baker_props.lp_collection
        else:
            col = baker_props.hp_collection
        if col is None:
            msg = 'No collection selected!'
            log(msg, type = 'ERROR')
            popup(msg)
            return{'CANCELLED'}
        cleanup_meshes(collection_get_mesh_objects(col))
        return {'FINISHED'}
classes.append(DAGOR_OT_CleanupMeshes)

class DAGOR_OT_Principled_BSDF_convert(Operator):
    '''
    Replaces PrincipledBSDF by nodegroup that supported by baker
    '''
    bl_idname = "dt.principled_bsdf_convert"
    bl_label = "Convert Principled BSDF"
    bl_options = {'UNDO'}

    def execute(self,context):
        socket_names = ["Base Color",
                "Metallic",
                "Roughness",
                "Alpha",
                "Normal",
                ]
        for mat in bpy.data.materials:
            if not mat.use_nodes:
                continue
            node_tree = mat.node_tree
            if node_tree.nodes.get('Shader') is not None:
                continue
            old_shader = node_tree.nodes.get('Principled BSDF')
            if old_shader is None:
                continue
            new_shader = node_tree.nodes.new('ShaderNodeGroup')
            new_shader.name = "Shader"
            new_shader.node_tree = get_principled_bsdf_node()
            new_shader.location = old_shader.location
            new_shader.width = old_shader.width
            new_shader.select = False
        # inputs
            for link in node_tree.links:
                if not link.to_node == old_shader:
                    continue
                output = link.from_socket
                for name in socket_names:
                    if link.to_socket == old_shader.inputs[name]:
                        input = new_shader.inputs[name]
                node_tree.links.new(input, output)
        # outputs
            for link in node_tree.links:
                if not link.from_node == old_shader:
                    continue
                output = new_shader.outputs['shader']
                input = link.to_socket
                node_tree.links.new(input, output)
            node_tree.nodes.remove(old_shader)
        return {'FINISHED'}
classes.append(DAGOR_OT_Principled_BSDF_convert)


class DAGOR_OT_RunBake(Operator):
    bl_idname = "dt.run_bake"
    bl_label = "Run bake"
    bl_description = "Bakes dag shaders to 'rendinst_simple'"

    @classmethod
    def poll(self, context):
        props = get_local_props()
        baker_props = props.baker
        if not (baker_props.tex_d or baker_props.tex_n):
            return False
        if baker_props.lp_collection is None:
            return False
        if baker_props.mode == 'HP_TO_LP' and baker_props.hp_collection is None:
            return False
        return True

    def execute(self,context):
        start = time()
        props = get_local_props()
        baker_props = props.baker
        msg = bake_validate()
        if msg is not None:
            log(msg+"\n", type = "ERROR", show = True)
            return{'CANCELLED'}
        if not exists(baker_props.tex_dirpath):
            makedirs(baker_props.tex_dirpath)
            return {'CANCELLED'}
        if not exists(baker_props.proxymat_dirpath):
            makedirs(baker_props.proxymat_dirpath)
            return {'CANCELLED'}
        if baker_props.mode == 'HP_TO_LP':
            bake_all_textures_hp_to_lp()
        else:
            bake_all_textures_uv_to_uv()
        compose_textures()
        spent = time() - start
        msg = f'Textures baked in {spent} sec!\n'
        log(msg, show = True)
        return {'FINISHED'}
classes.append(DAGOR_OT_RunBake)


class DAGOR_PT_Baker(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Bake"
    bl_category = "Dagor"
    bl_options = {'DEFAULT_CLOSED'}

    def draw_settings(self, context, layout):
        S = context.scene
        props = get_local_props()
        baker_props = props.baker
        settings = layout.box()
        settings = settings.column(align = True)
        header = settings.row(align = True)
        header.prop(baker_props,'settings_maximized', text = '',
            icon = 'DOWNARROW_HLT' if baker_props.settings_maximized else 'RIGHTARROW', emboss = False)
        header.prop(baker_props,'settings_maximized', text = 'Settings', emboss = False)
        header.prop(baker_props,'settings_maximized', text = '',icon = 'SETTINGS', emboss = False)
        if not baker_props.settings_maximized:
            return

        settings.separator()

        render_settings = settings.row()
        render_settings = render_settings.box()
        render_header = render_settings.row(align = True)
        render_header.prop(baker_props,'render_maximized', text = '',
            icon = 'DOWNARROW_HLT' if baker_props.render_maximized else 'RIGHTARROW', emboss = False)
        render_header.prop(baker_props,'render_maximized', text = 'Renderer', emboss = False)
        render_header.prop(baker_props,'render_maximized', text = '',icon = 'SCENE', emboss = False)
        if baker_props.render_maximized:
            render_settings = render_settings.column(align = True)
            render_settings.prop(S.cycles, 'device', text = "")
            render_settings.prop(S.cycles, 'samples')

        inputs_settings = settings.row()
        inputs_settings = inputs_settings.box()
        inputs_header = inputs_settings.row(align = True)
        inputs_header.prop(baker_props,'input_maximized', text = '',
            icon = 'DOWNARROW_HLT' if baker_props.input_maximized else 'RIGHTARROW', emboss = False)
        inputs_header.prop(baker_props,'input_maximized', text = 'Inputs', emboss = False)
        inputs_header.prop(baker_props,'input_maximized', text = '',icon = 'OPTIONS', emboss = False)
        if baker_props.input_maximized:
            inputs_settings = inputs_settings.column(align = True)
            mode = inputs_settings.row()
            mode.prop(baker_props, 'mode', expand = True)
            inputs_settings.separator()
            recursive = inputs_settings.row()
            recursive.prop(baker_props, 'recursive', toggle = True, icon = 'CHECKBOX_HLT' if baker_props.recursive else 'CHECKBOX_DEHLT')
            inputs_settings.prop(baker_props, 'lp_collection', text = "Asset" if baker_props.mode == 'UV_TO_UV' else "Lowpoly")
            if baker_props.mode == 'HP_TO_LP':
                row = inputs_settings.row()
                row.prop(baker_props, 'hp_collection', text = "Highpoly")
            inputs_settings.separator()
            if baker_props.mode == 'HP_TO_LP':
                inputs_settings.prop(S.render.bake, 'use_cage',
                    text = 'Use Cage(s)',
                    toggle = True,
                     icon = 'CHECKBOX_HLT' if S.render.bake.use_cage else 'CHECKBOX_DEHLT')
                row = inputs_settings.row()
                row.prop(S.render.bake,'cage_extrusion', text = "Extrusion")
                row = inputs_settings.row()
                row.prop(S.render.bake, 'max_ray_distance')

        if baker_props.mode == 'HP_TO_LP':
            row = settings.row()
            self.draw_node_properties(context, row)

        output_settings = settings.box()
        output_header = output_settings.row(align = True)
        output_header.prop(baker_props,'output_maximized', text = '',
            icon = 'DOWNARROW_HLT' if baker_props.output_maximized else 'RIGHTARROW', emboss = False)
        output_header.prop(baker_props,'output_maximized', text = 'Outputs', emboss = False)
        output_header.prop(baker_props,'output_maximized', text = '',icon = 'OUTPUT', emboss = False)
        if baker_props.output_maximized:
            output_settings = output_settings.column(align = True)
            textures = output_settings.row(align = True)
            tex_d = textures.row()
            tex_d.prop(baker_props, 'tex_d', toggle = True,
            icon = 'CHECKBOX_HLT' if baker_props.tex_d else 'CHECKBOX_DEHLT')
            tex_n = textures.row()
            tex_n.prop(baker_props, 'tex_n', toggle = True,
            icon = 'CHECKBOX_HLT' if baker_props.tex_n else 'CHECKBOX_DEHLT')
            output_settings.separator()
            if not (baker_props.tex_d or baker_props.tex_n):
                output_settings.label(icon = 'ERROR', text = "There's nothing to bake!")
                output_settings.separator()
            resolution_x = output_settings.row()
            resolution_x.prop(baker_props,'bake_width',text = 'Width')
            resolution_y = output_settings.row()
            resolution_y.prop(baker_props,'bake_height',text = 'Height')
            output_settings.separator()
            output_settings.prop(baker_props, 'reveal_result', toggle = True,
                icon = 'CHECKBOX_HLT'if baker_props.reveal_result else 'CHECKBOX_DEHLT')
            output_settings.separator()
            row = output_settings.row()
            row.prop(baker_props, 'tex_dirpath', text = "Tex Dirpath")
            row = output_settings.row()
            open = row.operator('wm.path_open',
                icon = 'FILE_FOLDER' if exists(baker_props.tex_dirpath) else 'ERROR',
                text='open textures folder'if exists(baker_props.tex_dirpath) else 'Does not exist!')
            open.filepath = baker_props.tex_dirpath
            row.active = exists(baker_props.tex_dirpath)
            row.enabled = exists(baker_props.tex_dirpath)
            output_settings.separator()
            output_settings.prop(baker_props, 'proxymat', text = 'Save Proxymat', toggle = True,
                icon = 'CHECKBOX_HLT' if baker_props.proxymat else 'CHECKBOX_DEHLT')
            if baker_props.proxymat:
                row = output_settings.row()
                row.prop(baker_props, 'apply_proxymat', text = 'Apply Proxymat', toggle = True,
                    icon = 'CHECKBOX_HLT' if baker_props.apply_proxymat else 'CHECKBOX_DEHLT')
                output_settings.separator()
                output_settings.prop(baker_props, 'proxymat_dirpath', text = 'Proxy Dirpath')
                row = output_settings.row()
                open = row.operator('wm.path_open',
                    icon = 'FILE_FOLDER' if exists(baker_props.proxymat_dirpath) else 'ERROR',
                    text='open proxymats folder' if exists(baker_props.proxymat_dirpath) else 'Does not exist!')
                open.filepath = baker_props.proxymat_dirpath
                row.active = exists(baker_props.proxymat_dirpath)
                row.enabled = exists(baker_props.proxymat_dirpath)
        return

    def draw_highpoly_block(self, context, layout, prop_name, all_hp_nodes, collected_hp_nodes):
        column = layout.column(align = True)
        node = context.object
        prop_value = node[prop_name]
        if prop_value is None:
            column.label(text = "No HP node selected!", icon = 'ERROR')
            active = False
        elif prop_value not in all_hp_nodes:
            active = False
            column.label(text = f"{prop_value.name} is not a HP node!", icon = 'ERROR')
        elif prop_value in collected_hp_nodes:
            column.label(text = "It's a duplicate!", icon = 'ERROR')
            active = False
        else:
            active = True
        row = column.row(align = True)
        row.prop(context.object, f'["{prop_name}"]', text = "")
        row.operator('dt.remove_datablock_property', text = "", icon = 'TRASH').prop_name = prop_name
        column.active = active
        return

    def draw_node_properties(self, context, layout):
        props = get_local_props()
        baker_props = props.baker
        S = context.scene
        box = layout.box()
        column = box.column(align = True)
        header = column.row(align = True)
        header.prop(baker_props,'node_maximized', text = '',
            icon = 'DOWNARROW_HLT' if baker_props.node_maximized else 'RIGHTARROW', emboss = False)
        header.prop(baker_props,'node_maximized', text = 'Actvie Node', emboss = False)
        header.prop(baker_props,'node_maximized', text = '',icon = 'SETTINGS', emboss = False)
        if not baker_props.node_maximized:
            return
        column.separator()
        node = context.object
        if node is None:
            column.label(text = "No active object!", icon = 'ERROR')
            return
        elif baker_props.lp_collection is None:
            column.label(text = "LP collection is not specified!", icon = 'ERROR')
            return
        elif node not in collection_get_mesh_objects(baker_props.lp_collection):
            column.label(text = "Active object is not in LP Collection!", icon = 'ERROR')
            return
        if S.render.bake.use_cage:
            cage = column.box()
            cage.label(text = 'Cage:')
            if "cage" not in node:
                add_cage = cage.operator('dt.add_datablock_property', text = "Init Custom Cage")
                add_cage.prop_name = "cage"
            else:
                row = cage.row(align = True)
                row.prop(node, '["cage"]', text = "")
                remove_cage = row.operator('dt.remove_datablock_property', text = "", icon = 'TRASH')
                remove_cage.prop_name = "cage"
        hp = column.box()
        hp.label(text = "HighPoly:")
        if baker_props.hp_collection is None:
            hp.label(text = "HP collection is not specified!", icon = 'ERROR')
            return
        all_hp_nodes = collection_get_mesh_objects(baker_props.hp_collection)
        hp_props = []
        collected_hp_nodes = []
        bake_all = True
        for key in node.keys():
            if key.startswith('hp.'):
                hp_props.append(key)
        if hp_props.__len__() == 0:
            hp.label(text = f"{node.name} will be baked from all {all_hp_nodes.__len__()} node(s)")
        for hp_prop_name in hp_props:
            self.draw_highpoly_block(context, hp, hp_prop_name, all_hp_nodes, collected_hp_nodes)
            collected_hp_nodes.append(node[hp_prop_name])
        index = 0
        while f"hp.{index}" in node.keys():
            index+=1
        hp.operator('dt.add_datablock_property', text = "ADD", icon = 'ADD').prop_name = f'hp.{index}'
        return

    def draw_operators(self, context, layout):
        props = get_local_props()
        baker_props = props.baker
        operators = layout.box()
        operators = operators.column(align = True)
        header = operators.row(align = True)
        header.prop(baker_props,'operators_maximized', text = '',
            icon = 'DOWNARROW_HLT' if baker_props.operators_maximized else 'RIGHTARROW', emboss = False)
        header.prop(baker_props,'operators_maximized', text = 'Operators', emboss = False)
        header.prop(baker_props,'operators_maximized', text = '',icon = 'TOOL_SETTINGS', emboss = False)
        if not baker_props.operators_maximized:
            return
        operators.separator()
        operators = operators.column(align = True)
        row = operators.row()
        row.operator('dt.principled_bsdf_convert')
        row = operators.row()
        row.operator('dt.prebake_cleanup_meshes', text = 'Cleanup geometry')
        row = operators.row()
        row.operator('dt.build_material_palette')
        operators.separator()
        bake = operators.row(align = True)
        bake.scale_y = 2
        t_exists = exists(baker_props.tex_dirpath)
        p_exists = exists(baker_props.proxymat_dirpath)
        btn_name = 'BAKE'
        if not t_exists:
            btn_name = 'CREATE TEX FOLDER'
        elif baker_props.proxymat and not p_exists:
            btn_name = 'CREATE PROXY FOLDER'
        bake.operator('dt.run_bake', text = btn_name, icon = 'RENDER_STILL')
        msg = None
        if not (baker_props.tex_d or baker_props.tex_n):
            msg = "Please, select at least one texture!"
        elif baker_props.lp_collection is None:
            lp_col = "asset" if baker_props.mode == 'UV_TO_UV' else "lowpoly"
            msg = f"Please, specify {lp_col} collection!"
        elif baker_props.mode == 'HP_TO_LP' and baker_props.hp_collection is None:
            msg = "Please, specify highpoly collection!"
        elif not exists(baker_props.tex_dirpath):
            msg = "Tex folder does not exist!"
        elif not exists(baker_props.proxymat_dirpath) and baker_props.proxymat:
            msg = "Proxymat folder does not exist!"
        if msg is not None:
            operators.separator()
            operators.label(text = msg, icon = 'ERROR')
        return

    def draw(self, context):
        layout = self.layout
        self.draw_settings(context, layout)
        self.draw_operators(context, layout)
        return

classes.append(DAGOR_PT_Baker)

def register():
    for cl in classes:
        register_class(cl)
    return

def unregister():
    for cl in classes:
        unregister_class(cl)
    return