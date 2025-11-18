import bpy
from re import search
from mathutils  import Vector
from math       import pi

from ..helpers.getters  import get_preferences, get_local_props, get_addon_directory
from ..helpers.names    import ensure_no_extension
from ..tools.tools_panel        import apply_modifiers
from ..tools.tools_functions    import *

# COLLECTION-related----------------------------------------------------------------------------------------------------

# Finds the collection in current view_layer
def find_interactive_vl_collection(current_vl_collection, collection):
    if current_vl_collection.collection.hide_select:
        return False  # selection disabled for all children
    if current_vl_collection.collection == collection:
        return current_vl_collection
    for child in current_vl_collection.children:
        found = find_interactive_vl_collection(child, collection)
        if found:
            return found
    return None

# Checks if collection content can be interacted from viewport
def is_collection_interactive(collection):
    view_layer = bpy.context.view_layer
    found_collection = find_interactive_vl_collection(view_layer.layer_collection, collection)
    if not found_collection:
        return False
    if not found_collection.visible_get():  # hidden by hierarchy
        return False
    if found_collection.exclude:
        return False
    if found_collection.collection.hide_viewport:  # hidden in all view layers
        return False
    if found_collection.collection.hide_select:  # non selectable
        return False
    return True

def get_collection(name):
    col = bpy.data.collections.get(name)
    if col is None:
        col = bpy.data.collections.new(name)
    return col

def get_lods_collection(ri_name):
    pattern = ri_name + ".lods"
    for collection in bpy.data.collections:
        if collection.name.startswith(pattern):  # could be "*.001+" after reimport
            return collection
    return None

#removes unused "ranom.NNN" collections
def cleanup_tech_stuff():
    scene = bpy.data.scenes.get('TECH_STUFF')
    if scene is None:
        scene = bpy.context.scene
    for col in scene.collection.children:
        if col.name.startswith('random.') and col.users == 1:
            for obj in list(col.objects):
                if obj.users == 1:
                    bpy.data.objects.remove(obj)
            bpy.data.collections.remove(col)
    return

#adds transefer_col to each scene
def upd_transfer_collection():
    t_col = get_collection('TRANSFER_COLLECTION')
    for s in bpy.data.scenes:
        if s.collection.children.get(t_col.name) is None:
            s.collection.children.link(t_col)
    return

#returns all "parents" of collection
def col_users_collection(subcol):
    users = []
    collections = list(bpy.data.collections)
    for scene in bpy.data.scenes:
        collections.append(scene.collection)
    for col in collections:
        if subcol in list(col.children):
            users.append(col)
    return users

# makes sure collection is empty
def clear_collection(collection):
    for child in list(collection.children):
        collection.children.unlink(child)
        if child.users == 0:
            del child
    for object in list(collection.objects):
        collection.objects.unlink(object)
        if object.users == 0:
            del object
    return


# ENTITIES EDITING------------------------------------------------------------------------------------------------------

#removes entity from random node
def node_remove_entity(node,index):
    node_col = node.instance_collection
    entity = node_col.objects[index]
    node_col.objects.unlink(entity)
    if entity.users == 0:
        bpy.data.objects.remove(entity)
    if node_col.objects.__len__() == 1:
        node.instance_collection = node_col.objects[0].instance_collection
    cleanup_tech_stuff()
    return

#turns basic node into random; adds extra entyty to random node
def node_add_entity(node):
    col = node.instance_collection
    if col is None or not col.name .startswith('random.'):
        parent_scene = bpy.data.scenes.get('TECH_STUFF')
        if parent_scene is None:
            parent = bpy.context.scene.collection
        else:
            parent = parent_scene.collection
        rand_col = bpy.data.collections.new(name = 'random.000')
        parent.children.link(rand_col)
        entity = bpy.data.objects.new('',None)
        entity.instance_type = 'COLLECTION'
        entity.instance_collection = col
        rand_col.objects.link(entity)
        node.instance_collection = rand_col
    else:
        rand_col = col
    new_entity = bpy.data.objects.new('',None)
    new_entity.instance_type = 'COLLECTION'
    rand_col.objects.link(new_entity)
    cleanup_tech_stuff()
    return


