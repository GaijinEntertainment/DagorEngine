import bpy,os
from bpy.utils          import register_class, unregister_class
from bpy.types          import Operator

from   time             import time

from mathutils          import Matrix,Vector,Euler
from math               import radians

from .cmp_const         import *
from ..helpers.texts    import get_text
from ..helpers.basename import basename
from ..helpers.popup    import show_popup

class cmp_node:
    def __init__(self):
        self.name='node'
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
        self.name='empty_entity'
        self.type=None
        self.weight=1
        self.parent=None
        self.is_node=False
        return

def clear_collection(col):
    for obj in col.objects:
        if obj.name.startswith('node.') and obj.instance_collection is not None:
            bpy.data.collections.remove(obj.instance_collection)
        bpy.data.objects.remove(obj)
    return

def read_cmp_hierarchy(lines):
    log=get_text('log')
    nodes=[]
    active=None

    for line in lines:
        try:
            if line.find('className:t="composit"')>=0:
                pass
            elif line.find('type:t="')>=0:
                pass
            elif line.find('include"')>=0:
                log.write('WARNING! Found "include", possible data loss!\n')
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
                name_type=name.split(':')
                if name_type[0]=='':
                    active.name='empty_entity'
                else:
                    active.name=name_type[0]
                    if name_type.__len__()>1:
                        active.type=name_type[1].lower()
            elif line.find('weight:r=')>=0:
                active.weight=float(line.split('=')[1].replace('"',''))
            elif line.find('tm:m=')>=0:
                active.tm=line.split('=')[1]
                active.found_tm=True
            elif line.find('=')>=0:
                active.param.append(line.split('='))
        except:
            log.write(line+'\n')
    return nodes

def build_nodes(nodes,collection,parent):
    log=get_text('log')
    nodes_to_import=[]
    child_nodes=[]
    ent_nodes=[]
    for node in nodes:
        col=None
        new_node=bpy.data.objects.new(node.name,None)#name,data
        collection.objects.link(new_node)
        if node.is_node and node.entities.__len__()>0:
            new_node.name='random.000'
            col=bpy.data.collections.new('random.000')
            new_node.instance_collection=col
        elif node.name!='node' and node.name!='empty_entity':
            if node.type !=None:
                col=bpy.data.collections.get(node.name+':'+node.type)
                if col is None:
                    col=bpy.data.collections.get(node.name)
            else:
                col=bpy.data.collections.get(node.name)
                if col is None:
                    col=bpy.data.collections.get(node.name+':composit')
                    if col is None:
                        col=bpy.data.collections.get(node.name+':rendinst')
            if col is None:
                name=node.name
                if node.type is not None:
                    name+=':'+node.type
                col=bpy.data.collections.new(name)
                if [node.name,node.type] not in nodes_to_import:
                    nodes_to_import.append([node.name,node.type])
            new_node.instance_collection=col
        new_node.instance_type='COLLECTION'
        if col!=None and col not in bpy.data.collections['NODES'].children_recursive:
            bpy.data.collections['NODES'].children.link(col)
        if node.is_node:
            apply_matrix(node,new_node)
            for par in node.param:
                log.write(f'{par}\n')
                if par[0] not in rand_tf or not node.found_tm:
                    new_node.dagorprops[par[0]]=par[1]
                else:
                    log.write(f'ERROR! {new_node.name}: transformation matrix overrides randomized offests!\n')
                    log.write(f'IGNORED:{par[0]}={par[1]}\n')
        else:
            new_node.dagorprops['weight:r']=node.weight
        if node.type is not None:
            new_node.dagorprops['type:t']=node.type
        if parent!=None:
            new_node.parent=parent
        new_node.empty_display_size=0.1
        new_node.empty_display_type='SPHERE'
        if node.name!='node':
            new_node.empty_display_type='ARROWS'
            new_node.empty_display_size=0.25
            if node.is_node:
                build_nodes(node.children,collection,new_node)
        if (node.is_node and node.type=='gameobj') or node.name=='envi_probe_box':
            new_node.empty_display_size=0.5
            new_node.empty_display_type='CUBE'
        elif node.is_node and node.entities.__len__()>0:
            extra_nodes=build_nodes(node.entities,col,None)
            for n in extra_nodes:
                if n not in nodes_to_import:
                    nodes_to_import.append(n)

        elif node.is_node and node.children.__len__()>0:
            extra_nodes=build_nodes(node.children,collection,new_node)
            for n in extra_nodes:
                if n not in nodes_to_import:
                    nodes_to_import.append(n)

    return nodes_to_import

