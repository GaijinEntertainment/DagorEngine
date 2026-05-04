from re import search
import bpy
from ..helpers.props        import fix_type, get_property_type, dagormat_prop_add, prop_value_to_string

def dagormat_reset(mat):
    DM=mat.dagormat
#textures
    for tex in DM.textures.keys():
        DM.textures[tex]=''
#optional
    params=[]
    for par in DM.optional.keys():
        params.append(par)
    for par in params:
        del DM.optional[par]
#main
    DM.sides = 0
    DM.shader_class = ''
    return

# TEXT TO DAGORMAT #####################################################################################################

#expects 'script:t="prop_name=prop_value"', converts prop_value from string, makes props[prop_name]=prop_value_converted
def dagormat_prop_from_string(dagormat, string):
    found = search('^ *script *: *t *= *"', string)
    if found is None:
        return
    param = string[found.span()[1]:]
    if param.find('"') == -1:
        return  # script was never closed. TODO: add warning
    param = param[:param.find('"')]
    name_value = param.split('=')
    if name_value.__len__() > 2:
        return  # something is wrong. TODO: add warning
    name = name_value[0].replace(' ', '') # no spaces allowed
    value = name_value[1]  # technically can have spaces
    dagormat_prop_add(dagormat, name, value)
    return

def dagormat_props_from_text(dagormat, text):
    for line in text.lines:
        string = line.body
        dagormat_prop_from_string(dagormat, string)
    return

def dagormat_from_text(mat,text):
    dagormat_reset(mat)
    dagormat=mat.dagormat
# main parameters and textures
    for line in text.lines:
        line=line.body
        line=line.replace('"','')
        line=line.replace(' ','')
        if line == "twosided:b=yes":
            dagormat.sides = 1
        elif line.startswith('tex16support:b='):
            pass#always yes for modern assets
        elif line.startswith('class:t='):
            dagormat.shader_class=line.replace('class:t=','')
        elif line.startswith('tex'):
            tex=line.split(':t=')
            dagormat.textures[tex[0]]=tex[1]
# optional parameters parsed separately, when shader_class is updated
    dagormat_props_from_text(dagormat, text)
    return

# DAGORMAT TO TEXT #####################################################################################################

# writes non-optional parameters
def dagormat_main_to_string(dagormat):
    string = f'class:t="{dagormat.shader_class}"\n'
    string += 'tex16support:b=yes\n'  # always
    if dagormat.sides == 1:
        string += 'twosided:b=yes\n'
    else:
        string += 'twosided:b=no\n'
    return string

#writes textures to text
def dagormat_textures_to_string(textures):
    string = ""
    for key in textures.keys():
        if textures[key] != "":
            string += f'\n{key}:t="{textures[key]}"'
    if string.__len__() > 0:
        string += '\n'
    return string

def dagormat_prop_to_string(props, prop_name):
    prop_type = get_property_type(props, prop_name)
    prop_str = prop_value_to_string(props[prop_name], prop_type)
    string = f'script:t="{prop_name}={prop_str}"'
    return string

def dagormat_properties_to_string(props):
    string = ""
    if props.keys().__len__() == 0:
        return string
    for prop_name in props.keys():
        string += '\n'
        string += dagormat_prop_to_string(props, prop_name)
    string += '\n'
    return string


def dagormat_to_string(dagormat):
    dagormat_string = dagormat_main_to_string(dagormat)
    dagormat_string += dagormat_textures_to_string(dagormat.textures)
    dagormat_string += dagormat_properties_to_string(dagormat.optional)
    return dagormat_string


def dagormat_to_text(material, text):
    text.clear()
    dagormat = material.dagormat
    dagormat_string = dagormat_to_string(dagormat)
    text.write(dagormat_string)
    text.select_set(0,0,0,0)  # cursor to the beginning of the first line, so it won't be scrolled down
    return