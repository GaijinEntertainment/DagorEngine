from os         import walk
from os         import sep as os_sep
from os.path    import exists, join, isfile

import bpy

from bpy.utils          import register_class, unregister_class
from bpy.types          import Operator

from   time             import time

from mathutils          import Matrix,Vector,Euler
from math               import radians

from .cmp_const         import *
from .cache_rw          import build_cache, read_cache
from ..helpers.texts    import get_text, log
from ..helpers.names    import asset_basename, ensure_no_extension, ensure_no_path
from ..helpers.props    import fix_type
from ..helpers.getters  import *

from ..popup.popup_functions    import show_popup
#TODO: find better solution than global variable?

dags_to_import = []
cmp_to_import = []

class cmp_node:
    def __init__(self):
        self.name=""
        self.tm='[[1,0,0][0,1,0][0,0,1][0,0,0]]'
        self.found_tm=False
        self.type=None
        self.parent=None
        self.entities=[]
        self.children=[]
        self.param=[]
        self.is_node=True
        return

class cmp_ent:
    def __init__(self):
        self.name=""
        self.type=None
        self.weight=1
        self.parent=None
        self.is_node=False
        return

def get_collection(name):
    collection = bpy.data.collections.get(name)
    if collection is None:
        collection = bpy.data.collections.new(name)
    return collection

def check_collection_name(collection, name):
    if 'name' in collection.keys():
        if ensure_no_extension(collection['name']) == name:
            return True
    elif collection.name == name:
        return True
    return False

def check_col_type(col,type):
    if 'type' in col.keys():
        if col['type'] != type:
            return False
    else:
        col['type'] = type
    return True

def get_node_collection(name,type):
    collections = bpy.data.collections
    for col in collections:
        if check_collection_name(col,name) and check_col_type(col, type):
            not_imported = col.objects.__len__()==col.children.__len__()==0
            return [col, not_imported]
    col = bpy.data.collections.new(name)
    col['name'] = name
    if type=='rendinst':
        col.name = name+'.lods'
    col['type'] = type
    if type == "composit":
        bpy.data.scenes["COMPOSITS"].collection.children.link(col)
    elif type in ["rendinst", "prefab"]:
        bpy.data.scenes["GEOMETRY"].collection.children.link(col)
    elif type == "gameobj":
        bpy.data.scenes["GAMEOBJ"].collection.children.link(col)
    return [col, True]

def clear_collection(col):
    for obj in col.objects:
        if obj.name.startswith('node.') and obj.instance_collection is not None:
            bpy.data.collections.remove(obj.instance_collection)
        bpy.data.objects.remove(obj)
    return

def read_cmp_hierarchy(lines,assets):
    nodes=[]
    active=None

    for line in lines:
        try:
            if line.find('className:t="composit"')>=0:
                pass
            elif line.find('type:t="')>=0:
                pass
            elif line.find('include"')>=0:
                msg = 'Found "include", possible data loss!\n'
                log(msg, type = 'WARNING')
            elif line.find('node{')>=0:
                node=cmp_node()
                node.parent=active
                if active is not None:
                    active.children.append(node)
                else:
                    nodes.append(node)
                active=node
            elif line.find('ent{')>=0:
                ent=cmp_ent()
                if active is not None and active.is_node:
                    active.entities.append(ent)
                    ent.parent=active
                else:
                    active.parent.entities.append(ent)
                    ent.parent=active.parent
                active=ent
            elif line.find('}')>=0:
                active=active.parent
            elif line.find('name:t=')>=0:
                name=line.split('=')[1].replace('"','')
                name_type=name.lower().split(':')
                active.name=name_type[0]
                try:
                    active.type=name_type[1]
                except:
                    if name !="":
                        active.type = assets[active.name][0][0]
            elif line.find('weight:r=')>=0:
                active.weight=float(line.split('=')[1].replace('"',''))
            elif line.find('tm:m=')>=0:
                active.tm=line.split('=')[1]
                active.found_tm=True
            elif line.find('=')>=0:
                active.param.append(line.split('='))
        except:
            msg = f'Something went wrong on reading this line: \n{line}\n'
            log(msg, type = 'WARNING', show = True)
    return nodes

