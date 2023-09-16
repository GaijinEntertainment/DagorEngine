import bpy, os
from os.path        import join
from bpy.props      import EnumProperty,PointerProperty, IntProperty, BoolProperty, StringProperty
from bpy.types      import PropertyGroup, Operator, Panel
from bpy.utils      import register_class, unregister_class, user_resource

from ..helpers.basename     import basename


tex_set = ['tex_d','tex_d_alpha', 'normal', 'tex_met_gloss', 'tex_n','tex_n_alpha', 'tex_mask']
tex_from =['tex_d','tex_d_alpha', 'normal', 'tex_met_gloss', 'tex_mask']
tex_self =['tex_d','tex_d_alpha', 'tex_n','tex_n_alpha', 'tex_mask']

#FUNCTIONS
def popup(msg):
    def draw(self, context):
        self.layout.label(text = msg)
    bpy.context.window_manager.popup_menu(draw)
    return

def get_supported_materials():
    mats = []
    for m in bpy.data.materials:
        sh = m.node_tree.nodes.get('Shader')
        if sh is None:
            continue
        else:
            mats.append(m)
        for t in tex_set:
            if t == 'normal':
                continue
            if sh.outputs.get(f'_{t}') is None:
                mats.remove(m)
                break
    return mats

#returns list of materials from set of mesh objects
def get_materials(objects):
    mats = []
    for obj in objects:
        mesh = obj.data
        if mesh.materials.__len__()==0:
            return None#error signal for validator
        for mat in obj.data.materials:
            if mat not in mats:
                mats.append(mat)
    return mats

#makes sure that baking is possible
def bake_validate():
    props = bpy.context.scene.dag_bake
    lp_col = props.col_to_bake
    hp_col = props.col_to_bake_from
    supported_materials = get_supported_materials()
#lp_collection
    if props.col_to_bake is None:
        return 'No lowpoly collection selected'
    objs_to_bake = [o for o in lp_col.objects if o.type == 'MESH']
    if objs_to_bake.__len__()==0:
        return f'No mesh objects in "{props.col_to_bake.name}"'
    lp_mats = get_materials(objs_to_bake)
    if lp_mats is None:
        return f'No material on some mesh in "{props.col_to_bake.name}"'
    for obj in objs_to_bake:
        if obj.data.uv_layers.get('Bake') is None:
            return f'"{obj.name}": no"Bake" uv layer'
    try:
        init_mats(lp_mats)
    except:
        return f'Something went wrong on materials initialization'
    if props.self:
        for mat in lp_mats:
            if mat not in supported_materials:
                return f'"{mat.name}" can not be baked'
        return
#hp_collection. Make sence only for hp to lp mode
    if props.col_to_bake_from is None:
        return 'No highpoly collection selected'
    objs_to_bake_from = [o for o in hp_col.objects if o.type == 'MESH']
    if objs_to_bake_from.__len__()==0:
        return f'No mesh objects in "{props.col_to_bake.name}"'
    hp_mats = get_materials(objs_to_bake_from)
    if hp_mats is None:
        return f'No material on some mesh in "{props.col_to_bake_from.name}"'
    for mat in hp_mats:
            if mat not in supported_materials:
                return f'"{mat.name}" can not be baked'
    return

#prepares materials to baking
def init_mats(mats):
    images = bpy.data.images
#necessary stuff for tetures
    props = bpy.context.scene.dag_bake
    lp_col = props.col_to_bake
    mat_name = lp_col.name.split('.')[0]#incase it has.lod0N or something lihe that in name
    dirpath = props.path
    h = int(props.y)
    w = int(props.x)
#images
    for t in tex_set:
        img_name = f'{mat_name}_{t}'
        img = images.get(img_name)
        if img is None:
            img = images.new(name = img_name, height = h, width = w)
        if props.get(t) is not None and props[t]:#initialized and should be rebaked
            img.source = 'GENERATED'
            img.generated_color = (0,0,0,1)
            img.filepath = join(dirpath,img_name+'.tif')
        img.generated_width = w
        img.generated_height = h
        img.colorspace_settings.name = 'sRGB' if t == 'tex_d' else 'Non-Color'
