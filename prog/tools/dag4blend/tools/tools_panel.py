import bpy, os, bmesh, math
from   bpy.types                    import Operator, Panel
from bpy.props                      import StringProperty,IntProperty
from   shutil                       import copyfile
from   time                         import time
from ..dagormat.build_node_tree     import buildMaterial
from ..dagormat.compare_dagormats   import compare_dagormats

from ..helpers.texts                import get_text
from ..helpers.basename             import basename
from ..helpers.popup                import show_popup

#FUNCTIONS#################################################################

#returns geometry nodegroup that turns collection into single mesh
def get_converter_ng():
    ng = bpy.data.node_groups.get('GN_col_to_mesh')
    if ng is not None:
        return ng
    ng = bpy.data.node_groups.new('GN_col_to_mesh','GeometryNodeTree')
    inp = ng.nodes.new(type="NodeGroupInput")
    inp.location = (-480,0)
    inp.outputs[0].type = 'COLLECTION'

    out = ng.nodes.new(type="NodeGroupOutput")

    col = ng.nodes.new(type="GeometryNodeCollectionInfo")
    col.location = (-320,0)

    realize = ng.nodes.new(type="GeometryNodeRealizeInstances")
    realize.location = (-160,0)

    ng.links.new(inp.outputs[0],col.inputs[0])
    ng.links.new(col.outputs[0],realize.inputs[0])
    ng.links.new(realize.outputs[0],out.inputs[0])
    return ng

#turns collection instance into mesh
def node_to_geo(node):
    #preparation
    mesh = bpy.data.meshes.new('')
    geo_node = bpy.data.objects.new('name',mesh)
    matr = node.matrix_local
    col = bpy.context.collection
    col.objects.link(geo_node)
    geo_node.matrix_local = matr
    geo_node.parent = node.parent
    name = node.name
    #convertion
    geo_node.name = name
    mod = geo_node.modifiers.new('','NODES')
    bpy.data.node_groups.remove(mod.node_group)
    converter_group = get_converter_ng()
    mod.node_group = converter_group
    mod['Input_0'] = node.instance_collection
    #cleanup
    ctx = bpy.context.copy()
    ctx['object'] = geo_node
    bpy.ops.object.modifier_apply(ctx, modifier=mod.name)
    bpy.data.objects.remove(node)
    #uv_fix
    attrs = sorted(geo_node.data.attributes.keys())
    for attr in attrs:
        if attr.startswith('UVMap'):
            geo_node.data.attributes.active = geo_node.data.attributes[attr]
            bpy.ops.geometry.attribute_convert(ctx,mode='UV_MAP')
    #shading
    geo_node.data.use_auto_smooth = True
    geo_node.data.auto_smooth_angle = 2*math.pi
    return

#returns gi_black material if exists or creates new
def get_gi_black():
    for mat in bpy.data.materials:
        if mat.dagormat.shader_class == 'gi_black':
            return mat
    mat = bpy.data.materials.new('gi_black')
    mat.dagormat.shader_class = 'gi_black'
    return mat

def remove_smooth_groups(objects):
    for obj in objects:
        if obj.type == 'MESH':
            mesh = obj.data
            SG = mesh.attributes.get('SG')
            if SG is not None:
                mesh.attributes.remove(SG)
    return

def save_textures(sel,path):
    all_tex=[]
#collecting all textures of selected mesh objects:
    for o in sel:
        for mat in o.data.materials:
            T=mat.dagormat.textures
            for key in mat.dagormat.textures.keys():
                if T[key]!="":
                    all_tex.append(basename(T[key]))
    all_tex=list(set(all_tex))#removing doubles
#separating textures only with existing paths, to avoid saving "checkers"
    tex_to_save=[]
    for t in all_tex:
        img=bpy.data.images.get(t)
        if img is None:
            continue
        if os.path.exists(img.filepath):
            tex_to_save.append(img.filepath)
#saving
    for tex in tex_to_save:
        tex_name=os.path.basename(tex)
        save_path=os.path.join(path,tex_name)
        copyfile(tex,save_path)
    return tex_to_save.__len__()

def clear_normals(objects):
    ctx = bpy.context.copy()
    for obj in objects:
        ctx['object']=obj
        bpy.ops.mesh.customdata_custom_splitnormals_clear(ctx)
    return