def build_nodes(nodes,parent_col,parent,assets):
    for node in nodes:
        instance_col=None
        node_obj=bpy.data.objects.new(node.name,None)#name,data
        parent_col.objects.link(node_obj)
        if node.is_node and node.entities.__len__()>0:
            node_obj.name='random.000'
            instance_col=bpy.data.collections.new('random.000')
            bpy.data.scenes["TECH_STUFF"].collection.children.link(instance_col)
        if node.name=='':
            node_obj.name = "node"
        else:
            instance_col,newly_created = get_node_collection(node.name,node.type)
            if newly_created:
                try:
                    entity = assets[node.name]
                except:
                    msg = f'Node "{node.name}:{node.type}" path not found in cache!\n'
                    msg+='\tMake sure that active propject is correct, rebuild cache '
                    msg+='\tand then reimport into new blend file to avoid unexpected issues, \n'
                    msg+='\tthat might be caused by importing from wronge source\n'
                    log(msg, type = 'ERROR', show = True)
                    return
                if node.type in ['rendinst','prefab']:
                    for asset in entity: #can be rendinst and cmp at the same time
                        if asset[0] == node.type:
                            dags_to_import.append(asset[1])
                elif node.type == 'composit':
                    for asset in entity: #can be rendinst and cmp at the same time
                        if asset[0] == node.type:
                            cmp_to_import.append(asset[1])
        node_obj.instance_collection=instance_col
        node_obj.instance_type='COLLECTION'
        if instance_col is not None:
            is_lost = True
            for S in bpy.data.scenes:
                if instance_col in S.collection.children_recursive:
                    is_lost = False
            if is_lost:
                bpy.context.scene.collection.children.link(instance_col)
        if parent!=None:
            node_obj.parent=parent
        if node.is_node:
            apply_matrix(node,node_obj)
            for par in node.param:
                if par[0] not in rand_tf or not node.found_tm:
                    node_obj.dagorprops[par[0]]=fix_type(par[1])
                else:
                    msg = f'{node_obj.name}: transformation matrix overrides randomized offests!\n'
                    msg+= f'\tIGNORED:{par[0]}={par[1]}\n'
                    log(msg, type = 'ERROR', show = True)
        else:
            node_obj.dagorprops['weight:r']=node.weight
        if node.type is not None:
            node_obj.dagorprops['type:t']=node.type
        node_obj.empty_display_size=0.1
        node_obj.empty_display_type='SPHERE'
        if node.name!="":
            node_obj.empty_display_type='ARROWS'
            node_obj.empty_display_size=0.25
        if node.is_node:
            if node.type=='gameobj' or node.name in ['envi_probe_box','wall_hole','indoor_walls']:
                node_obj.empty_display_size=0.5
                node_obj.empty_display_type='CUBE'
            if node.entities.__len__()>0:
                build_nodes(node.entities,instance_col,None,assets)
            if node.children.__len__()>0:
                build_nodes(node.children,parent_col,node_obj,assets)
    return

def apply_matrix(node,node_object):
    og_matrix=[]
    tm=node.tm
    if not node.found_tm:
        node_object.lock_rotation = [True, True, True]
        node_object.lock_location = [True, True, True]
        node_object.lock_scale =    [True, True, True]
        for p in node.param:
            try:
                value=fix_type(p[1])[0]
            except:
                continue
            if p[0]=='offset_x:p2':
                node_object.location.x=value
            elif p[0]=='offset_y:p2':
                node_object.location.z=value
            elif p[0]=='offset_y:p2':
                node_object.location.z=value
            elif p[0]=='rot_x:p2':
                node_object.rotation_euler.x=radians(value)
            elif p[0]=='rot_y:p2':
                node_object.rotation_euler.z=radians(value)
            elif p[0]=='rot_z:p2':
                node_object.rotation_euler.y=radians(value)
            elif p[0]=='yScale:p2':
                node_object.scale.z=value
            elif p[0]=='scale:p2':
                node_object.scale=(value,value,value)
