import bpy

from bpy.types                  import Panel, Operator, PropertyGroup
from bpy.utils                  import register_class, unregister_class, user_resource
from bpy.props                  import StringProperty, BoolProperty, PointerProperty
from mathutils                  import Vector
from math                       import pi
from ..helpers.basename         import basename
from ..helpers.popup            import show_popup
from ..helpers.texts            import log
from ..tools.tools_functions    import *
from ..tools.tools_panel        import apply_modifiers

from .cmp_import                import DAGOR_OP_CmpImport
from .cmp_export                import DAGOR_OP_CmpExport

#FUNCTIONS

def get_collection(name):
    col = bpy.data.collections.get(name)
    if col is None:
        col = bpy.data.collections.new(name)
    return col

#adds transefer_col to each scene
def upd_transfer_collection():
    t_col = get_collection('TRANSFER_COLLECTION')
    for s in bpy.data.scenes:
        if s.collection.children.get(t_col.name) is None:
            s.collection.children.link(t_col)
    return

#creates scenes for cmp supported data types
def upd_scenes():
    for S in ["COMPOSITS", "GAMEOBJ", "GEOMETRY", "TECH_STUFF"]:
        if bpy.data.scenes.get(S) is None:
            new_scene = bpy.data.scenes.new(name = S)
            new_scene.world = bpy.data.worlds[0]
    upd_transfer_collection()
    return

#removes unused "ranom.NNN" collections
def cleanup_tech_stuff():
    scene = bpy.data.scenes.get('TECH_STUFF')
    if scene is None:
        scene = bpy.context.scene
    for col in scene.collection.children:
        if col.name.startswith('random.') and col.users ==1:
            for obj in list(col.objects):
                if obj.users==1:
                    bpy.data.objects.remove(obj)
            bpy.data.collections.remove(col)
    return

#returns all "parents" of collection
def col_users_collection(subcol):
    users = []
    collections = list(bpy.data.collections)
    for scene in bpy.data.scenes:
        collections.append(scene.collection)
    for col in collections:
        if subcol in list(col.children):
            users.append(col)
    return users

#removes entity from random node
def node_remove_entity(node,entity):
    node.instance_collection.objects.unlink(entity)
    if entity.users==0:
        bpy.data.objects.remove(entity)
    if node.instance_collection.objects.__len__()==1:
        node.instance_collection = node.instance_collection.objects[0].instance_collection
    cleanup_tech_stuff()
    return

#turns basic node into random; adds extra entyty to random node
def node_add_entity(node):
    col = node.instance_collection
    if col is None or not col.name .startswith('random.'):
        parent_scene = bpy.data.scenes.get('TECH_STUFF')
        if parent_scene is None:
            parent = bpy.context.scene.collection
        else:
            parent = parent_scene.collection
        rand_col = bpy.data.collections.new(name = 'random.000')
        parent.children.link(rand_col)
        entity = bpy.data.objects.new('',None)
        entity.instance_type = 'COLLECTION'
        entity.instance_collection = col
        rand_col.objects.link(entity)
        node.instance_collection = rand_col
    else:
        rand_col = col
    new_entity = bpy.data.objects.new('',None)
    new_entity.instance_type = 'COLLECTION'
    rand_col.objects.link(new_entity)
    cleanup_tech_stuff()
    return

#returns geometry nodegroup that turns collection into single mesh
def get_converter_ng():
    group_name = "GN_col_to_mesh"
    addon_name = basename(__package__)
    lib_path = user_resource('SCRIPTS') + f'\\addons\\{addon_name}\\extras\\library.blend\\NodeTree'
    file = lib_path+f"\\{group_name}"
    bpy.ops.wm.append(filepath = file, directory = lib_path,filename = group_name, do_reuse_local_id = True)
    node_group = bpy.data.node_groups.get(group_name)
    return node_group

#turns bboxes of selected objects into chosen node
def bbox_to_gameobj(source, gameobj_collection):
#creating new node
    gameobj_node = bpy.data.objects.new(gameobj_collection.name,None)
    for col in source.users_collection:
        col.objects.link(gameobj_node)
    gameobj_node.empty_display_type = 'CUBE'
    gameobj_node.empty_display_size = 0.5
    gameobj_node.instance_type = 'COLLECTION'
    gameobj_node.instance_collection = gameobj_collection
    og_matrix = source.matrix_world
    rotation = og_matrix.to_euler()
#converting
    bbox = source.bound_box
    min = Vector(bbox[0])
    max = Vector(bbox[6])
    gameobj_node.matrix_world = og_matrix
    gameobj_node.location = (og_matrix @ min+og_matrix @ max)/2
    gameobj_node.scale *= (max-min)
