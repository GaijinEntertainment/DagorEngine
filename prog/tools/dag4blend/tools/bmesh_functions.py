import bpy
import bmesh

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


def bm_select_elements(elements):
    #works only after bm.<domain>.ensure_lookup_table()
    for el in elements:
        el.select_set(True)
    return


def bm_deselect_elements(elements):
    #works only after bm.<domain>.ensure_lookup_table()
    for el in elements:
        el.select_set(False)
    return

def bm_hide_elements(elements):
    for el in domain:
        el.hide_set(True)
    return

def bm_unhide_elements(domain):
    for el in domain:
        el.hide_set(False)
    return

def bm_deselect_all(bm):
    domains = [bm.verts, bm.edges, bm.faces]
    for domain in domains:
        for el in domain:
            el.select_set(False)
    return

def bm_unhide_deselect_all(bm):
    for domain in [bm.verts, bm.edges, bm.faces]:
        domain.ensure_lookup_table()
        bm_deselect_elements(domain)
        bm_unhide_elements(domain)
    return
