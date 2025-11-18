import bpy, os, bmesh, math
from os.path    import join, exists
from   bpy.types                    import Operator, Panel
from bpy.props                      import BoolProperty, StringProperty, IntProperty
from   shutil                       import copyfile
from   time                         import time
from ..dagormat.build_node_tree     import build_dagormat_node_tree
from ..dagormat.compare_dagormats   import compare_dagormats
from ..dagormat.dagormat_functions  import write_proxy_blk, cleanup_textures

from  .tools_functions              import *
from ..helpers.texts                import log, show_text
from ..helpers.getters              import get_preferences
from ..ui.draw_elements             import draw_custom_header
from ..helpers.names                import texture_basename, ensure_no_extension
from ..popup.popup_functions    import show_popup
from ..helpers.version              import get_blender_version

classes = []

#FUNCTIONS#################################################################

def mesh_remove_smooth_groups(objects):
    for obj in objects:
        if obj.type == 'MESH':
            mesh = obj.data
            SG = mesh.attributes.get('SG')
            if SG is not None:
                mesh.attributes.remove(SG)
    return

def save_textures(sel,path):
    cleanup_textures()
    all_tex=[]
#collecting all textures of selected mesh objects:
    for o in sel:
        for mat in o.data.materials:
            T=mat.dagormat.textures
            for key in mat.dagormat.textures.keys():
                if T[key]!="":
                    all_tex.append(texture_basename(T[key]))
    all_tex=list(set(all_tex))#removing doubles
#separating textures only with existing paths, to avoid saving "checkers"
    tex_to_save=[]
    for t in all_tex:
        img=bpy.data.images.get(t)
        if img is None:
            continue
        if exists(img.filepath):
            tex_to_save.append(img.filepath)
#saving
    for tex in tex_to_save:
        tex_name = texture_basename(tex)
        save_path = join(path,tex_name)
        copyfile(tex,save_path)
    saved = tex_to_save.__len__()
    skipped = all_tex.__len__() - saved
    return [saved, skipped]

def save_proxymats(selection, dirpath):
    proxymats = []
    for object in selection:
        for material in object.data.materials:
            if not material.dagormat.is_proxy:
                continue
            if material in proxymats:
                continue
            proxymats.append(material)
    for proxymat in proxymats:
        write_proxy_blk(proxymat, custom_dirpath = dirpath)
    return proxymats.__len__()

def clear_normals(objects):
    for obj in objects:
        with bpy.context.temp_override(object = obj):
            bpy.ops.mesh.customdata_custom_splitnormals_clear()
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
    C = bpy.context
    with C.temp_override(object = obj):
        if C.scene.collection.objects.get(obj.name) is not None:
            was_linked = False
        else:
            was_linked = True
            C.scene.collection.objects.link(obj)
        og_hide_mode = [obj.hide_viewport, obj.hide_get()]
        obj.hide_viewport = False   #one with monitor icon
        obj.hide_set(False)         #one with eye icon
        for mod in obj.modifiers:
            if mod.show_viewport:
                try:
                    bpy.ops.object.modifier_apply(modifier=mod.name)
                except:
                    node_name = obj['og_name'] if 'name' in obj.keys() else obj.name
                    message = f'"{mod.name}" modifier can not be applyed to "{node_name}" object\n'
                    log(message, type = 'ERROR', show = True)
        if was_linked:
            C.scene.collection.objects.unlink(obj)
        #restoring visibility
        obj.hide_viewport = og_hide_mode[0]
        obj.hide_set(og_hide_mode[1])
    obj.modifiers.clear()
    return