def sort_collections(COL):
    #looking for matches
    names=[]
    for col in COL.children:
        names.append(col.name)
    best_match=''
    for n1 in names:
        for n2 in names:
            if n1!=n2:
                i=0
                while n1[:i+1]==n2[:i+1]:
                    i+=1
                while n1[i-1] =='_' or n1[i-1].isdigit():
                    i-=1
                if i> best_match.__len__():
                    best_match=n1[:i]
    #all matched collections
    if best_match!='':
        i=best_match.__len__()
        children=[]
        for n in names:
            if n[:i]==best_match:
                children.append(bpy.data.collections[n])
    #moving to a new place
        new_col=bpy.data.collections.new(best_match)
        if new_col.name.endswith(".001"):
            new_col.name=new_col.name[:-4]+'_ALL'
        COL.children.link(new_col)
        for col in children:
            COL.children.unlink(col)
            new_col.children.link(col)
    return best_match.__len__()>0

def get_col(name):
    col=bpy.data.collections.get(name)
    if col is None:
        col=bpy.data.collections.new(name)
    return col

def pack_orphans(col):
    if col.children.__len__()>0 or col==bpy.context.scene.collection:
        for obj in col.objects:
            new_col=get_col(obj.name)
            new_col.user_clear()
            col.objects.unlink(obj)
            new_col.objects.link(obj)
            col.children.link(new_col)
        for child in col.children:
            pack_orphans(child)
    return

def apply_modifiers(obj):
    ctx=bpy.context.copy()
    ctx['object'] = obj
    for mod in obj.modifiers:
        if mod.show_viewport:
            bpy.ops.object.modifier_apply(ctx, modifier=mod.name)
    obj.modifiers.clear()
    attrs = sorted(obj.data.attributes.keys())
    for attr in attrs:
        if attr.startswith('UVMap'):
            obj.data.attributes.active = obj.data.attributes[attr]
            bpy.ops.geometry.attribute_convert(ctx,mode='UV_MAP')

def get_autonamed():
    mat=bpy.data.materials.get('autoNamedMat_0')
    if mat==None:
        mat=bpy.data.materials.new('autoNamedMat_0')
        mat.dagormat.shader_class='gi_black'
        buildMaterial(mat)
    return mat

def shortest_name(n1,n2):
    if n1.__len__() < n2.__len__():
        return n1
    if n1.__len__() > n2.__len__():
        return n2
    if n1[-3:].isnumeric() and n2[-3:].isnumeric():
        if int(n1[-3:]) < int(n2[-3:]):
            return n1
    return n2

def merge_mat_duplicates(obj):
    replaced = []
    for mat_slot in obj.material_slots:
        og_mat = mat_slot.material
        if og_mat is None:
            continue#no mat
        bn = basename(og_mat.name)
        if bn == og_mat.name:
            continue#no index
        bn_mat=bpy.data.materials.get(bn)
        if bn_mat is None:
            og_mat.name=bn#no mat without index
            continue
        elif compare_dagormats(og_mat,bn_mat):
            replaced.append(og_mat)
            mat_slot.material = bn_mat
            continue
        for new_mat in bpy.data.materials:
            if new_mat.name.startswith(f'{bn}.') and compare_dagormats(mat_slot.material, new_mat):
                if shortest_name(mat_slot.material.name,new_mat.name) == new_mat.name:
                    replaced.append(mat_slot.material)
                    mat_slot.material=new_mat

    for mat in set(replaced):
        if mat.users == 0:
            bpy.data.materials.remove(mat)
    return

def fix_mat_slots(obj):
    log=get_text('log')
    was_broken=False
    mesh=obj.data
    mat_slots=obj.material_slots
    #adding placeholder mat when no material assigned
    if mesh.materials.__len__()==0:
        name = obj.name
        if 'og_name' in obj.keys():
            name = obj['og_name']#for exported duplicates, that have additional indicies
        log.write(f"WARNING! {name} doesn't have material! Temporary autoNamedMat_0 applied\n")
        mesh.materials.append(get_autonamed())
    else:
        for slot in mat_slots:
            if slot.material==None:
                slot.material=get_autonamed()
    #fixing ids that greater than amount of materials by cycling it
    over_id=mat_slots.__len__()
    for poly in obj.data.polygons:
        id=poly.material_index
        if id!=id%over_id:
            poly.material_index=id%over_id
            was_broken=True
    return was_broken