#Clears instance_collection and puts node.chidren into it
def node_rebuild(node, col = None):
    if col is None:
        col = node.instance_collection
    if col.name.endswith('.lods'):
        for lod in list(node.children):
            node_rebuild(lod)
            bpy.data.objects.remove(lod)
    else:
        for obj in list(col.objects):
            col.objects.unlink(obj)
        for child in node.children_recursive:
            for user in child.users_collection:
                user.objects.unlink(child)
            col.objects.link(child)
            if child.parent == node:
                child.parent = None
            #random node can't have offsets inside
            if col.name.startswith('random.'):
                child.location = [0,0,0]
                child.rotation_euler = [0,0,0]
                child.scale = [1,1,1]
    node.instance_type = 'COLLECTION'
    return

#Clears node.children and turns instancing back
def node_revert(node):
    for child in list(node.children_recursive):
        bpy.data.objects.remove(child)
    node.instance_type = 'COLLECTION'
    return


# CONVERTERS------------------------------------------------------------------------------------------------------------

#turns collection instance into mesh
def node_to_geometry(node):
    #preparation
    mesh = bpy.data.meshes.new('')
    geo_node = bpy.data.objects.new('name',mesh)
    matr = node.matrix_local
    col = bpy.context.collection
    col.objects.link(geo_node)
    geo_node.matrix_local = matr
    geo_node.parent = node.parent
    name = node.name
    #convertion
    geo_node.name = name
    mod = geo_node.modifiers.new('','NODES')
    if mod.node_group is not None:
        bpy.data.node_groups.remove(mod.node_group)
    converter_group = get_converter_ng()
    mod.node_group = converter_group
    mod['Input_1'] = node.instance_collection
    #cleanup
    apply_modifiers(geo_node)
    bpy.data.objects.remove(node)
    #shading
    version = get_blender_version()
    if version <= 4.0:
        geo_node.data.use_auto_smooth = True
        geo_node.data.auto_smooth_angle = 2*pi
    else:
        for poly in geo_node.data.polygons:
            poly.use_smooth = True
    return

# converts selected nodes to parent_collection
# sets parent_collection as instance_collection of parent_node
# offsets nodes to keep their instances visually n the original coorrdinates
def nodes_to_composite(nodes, parent_node, parent_collection):
    clear_collection(parent_collection)
    matrix_offset = parent_node.matrix_world.copy()
    matrix_offset.invert()
    for node in nodes:
        for user_collection in list(node.users_collection):
            user_collection.objects.unlink(node)
        parent_collection.objects.link(node)
        if node.parent is None or not node.parent in nodes:
            old_matrix = node.matrix_world
            node.parent = None
            node.matrix_world = matrix_offset @ old_matrix
    parent_node.instance_type = 'COLLECTION'
    parent_node.instance_collection = parent_collection
    parent_collection['type'] = 'composit'
    return

# splits selected nodes recursively down to ri instances
# does similar stuff to nodes_to_composite
# but also splits lods to different collections
def nodes_to_rendinst(nodes, parent_node, parent_collection):
    nodes_to_composite(nodes, parent_node, parent_collection)
    parent_collection['type'] = 'rendinst'
    props = get_local_props()
    bpy.ops.object.select_all(action = 'DESELECT')
    nodes = parent_collection.objects
    nodes_split(nodes, split_to_lods = True, split_to_meshes = True)
    existing_lods = {}
    for obj in list(parent_collection.objects):
        if not obj.name.startswith("lod"):
            parent_collection.objects.unlink(obj)
            continue
        found_lod = obj.name[:5]
        if not found_lod in existing_lods:
            lod_collection = bpy.data.collections.new(ensure_no_extension(parent_collection.name) + "." + found_lod)
            lod_collection['type'] = 'rendinst'
            existing_lods[found_lod] = lod_collection
            parent_collection.children.link(existing_lods[found_lod])
        parent_collection.objects.unlink(obj)
        existing_lods[found_lod].objects.link(obj)
        for child in list(obj.children_recursive):
            existing_lods[found_lod].objects.link(child)
    bpy.context.view_layer.update()  # making sure that matrices are up to date
    for lod in parent_collection.children:
        for obj in list(lod.objects):
            if obj.type != "EMPTY":
                if obj.data.users > 1:
                    obj.data = obj.data.copy()  # making them independent from original meshes
                continue
            for child in obj.children:
                matrix_backup = child.matrix_world
                child.parent = None
                child.matrix_world = matrix_backup
            bpy.data.objects.remove(obj)
    return

