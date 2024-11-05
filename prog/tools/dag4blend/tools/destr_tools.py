import bpy
from .helpers.map_range import map_range

def shapekey_to_vcol(object, shapekey_name):
    mesh = object.data
    max_dist = 0
    coords_base = [vert.co for vert in mesh.vertices]

    shape_key = mesh.shape_keys.key_blocks.get(shapekey_name)
# TODO: reacto to None
    coords_moved = [vert.co for vert in shape_key.data]
    coords_offset = [[0, 0, 0, 1] for i in range(mesh.vertices.__len__())]
    for i in range(coords_base.__len__()):

        cur_distance = dist(coords_base[i], coords_moved[i])

# comparing X, Y and Z separately gives a bit higher precision than distance between vectors
        x_diff = coords_base[i][0] - coords_moved[i][0]
        y_diff = coords_base[i][1] - coords_moved[i][1]
        z_diff = coords_base[i][2] - coords_moved[i][2]

        coords_offset[i] = (x_diff, y_diff, z_diff)

        x_diff = abs(x_diff)
        y_diff = abs(y_diff)
        z_diff = abs(z_diff)

        if cur_distance > max_dist:
            max_dist = cur_distance

# blender automatically applies gamma correction to color attributes,
# but vector-to-color conversion keeps values intact
    if mesh.attributes.get("DEFORM") is not None:
        mesh.attributes.remove(mesh.attributes['DEFORM'])
    raw_color = mesh.attributes.new("DEFORM", 'FLOAT_VECTOR', 'POINT')
    for i in range(mesh.vertices.__len__()):

        raw_color.data[i].vector[0] = map_range(coords_offset[i][0],  -max_dist,max_dist,  0.0,1.0)
        raw_color.data[i].vector[1] = map_range(coords_offset[i][1],  -max_dist,max_dist,  0.0,1.0)
        raw_color.data[i].vector[2] = map_range(coords_offset[i][2],  -max_dist,max_dist,  0.0,1.0)

    with bpy.context.temp_override(object = object):
        mesh.attributes.active = raw_color
        bpy.ops.geometry.attribute_convert(mode = 'GENERIC', domain = 'POINT', data_type = 'FLOAT_COLOR')
    mesh.color_attributes.active_color = mesh.color_attributes[0]

    if mesh.materials.__len__() == 0:
        new_mat = bpy.data.materials.new("")
        mesh.materials.append(new_mat)
    for material in mesh.materials:
        material.dagormat.shader_class = "dynamic_deformed"
        material.dagormat.optional['max_height'] = max_dist * 2
    return
