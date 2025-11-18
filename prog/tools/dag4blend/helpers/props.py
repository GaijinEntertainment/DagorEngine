import bpy

from .getters               import get_preferences

# returns a string version of a prop value
def prop_value_to_string(prop_value, prop_type):
    try:
# bool
        if prop_type == "bool":
            prop_str = 'yes' if prop_value else 'no'
# matrix
        elif prop_type == 'm':
            m = prop_value
            prop_str = f'[[{m[0]}, {m[1]}, {m[2]}] [{m[3]}, {m[4]}, {m[5]}] [{m[6]}, {m[7]}, {m[8]}] [{m[9]}, {m[10]}, {m[11]}]]'
# integer array
        elif prop_type in ['ip2', 'ip3']:
            prop_strings = []
            for value in prop_value:
                prop_strings.append(str(int(value)))  # forsed conversion in case it was float
            prop_str = ", ".join(prop_strings)
# float array
        elif prop_type in ['p2', 'p3', 'p4', 'color']:
            prop_strings = []
            for value in prop_value:
                float_value = float(value)  # in case it was int
                prop_strings.append (str(round(float_value, 5)))  # 1e-5 is AV precision, smaller values do not matter
            prop_str = ", ".join(prop_strings)
# other
        else:
            prop_str = f'{prop_value}'
    except:  # smart conversion failed for some reason, so let's just use simple one
        prop_str = f'{prop_value}'
    return prop_str

# STRING TO <TYPE> CONVERTERS ##########################################################################################

def to_text(str):
    return (str, None)

def to_int(str):
    value = str
    message = None
    try:
        float_value = float(str)
        value = int(float_value)
        if value != float_value:
            message = f'Type mismatch resolved! "{str}" converted to integer as {value}\n'
    except:
        message = f'Can not convert "{str}" to integer!\n'
    return (value, message)

def to_bool(str):
    value = str.lower()
    message = None
    if value in ['1', 'yes', 'true']:
        value = True
    elif value in ['0', 'no', 'false']:
        value = False
    else:
        message = f'Can not convert "{str}" to boolean!\n'
        value = False
    return (value, message)

def to_color(str):
    value = str
    message = None
    values = value.split(",")
    if values.__len__() == 4:
        try:
            value = [float(values[0]), float(values[1]), float(values[2]), float(values[3])]
        except:
            message = f'Can not convert "{str}" to RGBA color!\n'
            value = str
    else:
        message = f'Can not convert "{str}" to RGBA color!\n'
        value = str
    return (value, message)

def to_matrix(str):
    message = None
    values_str = str
    values_str = values_str.replace(' ', '')
    values_str = values_str.replace('][', ',')  # to split cmp-like matrix properly
    for char in ['\t', '\n', '[', ']']:
        values_str = values_str.replace(char, '')
    values_str = values_str.split(',')
    if values_str.__len__() != 12:
        message = f'Can not convert "{str}" to transformation marix!\n'
        return (str, message)
    values = []
    for value in values_str:
        try:
            values.append(float(value))
        except:
            message = f'Can not convert "{str}" to transformation marix!\n'
            return (str, message)
    return (values, message)

def to_float(str):
    value = str
    message = None
    try:
        value = float(str)
    except:
        message = f'Can not convert "{str}" to float!\n'
    return (value, message)

def to_float_vector2(str):
    value = str
    message = None
    values = value.split(",")
    if values.__len__() == 2:
        try:
            value = [float(values[0]), float(values[1])]
        except:
            message = f'Can not convert "{str}" to 2D float vector!\n'
    else:
        message = f'Can not convert "{str}" to 2D float vector!\n'
    return (value, message)

def to_float_vector3(str):
    value = str
    message = None
    values = value.split(",")
    if values.__len__() == 3:
        try:
            value = [float(values[0]), float(values[1]), float(values[2])]
        except:
            message = f'Can not convert "{str}" to 3D float vector!\n'
    else:
        message = f'Can not convert "{str}" to 3D float vector!\n'
    return (value, message)