def import_nodes(nodes,with_dags,with_lods):
    log=get_text('log')
    pref = bpy.context.preferences.addons[basename(__package__)].preferences
    i=int(pref.project_active)
    search_path = pref.projects[i].path
    max=nodes.__len__()
    for subdir,dirs,files in os.walk(search_path):
        for file in files:
            path = os.path.join(subdir, file)
            for n in nodes:
                if file.endswith('.composit.blk'):
                    if n[1] in ['composit',None]:
                        if file.startswith(n[0]+'.'):
                            log.write(f'found node: {n[0]}:{n[1]} as {file}\n')
                            #>fixing names if ri and cmp have same names
                            if bpy.data.collections.get(f'{n[0]}:composit') is None:
                                col=bpy.data.collections.get(n[0])
                                if col is None:
                                    log.write(f'something wrong with collection of {n[0]} composit...\n')
                                else:
                                    for obj in col.users_dupli_group:
                                        DP=obj.dagorprops
                                        DP['type:t']='composit'
                                    col.name=col.name+':composit'
                                    log.write(f'cmp collection successfully renamed: {col.name}\n')
                            #<
                            read_cmp(path,True,with_dags,with_lods)
                            nodes.remove(n)
                            break
                elif file.endswith('.dag'):
                    if n[1] in ['rendinst','prefab',None]:
                        if file.startswith(n[0]+'.'):
                            log.write(f'Found {n[0]}:{n[1]} as {file}\n')
                            nodes.remove(n)
                            #importer migth be unavailable
                            addon = basename(__package__)
                            import_on = os.path.exists(bpy.utils.user_resource('SCRIPTS',path=f'addons\\{addon}\\importer'))
                            if with_dags and import_on:
                                bpy.ops.import_scene.dag(filepath=path)
                                if with_lods:
                                    for i in range (9):
                                        lodpath=f'{path[:-5]}{i+1}.dag'
                                        if os.path.exists(lodpath):
                                            bpy.ops.import_scene.dag(filepath=lodpath)
                            elif with_dags and not import_on:
                                log.write(f'{basename(path)} can not be imported: Importer not found!')
                            break
    if nodes.__len__()>0:
        log.write(f'INFO:not found {nodes.__len__()} node(s):\n')
        for n in nodes:
            log.write(f'{n[0]}:{n[1]}\n')
    return

def _rot_axes_xzy_to_xyz(node):
    node.rotation_euler=node.matrix_basis.to_euler('XYZ')
    node.rotation_mode = 'XYZ'

def apply_matrix(node,node_object):
    og_matrix=[]
    tm=node.tm
    if not node.found_tm:
        for p in node.param:
            try:
                value=float(p[2].split(',')[0])
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
        node_object.location = Vector((og_matrix[9],og_matrix[11],og_matrix[10]))
        node_object.scale = (og_scale[0],og_scale[2],og_scale[1])
        node_object.rotation_euler = Euler((-og_rot[0], -og_rot[2], -og_rot[1]))
#correction is necessary for both ways to set transforms
    node_object.rotation_mode = 'XZY'
    _rot_axes_xzy_to_xyz(node_object)
    return{'FINISHED'}