#node_trees of materials
    for mat in mats:
        nodes = mat.node_tree.nodes
        links = mat.node_tree.links
    #uv node for baked maps
        uv = nodes.get('bake_uv')
        if uv is None:
            uv = nodes.new(type = "ShaderNodeAttribute")
            uv.name = 'bake_uv'
        uv.select = False
        uv.attribute_name = "Bake"
        uv.location = (-680,-580)
        uv.select = False
        y_loc = -320
    #image_textures
        for t in tex_set:
            node = nodes.get(t)
            if node is None:
                node = nodes.new(type = 'ShaderNodeTexImage')
                node.name = t
            node.image = images[f'{mat_name}_{t}']
            node.location = (-480,y_loc)
            node.width = 140
            y_loc-=40
            node.hide = True
            node.select = False
            links.new(uv.outputs['Vector'],node.inputs[0])
    return

#baking
def bake_tex(tex):
    S = bpy.context.scene
    lp_col = S.dag_bake.col_to_bake
    tex_type = f'_{tex}'
    bpy.ops.object.select_all(action='DESELECT')
    for obj in lp_col.objects:
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
    mats = get_materials(lp_col.objects)
    for mat in mats:
        nodes = mat.node_tree.nodes
        nodes.active = nodes[tex]
    #setting up renderer
    S.render.engine = 'CYCLES'
    S.render.bake.use_selected_to_active = False
    S.render.bake.target = 'IMAGE_TEXTURES'
    S.render.bake.margin = 0
    #baking
    for mat in mats:
        nodes = mat.node_tree.nodes
        mat.node_tree.links.new(nodes['Shader'].outputs[tex_type], nodes['Material Output'].inputs['Surface'])
    bpy.ops.object.bake(type = 'EMIT')
    #saving changes
    for mat in mats:
        nodes = mat.node_tree.nodes
        img = nodes.active.image
        img.save()
        img.reload()
    for mat in mats:
        nodes = mat.node_tree.nodes
        mat.node_tree.links.new(nodes['Shader'].outputs['shader'], nodes['Material Output'].inputs['Surface'])
    return
#baking from hp
def bake_tex_from(tex):
    S = bpy.context.scene
    lp_col = S.dag_bake.col_to_bake
    hp_col = S.dag_bake.col_to_bake_from
    tex_type = f'_{tex}'
    for obj in lp_col.objects:
        bpy.ops.object.select_all(action='DESELECT')
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        S.render.bake.cage_object = None#we don't need a cage from previous mesh
        for prop in obj.dagorprops.keys():
            if prop.startswith('bake:'):
                hp = hp_col.objects.get(obj.dagorprops[prop])
                if hp is not None:
                    hp.select_set(True)
                else:
                    return f'HP "{obj.dagorprops[prop]}" is not found in "{hp_col.name}"'
            elif prop == 'cage:obj':
                cage = S.objects.get(obj.dagorprops[prop])
                if cage is not None:
                    S.render.bake.cage_object = cage
                else:
                    return f'Cage "{obj.dagorprops[prop]}" is not found"'
        if bpy.context.selected_objects.__len__() == 1:#no defined sources in porps
            for obj in hp_col.objects:
                obj.select_set(True)
        mats = get_materials(lp_col.objects)
        for mat in mats:
            nodes = mat.node_tree.nodes
            nodes.active = nodes[tex]
        src_mats = get_materials(hp_col.objects)

        #setting up renderer
        S.render.engine = 'CYCLES'
        S.render.bake.use_selected_to_active = True
        S.render.bake.target = 'IMAGE_TEXTURES'
        S.render.bake.margin = 0
        #baking
        if tex_type == '_normal':
            for mat in src_mats:
                nodes = mat.node_tree.nodes
                mat.node_tree.links.new(nodes['Shader'].outputs['shader'], nodes['Material Output'].inputs['Surface'])
            bpy.ops.object.bake(type = 'NORMAL')
        else:
            for mat in src_mats:
                nodes = mat.node_tree.nodes
                mat.node_tree.links.new(nodes['Shader'].outputs[tex_type], nodes['Material Output'].inputs['Surface'])
            bpy.ops.object.bake(type = 'EMIT')
        #saving changes
        for mat in mats:
            nodes = mat.node_tree.nodes
            img = nodes.active.image
            img.save()
            img.reload()
        for mat in src_mats:
            nodes = mat.node_tree.nodes
            mat.node_tree.links.new(nodes['Shader'].outputs['shader'], nodes['Material Output'].inputs['Surface'])
    return