def to_float_vector4(str):
    value = str
    message = None
    values = value.split(",")
    if values.__len__() == 4:
        try:
            value = [float(values[0]), float(values[1]), float(values[2]), float(values[3])]
        except:
            message = f'Can not convert "{str}" to 4D float vector!\n'
    else:
        message = f'Can not convert "{str}" to 4D float vector!\n'
    return (value, message)

def to_int_vector2(str):
    value = str
    message = None
    values = value.split(",")
    if values.__len__() == 2:
        try:
            float_value = [float(values[0]), float(values[1])]
            value = [int(float_value[0]), int(float_value[1])]
            if value != float_value:
                message = f'Type mismatch resolved! "{str}" converted to integer as {value}\n'
        except:
            message = f'Can not convert "{str}" to 2D integer vector!\n'
    else:
        message = f'Can not convert "{str}" to 2D integer vector!\n'
    return (value, message)

def to_int_vector3(str):
    value = str
    message = None
    values = value.split(",")
    if values.__len__() == 3:
        float_value = [float(values[0]), float(values[1]), float(values[2])]
        value = [int(float_value[0]), int(float_value[1]), int(float_value[2])]
        if value != float_value:
            message = f'Type mismatch resolved! "{str}" converted to integer as {value}\n'
    else:
        message = f'Can not convert "{str}" to 3D integer vector!\n'
    return (value, message)

# EASY WAY TO CHOOSE A CONVERTER #######################################################################################
type_converters = {
    't'     :to_text,
    'i'     :to_int,
    'b'     :to_bool,
    'm'     :to_matrix,
    'r'     :to_float,
    'p2'    :to_float_vector2,
    'p3'    :to_float_vector3,
    'p4'    :to_float_vector4,
    'ip2'   :to_int_vector2,
    'ip3'   :to_int_vector3,
    }

# type is not defined, guessing by string
def guess_type_convert(str):
    value = ''
# matrix ?
    if str.find('[') > -1:
        value = to_matrix(str)[0]
# bool ?
    elif str.lower() in ['1', 'yes', 'true']:
        value = True
    elif str.lower() in ['0', 'no', 'false']:
        value = False
# vector ?
    elif str.find(',')>-1:
        is_float = str.find('.') > -1
        split_str = str.split(',')
        try:
            value = []
            for el in split_str:
                if is_float:
                    value.append(float(el))
                else:
                    value.append(int(el))
        except:
            value = str
# float ?
    elif  str.find('.')>-1:
        try:
            value = float(str)
        except:
            value = str
# int ?
    else:
        try:
            value = int(str)
        except:
            value = str.replace('"','')
    return value

# trying to set a specific type if possible
def fix_type(str, expected_type = None):
    if expected_type != None and type_converters[expected_type] is None:
        print(f'Error! Unknown type: "{expected_type}"')
        #  TODO: better warn
        expected_type = None
# converting to specific type
    if expected_type is not None:
        value, message = type_converters[expected_type](str)
        if message is not None:
            print(f'Error! "{message}"')
        return value
        # TODO: better warn
# guessing type
    return guess_type_convert(str)

# type check ###########################################################################################################

type_str_remap = {
    'str'       :'t',
    'int'       :'i',
    'bool'      :'b',
    'float'     :'r',
    'float2'    :'p2',
    'float3'    :'p3',
    'float4'    :'p4',
    'float12'   :'m',
    'int2'      :'ip2',
    'int3'      :'ip3',
    }

def get_property_type(prop_owner, prop_name):
    prop = prop_owner[prop_name]
    type = prop.__class__.__name__
    if type == 'IDPropertyArray':
        length = prop.__len__()
        element_type = prop[0].__class__.__name__
        type = f'{element_type}{length}'
    type_corrected = type_str_remap.get(type)
    if type_corrected is not None:
        type = type_corrected
    return type