def ri_node_to_lods(node, split_to_meshes):
    ri_name_end, len = search("\\.lods($|\\.\\d*)", node.instance_collection.name).span()
    ri_name = node.instance_collection.name[:ri_name_end]
    for lod_collection in node.instance_collection.children:
        current_lod_name = lod_collection.name.replace(ri_name+".", "")
        lod_node = bpy.data.objects.new(current_lod_name, None)
        for collection in node.users_collection:
            collection.objects.link(lod_node)
        lod_node.parent = node
        lod_node.instance_collection = lod_collection
        if split_to_meshes:
            copy_collection_objects_to_new_parent(lod_node, lod_node.instance_collection.objects)
            lod_node.instance_type = 'NONE'  # objects copied already, making sure there is no overlap with instances
        else:
            lod_node.instance_type = 'COLLECTION'
    node.instance_type = 'NONE'
    return

# copies all the objects in the list while keeping their relations to each other
# moves them to all collections containing 'new_parent'
# makes "root" nodes into children of 'new_parent'
def copy_collection_objects_to_new_parent(new_parent, objects_to_copy, current_parent = None):
    if objects_to_copy == []:
        return []
    new_nodes = []
    if current_parent is None:
        current_level = [o for o in objects_to_copy if (o.parent is None) or not object_in_group(o.parent, objects_to_copy)]
    else:
        current_level = [o for o in objects_to_copy if o.parent is current_parent]
    for object in current_level:
        copied_entity = object.copy()
        matrix_backup = copied_entity.matrix_world if current_parent is None else copied_entity.matrix_local
        copied_entity.parent = new_parent
        copied_entity.matrix_local = matrix_backup
        #unlinked from original collection(s)
        for user_collection in copied_entity.users_collection:
            user_collection.objects.unlink(copied_entity)
        #linked to the collection(s) with the parent
        for user_collection in new_parent.users_collection:
            user_collection.objects.link(copied_entity)
        new_nodes.append(copied_entity)
        if object.children.__len__() > 0:
            new_nodes = new_nodes + copy_collection_objects_to_new_parent(copied_entity, objects_to_copy, current_parent = object)
    return new_nodes

# replaces _instance_collections by copies of their content
def nodes_split(nodes, split_to_lods = False, split_to_meshes = False, recursive = True, destructive = False):
    while nodes.__len__() > 0:
        node = nodes[0]
        nodes = nodes[1:]
        if node is None:
            print("oops")
            continue
        if node.type != 'EMPTY':
            continue
        if node.instance_collection is None:
            continue
        node.instance_type = 'NONE'
        col_name = node.instance_collection.name
    # rendinst lods
        if search("\\.lods($|\\.\\d*)", col_name):
            if split_to_lods:
                ri_node_to_lods(node, split_to_meshes)
            else:
                node.instance_type = 'COLLECTION'
    # single ri lod
        elif search("\\.lod\\d\\d($|\\.\\d*)", col_name):
            print("single lod")
            ri_name_end, len = search("\\.lod\\d\\d($|\\.\\d*)", col_name).span()
            ri_name = col_name[:ri_name_end]
            ri_lods_collection = get_lods_collection(ri_name)
            if (ri_lods_collection is not None
                and node.parent is not None
                and node.parent.type == 'EMPTY'
                and node.parent.instance_collection != ri_lods_collection):
                node.instance_collection = ri_lods_collection
                ri_node_to_lods(node, split_to_meshes)  # getting all lods if possible
            elif split_to_meshes:
                new_nodes = copy_collection_objects_to_new_parent(node, node.instance_collection.objects)
                if destructive:
                    bpy.context.view_layer.update()
                    for child in list(node.children):
                        wm = child.matrix_world
                        child.parent = None
                        child.matrix_world = wm
                    bpy.data.objects.remove(node)
                if recursive:
                    nodes = nodes + new_nodes
            else:
                print("child node, no parent, no split")
                node.instance_type = 'COLLECTION'  # not split, should keep instance
    # node with several entities
        elif search("^random\\.\\d*", col_name):
            print('random node')
            if node_has_overlaps(node):
                log(f'Overlapped nodes:\n\tNode "{node.name}" had several entities!\n\n',type = 'WARNING', show = True)
            new_nodes = copy_collection_objects_to_new_parent(node, node.instance_collection.objects)
            if destructive:
                bpy.context.view_layer.update()
                for child in list(node.children):
                    wm = child.matrix_world
                    child.parent = None
                    child.matrix_world = wm
                bpy.data.objects.remove(node)
            if recursive:
                nodes = nodes + new_nodes
    # sub-composite or prefab
        else:
            print("unknown node")
            new_nodes = copy_collection_objects_to_new_parent(node, node.instance_collection.objects)
            if destructive:
                bpy.context.view_layer.update()
                for child in list(node.children):
                    wm = child.matrix_world
                    child.parent = None
                    child.matrix_world = wm
                bpy.data.objects.remove(node)
            if recursive:
                nodes = nodes + new_nodes
    return