def get_resolutions(self, context):
    start = 128
    items = []
    for i in range(6):
        step = str(start*pow(2,i))
        items.append((step,step,step))
    return items

def cleanup_meshes(objects):
    bpy.ops.object.select_all(action='DESELECT')
    supported = get_supported_materials()
    for obj in objects:
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        mesh = obj.data
        indecies = []
        for i in range (mesh.materials.__len__()):
            if mesh.materials[i] not in supported:
                indecies.append(i)
        for v in mesh.vertices:
            v.select = False
        for e in mesh.edges:
            e.select = False
        for p in mesh.polygons:
            p.select = p.material_index in indecies
        bpy.ops.object.editmode_toggle()
        bpy.ops.mesh.delete()
        bpy.ops.object.editmode_toggle()
        bpy.ops.dt.mopt()
        obj.select_set(False)

def get_preview_node_group():
    group_name = '.preview_bake_b'
    node_group = bpy.data.node_groups.get(group_name)
    if node_group is not None:
        return node_group
    lib_path = user_resource('SCRIPTS') + f'\\addons\\dag4blend\\extras\\library.blend\\NodeTree'
    file = lib_path+f"\\{group_name}"
    bpy.ops.wm.append(filepath = file, directory = lib_path,filename = group_name, do_reuse_local_id = True)
    #if nodegroup not found in library it still be a None
    node_group = bpy.data.node_groups.get(group_name)
    return node_group

#CLASSES

#settings and UI config
class dag_baker_props(PropertyGroup):
#img realted
    x:      EnumProperty(items = get_resolutions)
    y:      EnumProperty(items = get_resolutions)

    path:   StringProperty(subtype='DIR_PATH', default = 'C:\\tmp\\')
#mode related
    self:               BoolProperty(default = False)
    col_to_bake:        PointerProperty(type = bpy.types.Collection)
    col_to_bake_from:   PointerProperty(type = bpy.types.Collection)
#list of textures
    tex_d:          BoolProperty(default = False, description = 'diffuse')
    tex_d_alpha:    BoolProperty(default = False, description = 'heightmap, should be stored in alpha of tex_d')
    tex_n:          BoolProperty(default = False, description = 'RG - normal map, B-metallic')
    tex_n_alpha:    BoolProperty(default = False, description = 'glossiness, should be stored in alpha of tex_n')
    normal:         BoolProperty(default = False, description = 'simple normal map, baked from "highpoly" object(s) G is pre-inverted')
    tex_met_gloss:      BoolProperty(default = False, description = 'R - metallic, G - glossiness')
    tex_mask:       BoolProperty(default = False, description = 'mask to generate padding in Substance Designer')
#ui stuff
    settings_unfold:BoolProperty(default = True)
    tex_unfold:     BoolProperty(default = True)

