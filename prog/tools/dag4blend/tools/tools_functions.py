import bpy
from bpy.utils  import register_class, unregister_class, user_resource
import bmesh
from math import dist

from ..helpers.version      import get_blender_version
from ..helpers.map_range    import map_range
from ..helpers.texts        import log


DYNAMIC_DEFORMED_DEFAULTS ={"diffuse_tex_scale":1.0,
                            "normals_tex_scale":1.0,
                            "diffuse_power":    1.0,
                            "normals_power":    1.0,
                            "springback":       0.05,
                            "expand_atten":     0.5,
                            "expand_atten_pow": 0.5,
                            "noise_scale":      1.0,
                            "noise_power":      1.3,
                            "crumple_rnd":      0.5,
                            "crumple_force":    0.1,
                            "crumple_dist":     0.5,
                            }


#returns gi_black material if exists or creates new
def get_gi_black():
    for mat in bpy.data.materials:
        if mat.dagormat.shader_class == 'gi_black':
            return mat
    mat = bpy.data.materials.new('gi_black')
    mat.dagormat.shader_class = 'gi_black'
    return mat

# makes sure that all vcol stored in face corner instead of vertices
def color_attributes_fix_domain(object):
    with bpy.context.temp_override(object = object):
        attr_names = [attr.name for attr in object.data.attributes]
        for name in attr_names:
            attr = object.data.attributes.get(name)
            if attr.domain == 'CORNER':
                continue
            if attr.data_type not in ['FLOAT_COLOR', 'BYTE_COLOR']:
                continue
            object.data.attributes.active = attr
            bpy.ops.geometry.attribute_convert(domain='CORNER', data_type = attr.data_type)
    return

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
        try:
            # 4.2 does not have this parameter and always keeps EXISTING normals, 4.2.1 was reversed for some reason
            mod.keep_custom_normals = True
        except:
            pass
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

#shows the preview of deformatins that stored in VCol
def preview_vcol_deformation(object):
    srgb_deform = object.data.color_attributes[0]
    domain = srgb_deform.domain
    vector_deform = object.data.attributes.get("vector_deform")
    if vector_deform is not None:
        object.data.attributes.remove(vector_deform)
    vector_deform = object.data.attributes.new("vector_deform", 'FLOAT_VECTOR', domain)
    for index in range(0, srgb_deform.data.__len__()):
        vector_deform.data[index].vector = srgb_deform.data[index].color_srgb[:-1]
    gn_node = get_preview_node()
    try:
        for key in object.data.shape_keys.key_blocks:
            key.value = 0
    except:
        pass
    modifier = object.modifiers.new("GN_dynamic_deformed_preview", 'NODES')
    modifier.node_group = get_preview_node()
    modifier['Socket_2_use_attribute'] = True
    modifier['Socket_2_attribute_name'] = 'vector_deform'
    max_height = 1.0
    try:
        max_height = object.data.materials[0].dagormat.optional['max_height']
    except:
        pass
    modifier["Socket_3"] = max_height
    return

# stores vert offsets of first shape key into color attribute named "DEFORM"
# writes multiplier value to the shader
def shapekey_to_color_attribute(object, configure_shaders = False, preview_deformation = False):
    log_message = ""
    mesh = object.data
    max_dist = 0
    coords_base = [vert.co for vert in mesh.vertices]

    shape_key = mesh.shape_keys.key_blocks[1]

    coords_moved = [vert.co for vert in shape_key.data]
    coords_offset = [[0, 0, 0, 1] for i in range(mesh.vertices.__len__())]
    for i in range(coords_base.__len__()):

        cur_distance = dist(coords_base[i], coords_moved[i])

        x_diff = coords_base[i][0] - coords_moved[i][0]
        y_diff = coords_base[i][1] - coords_moved[i][1]
        z_diff = coords_base[i][2] - coords_moved[i][2]

        coords_offset[i] = (x_diff, y_diff, z_diff)

        x_diff = abs(x_diff)
        y_diff = abs(y_diff)
        z_diff = abs(z_diff)

        if cur_distance > max_dist:
            max_dist = cur_distance

    if mesh.attributes.get("DEFORM") is not None:
        mesh.attributes.remove(mesh.attributes["DEFORM"])
    color = mesh.attributes.new("DEFORM", 'FLOAT_COLOR', 'POINT')
    for i in range(mesh.vertices.__len__()):
        color.data[i].color_srgb[0] = map_range(coords_offset[i][0],  -max_dist,max_dist,  0.0,1.0)
        color.data[i].color_srgb[1] = map_range(coords_offset[i][1],  -max_dist,max_dist,  0.0,1.0)
        color.data[i].color_srgb[2] = map_range(coords_offset[i][2],  -max_dist,max_dist,  0.0,1.0)

    mesh.color_attributes.active_color = mesh.color_attributes["DEFORM"]

    message = f'\n\tObject "{object.name}" processed. "max_height" is {max_dist*2}\n'
    log(message, type = "INFO")
    if configure_shaders:
        new_mat = bpy.data.materials.new("")
        if mesh.materials.__len__() == 0:
            mesh.materials.append(new_mat)
            message = f'Object "{object.name}" have no material to configure! Creating new...\n'
            log(message, type = 'WARNING', show = True)

        for slot in object.material_slots:
            if slot.material is None:
                slot.material = new_mat
            material = slot.material
            material.dagormat.shader_class = "dynamic_deformed"
            material.dagormat.optional['max_height'] = max_dist * 2
            for key in DYNAMIC_DEFORMED_DEFAULTS.keys():
                material.dagormat.optional[key] = DYNAMIC_DEFORMED_DEFAULTS[key]
            message = f'Material(s) of "{object.name}" successfully pre-configured\n'
            log(message, type = 'INFO')
        if new_mat.users == 0:
            bpy.data.materials.remove(new_mat)
    if preview_deformation:
        preview_vcol_deformation(object)
    return

def get_preview_node():
    group_name = 'GN_dynamic_deformed_preview'
    node_group = bpy.data.node_groups.get(group_name)
    if node_group is not None:
        return node_group
    lib_path = user_resource('SCRIPTS') + f'/addons/dag4blend/extras/library.blend/NodeTree'
    file = lib_path+f"/{group_name}"
    bpy.ops.wm.append(filepath = file, directory = lib_path,filename = group_name, do_reuse_local_id = True)
    #if nodegroup not found in library it still be a None
    node_group = bpy.data.node_groups.get(group_name)
    return node_group