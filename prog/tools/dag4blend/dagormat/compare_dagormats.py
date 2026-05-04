import bpy, os
from ..helpers.names    import ensure_no_extension, texture_basename
from ..helpers.getters  import get_preferences
from ..helpers.props    import prop_value_to_string, fix_type
from ..constants        import DAGTEXNUM


def _get_shader_class(material):
    dagormat = material.dagormat
    if dagormat.is_proxy:
        proxymat_basename = ensure_no_extension(material.name)
        return proxymat_basename + ":proxymat"
    elif dagormat.shader_class.endswith(":proxymat"):
        dagormat.is_proxy = True  # it's dictated by shader class
    return dagormat.shader_class

def _get_prop_value_corrected(prop_name, prop_owner, known_prop_parameters):
    value = prop_owner.get(prop_name)
    raw_value = prop_owner.get(prop_name)
    if known_prop_parameters is None:
        return raw_value
    if raw_value is None:
        raw_walue = known_prop_parameters.get('default')
    prop_type = known_prop_parameters.get('type')
    str_value = prop_value_to_string(raw_value, prop_type)
    corrected_value = fix_type(str_value, prop_type)
    return corrected_value

def compare_dagormats(material_a, material_b):
    if material_a.is_grease_pencil or material_b.is_grease_pencil:
        return False
    shader_a = _get_shader_class(material_a)
    shader_b = _get_shader_class(material_b)
    dagormat_a = material_a.dagormat
    dagormat_b = material_b.dagormat
#proxymats
    if dagormat_a.is_proxy != dagormat_b.is_proxy:
        return False
    if dagormat_a.is_proxy == True:  # then dagormat_b.is_proxy == True as well
        return shader_a == shader_b
# non-proxymats main parameters
    elif shader_a != shader_b:
        return False
    if dagormat_a.sides != dagormat_b.sides:
        return False
#textures
    textures_a = dagormat_a.textures
    textures_b = dagormat_b.textures
    for index in range(DAGTEXNUM):
        texture_a = getattr(textures_a, f"tex{index}")
        texture_b = getattr(textures_b, f"tex{index}")
        if texture_basename(texture_a) != texture_basename(texture_b):
            return False
#optional parameters
    parameters_a = dagormat_a.optional
    parameters_b = dagormat_b.optional
    keys_a = list(parameters_a.keys())
    keys_b = list(parameters_b.keys())
    keys_joined = list(set(keys_a + keys_b))
    pref = get_preferences()
    current_shader_config = pref.shaders.get(dagormat_a.shader_class)
    if current_shader_config is None:
        current_shader_config = []
    for key in keys_joined:
        known_prop_parameters = current_shader_config.get(key)
        if known_prop_parameters is None:
            known_prop_parameters = {}
        value_a = _get_prop_value_corrected(key, parameters_a, known_prop_parameters)
        value_b = _get_prop_value_corrected(key, parameters_b, known_prop_parameters)
        if value_a != value_b:
            return False
#        if current_shader_config is None:  # types are unknown, default values are unknown
    return True