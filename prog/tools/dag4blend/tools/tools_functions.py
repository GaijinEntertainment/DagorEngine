import bpy, bmesh

#returns gi_black material if exists or creates new
def get_gi_black():
    for mat in bpy.data.materials:
        if mat.dagormat.shader_class == 'gi_black':
            return mat
    mat = bpy.data.materials.new('gi_black')
    mat.dagormat.shader_class = 'gi_black'
    return mat

#MESHCHECK
#checks if mesh has degenerated triangles
#and (optionally) removes them
def mesh_verify_fix_degenerates(mesh, fix = True):
    msg = None
    bm = bmesh.new()
    bm.from_mesh(mesh)
    og_count = [bm.verts.__len__(), bm.edges.__len__(), bm.faces.__len__()]
    bmesh.ops.dissolve_degenerate(bm, dist = 0.0001, edges = bm.edges)
    if fix:
        bm.to_mesh(mesh)
    #if count changed, then some parts was removed as "bad geometry"
    if og_count != [bm.verts.__len__(), bm.edges.__len__(), bm.faces.__len__()]:
        msg = f"{og_count[2]-bm.faces.__len__()} degenerated triangles"
        if fix:
            msg+=" removed"
    del bm
    return msg

#checks if mesh has loose geometry
#and (optionally) removes it
def mesh_verify_fix_loose(mesh, fix = True):
    msg = None
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.verts.ensure_lookup_table()
    verts_to_delete = [vert for vert in bm.verts if vert.link_faces.__len__()==0]
    if verts_to_delete.__len__()>0:
        msg = f"{verts_to_delete.__len__()} loose vertices!"
    if fix and verts_to_delete.__len__()>0:
        bmesh.ops.delete(bm, geom = verts_to_delete, context = 'VERTS')
    bm.to_mesh(mesh)
    del bm
    return msg

#Checks if mesh have flat_shaded face(s) or not
def is_mesh_smooth(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bm.faces.ensure_lookup_table()
    for face in bm.faces:
        if not face.smooth:
            del bm
            return False
    del bm
    return True

#Checks if mesh lacks of faces
def is_mesh_empty(mesh):
    if mesh.polygons.__len__()==0:
        return True
    return False

#Triangulates mesh
def triangulate(mesh):
#slower hack that preserves custom normals
    if mesh.has_custom_normals:
        temp_mesh = bpy.data.meshes.new("")
    #can't apply mods to mesh with multiple users, so current mesh temporary replaced by empty one
    #to make sure that current mesh has only one user
        mesh.user_remap(temp_mesh)
    #can't apply modifier to the mesh directly, we also need temp object
        temp_obj = bpy.data.objects.new("", mesh)
        mod = temp_obj.modifiers.new("", 'TRIANGULATE')
        mod.quad_method = 'BEAUTY'
        mod.ngon_method = 'BEAUTY'
        mod.keep_custom_normals = True
        with bpy.context.temp_override(object = temp_obj):
            bpy.ops.object.modifier_apply(modifier=mod.name)
    #returning our mesh to its owners after triangulation
        temp_mesh.user_remap(mesh)
        bpy.data.objects.remove(temp_obj)
        bpy.data.meshes.remove(temp_mesh)
#faster alorithm, but can't preserve custom normals
    else:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces = bm.faces, quad_method = 'BEAUTY', ngon_method = 'BEAUTY')
        bm.to_mesh(mesh)
        del bm
    return

def is_matrix_ok(matrix):
    scale = matrix.to_scale()
    inverted = 0
    zero = 0
    for el in scale:
        if el<0:
            inverted+=1
        elif el == 0:
            zero +=1
    msg = ""
    if inverted>0:
        msg+=f" {inverted} axis have negative scale."
    if zero>0:
        msg+=f" {zero} axis scaled to 0."
    if msg!="":
        return msg
    return True

#sort uvs alphabetically
def reorder_uv_layers(mesh):
    uvs = mesh.uv_layers
    og_uvs = []
    #in case object has UVs with non-standart name
    for name in ['UVMap','UVMap.001', 'UVMap.002']:
        if uvs.get(name) is not None:
            og_uvs.append(name)
    #every other UV layer placed after coorect three, but also sorted
    names = uvs.keys()
    names.sort()
    for name in names:
        if name not in og_uvs:
            og_uvs.append(name)
    #changing order
    for name in og_uvs:
        old = uvs.get(name)
        uvs.active = old
        new = uvs.new()
        uvs.remove(uvs[name])
        new.name = name
    print(og_uvs)
    return