#cleanup
    bpy.data.objects.remove(source)
    return


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
    if mod.node_group is not None:
        bpy.data.node_groups.remove(mod.node_group)
    converter_group = get_converter_ng()
    mod.node_group = converter_group
    mod['Input_1'] = node.instance_collection
    #cleanup
    apply_modifiers(geo_node)
    bpy.data.objects.remove(node)
    #shading
    geo_node.data.use_auto_smooth = True
    geo_node.data.auto_smooth_angle = 2*pi
    #fixing UV order
    reorder_uv_layers(geo_node.data)
    return

#hides instance_collection, spawns objects of instance_collection instead
def node_explode(node):
    tm = node.matrix_world
    col = node.instance_collection
    #rendinsts
    if col.name.endswith('.lods'):
        for lod in col.children:
            lod_name = lod.name[lod.name.find(".")+1:]
            subnode = bpy.data.objects.new(f'{lod_name}',None)
            subnode.instance_collection = lod
            subnode.instance_type = 'COLLECTION'
            for col_user in node.users_collection:
                col_user.objects.link(subnode)
            if subnode.parent is None:
                subnode.parent = node
            node_explode(subnode)
    #random_stuff and subcomposits
    else:
        for obj in col.objects:
            entity = obj.copy()
            for col in node.users_collection:
                col.objects.link(entity)
                if entity.parent is None:
                    entity.parent = node
                elif col.objects.get(entity.parent.name) is not None:
                    pass
                else:
                    entity.parent = node
                    entity.matrix_world = node.matrix_world @ obj.matrix_world
    node.instance_type = 'NONE'
    return

#Clears instance_collection and puts node.chidren into it
def node_rebuild(node):
    col = node.instance_collection
    if col.name.endswith('.lods'):
        for lod in list(node.children):
            node_rebuild(lod)
            bpy.data.objects.remove(lod)
    else:
        for obj in list(col.objects):
            col.objects.unlink(obj)
        for child in node.children_recursive:
            for user in child.users_collection:
                user.objects.unlink(child)
            col.objects.link(child)
            if child.parent == node:
                child.parent = None
            #random node can't have offsets inside
            if col.name.startswith('random.'):
                child.location = [0,0,0]
                child.rotation_euler = [0,0,0]
                child.scale = [1,1,1]
    node.instance_type = 'COLLECTION'
    return

#Clears node.children and turns instancing back
def node_revert(node):
    #RI with multiple nodes
    if node.instance_collection.name.endswith('.lods'):
        for lod in list(node.children):
            for child in list(lod.children):
                bpy.data.objects.remove(child)
            bpy.data.objects.remove(lod)
    else:
        for child in list(node.children):
            bpy.data.objects.remove(child)
    node.instance_type = 'COLLECTION'
    return

#OPERATORS
class DAGOR_OT_node_explode(Operator):
    bl_idname = 'dt.node_explode'
    bl_label = 'Explode'
    bl_description = 'Replace node.instance_collection by content of it, parented to the same node'
    bl_options = {'UNDO'}

    node: StringProperty(default = '')

    def execute(self, context):
        node = bpy.data.objects.get(self.node)
        err = None
        if node is None:
            err = 'No active object detected!'
        elif node.type!='EMPTY':
            err = 'Active object is not an empty!'
        elif node.instance_type!='COLLECTION':
            err = 'Instancing is turned off!'
        elif node.instance_collection is None:
            err = 'Node does not have instance collection to begin with!'
        if err is not None:
            log(f'"{err}"\n', type = 'ERROR', show = True)
            show_popup(message = err)
            return {'CANCELLED'}
        node_explode(node)
        return {'FINISHED'}

class DAGOR_OT_node_revert(Operator):
    bl_idname = 'dt.node_revert'
    bl_label = 'Revert'
    bl_description = 'Turn exploded node back to original state'
    bl_options = {'UNDO'}

    node: StringProperty(default = '')

    def execute(self, context):
        node = bpy.data.objects.get(self.node)
        err = None
        if node is None:
            err = 'No active object detected!'
        elif node.type!='EMPTY':
            err = 'Active object is not an empty!'
        elif node.instance_collection is None:
            err = 'Node does not have instance collection to begin with!'
        if err is not None:
            log(f'{err}\n', type = 'ERROR', show = True)
            show_popup(message = err)
            return {'CANCELLED'}
        node_revert(node)
        return {'FINISHED'}