# default values #######################################################################################################

default_values = {
    't'     :"",
    'i'     :"0",
    'b'     :"yes",
    'm'     :"[[1.0, 0.0, 0.0] [0.0, 1.0, 0.0] [0.0, 0.0, 1.0] [0.0, 0.0, 0.0]]",
    'r'     :"0.0",
    'p2'    :"0.0, 0.0",
    'p3'    :"0.0, 0.0, 0.0",
    'p4'    :"0.0, 0.0, 0.0, 0.0",
    'ip2'   :"0, 0",
    'ip3'   :"0, 0, 0",
    }

def get_default_shader_prop_value(shader_name, prop_name):
    prefs = get_preferences()
    shaders = prefs.shaders
    shader = shaders.get(shader_name)
    if shader is None:
        return "unknown"
    prop = shader['props'].get(prop_name)
    if prop is None:
        return "unknown"
    default_value = prop.get('default')
    if default_value is None:
        default_value = default_values[prop['type']]
    return default_value

def get_shader_prop_parameters(shader_name, prop_name):
    prefs = get_preferences()
    shader = prefs.shaders.get(shader_name)
    if shader is None:
        return {}
    prop = shader['props'].get(prop_name)
    if prop is None:
        return{}
    parameters = dict(prop)
    return parameters


# Creation / Removal ###################################################################################################

def dagormat_prop_add(dagormat, prop_name, prop_value):
    prop_parameters = get_shader_prop_parameters(dagormat.shader_class, prop_name)
    if prop_parameters is None:
        prop_parameters = {}
    add_custom_prop(dagormat.optional, prop_name, prop_value, prop_parameters)
    return


def add_custom_prop(owner, name, value_string, parameters):
#validation
    for char in [" ", "\t", "\n"]:  # removing unsupported characters, that could be parsed accidentally
        name = name.replace(char, "")
    if name.__len__() == 0:
        return 'Prop name can not be set to "{prop_name}"!\n'
# prop creation
    try_remove_custom_prop(owner, name)  # to make sure that there's no UI with altered settings
    type = parameters.get('type')
    value = fix_type(value_string, expected_type = type)
    owner[name] = value
#optional parameters
    warnings = []
    default = parameters.get('default')
    if default is not None:
        try:
            owner.id_properties_ui(name).update(default = default)
        except:
            warnings.append("can't set a default value!")

# custom UI
    custom_ui  = parameters.get('custom_ui')
    if custom_ui == 'color':
        try:
            owner.id_properties_ui(name).update(subtype = 'COLOR')
        except:
            warnings.append("can't set a subtype!")

#expected range
    soft_min = parameters.get('soft_min')
    if soft_min is not None:
        try:
            owner.id_properties_ui(name).update(soft_min = soft_min)
        except:
            warnings.append("can't set a min value")

    soft_max = parameters.get('soft_max')
    if soft_max is not None:
        try:
            owner.id_properties_ui(name).update(soft_max = soft_max)
        except:
            warnings.append("can't set a max value")
# description
    description = parameters.get('description')
    if description is None:
        description = ""
    if soft_min is not None or soft_max is not None:
        if soft_min is None:
            soft_min = "- inf"
        if soft_max is None:
            soft_max = "inf"
        description += f"; Expected range: [{soft_min} .. {soft_max}]"
    if description != "":
        try:
            owner.id_properties_ui(name).update(description = description)
        except:
            warnings.append("can't set a description!")
    if warnings.__len__() > 0:
        warning = f'WARNING! Some parameters of "{name}" property were not set:\n\t'
        warning += "\n\t".join(warnings)
        warning += '\n'
        print(warning)
        return warning
    return

def try_remove_custom_prop(prop_owner, name):
    if name not in prop_owner.keys():
        return
    del prop_owner[name]
    return