#random transforms would be ignored, when matrix was initialized
    else:
#we need only numbers
        tm=tm.replace('][',',')
        tm=tm.replace('[[','')
        tm=tm.replace(']]','')
        str_values=tm.split(',')
        for value in str_values:
            og_matrix.append(float(value))
        m = Matrix().to_3x3()
        m[0][0] = og_matrix[0]
        m[1][0] = og_matrix[1]
        m[2][0] = og_matrix[2]
        m[0][1] = og_matrix[3]
        m[1][1] = og_matrix[4]
        m[2][1] = og_matrix[5]
        m[0][2] = og_matrix[6]
        m[1][2] = og_matrix[7]
        m[2][2] = og_matrix[8]
        og_scale = m.to_scale()
        og_rot = m.to_euler()
#converting axis from Y-up to Z-up
        tm_loc = Matrix.Translation(Vector((og_matrix[9],og_matrix[11],og_matrix[10])))
        tm_scale = Matrix.Diagonal(Vector((og_scale[0],og_scale[2],og_scale[1])).to_4d())
        tm_rot = Euler((-og_rot[0], -og_rot[2], -og_rot[1]),'XZY').to_matrix().to_4x4()
#combining matrix
        tm = tm_loc @ tm_rot @ tm_scale
        node_object.matrix_local = tm
    return{'FINISHED'}

def read_cmp(path_import,with_sub_cmp,with_dags,with_lods,assets):
    pref = get_preferences()
    i = int(pref.project_active)
    search_path = pref.projects[i].path
    if path_import.replace(' ','')=='':
        msg='No import path or filename found!\n'
        show_popup(message=msg,title='ERROR!',icon='ERROR')
        log(msg, type = 'ERROR', show = True)
        return{"CANCELLED"}
    path_import = path_import.replace('/', os_sep)
    if path_import.find(os_sep)==-1:# if no folders, that might be name of composit
        found=False
        for subdir,dirs,files in walk(search_path):
            for file in files:
                if file in [path_import, path_import+'.composit.blk']:
                    path_import = join(subdir, file)
                    found=True
                    break
            if found:
                break
        if not found:
            msg=f'Composit not found: {path_import}\n'
            show_popup(message=msg,title='ERROR!',icon='ERROR')
            log(msg, type = 'ERROR', show = True)
            return{"CANCELLED"}
    if not isfile(path_import):
        msg=f'{path_import} is not a file!\n'
        log(msg, type = 'ERROR')
        show_popup(message=msg,title='ERROR!',icon='ERROR')
        return{"CANCELLED"}
    if not path_import.endswith('.blk'):
        filename = ensure_no_path(path_import)
        msg = f'{filename} is not a .blk file!\n'
        log(msg, type = 'ERROR', show = True)
        show_popup(message=msg,title='ERROR!',icon='ERROR')
    msg = f'IMPORTING {path_import}\n'
    log(msg)
    cmp = open(path_import,'r')
    src = cmp.readlines()
    cmp.close()
    split = ''
#removing one-line comments
    for line in src:
        line = line=line[:line.find("//")]#one string comments removed
        split+=line
#removing multi-line comments
    while split.find('/*')>-1 and split.find('*/')>-1:
        c_start = split.find('/*')
        c_end = split.find('*/')+2
        split = split[:c_start]+split[c_end:]
    split=split.replace(' ','')# unnecessary spaces removed

