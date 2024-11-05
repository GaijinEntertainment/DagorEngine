import bpy
import bmesh
from  .bmesh_functions  import *
from   math             import pi, radians, dist, sqrt

# UNNECESSARY-VERTICES--------------------------------------------------------------------------------------------------
# finds verts with tho linked edges, forming almost straight line
def bm_find_unnecessary_verts(bm, degrees_threshold = 1):
    found_verts = []
    radians_threshold = radians(degrees_threshold)
    verts = bm.verts
    for vert in verts:
        if vert.link_faces.__len__() == 0:
            found_verts.append(vert)
        elif vert.link_edges.__len__() == 2:
            vector_a = vert.co - vert.link_edges[0].other_vert(vert).co
            vector_b = vert.co - vert.link_edges[1].other_vert(vert).co
            angle_between_edges = abs(vector_a.angle(vector_b))
            if angle_between_edges >= pi - radians_threshold:
                if angle_between_edges <= pi + radians_threshold:
                    found_verts.append(vert)
        else:
            continue
    return found_verts


def bm_select_unnecessary_verts(bm, degrees_threshold = 1):
    found_verts = bm_find_unnecessary_verts(bm, degrees_threshold)
    bm_deselect_all(bm)
    bm_select_elements(found_verts)
    bm.select_mode = {'VERT'}
    bm.select_flush_mode()
    return


def bm_dissolve_unnecessary_verts(bm, degrees_threshold = 1):
    found_verts = bm_find_unnecessary_verts(bm, degrees_threshold)
    end_verts = []
    for vert in found_verts:
        if vert.link_edges.__len__() == 1 and vert.link_faces.__len__() == 0:
            end_verts.append(vert)
    bmesh.ops.dissolve_verts(bm, verts = found_verts)
    bmesh.ops.delete(bm, geom = end_verts, context = 'VERTS')  # verts with one edge was not dissolved
    bm.select_mode = {'VERT'}
    return


# DEGENERATED TRIANGLES-------------------------------------------------------------------------------------------------

#constants
#degenerativeTriAreaThresholdSq value from application.blk, currently used by dabuild
DEGENERATIVE_TRI_AREA_THRESHOLD_SQ = 3e-10  # 0.0000000003

#remembers original indices before triangulation
def store_face_indices(bm):
    face_index = bm.faces.layers.int.get('face_index')
    if face_index is None:
        face_index = bm.faces.layers.int.new('face_index')
    for f in bm.faces:
        f[face_index] = f.index
    return face_index

def calc_area(tri, matrix_world = None):
    if matrix_world is None:
        area = tri.calc_area()
    else:  # should calculate manually from modified coordinates
        co1 = matrix_world @ tri.verts[0].co
        co2 = matrix_world @ tri.verts[1].co
        co3 = matrix_world @ tri.verts[2].co
        len_a = dist(co1, co2)
        len_b = dist(co2, co3)
        len_c = dist(co3, co1)
        p = (len_a + len_b + len_c)/2  # half perimeter
        area = sqrt(p * (p - len_a) * (p - len_b) * (p - len_c))
    return area

#area/perimeter
def calc_ratio(tri, matrix_world = None):
    if matrix_world is None:
        len_a = tri.edges[0].calc_length()
        len_b = tri.edges[1].calc_length()
        len_c = tri.edges[2].calc_length()
    else:
        co1 = matrix_world @ tri.verts[0].co
        co2 = matrix_world @ tri.verts[1].co
        co3 = matrix_world @ tri.verts[2].co
        len_a = dist(co1, co2)
        len_b = dist(co2, co3)
        len_c = dist(co3, co1)
    perimeter = sum((len_a, len_b, len_c))
    area = calc_area(tri, matrix_world)
    return area/perimeter


def is_degenerated_legacy(tri, matrix_world = None):
    area = calc_area(tri, matrix_world)
    return (area/4) ** 2/2 < DEGENERATIVE_TRI_AREA_THRESHOLD_SQ  # dabuild checks like that


def is_degenerated(tri, area_threshold, ratio_treshold, matrix_world = None):
    area = calc_area(tri, matrix_world = matrix_world)
    if area < area_threshold:
        return True
    ratio = calc_ratio(tri, matrix_world = matrix_world)
    if ratio < ratio_treshold:
        return True
    return False


def find_degenerates(object, legacy_mode, destructive, area_threshold, ratio_treshold):
    matrix_world = object.matrix_world
    bm = mesh_to_bmesh(object.data)
    bm.select_mode = {'FACE'}
    bm_tris = bm.copy()
    orig_index = store_face_indices(bm_tris)
    bmesh.ops.triangulate(bm_tris,
            faces = bm_tris.faces,
            quad_method = 'BEAUTY',
            ngon_method = 'BEAUTY')
    found_indices = []
    for tri in bm_tris.faces:
        if legacy_mode and is_degenerated_legacy(tri, matrix_world = matrix_world):
            found_indices.append(tri[orig_index])
        elif not legacy_mode and is_degenerated(tri, area_threshold, ratio_treshold, matrix_world = matrix_world):
            found_indices.append(tri[orig_index])
    found_indices = list(set(found_indices))
    del bm_tris

    bm_unhide_deselect_all(bm)
    deg_faces = [f for f in bm.faces if f.index in found_indices]
    if destructive:
        count = 0
        updated_bm = bmesh.ops.triangulate(bm,
            faces = deg_faces,
            quad_method = 'BEAUTY',
            ngon_method = 'BEAUTY')
        for tri in updated_bm['faces']:
            if legacy_mode:
                is_deg = is_degenerated_legacy(tri, matrix_world = matrix_world)
            else:
                is_deg = is_degenerated(tri, area_threshold, ratio_treshold, matrix_world = matrix_world)
            tri.select_set(is_deg)
            if is_deg:
                count +=1
    else:
        bm_select_elements(deg_faces)
        count = deg_faces.__len__()
    bmesh_to_mesh(bm, object.data)
    return count