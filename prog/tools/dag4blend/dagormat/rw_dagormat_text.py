import bpy
from ..helpers.props            import fix_type

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
    DM.shader_class = 'rendinst_simple'

def dagormat_from_text(mat,text):
    dagormat_reset(mat)
    DM=mat.dagormat
    for line in text.lines:
        line=line.body
        line=line.replace('"','')
        line=line.replace(' ','')
        if line == "twosided:b=yes":
            DM.sides = 1
        elif line.startswith('tex16support:b='):
            pass
        elif line.startswith('power:r='):
            pass
        elif line.startswith('class:t='):
            DM.shader_class=line.replace('class:t=','')
        elif line.startswith('diff:ip3='):
            attr=line[9:].split(',')
            value=[]
            for el in attr:
                value.append(float(el)/255)
            DM.diffuse=value
        elif line.startswith('spec:ip3='):
            attr=line[9:].split(',')
            value=[]
            for el in attr:
                value.append(float(el)/255)
            DM.specular=value
        elif line.startswith('emis:ip3='):
            attr=line[9:].split(',')
            value=[]
            for el in attr:
                value.append(float(el)/255)
            DM.emissive=value
        elif line.startswith('amb:ip3='):
            attr=line[8:].split(',')
            value=[]
            for el in attr:
                value.append(float(el)/255)
            DM.ambient=value
        elif line.startswith('power:r='):
            DM.power=float(line[8:])
        elif line.startswith('script:t='):
            opt=line[9:].split('=')
            if opt[0]=="real_two_sided":
                DM.sides = 2 if opt[1] == 'yes' else '0'
            else:
                DM.optional[opt[0]]=fix_type(opt[1])
        elif line.startswith('tex'):
            tex=line.split(':t=')
            DM.textures[tex[0]]=tex[1]

def dagormat_to_text(mat,text):
    DM=mat.dagormat
    text.clear()
    text.write('  class:t="'+DM.shader_class+'"')
    text.write('\n  twosided:b=')
    if DM.sides == 1:
        text.write('yes\n')
    elif DM.sides == 2:
        text.write('\n  script:t="real_two_sided=yes"\n')
    else:
        text.write('no\n')
    text.write('\n  tex16support:b=yes')
    text.write('\n  power:r=32')
    text.write('\n  amb:ip3=')
    text.write(str(int(DM.ambient[0]*255))+',')
    text.write(str(int(DM.ambient[1]*255))+',')
    text.write(str(int(DM.ambient[2]*255)))
    text.write('\n  diff:ip3=')
    text.write(str(int(DM.diffuse[0]*255))+',')
    text.write(str(int(DM.diffuse[1]*255))+',')
    text.write(str(int(DM.diffuse[2]*255)))
    text.write('\n  spec:ip3=')
    text.write(str(int(DM.specular[0]*255))+',')
    text.write(str(int(DM.specular[1]*255))+',')
    text.write(str(int(DM.specular[2]*255)))
    text.write('\n  emis:ip3=')
    text.write(str(int(DM.emissive[0]*255))+',')
    text.write(str(int(DM.emissive[1]*255))+',')
    text.write(str(int(DM.emissive[2]*255)))
    for param in list(DM.optional.keys()):
        text.write('\n  script:t="'+param+'=')
        try:
            values = DM.optional[param].to_list()
            value = ''
            for el in values:
                value+=f'{round(el,7)}, '
            value = value[:-2]
        except:
            try:
                value = f'{round(DM.optional[param],7)}'
            except:
                value=str(DM.optional[param])
        text.write(value+'"')
    for tex in list(DM.textures.keys()):
        if DM.textures[tex]!='':
            text.write('\n  '+tex+':t="')
            text.write(DM.textures[tex])
            text.write('"')