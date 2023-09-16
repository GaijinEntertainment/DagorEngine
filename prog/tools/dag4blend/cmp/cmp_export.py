import bpy,os
from   bpy.utils        import register_class, unregister_class
from   bpy.types        import Operator

from   mathutils        import Matrix,Vector,Euler
from   math             import radians

from   time             import time

from .cmp_const         import *
from ..helpers.texts    import get_text_clear, get_text
from ..helpers.basename import basename
from ..helpers.popup    import show_popup


#functions
def _tabs(i):
    cmp=bpy.data.texts['cmp']
    for i in range(i):
        cmp.write('\t')

def _to_str(value):
    try:#float/integer
        if value.is_integer():
            return str(int(value))
        else:
            return str(round(value,7))
    except:
        try:#multiple fields
            v=''
            for el in value:
                if el.is_integer():
                    v+=str(int(el))+','
                else:
                    v+=str(round(el,7)) + ','# might contain float error
            return v[:-1]
        except:#string
            return str(value)

def get_matrix(obj):
    r1 = obj.matrix_world.to_euler()
    r = Euler((-r1[0],-r1[2],-r1[1]),'XZY').to_matrix()
    s1 = obj.matrix_world.to_scale()
    s = Matrix().to_3x3()
    s[0][0]=s1[0]
    s[1][1]=s1[2]
    s[2][2]=s1[1]
    m = r @ s

    l = obj.matrix_world.to_translation()

    matrix = '['
    matrix+=(f'[{_to_str(m[0][0])},{_to_str(m[1][0])},{_to_str(m[2][0])}]')
    matrix+=(f'[{_to_str(m[0][1])},{_to_str(m[1][1])},{_to_str(m[2][1])}]')
    matrix+=(f'[{_to_str(m[0][2])},{_to_str(m[1][2])},{_to_str(m[2][2])}]')
    matrix+=(f'[{_to_str(l[0])},{_to_str(l[2])},{_to_str(l[1])}]')
    matrix+=']'
    return(matrix)

def write_entities(cmp,node,tabs):
    col=node.instance_collection
    for ent in col.objects:
        DP=ent.dagorprops
        if 'weight:r' in DP.keys() and DP['weight:r']!=1:
            end=f"weight:r={_to_str(DP['weight:r'])};"+'}\n'
        else:
            end='}\n'
        if ent.name.startswith('empty_entity'):
            name=''
        else:
            name=basename(ent.instance_collection.name)
        _tabs(tabs)
        cmp.write('ent{ name:t="'+name+'";'+end)

def write_node(cmp,node,tabs):
    if node.type!='EMPTY':
        return
    _tabs(tabs)
    cmp.write('node{\n')
    DP=node.dagorprops
    props=list(DP.keys())
    randomized=False
    for prop in props:
        if prop=='type:t':
            continue
        value=DP[prop]
        try:
            value=value.to_list()
        except:
            pass
        _tabs(tabs+1)

        cmp.write(f'{prop}={_to_str(value)}\n')
        if prop in rand_tf:
            randomized=True
    if not randomized:
        m=get_matrix(node)
        #if m!=default_tm:
        _tabs(tabs+1)
        cmp.write(f'tm:m={m}\n')
    #parent node, can't have name
    if node.children.__len__()>0:
        for child in node.children:
            write_node(cmp,child,tabs+1)
    #random node, can't have name either
    elif node.name.startswith('random.'):
        write_entities(cmp,node,tabs+1)
    #should have name
    else:
        node_name=basename(node.instance_collection.name.split(':')[0])
        if 'type:t' in props:
            node_name+=f':{DP["type:t"]}'
        _tabs(tabs)
        cmp.write(f'\tname:t="{node_name}"\n')
    _tabs(tabs)
    cmp.write('}\n')

def cmp_export(col,path):
    cmp = get_text_clear('cmp')
    log = get_text('log')
    S = bpy.context.scene
    cmp.write('className:t="composit"\n\n')
    if col.objects.__len__()==0:
        #nothing to export
        return {"CANCELLED"}
    elif col.children.__len__()!=0:
        #not a cmp
        return {"CANCELLED"}
    else:
        nodes=[n for n in col.objects if n.parent is None]
        for node in nodes:
           write_node(cmp,node,0)
#saving bpy.data.texts['cmp'] into an actual file
    if not os.path.exists(S.cmp_e_path):
        os.makedirs(S.cmp_e_path)
        log.write(f'directory successfully created: {S.cmp_e_path}\n')
    path=os.path.join(S.cmp_e_path,S.cmp_col.name.split(':')[0]+'.composit.blk')
    with open((path), 'w') as outfile:#TODO: remove that blend text thing, work directly with .blk
        outfile.write(cmp.as_string())
        outfile.close()
        log.write(f'EXPORTED cmp: {path}\n')
#CLASSES

class DAGOR_OP_CmpExport(Operator):
    bl_idname = "dt.cmp_export"
    bl_label = "Export CMP"
    bl_description = "Export composit"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        S=context.scene
        if S.cmp_col is None:
            show_popup(message='Select cmp collection!', title = 'ERROR', icon = 'ERROR')
            return {'CANCELLED'}
        start=time()
        cmp_export(S.cmp_col,S.cmp_e_path)
        show_popup(f'finished in {round(time()-start,4)} sec')
        return{'FINISHED'}

classes=[DAGOR_OP_CmpExport]

def register():
    for cl in classes:
        register_class(cl)

def unregister():
    for cl in classes:
        unregister_class(cl)