def get_autonamed():
    mat=bpy.data.materials.get('autoNamedMat_0')
    if mat==None:
        mat=bpy.data.materials.new('autoNamedMat_0')
        mat.dagormat.shader_class='gi_black'
        build_dagormat_node_tree(mat)
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
        bn = ensure_no_extension(og_mat.name)
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
    was_broken=False
    mesh=obj.data
    mat_slots=obj.material_slots
    #adding placeholder mat when no material assigned
    if mesh.materials.__len__()==0:
        name = obj.name
        if 'og_name' in obj.keys():
            name = obj['og_name']#for exported duplicates, that have additional indicies
        log(f'object "{name}" does not have material! Temporary autoNamedMat_0 applied\n', type = 'WARNING', show = True)
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
    bm=bmesh.new()
    bm.from_mesh(obj.data)
    bm.edges.ensure_lookup_table()
    smooth_angle = obj.data.auto_smooth_angle
    name = obj.name
    if 'og_name' in obj.keys():
        name = obj['og_name']
    for e in bm.edges:
        if e.is_boundary:
            e.smooth=False
        elif e.link_faces.__len__()>2:
            log(f'Object "{name}" has incorrect geometry, edge {e.index} used in more than two faces!\n', type ='WARNING')
            e.smooth=False
        elif e.link_faces.__len__()==0:
            log(f'Object "{name}" has incorrect geometry, edge {e.index} is not used in any faces!\n', type = 'WARNING')
        elif e.smooth:
            e.smooth = e.calc_face_angle()<smooth_angle
    bm.to_mesh(obj.data)
    del bm
    obj.data.auto_smooth_angle=math.radians(180)
    return

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
        mesh_remove_smooth_groups(objects)
        return {'FINISHED'}
classes.append(DAGOR_OT_ClearSG)


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
classes.append(DAGOR_OT_mopt)


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
classes.append(DAGOR_OT_merge_mat_duplicates)


class DAGOR_OT_SaveTextures(Operator):
    bl_idname = "dt.save_textures"
    bl_label = "Copy textures to"
    bl_description = "Copies existing textures of selected mesh objects to custom directory"

    def execute(self,context):
        start = time()
        pref_local = bpy.data.scenes[0].dag4blend
        sel = [o for o in context.selected_objects if o.type == "MESH"]
        if sel.__len__()==0:
            show_popup(
                message = 'No selected meshes to process!',
                title = 'Error!',
                icon = 'ERROR')
            return {'CANCELLED'}
        path = pref_local.tools.save_textures_dirpath
        if not exists(path):
            os.mkdir(path)
            log(f'\n\tDirectory \n\t"{path}"\n\tcreated\n')
        tex_count = save_textures(sel, path)
        log(f'saved {tex_count[0]} texture(s)\n\tskipped {tex_count[1]} texture(s) without filepath\n')
        show_text(bpy.data.texts['log'])
        show_popup(f'finished in {round(time()-start,4)} sec')
        return{'FINISHED'}
classes.append(DAGOR_OT_SaveTextures)


class DAGOR_OT_SaveProxymats(Operator):
    bl_idname = "dt.save_proxymats"
    bl_label = "Save proxymats to"
    bl_description = "saves existing proxymats of selected mesh objects to custom directory"

    def execute(self,context):
        start = time()
        pref_local = bpy.data.scenes[0].dag4blend
        sel = [o for o in context.selected_objects if o.type == "MESH"]
        if sel.__len__()==0:
            show_popup(
                message='No selected meshes to process!',
                title='Error!',
                icon='ERROR')
            return {'CANCELLED'}
        path = pref_local.tools.save_proxymats_dirpath
        if not exists(path):
            os.mkdir(path)
            log(f'\n\tDirectory \n\t"{path}"\n\tcreated\n')
        proxy_count = save_proxymats(sel, path)
        log(f'saved {proxy_count} proxymat(s)\n')
        show_text(bpy.data.texts['log'])
        show_popup(f'finished in {round(time()-start,4)} sec')
        return{'FINISHED'}
classes.append(DAGOR_OT_SaveProxymats)


class DAGOR_OT_PreserveSharp(Operator):
    bl_idname = "dt.preserve_sharp"
    bl_label = "Preserve Sharp Edges"
    bl_description = "Marks sharp edges based on smoothing angle and set it to 180 degrees"
    if get_blender_version()>=4.1:
        bl_description = "Does nothing in this blender version, auto_smooth_angle was removed in 4.1..."
    bl_options = {'UNDO'}
    @classmethod
    def poll(cls, context):
        ver = get_blender_version()
        return ver < 4.1

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
classes.append(DAGOR_OT_PreserveSharp)


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
classes.append(DAGOR_OT_ApplyMods)


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
classes.append(DAGOR_OT_GroupCollections)


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
classes.append(DAGOR_OT_PackOrphans)


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
classes.append(DAGOR_OT_ClearNormals)


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
classes.append(DAGOR_OT_SetupDestruction)