class DAGOR_OT_CleanupMeshes(Operator):
    bl_idname = "dt.prebake_cleanup_meshes"
    bl_label = "Cleanup"
    bl_description = "Removes geometry with unsupported shaders (mostly decals)"
    def execute(self,context):
        log = bpy.data.texts.get('log')
        if log is None:
            log = bpy.data.texts.new('log')
        if context.mode!='OBJECT':
            popup('go to object mode first!')
            return{'CANCELLED'}
        S = context.scene
        if S.dag_bake.self:
            col = S.dag_bake.col_to_bake
        else:
            col = S.dag_bake.col_to_bake_from
        if col is None:
            msg = 'No collection selected!'
            log.write(f'ERROR!: {msg}\n')
            popup(msg)
            return{'CANCELLED'}
        cleanup_meshes(col.objects)
        return {'FINISHED'}

class DAGOR_OT_PreviewBake(Operator):
    bl_idname = "dt.preview_bake"
    bl_label = "Preview bake"
    bl_description = "Shows baked textures on new object'"
    def execute(self,context):
        log = bpy.data.texts.get('log')
        if log is None:
            log = bpy.data.texts.new('log')
        S = context.scene
        col = S.dag_bake.col_to_bake
        if col is None:
            msg = 'No collection selected!'
            log.write(f'ERROR!: {msg}\n')
            popup(msg)
            return{'CANCELLED'}
        prev_col = bpy.data.collections.get(f'{col.name}_baked')
        if prev_col is None:
            prev_col=bpy.data.collections.new(name = f'{col.name}_baked')
            S.collection.children.link(prev_col)
        else:
            meshes = []
            for obj in prev_col.objects:
                if obj.data not in meshes:
                    meshes.append(obj.data)
                bpy.data.objects.remove(obj)
            for mesh in meshes:
                bpy.data.meshes.remove(mesh)
        for obj in col.objects:
            new = obj.copy()
            new.data = obj.data.copy()
            for l in new.data.uv_layers.keys():
                if l!='Bake':
                    new.data.uv_layers.remove(new.data.uv_layers[l])
            new.data.uv_layers[0].name = 'UVMap'
            prev_col.objects.link(new)
        name = col.name.split('.')[0]
        mat = bpy.data.materials.get(f'{name}_simple')
        if mat is None:
            mat = bpy.data.materials.new(f'{name}_simple')
        mat.dagormat.shader_class = 'rendinst_simple'
        mat.dagormat.textures.tex0 = f'{name}_tex_d'
        mat.dagormat.textures.tex1 = f'{name}_tex_n'
        mat.dagormat.textures.tex2 = f'{name}_tex_n_alpha'
        mat.dagormat.textures.tex3 = f'{name}_normal'
        mat.dagormat.textures.tex4 = f'{name}_tex_met_gloss'
        bpy.ops.dt.dagormats_update(mats = f'{name}_simple')
        nodes = mat.node_tree.nodes
        links = mat.node_tree.links
        if S.dag_bake.self:
            links.new(nodes['tex1'].outputs[0],nodes['Shader'].inputs['tex2'])
            links.new(nodes['tex2'].outputs[0],nodes['Shader'].inputs['tex2_alpha'])
        else:
            node_tree = get_preview_node_group()
            converter = nodes.new(type="ShaderNodeGroup")
            converter.node_tree = node_tree
            converter.location = [-260,-320]
            converter.width = 60
            converter.hide = True
            converter.select = False
            links.new(nodes['tex3'].outputs[0],converter.inputs[0])
            links.new(nodes['tex4'].outputs[0],converter.inputs[1])
            links.new(converter.outputs[0],nodes['Shader'].inputs['tex2'])
            links.new(converter.outputs[1],nodes['Shader'].inputs['tex2_alpha'])
        for obj in prev_col.objects:
            obj.data.materials.clear()
            obj.data.materials.append(mat)
        return {'FINISHED'}

class DAGOR_OT_RunBake(Operator):
    bl_idname = "dt.run_bake"
    bl_label = "Run bake"
    bl_description = "Bakes dag shaders to 'rendinst_simple'"
    def execute(self,context):
        props = context.scene.dag_bake
        log = bpy.data.texts.get('log')
        if log is None:
            log = bpy.data.texts.new('log')
        msg = bake_validate()
        if msg is not None:
            popup(msg)
            log.write(f'ERROR: {msg}\n')
            return{'CANCELLED'}
        if props.self:
            for tex in tex_self:
                if props.get(tex) is not None and props[tex]:
                    bake_tex(tex)
        else:
            for tex in tex_from:
                if props.get(tex) is not None and props[tex]:
                    bake_tex_from(tex)
        msg = 'Baking comlpete!'
        popup(msg)
        log.write(f'{msg}\n')
        return {'FINISHED'}

