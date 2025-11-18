from os.path import abspath, exists

from ..blk.blk_reader   import parse_blk
from ..helpers.getters  import get_preferences, get_addon_directory
from ..helpers.props    import fix_type

def read_param(parameter_block, parameters_group = None):
    data = {}
    parameters = parameter_block.parameters
    for key in parameters.keys():
        if key == "name":
            continue
        raw_value = parameters[key]['value']
        type = parameters[key]['type']

        data[key] = fix_type(raw_value, expected_type = type)
    name = parameters['name']['value']
    if parameters_group is not None:
        data['property_group'] = parameters_group
    return [name, data]

def clear_shader_config():
    pref = get_preferences()
    for key in list(pref.shaders.keys()):
        del pref.shaders[key]
    for key in list(pref.shader_categories.keys()):
        del pref.shader_categories[key]
    return

def read_shader_config():
    pref = get_preferences()
    blk_path = pref.shader_config_path
    if not exists(blk_path):
        print("WARNING! Can't find custom shader config, using default one")
        blk_path = get_addon_directory() + '/dagorShaders.blk'
    shaders_datablock = parse_blk(blk_path)
    clear_shader_config()
    shaders = pref.shaders
    categories = pref.shader_categories
    for category_block in shaders_datablock.datablocks:
        category_name = category_block.parameters['name']['value']
        category = {'shaders':[]}
        for key in category_block.parameters.keys():
            if key == 'name':
                continue
            category[key] = category_block.parameters[key]['value']
        for shader_block in category_block.datablocks:
            shader = {}
            shader['category'] = category_name
            shader_name = shader_block.parameters['name']['value']
            category['shaders'].append(shader_name)
            for key in shader_block.parameters.keys():
                if key == 'name':
                    continue
                shader[key] = shader_block.parameters[key]['value']
            props = {}
            prop_groups = {}
            for datablock in shader_block.datablocks:
                if datablock.type == 'parameter':
                    name, data = read_param(datablock)
                    props[name] = data
                elif datablock.type == 'parameters_group':
                #proprties of current group itself - name, description, etc.
                    prop_group_name, prop_group_data = read_param(datablock)
                    prop_groups[prop_group_name] = prop_group_data
                #shader properties inside of current group
                    for param_block in datablock.datablocks:
                        name, data = read_param(param_block, prop_group_name)
                        props[name] = data
            shader['props'] = props
            shader['prop_groups'] = prop_groups
            shaders[shader_name] = shader
        categories[category_name] = category
    return