class DAGOR_OT_Preview_VCol_Deformation(Operator):
    bl_idname = 'dt.vcol_deform_preview'
    bl_label = 'Preview Deformation'
    bl_description = 'Applies Geometry Nodes modifier to preview deformation'
    bl_options = {'UNDO'}

    @classmethod
    def poll(self, context):
        selection = [o for o in context.selected_objects
            if o.type == 'MESH'
            and o.data.color_attributes.__len__() > 0]
        len = selection.__len__()
        return len > 0

    def execute(self, context):
        selection = [o for o in context.selected_objects
            if o.type == 'MESH'
            and o.data.color_attributes.__len__() > 0]
        for object in selection:
            preview_vcol_deformation(object)
        return{'FINISHED'}
classes.append(DAGOR_OT_Preview_VCol_Deformation)

class DAGOR_OT_Shapekey_to_Color_Attribute(Operator):
    bl_idname = 'dt.shape_to_vcol'
    bl_label = "Shapekey to vcol"
    bl_description = "Stores shapekey [1] offsets as color attribute"
    bl_options = {'UNDO'}

    configure_shaders:  BoolProperty(default = False)
    preview_deformation:BoolProperty(default = False)

    @classmethod
    def poll(self, context):
        if context.mode != 'OBJECT':
            return False
        # only meshes with two shape keys are valid. First one is "basis", and second is deformation itself.
        selection = context.selected_objects
        valid_selection = [ob for ob in selection if ob.type == 'MESH'
                            and ob.data.shape_keys is not None
                            and ob.data.shape_keys.key_blocks.__len__() == 2]
        if valid_selection.__len__() == 0:
            return False
        return True

    def execute(self, context):
        selection = context.selected_objects
        for obj in selection:
            if obj.type != 'MESH':
                log(f'Object "{obj.name}" is not a mesh, skipped\n', type = 'WARNING', show = True)

            elif obj.data.shape_keys is None or obj.data.shape_keys.key_blocks.__len__() < 2:
                log(f'Object "{obj.name}" has no deformation shape key, skipped\n', type = 'WARNING', show = True)

            elif obj.data.shape_keys.key_blocks.__len__() > 2:
                log(f'Object "{obj.name}" has too many shape keys, it is unclear which one to use, skipped\n',
                    type = 'WARNING', show = True)
            else:
                shapekey_to_color_attribute(obj, configure_shaders = self.configure_shaders, preview_deformation = self.preview_deformation)
        return{'FINISHED'}
classes.append(DAGOR_OT_Shapekey_to_Color_Attribute)