def optimize_mat_slots(obj):
    log=get_text('log')
    proxymat=None
    proxy = obj.dagorprops.get('apex_interior_material:t')
    if proxy is not None:
        proxymat=bpy.data.materials.get(proxy)
        print(proxymat)
    mats = []
    groups=[]
    slots = obj.material_slots
    #no reason to search for similar materials
    if slots.__len__()<=1:
        return
    for m_sl in slots:
        if m_sl.material not in mats:
            mats.append(m_sl.material)
    #looking for groups of sililar materials
    while mats.__len__()>0:
        groups.append([mats[0]])
        mats.remove(mats[0])
        for mat in mats:
            if compare_dagormats(groups[-1][0],mat):
                groups[-1].append(mat)
                mats.remove(mat)
    used_mats = []
    new_ids=[]
    #since materials.clear() kills ids, it can't be updated yet
    for poly in obj.data.polygons:
        id=poly.material_index
        mat = obj.material_slots[id].material
        if mat not in used_mats:
            for gr in groups:
                if mat in gr:
                    mat=gr[0]
                    if gr[0] not in used_mats:
                        used_mats.append(gr[0])
        new_ids.append(used_mats.index(mat))
    obj.data.materials.clear()
    #only unic used materials this time
    for mat in used_mats:
        obj.data.materials.append(mat)
    #ids updated. Finally.
    for poly in obj.data.polygons:
        poly.material_index=new_ids[0]
        new_ids.remove(new_ids[0])
    if proxymat is not None:
        if obj.data.materials.get(proxymat.name) is None:
            obj.data.materials.append(proxymat)
    return

def preserve_sharp(obj):
    log=get_text('log')
    bm=bmesh.new()
    bm.from_mesh(obj.data)
    bm.edges.ensure_lookup_table()
    smooth_angle = obj.data.auto_smooth_angle
    for e in bm.edges:
        if e.is_boundary:
            e.smooth=False
        elif e.link_faces.__len__()>2:
            log.write(f'WARNING!{obj.name}: incorrect geometry, edge {e.index} used more than in two faces!\n')
            e.smooth=False
        elif e.smooth:
            e.smooth = e.calc_face_angle()<smooth_angle
    bm.to_mesh(obj.data)
    del bm
    obj.data.auto_smooth_angle=math.radians(180)

def clear_dagorprops(obj):
    DP = obj.dagorprops
    props=list(DP.keys())
    for prop in props:
        del DP[prop]
    return

def add_bbox(obj):
    bbox=obj.bound_box
    correct_col = obj.users_collection[0]
    bbox_mesh=bpy.data.meshes.new(name=f'{obj.name}.bbox')
    verts = []
    edges=[]
    for coord in bbox:
        verts.append([coord[0],coord[1],coord[2]])
    polygons = [[0,1,2,3],[5,6,2,1],[7,6,5,4],[0,3,7,4],[3,2,6,7],[4,5,1,0]]
    bbox_mesh.from_pydata(verts, edges, polygons)
    bbox_obj = bpy.data.objects.new(bbox_mesh.name,bbox_mesh)
    bbox_obj.data.materials.append(get_gi_black())
    for col in bbox_obj.users_collection: #can be added into wrong collection
        col.objects.unlink(bbox_obj)
    correct_col.objects.link(bbox_obj)
    return bbox_obj

def setup_destr(obj, materialName, density):
    clear_dagorprops(obj)
    DP = obj.dagorprops
    DP['animated_node:b']='yes'
    DP['physObj:b']='yes'
    DP['collidable:b']='no'
    DP['massType:t']='none'
    bbox_obj = add_bbox(obj)
    bbox_obj.name = f'{obj.name}_cls'
    DP = bbox_obj.dagorprops
    DP['materialName:t'] = materialName
    DP['density:r']  = density
    DP['massType:t'] = 'box'
    DP['collType:t'] = 'box'
    bbox_obj.parent=obj
    return

#OPERATORS#################################################################

class DAGOR_OT_ClearSG(Operator):
    bl_idname       = 'dt.clear_smooth_groups'
    bl_label        = 'Clear smoothing groups'
    bl_description  = "Removes SG attribute from selected objects"
    bl_options      = {'UNDO'}
    def execute(self, context):
        objects = context.view_layer.objects.selected
        remove_smooth_groups(objects)
        return {'FINISHED'}

class DAGOR_OT_Materialize(Operator):
    bl_idname       = 'dt.materialize_nodes'
    bl_label        = 'Materialize nodes'
    bl_description  = "Turn selected collection instances into real geometry"
    bl_options      = {'UNDO'}

    def execute(self,context):
        if context.mode != 'OBJECT':
            show_popup('Works only in OBJECT mode!')
            return {'CANCELLED'}

        sel=[obj for obj in context.selected_objects if obj.type=='EMPTY'
                 and obj.instance_collection is not None]

        if sel.__len__()==0:
            show_popup('No collection instances selected!')
            return {'CANCELLED'}

        for obj in sel:
            node_to_geo(obj)
        return {'FINISHED'}

