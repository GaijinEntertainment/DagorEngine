import bpy, os
from ..helpers.basename import basename


def compare_dagormats(mat1,mat2):
    if mat1.is_grease_pencil or mat2.is_grease_pencil:
        return False
    DM1=mat1.dagormat
    DM2=mat2.dagormat
#can't compare dagormat.keys(), since default values not stored in keys(), only manually edited, and it would count as differnence
#bunch of returns to make it faster, no need to compare whole set of parameters after first difference was found

#proxy
    if DM1.is_proxy != DM2.is_proxy:
        return False
    if DM1.is_proxy and DM2.is_proxy:
        #both just imported
        if DM1.shader_class==basename(mat2.name)+':proxy' or DM2.shader_class==basename(mat1.name)+':proxy':
            return True
        #one of mats was rebuilt for viewport from blk
        if basename(mat1.name)==DM2.shader_class[0:DM2.shader_class.find(':')] or basename(mat2.name)==DM1.shader_class[0:DM1.shader_class.find(':')]:
            return True
#main
    if DM1.shader_class!=DM2.shader_class:
        return False
    if DM1.sides!=DM2.sides:
        return False
    #other main parameters doesn't really affect material in game, so let's ignore them
#textures
    if basename(DM1.textures.tex0)!= basename(DM2.textures.tex0): return False
    if basename(DM1.textures.tex1)!= basename(DM2.textures.tex1): return False
    if basename(DM1.textures.tex2)!= basename(DM2.textures.tex2): return False
    if basename(DM1.textures.tex3)!= basename(DM2.textures.tex3): return False
    if basename(DM1.textures.tex4)!= basename(DM2.textures.tex4): return False
    if basename(DM1.textures.tex5)!= basename(DM2.textures.tex5): return False
    if basename(DM1.textures.tex6)!= basename(DM2.textures.tex6): return False
    if basename(DM1.textures.tex7)!= basename(DM2.textures.tex7): return False
    if basename(DM1.textures.tex8)!= basename(DM2.textures.tex8): return False
    if basename(DM1.textures.tex9)!= basename(DM2.textures.tex9): return False
    if basename(DM1.textures.tex10)!=basename(DM2.textures.tex10):return False
#optional properties
    #TODO: when length isn't equal, add compare additional parameters with default values
    keys_1=DM1.optional.keys()
    keys_2=DM2.optional.keys()
    if keys_1.__len__()!=keys_2.__len__():
        return False
    for key in keys_1:
        if key not in keys_2:
            print('1')
            return False
        # strings, arrays. Will work correctly even for [1.0,0.0] vs [1,0], until float is *.0
        try:
            prop1=list(DM1.optional[key])
            prop2=list(DM2.optional[key])
            if prop1!=prop2:
                return False
        except:
            if DM1.optional[key]!=DM2.optional[key]:
                return False
    return True