#PANELS###################################################################
class DAGOR_PT_Tools(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Tools"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    def draw(self,context):
        pref = get_preferences()
        pref_local = bpy.data.scenes[0].dag4blend
        S = context.scene
        layout = self.layout
        toolbox = layout.column(align = True)

        matbox=toolbox.box()
        draw_custom_header(matbox, 'MaterialTools', pref, 'matbox_maximized', icon ="MATERIAL")
        if pref.matbox_maximized:
            matbox = matbox.column(align = True)
            row = matbox.row()
            row.operator('dt.mopt',text='Optimize material slots')
            row = matbox.row()
            row.operator('dt.merge_mat_duplicates',text='Merge Duplicates')

        searchbox=toolbox.box()
        draw_custom_header(searchbox, 'SearchTools', pref, 'searchbox_maximized', icon ="VIEWZOOM")
        if pref.searchbox_maximized:
            searchbox = searchbox.column(align = True)

            searchbox.context_pointer_set(name = "prop_owner", data = pref)
            row = searchbox.row(align = True)
            single = row.operator('dt.set_value', text = 'Current material', depress = not pref.process_all_materials)
            single.prop_value = ('0')
            single.prop_name = 'process_all_materials'
            single.description = 'Process only active material'
            all = row.operator('dt.set_value', text = "All materials", depress = pref.process_all_materials)
            all.prop_value = '1'
            all.prop_name = 'process_all_materials'
            all.description = "Process every dagormat in the blend file"

            searchbox.separator()

            row = searchbox.row()
            row.operator('dt.find_textures',  text='Find missing textures',  icon='TEXTURE')
            row = searchbox.row()
            row.operator('dt.find_proxymats',
                text = 'Find missing proxymats' if pref.process_all_materials else 'Find missing proxymat',
                icon='MATERIAL')

        savebox = toolbox.box()
        draw_custom_header(savebox, 'SaveTools', pref, 'savebox_maximized', icon ="FILE_TICK")
        if pref.savebox_maximized:
            box = savebox.box()
            save_tex = box.column(align = True)
            button = save_tex.row()
            button.scale_y = 2.0
            button.operator('dt.save_textures', icon = 'FILE_TICK')
            save_tex.separator()
            save_tex.prop(pref_local.tools, 'save_textures_dirpath')
            row = save_tex.row()
            open_dir = row.operator('wm.path_open', text = "Open Directory", icon = 'FILE_FOLDER')
            open_dir.filepath = pref_local.tools.save_textures_dirpath
            row.active = row.enabled = exists(pref_local.tools.save_textures_dirpath)

            box = savebox.box()
            save_proxy = box.column(align = True)
            button = save_proxy.row()
            button.scale_y = 2.0
            button.operator('dt.save_proxymats', icon = 'FILE_TICK')
            save_proxy.separator()
            save_proxy.prop(pref_local.tools, 'save_proxymats_dirpath')
            row = save_proxy.row()
            open_dir = row.operator('wm.path_open', text = "Open Directory", icon = 'FILE_FOLDER')
            open_dir.filepath = pref_local.tools.save_proxymats_dirpath
            row.active = row.enabled = exists(pref_local.tools.save_proxymats_dirpath)

        meshbox=toolbox.box()
        header = meshbox.row()
        header.prop(pref, 'meshbox_maximized',icon = 'DOWNARROW_HLT'if pref.meshbox_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='MeshTools')
        header.prop(pref, 'meshbox_maximized', text = "", icon='MESH_DATA', emboss = False)
        if pref.meshbox_maximized:
            meshbox = meshbox.column(align = True)
            row = meshbox.row()
            row.operator('dt.apply_modifiers',icon='MODIFIER_ON')
            row = meshbox.row()
            row.operator('dt.clear_normals',icon='NORMALS_VERTEX_FACE')
            row = meshbox.row()
            row.operator('dt.clear_smooth_groups',icon='NORMALS_FACE')

        scenebox=toolbox.box()
        draw_custom_header(scenebox, 'SceneTools', pref, 'scenebox_maximized', icon ="OUTLINER_COLLECTION")
        if pref.scenebox_maximized:
            scenebox = scenebox.column(align = True)
            row = scenebox.row()
            row.operator('dt.group_collections')
            row = scenebox.row()
            row.operator('dt.pack_orphans')

        destrbox=toolbox.box()
        draw_custom_header(destrbox, 'Destruction', pref, 'destrbox_maximized', icon ="MOD_PHYSICS")
        if pref.destrbox_maximized:
            box = destrbox.box()
            draw_custom_header(box, 'Basic Destr', pref, 'destrsetup_maximized', icon ="MOD_PHYSICS")
            if pref.destrsetup_maximized:
                box = box.column(align = True)
                button = box.row()
                button.scale_y = 2.0
                destr = button.operator('dt.setup_destr', icon = 'MOD_PHYSICS')
                destr.materialName = pref_local.tools.destr_materialName
                destr.density = pref_local.tools.destr_density
                mat = box.row()
                mat.prop(pref_local.tools,'destr_materialName', text='material')
                dens=box.row()
                dens.prop(pref_local.tools, 'destr_density', text='density')
                box.separator()

            box = destrbox.box()
            draw_custom_header(box, 'Shapekey Deform', pref, 'destrdeform_maximized', icon ="SHAPEKEY_DATA")
            if pref.destrdeform_maximized:
                column = box.column(align = True)
                vcol = column.row()
                vcol.scale_y = 2.0
                preview = vcol.operator('dt.shape_to_vcol', icon = 'GROUP_VCOL')
                preview.configure_shaders = pref_local.tools.configure_shaders
                preview.preview_deformation = pref_local.tools.preview_deformation
                prop = column.row()
                prop.prop(pref_local.tools, 'configure_shaders', toggle = True,
                icon = 'CHECKBOX_HLT' if pref_local.tools.configure_shaders else 'CHECKBOX_DEHLT')
                prop = column.row()
                prop.prop(pref_local.tools, 'preview_deformation', toggle = True,
                icon = 'CHECKBOX_HLT' if pref_local.tools.preview_deformation else 'CHECKBOX_DEHLT')
                column.separator()
                column.operator('dt.vcol_deform_preview', icon = 'HIDE_OFF')
        return
classes.append(DAGOR_PT_Tools)


def register():
    for cl in classes:
        bpy.utils.register_class(cl)


def unregister():
    for cl in classes[::-1]:
        bpy.utils.unregister_class(cl)