class DAGOR_OT_mopt(Operator):
    bl_idname = "dt.mopt"
    bl_label = "mopt"
    bl_description = "Merges material slots with similar materials; removes unused slots"
    bl_options = {'UNDO'}
    def execute(self, context):
        for obj in context.selected_objects:
            if obj.type=='MESH':
                fix_mat_slots(obj)#fixing ids, replacing None mat by gi_black
                optimize_mat_slots(obj)
        return{'FINISHED'}

class DAGOR_OT_merge_mat_duplicates(Operator):
    bl_idname = "dt.merge_mat_duplicates"
    bl_label = "Merge material duplicates"
    bl_description = "Replace materials by copies with shorter names if it exist"
    bl_options = {'UNDO'}
    def execute(self, context):
        for obj in context.selected_objects:
            if obj.type=='MESH':
                merge_mat_duplicates(obj)
        return{'FINISHED'}

class DAGOR_OT_SaveTextures(Operator):
    bl_idname = "dt.save_textures"
    bl_label = "Save textures"
    bl_description = "save existing textures of selected mesh objects to 'export folder/textures/...'"

    def execute(self,context):
        start = time()
        log=get_text('log')
        sel=[o for o in context.selected_objects if o.type=="MESH"]
        if sel.__len__()==0:
            show_popup(
                message='No selected meshes to process!',
                title='Error!',
                icon='ERROR')
            return {'CANCELLED'}
        path=os.path.join(context.scene.dag_export_path,'textures\\')
        if not os.path.exists(path):
            os.mkdir(path)
            log.write(f'INFO: directory \n{path}\ncreated\n')
        tex_count=save_textures(sel,path)
        log.write(f'INFO: saved {tex_count} texture(s)\n')
        show_popup(f'finished in {round(time()-start,4)} sec')
        return{'FINISHED'}

class DAGOR_OT_PreserveSharp(Operator):
    bl_idname = "dt.preserve_sharp"
    bl_label = "Preserve Sharp Edges"
    bl_description = "Marks sharp edges based on smoothing angle and set it to 180 degrees"
    bl_options = {'UNDO'}
    def execute(self,context):
        sel=list(obj for obj in context.view_layer.objects.selected if obj.type=='MESH')
        if sel.__len__()==0:
            show_popup(
                message='No selected meshes to process!',
                title='Error!',
                icon='ERROR')
            return {'CANCELLED'}
        for obj in sel:
            preserve_sharp(obj)
        return{'FINISHED'}

class DAGOR_OT_ApplyMods(Operator):
    bl_idname = "dt.apply_modifiers"
    bl_label = "Apply Modifiers"
    bl_description = "Apply visible modifiers, delete hidden"
    bl_options = {'UNDO'}
    def execute(self,context):
        sel=list(obj for obj in context.view_layer.objects.selected if obj.type=='MESH')
        if sel.__len__()==0:
            show_popup(
                message='No selected meshes to process!',
                title='Error!',
                icon='ERROR')
            return {'CANCELLED'}
        for obj in sel:
            apply_modifiers(obj)
        return{'FINISHED'}

class DAGOR_OT_GroupCollections(Operator):
    bl_idname = "dt.group_collections"
    bl_label = "sort collections"
    bl_description = "group collections based on their names"
    bl_options = {'UNDO'}
    def execute(self,context):
        Cols=bpy.data.collections
        C=bpy.context
        S=C.scene
        SC=S.collection
#sorting lods
        for col in SC.children:
            if col.name.find('.lod0')!=-1:
                a_name=col.name[:-6]
                try:
                    Cols[a_name]
                except:
                    Cols.new(name=a_name)
                    SC.children.link(Cols[a_name])
                SC.children.unlink(col)
                Cols[a_name].children.link(col)
        found=True
        while found:
            found=sort_collections(C.collection)
        return{'FINISHED'}

class DAGOR_OT_PackOrphans(Operator):
    bl_idname = "dt.pack_orphans"
    bl_label = "pack orphans"
    bl_description = "put objects not in the bottom of hierarchy into new collections with same name"
    bl_options = {'UNDO'}
    def execute(self,context):
        C=bpy.context
        if C.collection is None:
            pack_orphans(C.scene.collection)
        else:
            pack_orphans(C.collection)
        return{'FINISHED'}

class DAGOR_OT_ClearNormals(Operator):
    bl_idname = "dt.clear_normals"
    bl_label = "Clear normals"
    bl_description = "Delete split normal data from selected objects"
    bl_options = {'UNDO'}
    def execute(self,context):
        C=bpy.context
        objects = [obj for obj in C.selected_objects if obj.type=='MESH']
        clear_normals(objects)
        return{'FINISHED'}

