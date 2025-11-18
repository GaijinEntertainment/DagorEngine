from os         import makedirs
from os.path    import exists, join

import bpy

from   bpy.utils        import register_class, unregister_class
from   bpy.types        import Operator

from   mathutils        import Matrix,Vector,Euler
from   math             import radians

from   time             import time

from .cmp_const         import *
from ..helpers.texts    import get_text_clear, get_text, log
from ..helpers.names    import ensure_no_extension
from ..helpers.getters  import get_local_props

from ..popup.popup_functions    import show_popup

TAB = "  "
EXISTING_NODE_TYPES = [
                        "prefab",
                        "rendinst",
                        "composit",
                        "dynmodel",
                        "gameobj",
                        ]

PROPS_TO_SKIP = [
                "type:t",
                ]

#functions
def _tabs(i):
    cmp=bpy.data.texts['cmp']
    if i>0:
        for i in range(i):
            cmp.write(TAB)
    return

def _to_str(value):
    try:#float/integer
        if value.is_integer():
            return str(int(value))
        else:
            return str(round(value,4))
    except:
        try:#multiple fields
            v=''
            for el in value:
                if el.is_integer():
                    v+=str(int(el))+','
                else:
                    v+=str(round(el,4)) + ','# might contain float error
            return v[:-1]
        except:#string
            return str(value)

def get_og_matrix(obj):
    if obj.parent is None:
        return obj.matrix_world
    if obj.parent.type!='EMPTY':
        return obj.matrix_world
    #local matrix only when obj and parent exported at the same time
    #can't be exported from different colections
    for col in list(obj.users_collection):
        if col in obj.parent.users_collection:
            return obj.matrix_local
    return obj.matrix_world

def get_matrix(obj):
    og_matrix = get_og_matrix(obj)
    r1 = og_matrix.to_euler()
    r = Euler((-r1[0],-r1[2],-r1[1]),'XZY').to_matrix()
    s1 = og_matrix.to_scale()
    s = Matrix().to_3x3()
    s[0][0]=s1[0]
    s[1][1]=s1[2]
    s[2][2]=s1[1]
    m = r @ s

    l = og_matrix.to_translation()

    matrix = '['
    matrix+=(f'[{_to_str(m[0][0])}, {_to_str(m[1][0])}, {_to_str(m[2][0])}] ')
    matrix+=(f'[{_to_str(m[0][1])}, {_to_str(m[1][1])}, {_to_str(m[2][1])}] ')
    matrix+=(f'[{_to_str(m[0][2])}, {_to_str(m[1][2])}, {_to_str(m[2][2])}] ')
    matrix+=(f'[{_to_str(l[0])}, {_to_str(l[2])}, {_to_str(l[1])}]')
    matrix+=']'
    return(matrix)

def get_node_name(node):
    if node.instance_type != 'COLLECTION':
        return None
    instance_collection = node.instance_collection
    if node.instance_collection is None:
        return None
    instance_col = node.instance_collection
    if "name" in instance_col.keys(): # was overriden
        node_name = instance_col["name"]
    else:
        node_name = instance_col.name
    node_name = ensure_no_extension(node_name)
    if "type" in instance_col.keys():
        node_type = instance_col["type"]
        if node_type in EXISTING_NODE_TYPES:
            node_name = ":".join([node_name, node_type])
    return node_name

def write_entities(cmp,node,tabs):
    for ent in node.instance_collection.objects:
        DP=ent.dagorprops
        col = ent.instance_collection
        if 'weight:r' in DP.keys() and DP['weight:r']!=1:
            end=f"weight:r={_to_str(DP['weight:r'])};"+'}\n'
        else:
            end='}\n'
        if ent.instance_type!='COLLECTION' or col is None:
            name=''
        else:
            name=get_node_name(ent)
        _tabs(tabs)
        cmp.write('ent{ name:t="'+name.lower()+'";' + end)

def write_node(cmp,node,tabs):
    if node.type!='EMPTY':
        msg = f'node "{node.name}" is not Empty object!\n'
        log(msg, type = 'ERROR')
        return
    cmp.write('\n')
    _tabs(tabs)
    cmp.write('node{\n')
    #does it have name?
    instance_col = node.instance_collection
    if node.instance_type == 'COLLECTION' and instance_col is not None:
        if instance_col.name.startswith('random.'):
            write_entities(cmp,node,tabs+1)
        else:
            node_name=get_node_name(node)
            _tabs(tabs+1)
            cmp.write(f'name:t="{node_name.lower()}"\n')
    DP=node.dagorprops
    props=list(DP.keys())
    randomized_tf=False
    for prop in props:
        if prop in PROPS_TO_SKIP:
            continue
        value=DP[prop]
        try:
            value=value.to_list()
        except:
            pass
        _tabs(tabs+1)

        cmp.write(f'{prop}={_to_str(value)}\n')
        if prop in rand_tf:
            randomized_tf=True
    if not randomized_tf:
        m=get_matrix(node)
        _tabs(tabs+1)
        cmp.write(f'tm:m={m}\n')
    if node.children.__len__()>0:
        for child in node.children:
            write_node(cmp,child,tabs+1)
    _tabs(tabs)
    cmp.write('}\n')
    return

def cmp_export(col,path):
    cmp = get_text_clear('cmp')
    props = get_local_props()
    cmp_export_props = props.cmp.exporter
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
    if not exists(cmp_export_props.dirpath):
        makedirs(cmp_export_props.dirpath)
        msg = f'directory successfully created: {cmp_export_props.dirpath}\n'
        log(msg)
    path = join(cmp_export_props.dirpath, cmp_export_props.collection.name + '.composit.blk')
    with open((path), 'w') as outfile:#TODO: remove that blend text thing, work directly with .blk
        outfile.write(cmp.as_string())
        outfile.close()
        msg = f'EXPORTED cmp: {path}\n'
        log(msg, show = True)
    return
#CLASSES

class DAGOR_OP_CmpExport(Operator):
    bl_idname = "dt.cmp_export"
    bl_label = "Export CMP"
    bl_description = "Export composit"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        props = get_local_props()
        cmp_export_props = props.cmp.exporter
        if cmp_export_props.collection is None:
            show_popup(message='Select cmp collection!', title = 'ERROR', icon = 'ERROR')
            return {'CANCELLED'}
        start=time()
        cmp_export(cmp_export_props.collection, cmp_export_props.dirpath)
        show_popup(f'finished in {round(time()-start,4)} sec')
        return{'FINISHED'}

classes=[DAGOR_OP_CmpExport]

def register():
    for cl in classes:
        register_class(cl)
    return

def unregister():
    for cl in classes:
        unregister_class(cl)
    return
