import bpy
from ..helpers.basename import basename
from ..helpers.version  import get_blender_version

def get_shader_names():
    shader_names = ["default", "default_uv"]
    preferences = bpy.context.preferences.addons[basename(__package__)].preferences
    for category in preferences['shader_categories']:
        for shader in category['shaders']:
            shader_names.append(shader['name'])
            shader_names.append(shader['name']+"_uv")
    return shader_names

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
    shader_names = get_shader_names()

    shader_nodegroups = [gr for gr in bpy.data.node_groups if gr.type == 'SHADER']
    shader_nodegroups = list(shader_nodegroups)

    preserved_groups = [gr for gr in shader_nodegroups  # this could be preserved by user for a reason
        if gr.use_fake_user and gr.users == 1 and gr.name not in shader_names]

    groups_to_replace = [gr for gr in shader_nodegroups
        if gr.name in shader_names
        or gr.name.endswith("_uv") and gr.name[:-3] in shader_names
        ]

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