class DAGOR_OT_node_rebuild(Operator):
    bl_idname = 'dt.node_rebuild'
    bl_label = 'Rebuild'
    bl_description = 'Update instance collection based on exploded node'
    bl_options = {'UNDO'}

    node: StringProperty(default = '')

    def execute(self, context):
        node = bpy.data.objects.get(self.node)
        err = None
        if node is None:
            err = 'No active object detected!'
        elif node.type!='EMPTY':
            err = 'Active object is not an empty!'
        elif node.instance_collection is None:
            err = 'Node does not have instance collection to begin with!'
        if err is not None:
            log(f'{err}\n', type = 'ERROR', show = True)
            show_popup(message = err)
            return {'CANCELLED'}
        node_rebuild(node)
        return {'FINISHED'}

class DAGOR_OT_node_make_unic(Operator):
    bl_idname = 'dt.node_make_unic'
    bl_label = 'Make unic'
    bl_description = 'Replaces instance collection by exact copy'
    bl_options = {'UNDO'}

    node: StringProperty(default = '')

    def execute(self,context):
        node = bpy.data.objects.get(self.node)
        err = None
        if node is None:
            err = 'No active object detected!'
        elif node.type!='EMPTY':
            err = 'Active object is not an empty!'
        elif node.instance_type!='COLLECTION':
            err = 'Instancing is turned off!'
        elif node.instance_collection is None:
            err = 'Node does not have instance collection to begin with!'
        elif not node.instance_collection.name.startswith('random.'):
            err = 'Node is not random!'
        if err is not None:
            log(f'"{err}"\n', type = 'ERROR')
            show_popup(message = err)
            return {'CANCELLED'}
        cleanup_tech_stuff()
        col = node.instance_collection
        new_col = bpy.data.collections.new(name = 'random.000')
        for parent in col_users_collection(col):
            parent.children.link(new_col)
        for obj in col.objects:
            new_obj = obj.copy()
            new_col.objects.link(new_obj)
        node.instance_collection = new_col
        return {'FINISHED'}

class DAGOR_OT_node_remove_entity(Operator):
    bl_idname = "dt.node_remove_entity"
    bl_label = 'Remove entity'
    bl_description = 'Removes an entity from random node'
    bl_options = {'UNDO'}

    node: StringProperty(default = '')
    entity: StringProperty(default = '')

    def execute(self, context):
        node = bpy.data.objects.get(self.node)
        entity = bpy.data.objects.get(self.entity)
        err = None
        if node is None:
            err = 'No node selected!'
        elif entity is None:
            err = 'No entity to delete!'
        if err is not None:
            log(f'{err}\n', type = 'ERROR', show = True)
            show_popup(message = err)
            return {'CANCELLED'}
        node_remove_entity(node,entity)
        return {'FINISHED'}

class DAGOR_OT_node_add_entity(Operator):
    bl_idname = "dt.node_add_entity"
    bl_label = 'Add entity'
    bl_description = 'Adds an entity to random node,turns static node into random one'
    bl_options = {'UNDO'}

    node: StringProperty(default = '')

    def execute(self, context):
        node = bpy.data.objects.get(self.node)
        err = None
        if node is None:
            err = 'No node selected!'
        if err is not None:
            log(f'{err}\n', type = 'ERROR', show = True)
            show_popup(message = err)
            return {'CANCELLED'}
        node.instance_type = 'COLLECTION'
        node_add_entity(node)
        return {'FINISHED'}

class DAGOR_OT_node_init_weight(Operator):
    bl_idname = "dt.node_init_weight"
    bl_label = 'Init weight'
    bl_description = 'Add a "weight:r" property to set it to something different than 1.0'
    bl_options = {'UNDO'}

    node: StringProperty(default = '')

    def execute(self, context):
        node = bpy.data.objects.get(self.node)
        if node is None:
            return {'CANCELLED'}
        node.dagorprops['weight:r'] = 1.0
        return {'FINISHED'}

class DAGOR_OT_bbox_to_gameobj(Operator):
    bl_idname = "dt.bbox_to_gameobj"
    bl_label = "Bbox to gameobj"
    bl_description = "Replace mesh(es) by selected node, scaled to match bbox"
    bl_options = {'UNDO'}

    @classmethod
    def poll(self, context):
        P = bpy.data.scenes[0].dag4blend.cmp
        return P.node is not None

    def execute(self, context):
        P = bpy.data.scenes[0].dag4blend.cmp
        err = None
        nodes = [obj for obj in context.selected_objects if obj.type=='MESH']
        if P.node is None:
            err = 'no gameobj selected!'
        elif nodes.__len__()==0:
            err = 'No mesh objects selected!'
        if err is not None:
            log(f'{err}\n', type = 'ERROR', show = True)
            show_popup(err)
            return{'CANCELLED'}
        for obj in nodes:
            bbox_to_gameobj(obj, P.node)
        return {'FINISHED'}

