import bpy, bmesh

#bmesh functions--------------------------------------------------------------------------------------------------------

#converts mesh/edit_mesh to bmesh
def mesh_to_bmesh(mesh):
    current_mode = bpy.context.mode
    if current_mode == 'EDIT_MESH':
        bm = bmesh.from_edit_mesh(mesh)
    else:
        bm = bmesh.new()
        bm.from_mesh(mesh)
    #it's not always necessary, but in most cases it's faster than re-calling function several times
    bm.verts.ensure_lookup_table()
    bm.edges.ensure_lookup_table()
    bm.faces.ensure_lookup_table()
    return bm

#converts bmesh data back to mesh/edit_mesh
def bmesh_to_mesh(bm, mesh):
    current_mode = bpy.context.mode
    if current_mode == 'EDIT_MESH':
        bmesh.update_edit_mesh(mesh)
    else:
        bm.to_mesh(mesh)
    return mesh


def bmesh_select_elements(elements):
    #works only after bm.<domain>.ensure_lookup_table()
    for el in elements:
        el.select_set(True)
    return


def bmesh_deselect_elements(elements):
    #works only after bm.<domain>.ensure_lookup_table()
    for el in elements:
        el.select_set(False)
    return

#returns faces, that have at least one
def bmesh_get_perimeter_faces(faces):
    neighbours = []
    for f in faces:
        for v in f.verts:
            for link_face in v.link_faces:
                if link_face.select or link_face == f:
                    continue
                link_face.select_set(True)
                neighbours.append(link_face)
    return neighbours


#SmoothGroups specific functions----------------------------------------------------------------------------------------

#constants
SG_MAX_UINT = 2**32#overflow happens here
SG_CONVERT  = 2**31#greater than signed int32 max

#unsigned to signed
def uint_to_int(int):
    if int<SG_CONVERT:
        return int
    else:
        return int-SG_MAX_UINT

def int_to_uint(int):
    if int>0:
        return int
    else:
        return SG_MAX_UINT+int#2**32-something

#returns logical OR of SG for list of faces
def get_sg_combined(faces, SmoothGroups):
    sg = 0
    for face in faces:
        sg = sg|SmoothGroups[face.index]
    return sg

'''
Returns an int value, that has no shared bits with <exclude_sg> (in binary representation)
and has at least one shared bit with each value in [includes]
Returns 0, if such SG does not exist
'''
def get_free_sg(includes, exclude_sg):
    for el in includes:
        el = int_to_uint(el)
    exclude_sg = int_to_uint(exclude_sg)
    #no SG should be included. One would be enough. Each bit is a power of 2
    if includes.__len__() == 0:
        for pow in range (32):
            if 2**pow & exclude_sg == 0:
                return uint_to_int(2**pow)
        #if we're here, all 32 bits were skipped, no more legit options.
        return 0
    optimized = []#includes, but without bits of exclude_sg
    for value in includes:
        shared_bits = exclude_sg & value
        if shared_bits == value:
            #SG can't include and exclude same set of bits
            return 0
        else:
            diff = value - shared_bits
            if diff not in optimized:
                optimized.append(diff)
    #combining into single SG
    found_sg = 0
    for value in optimized:
        found_sg = found_sg|value

    #removing unnecessary bits if they exist
    current_bit = 1
    for power in range(32):#['0b1', '0b10', '0b100', etc.]
        is_necessary = False
        if found_sg & current_bit == 0:
            current_bit = current_bit*2
            continue
        possible_sg = found_sg - current_bit
        for opt in optimized:
            if opt & possible_sg == 0:
                is_necessary = True
            if is_necessary:
                break
        current_bit = current_bit*2
        if is_necessary:
            continue
        found_sg = possible_sg
    found_sg = uint_to_int(found_sg)
    return found_sg

#converts list of island indecies to islands
def ids_to_islands(IDs, faces):
    islands = [[] for island in range(IDs[1])]
    for face_index in range(IDs[0].__len__()):
        current_island = IDs[0][face_index]-1 #starts with 1 instead of 0
        islands[current_island].append(faces[face_index])
    return islands

#Calcs and assigns SG attribute
def mesh_calc_smooth_groups(mesh):
    bm = mesh_to_bmesh(mesh)
    SmoothGroups = [0 for face in range(bm.faces.__len__())]
    IDs = mesh.calc_smooth_groups(use_bitflags = False)#each island gets unic int index. ([numbers], amount of islands)
    islands = ids_to_islands(IDs, bm.faces)
    bmesh_deselect_elements(bm.faces)#selection used to speed up search for neighbours
    for island in islands:
        bmesh_select_elements(island)
        neighbours = bmesh_get_perimeter_faces(island)
        bmesh_deselect_elements(island+neighbours)
        exclude_sg = get_sg_combined(neighbours, SmoothGroups)
        value = get_free_sg([],exclude_sg)
        for f in island:
            SmoothGroups[f.index] = value
    Attr = bm.faces.layers.int.get("SG")
    if Attr is None:
        Attr = bm.faces.layers.int.new("SG")
    for face in bm.faces:
        face[Attr] = SmoothGroups[face.index] if face.smooth else 0
    return

def init_sg_attribute(object):
    attributes = object.data.attributes
    attr = attributes.get("SG")
    if attr is None:
        attributes.new("SG", 'INT', 'FACE')
    elif attr.domain!='FACE' or attr.data_type!='INT':
        attr.name = "SG.incorrect"
        attributes.new("SG", 'INT', 'FACE')
    return

#Calculates SG attribute for input 'MESH' objects
def objects_calc_smooth_groups(objects):
    current_mode = bpy.context.mode
    for obj in objects:
        init_sg_attribute(obj)
    #applying edit_mesh changes to meshes if necessary
    if current_mode == 'EDIT_MESH':
        needs_update = False
        for obj in objects:
            if obj.mode == 'EDIT':
                needs_update = True
                break
        if needs_update:
            bpy.ops.object.editmode_toggle()# "edit_mesh" might not match "mesh", it updates only on switching
            bpy.ops.object.editmode_toggle()# keepeng user in the same mode by switching back to edit
    for obj in objects:
        mesh_calc_smooth_groups(obj.data)
    return