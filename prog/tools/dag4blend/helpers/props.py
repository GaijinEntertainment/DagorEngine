import bpy

def fix_type(str):
    value=''
    len=str.__len__()
    if str.find(',')>-1:
        sep_str=str.split(',')
        try:
            value=[]
            for el in sep_str:
                value.append(float(el))
        except:
            value=str
    elif  str.find('.')>-1:
        try:
            value=float(str)
        except:
            value=str
    else:
        try:
            value=int(str)
        except:
            value=str.replace('"','')
    return value

def valid_prop(line):
    line=line.replace(' ','')
    prop=line.split('=')
    if prop.__len__()!=2:
        return False
    if prop[0].find(':')==-1:
        return False
    if prop[0].endswith(':b') and prop[1] not in ['yes','no']:
        return False
    if prop[0].endswith('3') and prop[1].split(',').__len__()!=3:
        return False
    return True