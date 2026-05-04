import bpy

from ..helpers.version  import get_blender_version
from ..helpers.getters  import get_preferences

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
