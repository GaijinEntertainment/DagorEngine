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
            pass#always yes for modern assets
        elif line.startswith('class:t='):
            DM.shader_class=line.replace('class:t=','')
        elif line.startswith('script:t='):
            optional=line[9:].split('=')
            if optional[0]=="real_two_sided":
                DM.sides = 2 if optional[1] == 'yes' else 0
            else:
                DM.optional[optional[0]]=fix_type(optional[1])
        elif line.startswith('tex'):
            tex=line.split(':t=')
            DM.textures[tex[0]]=tex[1]

def dagormat_to_text(mat,text):
    DM=mat.dagormat
    text.clear()
    text.write(f'class:t="{DM.shader_class}"\n')
    text.write('tex16support:b=yes\n')
    if DM.sides == 1:
        text.write('twosided:b=yes\n')
    else:
        text.write('twosided:b=no\n')
    if DM.sides == 2:
        text.write('\nscript:t="real_two_sided=yes"\n')
    for param in list(DM.optional.keys()):
        text.write('\nscript:t="'+param+'=')
        try:
            values = DM.optional[param].to_list()
            value = ''
            for el in values:
                value+=f'{round(el,5)}, '
            value = value[:-2]
        except:
            try:
                value = f'{round(DM.optional[param],5)}'
            except:
                value=str(DM.optional[param])
        text.write(value+'"')
    for tex in list(DM.textures.keys()):
        if DM.textures[tex]!='':
            text.write('\n  '+tex+':t="')
            text.write(DM.textures[tex])
            text.write('"')