#line separation
    separate_by=['node{','ent{','type:t','name:t','weight:r','tm:m','place_type:i','include',
                 'rot_x:p2','rot_y:p2','rot_z:p2',
                 'offset_x:p2','offset_y:p2','offset_z:p2',
                 'scale:p2','yScale:p2',#separate x and z scale not supported
                 'placeOnCollision:b',
                 'ignoreParentInstSeed:b',
                 '}']
    for el in separate_by:
        split=split.replace(el,'\n'+el)
    split=split.replace(';','\n')
    split=split.replace('\t','')
#hierarchy
    nodes = read_cmp_hierarchy(split.splitlines(), assets)
    name = asset_basename(path_import)

    cmp_collection = get_node_collection(name,'composit')[0]
    clear_collection(cmp_collection)

#node creation
    build_nodes(nodes,cmp_collection, None, assets)
#import parts
    if with_sub_cmp:
        sub_composits = list(dict.fromkeys(cmp_to_import))
        if sub_composits.__len__()>0:
            msg = 'Sub-composits to import:\n'
        for cmp in sub_composits:
            msg+=f'\t{cmp}\n'
        log(msg)
        cmp_to_import.clear()#we don't need it to be modified dynamically
        for cmp in sub_composits:
            read_cmp(cmp,with_sub_cmp,with_dags,with_lods,assets)
    if with_dags:
        dags = list(dict.fromkeys(dags_to_import))
        if dags.__len__()>0:
            msg = 'dag files to import:\n'
        for dag in dags:
            msg+=f'\t{dag}\n'
        log(msg)
        dags_to_import.clear()#we don't need it to be modified dynamically
        for dag in dags:
            bpy.ops.import_scene.dag(
                filepath = dag,
                with_lods = with_lods,
#making sure no parameters were overridden by previous call
                dirpath = "",
                includes = "",
                excludes = "",
                includes_re = "",
                excludes_re = "",
                check_subdirs = False,
                with_dps= False,
                with_dmgs= False,
                with_destr = False,
                )
    log(f'IMPORTED {asset_basename(path_import)}\n')
    return

class DAGOR_OP_CmpImport(Operator):
    bl_idname = "dt.cmp_import"
    bl_label = "Import CMP"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(self, context):
        pref = get_preferences()
        if pref.projects.__len__() == 0:
            return False
        project = pref.projects[int(pref.project_active)]
        if not exists(project.path):
            return False
        return True

    @classmethod
    def description(cls, context, properties):
        pref = get_preferences()
        if pref.projects.__len__() == 0:
            return "No active project found!"
        project = pref.projects[int(pref.project_active)]
        if not exists(project.path):
            return f'Path of project "{project.name}" does not exist!'
        return "Import Composite"

    def execute(self, context):
        pref = get_preferences()
        index = pref.project_active
        project = pref.projects[int(index)]
        if not exists(project.path):
            msg = f'Pafth of project "{project.name}" does not exist, can not search for resources'
            log(msg, type = 'ERROR')
            show_popup(msg)
            return {'CANCELLED'}
        props = get_local_props()
        cmp_import_props = props.cmp.importer
        start=time()
        bpy.ops.dt.init_blend()
        if cmp_import_props.refresh_cache:
            assets = build_cache()
        else:
            try:
                assets = read_cache()
            except:
                asset = build_cache()
                log('Can not read the cached paths, updating...\n', type = 'WARNING')
        dags_to_import.clear()
        cmp_to_import.clear()

        read_cmp(cmp_import_props.filepath,
                cmp_import_props.with_sub_cmp,
                cmp_import_props.with_dags,
                cmp_import_props.with_lods,
                assets)
        context.window.scene = bpy.data.scenes['COMPOSITS']
        show_popup(f'finished in {round(time()-start,4)} sec')
        return{'FINISHED'}

classes = [DAGOR_OP_CmpImport]

def register():
    for cl in classes:
        register_class(cl)
    return

def unregister():
    for cl in classes:
        unregister_class(cl)
    return