class DAGOR_OT_InitBlendFile(Operator):
    bl_idname       = 'dt.init_blend'
    bl_label        = 'Init'
    bl_description  = "Adds exstra scenes for different data types and collection for data transfer between them"
    bl_options      = {'UNDO'}

    def execute(self,context):
        upd_scenes()
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
#PANELS
class DAGOR_PT_composits(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_context = "objectmode"
    bl_label = "Composits"
    bl_category = 'Dagor'
    bl_options = {'DEFAULT_CLOSED'}
    @classmethod
    def poll(self, context):
        addon_name = basename(__package__)
        pref = bpy.context.preferences.addons[addon_name].preferences
        return pref.use_cmp_editor

#Entity editor
    def draw_entity_editor(self,context,layout):
        node = context.object
        ent_editor = layout.box()
        name = ent_editor.row()
        addon_name = basename(__package__)
        pref = bpy.context.preferences.addons[addon_name].preferences
        
        name.prop(pref, 'cmp_entities_maximized', icon = 'DOWNARROW_HLT' if pref.cmp_entities_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='Entities',expand=True)
        name.label(icon = 'MONKEY')
        if pref.cmp_entities_maximized:
            if node is None:
                ent_editor.label(text = 'Select node', icon = 'INFO')
                return
            elif node.type!='EMPTY':
                ent_editor.label(text = 'Only Empty supported', icon = 'INFO')
                return
            if node.instance_type!='COLLECTION':
                ent_editor.label(text = 'Turn instansing on:')
                row = ent_editor.row()
                row.prop(node, 'instance_type',expand=True)
                return
            col = node.instance_collection
            if col is not None and col.name.startswith('random.'):
                if col.users>2:
                    warning = ent_editor.box()
                    warning.label(text = 'WARNING!')
                    warning.label(text = 'Node data has multiple users,')
                    warning.label(text = 'changes will affect other copies')
                    warning.operator('dt.node_make_unic', text = 'Make unic').node = context.object.name
                for obj in col.objects:
                    node = ent_editor.box()
                    row = node.row()
                    row.prop(obj,'hide_viewport',text='',emboss = False)
                    row.prop(obj,'instance_collection', text = '')
                    remove = row.operator('dt.node_remove_entity', icon = 'TRASH', text = '')
                    remove.node = context.object.name
                    remove.entity = obj.name
                    if obj.dagorprops.get('weight:r') is None:
                        node.operator('dt.node_init_weight').node = obj.name
                    else:
                        node.prop(obj.dagorprops,'["weight:r"]')
            else:
                node = ent_editor.box()
                row = node.row()
                obj = context.object
                if obj.instance_type == 'COLLECTION':
                    row.prop(obj,'instance_collection', text = '')
            ent_editor.operator('dt.node_add_entity', text = '', icon = 'ADD').node = context.object.name
        return
#Node explode n stuff
    def draw_node_converter(self,context,layout):
        P = bpy.data.scenes[0].dag4blend.cmp
        addon_name = basename(__package__)
        pref = bpy.context.preferences.addons[addon_name].preferences
        box = layout.box()
        header = box.row()
        header.prop(pref, 'cmp_converter_maximized',icon = 'DOWNARROW_HLT'if pref.cmp_converter_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='Convert',expand=True)
        header.label(text='', icon='ARROW_LEFTRIGHT')
        node = context.object
        if pref.cmp_converter_maximized:
            if node is None:
                box.label(text = 'Select object', icon = 'INFO')
                return
            elif node.type not in ['EMPTY','MESH']:
                box.label(text = f'{node.type} objects not supported', icon = 'INFO')
                return
            if node.type=='EMPTY':
                box.operator('dt.materialize_nodes', icon='MESH_MONKEY', text = 'To mesh')
            if node.instance_type == 'COLLECTION':
                box.operator('dt.node_explode', icon = 'FULLSCREEN_ENTER').node = context.object.name
            elif node.instance_collection is not None:
                box.operator('dt.node_revert', icon = 'FILE_REFRESH').node = context.object.name
                box.operator('dt.node_rebuild', icon = 'FULLSCREEN_EXIT').node = context.object.name

            elif node.type == 'MESH':
                box.label(text = 'node:')
                box.prop(P, 'node', text = '')
                box.operator('dt.bbox_to_gameobj', text = 'BBOX to node', icon = 'CUBE')
                if P.node is None:
                    box.label(text = 'Choose node collection!', icon = 'ERROR')
        return

    def draw_init(self,context,layout):
        addon_name = basename(__package__)
        pref = bpy.context.preferences.addons[addon_name].preferences
        box = layout.box()
        header = box.row()
        first_time = True
        for sc in ["COMPOSITS", "GAMEOBJ", "GEOMETRY", "TECH_STUFF"]:
            if bpy.data.scenes.get(sc) is not None:
                first_time = False
        header.prop(pref, 'cmp_init_maximized',icon = 'DOWNARROW_HLT'if pref.cmp_init_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='Scenes',expand=True)
        header.label(text='', icon='ERROR' if first_time else 'FILE_REFRESH')
        if pref.cmp_init_maximized:
            window = context.window
            box.operator('dt.init_blend', text = 'Init scenes' if first_time else 'Refresh scenes')
            box.template_ID(window, "scene")
        return

    def draw_node_props(self,context,layout):
        node = context.object
        if node is None or node.type!="EMPTY":
            return

        addon_name = basename(__package__)
        pref = bpy.context.preferences.addons[addon_name].preferences
        box = layout.box()
        header = box.row()
        header.prop(pref, 'cmp_node_prop_maximized',icon = 'DOWNARROW_HLT'if pref.cmp_node_prop_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='Node Properties',expand=True)
        header.label(text = '', icon = 'OPTIONS')
        if pref.cmp_node_prop_maximized:
            tm = box.column(align = True)
            tm.label(text = "Transforms:")
            loc = tm.row(align = True)
            loc.prop(node, 'location', text = '')
            rot = tm.row(align = True)
            rot.prop(node, 'rotation_euler', text = '')
            scale = tm.row(align = True)
            scale.prop(node, 'scale', text = '')
        return

    def draw(self,context):
        addon_name = basename(__package__)
        pref = bpy.context.preferences.addons[addon_name].preferences
        P = bpy.data.scenes[0].dag4blend.cmp

        l = self.layout
        importer = l.box()
        header = importer.row()
        header.prop(pref, 'cmp_imp_maximized',icon = 'DOWNARROW_HLT'if pref.cmp_imp_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='CMP Import',expand=True)
        header.label(text='', icon='IMPORT')
        if pref.cmp_imp_maximized:
            importer.label(text = 'import path:')
            importer.prop(P.importer,'filepath',text='')
            importer.prop(P.importer,'refresh_cache')
            importer.prop(P.importer,'with_sub_cmp')
            importer.prop(P.importer,'with_dags')
            if P.importer.with_dags:
                importer.prop(P.importer,'with_lods')
            importer.operator('dt.cmp_import')

        exporter = l.box()
        header = exporter.row()
        header.prop(pref, 'cmp_exp_maximized',icon = 'DOWNARROW_HLT'if pref.cmp_exp_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='CMP Export',expand=True)
        header.label(text='', icon='EXPORT')
        if pref.cmp_exp_maximized:
            exporter.label(text = 'export path:')
            exporter.prop(P.exporter, 'dirpath',text = '')
            exporter.prop(P.exporter, 'collection')
            exporter.operator('dt.cmp_export')

        tools = l.box()
        header = tools.row()
        header.prop(pref, 'cmp_tools_maximized',icon = 'DOWNARROW_HLT'if pref.cmp_tools_maximized else 'RIGHTARROW_THIN',
            emboss=False,text='CMP Tools',expand=True)
        header.label(text='', icon='TOOL_SETTINGS')
        if pref.cmp_tools_maximized:
            self.draw_init(context,tools)
            self.draw_node_converter(context,tools)
            self.draw_node_props(context,tools)
            self.draw_entity_editor(context,tools)
        return

classes = [
            DAGOR_OT_InitBlendFile,
            DAGOR_OT_Materialize,
            DAGOR_OT_node_explode,
            DAGOR_OT_node_revert,
            DAGOR_OT_node_rebuild,
            DAGOR_OT_bbox_to_gameobj,
            DAGOR_OT_node_init_weight,
            DAGOR_OT_node_remove_entity,
            DAGOR_OT_node_add_entity,
            DAGOR_OT_node_make_unic,
            DAGOR_PT_composits,
            ]

def register():
    for cl in classes:
        register_class(cl)

    return

def unregister():
    for cl in classes:
        unregister_class(cl)
    return