def read_cmp(cmp_i_path,recursive,with_dags,with_lods):
    log=get_text('log')

    pref = bpy.context.preferences.addons[basename(__package__)].preferences
    i = int(pref.project_active)
    search_path = pref.projects[i].path
    if cmp_i_path.replace(' ','')=='':
        msg='No import path or filename found!'
        show_popup(message=msg,title='ERROR!',icon='ERROR')
        log.write(f'{msg}\n')
        return{"CANCELLED"}
    if cmp_i_path.find('\\')==-1:# if no folders, that might be name of composit
        found=False
        for subdir,dirs,files in os.walk(search_path):
            for file in files:
                if file in [cmp_i_path, cmp_i_path+'.composit.blk']:
                    cmp_i_path=subdir + os.sep + file
                    found=True
                    break
            if found:
                break
        if not found:
            msg=f'Composit not found: {cmp_i_path}'
            show_popup(message=msg,title='ERROR!',icon='ERROR')
            log.write(f'{msg}\n')
            return{"CANCELLED"}
    if not os.path.isfile(cmp_i_path):
        msg=f'{cmp_i_path} is not a file!'
        log.write(f'ERROR! {msg}\n')
        show_popup(message=msg,title='ERROR!',icon='ERROR')
        return{"CANCELLED"}
    if not cmp_i_path.endswith('.blk'):
        msg=f'{os.path.basename(cmp_i_path)} is not a .blk file!'
        log.write(f'ERROR! {msg}\n')
        show_popup(message=msg,title='ERROR!',icon='ERROR')
    log.write(f'IMPORTING {cmp_i_path}\n')
    cmp = open(cmp_i_path,'r')
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
                 '}']
    for el in separate_by:
        split=split.replace(el,'\n'+el)
    split=split.replace(';','\n')
    split=split.replace('\t','')
#hierarchy
    nodes=read_cmp_hierarchy(split.splitlines())
#preparation
    if bpy.data.collections.get('NODES') is None:
        bpy.data.collections.new('NODES')
        bpy.context.scene.collection.children.link(bpy.data.collections['NODES'])
    name=os.path.basename(cmp_i_path).replace('.composit.blk','')

    cmp_col=bpy.data.collections.get(name)
    if cmp_col is None:
        cmp_col=bpy.data.collections.get(name+':composit')
        if cmp_col is None:
            cmp_col=bpy.data.collections.new(name+':composit')
            bpy.context.scene.collection.children.link(cmp_col)
        else:#[name:composit], can't be a dag
            clear_collection(cmp_col)

    elif cmp_col.children.__len__()>0:#has subcollections, must be a dag with lods
        cmp_col=bpy.data.collections.get(name+':composit')
        if cmp_col is None:
            cmp_col=bpy.data.collections.new(name+':composit')
            bpy.context.scene.collection.children.link(cmp_col)

    elif cmp_col.objects.__len__()>0:#might be a previously imported cmp or prefab
        for obj in cmp_col.objects:
            if obj.type!='EMPTY':#it's not a composit, we need new collection
                cmp_col=bpy.data.collections.get(name+':composit')
                if cmp_col is None:
                    cmp_col=bpy.data.collections.new(name+':composit')
                    bpy.context.scene.collection.children.link(cmp_col)
                break
#node creation
    nodes_to_import=build_nodes(nodes,cmp_col,None)
    log.write(f' unic nodes:{nodes_to_import.__len__()}\n')
    for n in nodes_to_import:
        log.write(f'{n}\n')
    if recursive:
        if with_dags:
            import_nodes(nodes_to_import,with_dags, with_lods)
        else:
            import_nodes(nodes_to_import,with_dags, False)
    log.write(f'IMPORTED {os.path.basename(cmp_i_path)}\n')
    return

class DAGOR_OP_CmpImport(Operator):
    bl_idname = "dt.cmp_import"
    bl_label = "Import CMP"
    bl_description = "Import composit"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        log=get_text('log')
        S = context.scene
        cmp_i_path      = S.cmp_i_path
        cmp_i_recursive = S.cmp_i_recursive
        cmp_i_dags      = S.cmp_i_dags
        cmp_i_lods      = S.cmp_i_lods
        try:
            start=time()
            read_cmp(cmp_i_path,cmp_i_recursive,cmp_i_dags,cmp_i_lods)
            show_popup(f'finished in {round(time()-start,4)} sec')
        except:
            log.write(f'ERROR! something went wrong on importing {os.path.basename(cmp_i_path)}!!!\n')
        return{'FINISHED'}

classes = [DAGOR_OP_CmpImport]

def register():
    S=bpy.types.Scene
    for cl in classes:
        register_class(cl)

def unregister():
    S=bpy.types.Scene
    for cl in classes:
        unregister_class(cl)