class DAGOR_PT_Baker(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Bake"
    bl_category = "Dagor"
    bl_options = {'DEFAULT_CLOSED'}
    @classmethod
    def poll(self, context):
        pref = bpy.context.preferences.addons[basename(__package__)].preferences
        return pref.use_tex_baker
    def draw(self, context):
        S = context.scene
        props = context.scene.dag_bake
        l = self.layout

        main = l.box()
        name = main.row()
        name.prop(props,'settings_unfold', text = '', icon = 'DOWNARROW_HLT' if props.settings_unfold else 'RIGHTARROW', emboss = False)
        name.label(text = "Settings")
        name.label(text = "", icon = 'SETTINGS')
        if props.settings_unfold:
            main.prop(S.cycles, 'device')
            main.prop(S.cycles, 'samples')
            main.label(text = 'Resolution:')
            res = main.row()
            res.prop(props,'x',text = '')
            res.label(text = '', icon = 'LAYER_ACTIVE')
            res.prop(props,'y',text = '')
            main.label (text = 'Mode:')
            mod_a = main.row()
            mod_a.prop(props,'self',text = '', icon = 'CHECKBOX_HLT' if props.self else 'CHECKBOX_DEHLT', emboss = False)
            mod_a.label(text = 'Mesh(es) to itself')
            mod_b = main.row()
            mod_b.prop(props,'self',text = '', icon = 'CHECKBOX_DEHLT' if props.self else 'CHECKBOX_HLT', emboss = False)
            mod_b.label(text = 'From other mesh(es)')
            main.prop(props,'col_to_bake', text = 'Asset' if props.self else 'Lowpoly')
            if not props.self:
                main.prop(props,'col_to_bake_from', text = 'Asset' if props.self else 'Highpoly')
                main.prop(S.render.bake,'use_cage', text = 'Cage mode')
                main.prop(S.render.bake,'cage_extrusion')
                main.prop(S.render.bake, 'max_ray_distance')
            main.prop(props, 'path', text = 'Save to')

        tex = l.box()
        name = tex.row()
        name.prop(props, 'tex_unfold', text = '', icon = 'DOWNARROW_HLT' if props.tex_unfold else 'RIGHTARROW', emboss = False)
        name.label(text = 'Textures to bake')
        name.label(text = '', icon = 'TEXTURE')
        if props.tex_unfold:
            tex.prop(props,'tex_d', toggle = True)
            tex.prop(props,'tex_d_alpha', toggle = True)
            if props.self:
                tex.prop(props,'tex_n', toggle = True)
                tex.prop(props,'tex_n_alpha', toggle = True)
            else:
                tex.prop(props,'normal', toggle = True)
                tex.prop(props,'tex_met_gloss', toggle = True)
            tex.prop(props,'tex_mask', toggle = True)

        l.operator('dt.prebake_cleanup_meshes', text = 'Cleanup geometry')
        l.operator('dt.run_bake', text = 'BAKE')
        l.operator('dt.preview_bake', text = 'Preview baked')
        return
classes = [
            dag_baker_props,
            DAGOR_OT_CleanupMeshes,
            DAGOR_OT_RunBake,
            DAGOR_PT_Baker,
            DAGOR_OT_PreviewBake,
            ]

def register():
    for cl in classes:
        register_class(cl)
    S = bpy.types.Scene
    S.dag_bake = PointerProperty (type = dag_baker_props)
    return

def unregister():
    del bpy.types.Scene.dag_bake
    for cl in classes:
        unregister_class(cl)
    return