#turns bboxes of selected objects into chosen node
def bbox_to_gameobj(source, gameobj_collection):
#creating new node
    gameobj_node = bpy.data.objects.new(gameobj_collection.name,None)
    for col in source.users_collection:
        col.objects.link(gameobj_node)
    gameobj_node.empty_display_type = 'CUBE'
    gameobj_node.empty_display_size = 0.5
    gameobj_node.instance_type = 'COLLECTION'
    gameobj_node.instance_collection = gameobj_collection
    og_matrix = source.matrix_world
    rotation = og_matrix.to_euler()
#converting
    bbox = source.bound_box
    min = Vector(bbox[0])
    max = Vector(bbox[6])
    gameobj_node.matrix_world = og_matrix
    gameobj_node.location = (og_matrix @ min+og_matrix @ max)/2
    gameobj_node.scale *= (max-min)
#cleanup
    bpy.data.objects.remove(source)
    return


# MISC------------------------------------------------------------------------------------------------------------------

#creates scenes for cmp supported data types
def upd_scenes():
    for S in ["COMPOSITS", "GAMEOBJ", "GEOMETRY", "TECH_STUFF"]:
        if bpy.data.scenes.get(S) is None:
            new_scene = bpy.data.scenes.new(name = S)
            new_scene.world = bpy.data.worlds[0]
    upd_transfer_collection()
    return

#returns geometry nodegroup that turns collection into single mesh
def get_converter_ng():
    group_name = "GN_col_to_mesh"
    lib_path = get_addon_directory() + '/extras/library.blend/NodeTree'
    file = lib_path+f"/{group_name}"
    bpy.ops.wm.append(filepath = file, directory = lib_path,filename = group_name, do_reuse_local_id = True)
    node_group = bpy.data.node_groups.get(group_name)
    return node_group

#checks if node has several entities
def node_has_overlaps(node):
    col = node.instance_collection
    if col is None:
        return False
    if not col.name.startswith("random."):
        return False
    unic_objects = 0
    for obj in col.objects:
        if obj.instance_type == 'COLLECTION' and obj.instance_collection is not None:
            unic_objects += 1
    return unic_objects > 1


def get_valid_nodes_to_split():
    selection = list(bpy.context.selected_objects)
    nodes_to_split = []
    for node in selection:
        if node.type != 'EMPTY':
            continue
        if node.instance_type != 'COLLECTION':
            continue
        if node.instance_collection is None:
            continue
        if is_rendinst(node.instance_collection):
            continue
        nodes_to_split.append(node)
    return nodes_to_split

def is_rendinst(collection):
    if search('\.lods(\.\d\d\d|)$', collection.name):
        return True
    if search('\.lod\d\d(\.\d\d\d|)$', collection.name):
        return True
    for child in collection.children:
        if search('\.lod\d\d(\.\d\d\d|)$', child.name):
            return True  # if colection has lods in it it must be a ri/dynmodel collection
    return False

def object_in_group(object, group):
    for el in group:
        if object == el:
            return True
    return False