class DAGOR_OT_SetupDestruction(Operator):
    bl_idname = "dt.setup_destr"
    bl_label = "Setup Destruction"
    bl_description = "Add box colliders, set object properties for destr objects"
    bl_options = {'UNDO'}

    materialName:   StringProperty(default = "")
    density:        IntProperty(default = 100)

    def execute(self,context):
        objects = [obj for obj in context.selected_objects if obj.type=='MESH']
        for obj in objects:
            setup_destr(obj,self.materialName,self.density)
        return{'FINISHED'}

#PANELS###################################################################
class DAGOR_PT_Tools(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Tools"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    def draw(self,context):
        pref=context.preferences.addons[basename(__package__)].preferences
        S=context.scene
        layout = self.layout

        matbox=layout.box()
        header = matbox.row()
        header.prop(pref, 'matbox_maximized',icon = 'DOWNARROW_HLT'if pref.matbox_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='MaterialTools',expand=True)
        header.label(icon ="MATERIAL")
        if pref.matbox_maximized:
            mats=','.join([m.name for m in bpy.data.materials])
            matbox.operator('dt.mopt',text='Optimize material slots')
            matbox.operator('dt.merge_mat_duplicates',text='Merge Duplicates')
        searchbox=layout.box()
        header = searchbox.row()
        header.prop(pref, 'searchbox_maximized',icon = 'DOWNARROW_HLT'if pref.searchbox_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='SearchTools',expand=True)
        header.label(text='', icon='VIEWZOOM')
        if pref.searchbox_maximized:
            mats=','.join([m.name for m in bpy.data.materials])
            searchbox.operator('dt.find_textures',  text='Find missing textures',  icon='TEXTURE').mats=mats
            searchbox.operator('dt.find_proxymats', text='Find missing proxymats',icon='MATERIAL').mats=mats
        savebox = layout.box()
        header = savebox.row()
        header.prop(pref, 'savebox_maximized',icon = 'DOWNARROW_HLT'if pref.savebox_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='SaveTools',expand=True)
        header.label(text='', icon='FILE_TICK')
        if pref.savebox_maximized:
            savebox.operator('dt.save_textures', text='Save textures',icon='FILE_TICK')

        meshbox=layout.box()
        header = meshbox.row()
        header.prop(pref, 'meshbox_maximized',icon = 'DOWNARROW_HLT'if pref.meshbox_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='MeshTools',expand=True)
        header.label(icon='MESH_DATA')
        if pref.meshbox_maximized:
            meshbox.operator('dt.preserve_sharp',icon='EDGESEL')
            meshbox.operator('dt.apply_modifiers',icon='MODIFIER_ON')
            meshbox.operator('dt.clear_normals',icon='NORMALS_VERTEX_FACE')
            meshbox.operator('dt.clear_smooth_groups',icon='NORMALS_FACE')
            meshbox.operator('dt.materialize_nodes', icon='MESH_MONKEY')

        scenebox=layout.box()
        header=scenebox.row()
        header.prop(pref, 'scenebox_maximized',icon = 'DOWNARROW_HLT'if pref.scenebox_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='SceneTools',expand=True)
        header.label(icon='OUTLINER_COLLECTION')
        if pref.scenebox_maximized:
            scenebox.operator('dt.group_collections')
            scenebox.operator('dt.pack_orphans')

        destrbox=layout.box()
        header = destrbox.row()
        header.prop(pref, 'destrbox_maximized',icon = 'DOWNARROW_HLT'if pref.destrbox_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='DestrSetup',expand=True)
        header.label(text="", icon = 'MOD_PHYSICS')
        if pref.destrbox_maximized:
            mat = destrbox.row()
            mat.label(text="material:")
            mat.prop(pref,'destr_materialName',text='')
            dens=destrbox.row()
            dens.label(text="density:")
            dens.prop(pref, 'destr_density',text='')
            destr = destrbox.operator('dt.setup_destr')
            destr.materialName = pref.destr_materialName
            destr.density = pref.destr_density


classes = [
           DAGOR_OT_ClearNormals,
           DAGOR_OT_ClearSG,
           DAGOR_OT_SaveTextures,
           DAGOR_OT_mopt,
           DAGOR_OT_merge_mat_duplicates,
           DAGOR_OT_PreserveSharp,
           DAGOR_OT_SetupDestruction,
           DAGOR_OT_ApplyMods,
           DAGOR_OT_GroupCollections,
           DAGOR_OT_PackOrphans,
           DAGOR_OT_Materialize,
           DAGOR_PT_Tools
           ]

def register():
    for cl in classes:
        bpy.utils.register_class(cl)


def unregister():
    for cl in classes[::-1]:
        bpy.utils.unregister_class(cl)