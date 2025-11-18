import bpy


from bpy.types  import Panel, Operator
from bpy.utils  import register_class, unregister_class, user_resource
from bpy.props  import IntProperty, BoolProperty, StringProperty

from ..helpers.names    import ensure_no_extension
from ..helpers.texts    import log
from ..helpers.getters  import *
from ..helpers.version  import get_blender_version

from ..popup.popup_functions    import show_popup
from ..ui.draw_elements         import draw_custom_header, draw_custom_toggle

from .cmp_import        import DAGOR_OP_CmpImport
from .cmp_export        import DAGOR_OP_CmpExport
from .node_properties   import draw_matrix, draw_place_type
from .composite_functions   import *

classes = []

#OPERATORS
class DAGOR_OT_nodes_split(Operator):
    bl_idname = 'dt.nodes_split'
    bl_label = 'Split Node'
    bl_description = 'Turns node entities or composite content into actual children nodes'
    bl_options = {'UNDO'}

    recursive:   BoolProperty(default = False)
    destructive: BoolProperty(default = False)

    @classmethod
    def poll(cls, context):
        nodes_to_split = get_valid_nodes_to_split()
        if nodes_to_split.__len__() == 0:
            return False
        return True

    @classmethod
    def description(cls, context, properties):
        nodes_to_split = get_valid_nodes_to_split()
        if nodes_to_split.__len__() == 0:
            return "No valid nodes to split"
        return "Split selected composite(s) into separate parts"

    def execute(self, context):
        nodes_to_split = get_valid_nodes_to_split()
        if not self.recursive:
            nodes_split(nodes_to_split, recursive = False, destructive = self.destructive)
            return {'FINISHED'}
        old_children = []
        for node in nodes_to_split:
            for child in node.children:
                if child in nodes_to_split:
                    continue
                old_children.append(child)
        nodes_split(nodes_to_split, destructive = self.destructive)
        bpy.ops.object.select_all(action = 'DESELECT')
        if self.destructive:
            return {'FINISHED'}
        for node in nodes_to_split:
            for child in node.children:
                if child in old_children or child in nodes_to_split:
                    continue
                child.select_set(True)
                context.view_layer.objects.active = child
        return {'FINISHED'}
classes.append(DAGOR_OT_nodes_split)


class DAGOR_OT_node_revert(Operator):
    bl_idname = 'dt.node_revert'
    bl_label = 'Revert'
    bl_description = 'Turn exploded node back to original state'
    bl_options = {'UNDO'}

    def execute(self, context):
        node = context.object
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
classes.append(DAGOR_OT_node_revert)


class DAGOR_OT_node_rebuild(Operator):
    bl_idname = 'dt.node_rebuild'
    bl_label = 'Rebuild'
    bl_description = 'Update instance collection based on exploded node'
    bl_options = {'UNDO'}

    def execute(self, context):
        node = context.object
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
classes.append(DAGOR_OT_node_rebuild)


class DAGOR_OT_nodes_to_asset(Operator):
    bl_idname = 'dt.nodes_to_asset'
    bl_label = "To asset"
    bl_options = {'UNDO'}

    to_rendinst:    BoolProperty()

    @classmethod
    def poll(cls, context):
        props = get_local_props()
        cmp_tools_props = props.cmp.tools
        naming_mode = cmp_tools_props.naming_mode
        if naming_mode == 'NAME':
            asset_name = cmp_tools_props.asset_name
            if asset_name == "":
                return False
        parent_node = cmp_tools_props.parent_node
        parent_collection = cmp_tools_props.parent_collection
        if parent_collection is None and parent_node:
            parent_collection = parent_node.instance_collection
        nodes = [ node for node in context.selected_objects if ( node != parent_node
            and node.type == 'EMPTY'
            and node.instance_type == 'COLLECTION'
            and node.instance_collection is not None
            and node.instance_collection != parent_collection)]
        if nodes.__len__() == 0:
            return False
        return True

    @classmethod
    def description(cls, context, properties):
        type = "rendinst" if properties.to_rendinst else "composit"
        props = get_local_props()
        cmp_tools_props = props.cmp.tools
        naming_mode = cmp_tools_props.naming_mode
        asset_name = cmp_tools_props.asset_name
        parent_node = cmp_tools_props.parent_node
        pivot = "world origin" if parent_node is None else f'"{parent_node.name}" node'
        if naming_mode == 'COLLECTION':
            parent_collection = cmp_tools_props.parent_collection
            if parent_collection is None and parent_node is not None:
                parent_collection = parent_node.instance_collection
            if parent_collection is not None:
                asset = f'"{parent_collection.name}:{type}"'
            elif parent_node is not None and parent_node.instance_collection is not None:
                asset = f'"{parent_node.instance_collection.name}:{type}"'
            else:
                asset = f'new unnamed "{type}" asset'
        else:
            asset = f'"{asset_name}:{type}"' if asset_name != "" else f'new unnamed "{type}" asset'
        return f'Convert selected nodes into {asset} with pivot in the {pivot}'


    def execute(self, context):
        props = get_local_props()
        cmp_tools_props = props.cmp.tools
        nodes = [ o for o in context.selected_objects if o.type == 'EMPTY']

        parent_node = cmp_tools_props.parent_node
        if parent_node is None:
            parent_node = bpy.data.objects.new("", None)
            context.scene.collection.objects.link(parent_node)
        elif parent_node in nodes:
            nodes.remove(parent_node)

        if cmp_tools_props.naming_mode == 'COLLECTION':
            collection = cmp_tools_props.parent_collection
            if collection is None:
                collection = parent_node.instance_collection
            if collection is None:
                collection = bpy.data.collections.new("new_asset")
                context.scene.collection.children.link(collection)
        else:
            asset_name = cmp_tools_props.asset_name
            if self.to_rendinst and (not asset_name.endswith('.lods')):
                asset_name += '.lods'
            collection = bpy.data.collections.new(asset_name)
            context.scene.collection.children.link(collection)
        if self.to_rendinst:
            nodes_to_rendinst(nodes, parent_node, collection)
        else:
            nodes_to_composite(nodes, parent_node, collection)
        return {'FINISHED'}
classes.append(DAGOR_OT_nodes_to_asset)


class DAGOR_OT_composite_to_rendinst(Operator):
    bl_idname = 'dt.composite_to_rendinst'
    bl_label = "To rendinst"

    def execute(self, context):
        props = get_local_props()
        collection_to_convert = props.cmp.tools.collection_to_convert
        bpy.ops.object.select_all(action = 'DESELECT')
        nodes = collection_to_convert.objects
        for node in nodes:
            node.select_set(True)
        while context.selected_objects.__len__()>0:
            for node in context.selected_objects:
                nodes_split(node, split_to_lods = True, split_to_meshes = True)
        existing_lods = {}
        for obj in list(collection_to_convert.objects):
            if not obj.name.startswith("lod"):
                collection_to_convert.objects.unlink(obj)
                continue
            found_lod = obj.name[:5]
            if not found_lod in existing_lods:
                lod_collection = bpy.data.collections.new(ensure_no_extension(collection_to_convert.name) + "." + found_lod)
                existing_lods[found_lod] = lod_collection
                collection_to_convert.children.link(existing_lods[found_lod])
            collection_to_convert.objects.unlink(obj)
            existing_lods[found_lod].objects.link(obj)
            for child in list(obj.children_recursive):
                existing_lods[found_lod].objects.link(child)
        return {'FINISHED'}
classes.append(DAGOR_OT_composite_to_rendinst)


class DAGOR_OT_parent_node_link_to_scene(Operator):
    bl_idname = 'dt.parent_node_link_to_scene'
    bl_label = "Link to this scene"
    bl_options = {'UNDO'}

    @classmethod
    def poll(cls, context):
        props = get_local_props()
        parent_node = props.cmp.tools.parent_node
        if parent_node is None:
            return False
        if context.scene.objects.get(parent_node.name):
            return False
        return True

    @classmethod
    def description(cls, context, properties):
        scene_name = context.scene.name
        props = get_local_props()
        parent_node = props.cmp.tools.parent_node
        if parent_node is None:
            return "No parent node to process"
        if context.scene.objects.get(parent_node.name):
            return f"\"{parent_node.name}\" object is already in \"{scene_name}\" scene!"
        else:
            return f"link \"{parent_node.name}\" object to \"{scene_name}\" scene"
        return ""

    def execute(self, context):
        props = get_local_props()
        parent_node = props.cmp.tools.parent_node
        context.scene.collection.objects.link(parent_node)
        return {'FINISHED'}
classes.append(DAGOR_OT_parent_node_link_to_scene)


class DAGOR_OT_node_make_unic(Operator):
    bl_idname = 'dt.node_make_unic'
    bl_label = 'Make unic'
    bl_description = 'Replaces instance collection by exact copy'
    bl_options = {'UNDO'}

    def execute(self,context):
        node = context.object
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
classes.append(DAGOR_OT_node_make_unic)


class DAGOR_OT_node_remove_entity(Operator):
    bl_idname = "dt.node_remove_entity"
    bl_label = 'Remove entity'
    bl_description = 'Removes an entity from random node'
    bl_options = {'UNDO'}

    index: IntProperty(default = -1)

    @classmethod
    def poll(self, context):
        if context.object is None:
            return False
        if context.object.type != 'EMPTY':
            return False
        if context.object.instance_type != 'COLLECTION':
            return False
        if context.object.instance_collection is None:
            return False
        return True

    def execute(self, context):
        node = context.object
        index = self.index
        if index < 0 or index >= node.instance_collection.objects.__len__():
            log('Wrong entity index!', type = 'ERROR', show = True)
            return {'CANCELLED'}
        node_remove_entity(node, index)
        return {'FINISHED'}
classes.append(DAGOR_OT_node_remove_entity)


class DAGOR_OT_node_add_entity(Operator):
    bl_idname = "dt.node_add_entity"
    bl_label = 'Add entity'
    bl_description = 'Adds an entity to random node,turns static node into random one'
    bl_options = {'UNDO'}

    def execute(self, context):
        node = context.object
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
classes.append(DAGOR_OT_node_add_entity)


class DAGOR_OT_node_init_weight(Operator):
    bl_idname = "dt.node_init_weight"
    bl_label = 'Init weight'
    bl_description = 'Add a "weight:r" property to set it to something different than 1.0'
    bl_options = {'UNDO'}

    index: IntProperty(default = -1)

    def execute(self, context):
        node = context.object
        entity = node.instance_collection.objects[self.index]
        entity.dagorprops['weight:r'] = 1.0
        return {'FINISHED'}
classes.append(DAGOR_OT_node_init_weight)


class DAGOR_OT_node_clear_weight(Operator):
    bl_idname = "dt.node_clear_weight"
    bl_label = 'Clear weight'
    bl_description = 'Remove a "weight:r" property'
    bl_options = {'UNDO'}

    index: IntProperty(default = -1)

    def execute(self, context):
        node = context.object
        entity = node.instance_collection.objects[self.index]
        del entity.dagorprops['weight:r']
        return {'FINISHED'}
classes.append(DAGOR_OT_node_clear_weight)


class DAGOR_OT_bbox_to_gameobj(Operator):
    bl_idname = "dt.bbox_to_gameobj"
    bl_label = "Bbox to gameobj"
    bl_description = "Replace mesh(es) by selected node, scaled to match bbox"
    bl_options = {'UNDO'}

    @classmethod
    def poll(self, context):
        props = get_local_props()
        cmp_tools_props = props.cmp.tools
        return cmp_tools_props.node is not None

    def execute(self, context):
        props = get_local_props()
        cmp_tools_props = props.cmp.tools
        err = None
        nodes = [obj for obj in context.selected_objects if obj.type=='MESH']
        if cmp_tools_props.node is None:
            err = 'no gameobj selected!'
        elif nodes.__len__()==0:
            err = 'No mesh objects selected!'
        if err is not None:
            log(f'{err}\n', type = 'ERROR', show = True)
            show_popup(err)
            return{'CANCELLED'}
        for obj in nodes:
            bbox_to_gameobj(obj, cmp_tools_props.node)
        return {'FINISHED'}
classes.append(DAGOR_OT_bbox_to_gameobj)


class DAGOR_OT_InitBlendFile(Operator):
    bl_idname       = 'dt.init_blend'
    bl_label        = 'Init'
    bl_description  = "Adds exstra scenes for different data types and collection for data transfer between them"
    bl_options      = {'UNDO'}

    def execute(self,context):
        upd_scenes()
        return {'FINISHED'}
classes.append(DAGOR_OT_InitBlendFile)


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
            node_to_geometry(obj)
        return {'FINISHED'}
classes.append(DAGOR_OT_Materialize)


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
        pref = get_preferences()
        return pref.use_cmp_editor

#Entity editor
    def draw_entity_editor(self,context,layout):
        node = context.object
        ent_editor = layout.box()
        pref = get_preferences()
        header = ent_editor.row()
        draw_custom_header(header, "Entities", pref, 'cmp_entities_maximized', icon = 'MONKEY')
        if not pref.cmp_entities_maximized:
            return
        if node.instance_type!='COLLECTION':
            ent_editor.label(text = 'Turn instansing on:')
            row = ent_editor.row()
            row.prop(node, 'instance_type',expand = True)
            return
        col = node.instance_collection
        if col is not None and col.name.startswith('random.'):
            if col.users>2:
                warning = ent_editor.box()
                warning.label(text = 'WARNING!')
                warning.label(text = 'Node data has multiple users,')
                warning.label(text = 'changes will affect other copies')
                warning.operator('dt.node_make_unic', text = 'Make unic')
            index = 0
            for obj in col.objects:
                node = ent_editor.box()
                row = node.row(align = True)
                row.prop(obj,'hide_viewport',text='',emboss = False)
                row.prop(obj,'instance_collection', text = '')
                row.operator('dt.node_remove_entity', icon = 'TRASH', text = '').index = index
                if obj.dagorprops.get('weight:r') is None:
                    node.operator('dt.node_init_weight').index = index
                else:
                    weight = node.row(align = True)
                    weight.prop(obj.dagorprops,'["weight:r"]')
                    weight.operator('dt.node_clear_weight', text = "", icon = 'TRASH').index = index
                index += 1
        else:
            node = ent_editor.box()
            row = node.row()
            obj = context.object
            if obj.instance_type == 'COLLECTION':
                row.prop(obj,'instance_collection', text = '')
        ent_editor.operator('dt.node_add_entity', text = '', icon = 'ADD')
        return

    def draw_nodes_to_asset_converter(self, context, layout):
        pref = get_preferences()
        props = get_local_props()
        cmp_tools_props = props.cmp.tools
        box = layout.box()
        draw_custom_header(box, "Nodes to Asset", pref, 'nodes_to_asset_maximized', icon = 'STICKY_UVS_LOC')
        if not pref.nodes_to_asset_maximized:
            return

        parameters = box.column(align = True)
        row = parameters.row()
        row.label(text = "Parent node:")
        row = parameters.row(align = True)
        row.prop(cmp_tools_props, 'parent_node', text = "")
        row.operator('dt.parent_node_link_to_scene', text = "", icon = 'LINKED')
        parameters.separator()

        row = parameters.row()
        row.label(text = "Naming mode:")
        row = parameters.row()
        row.prop(cmp_tools_props, 'naming_mode', expand = True)
        parameters.separator()

        row = parameters.row()
        if cmp_tools_props.naming_mode == 'COLLECTION':
            row.label(text = "Asset collection:")
            row = parameters.row()
            row.prop(cmp_tools_props, 'parent_collection', text = "")
        else:
            row.label(text = "Asset name:")
            row = parameters.row()
            row.prop(cmp_tools_props, 'asset_name', text = "")
        parameters.separator()

        buttons = box.row(align = True)
        buttons.scale_y = 2
        button_a = buttons.row()
        op = button_a.operator('dt.nodes_to_asset', text = 'To sub-composite')
        op.to_rendinst = False
        button_b = buttons.row()
        op = button_b.operator('dt.nodes_to_asset', text = 'To rendinst node')
        op.to_rendinst = True
        return

#Node explode n stuff
    def draw_hyerarchy_editor(self,context,layout):
        pref = get_preferences()
        props = get_local_props()
        cmp_tools_props = props.cmp.tools
        self.draw_nodes_to_asset_converter(context, layout)
        box = layout.box()
        draw_custom_header(box, "Edit Sub-Composites", pref, 'cmp_hyerarchy_maximized', icon = 'ARROW_LEFTRIGHT')
        node = context.object
        if not pref.cmp_hyerarchy_maximized:
            return

        props = box.column(align = True)
        draw_custom_toggle(props, cmp_tools_props, 'recursive')
        draw_custom_toggle(props, cmp_tools_props, 'destructive')
        row = props.row()
        button = row.operator('dt.nodes_split', icon = 'FULLSCREEN_ENTER')
        button.recursive = cmp_tools_props.recursive
        button.destructive = cmp_tools_props.destructive
        row.scale_y = 2

        row = box.row()
        row.scale_y = 2
        row.operator('dt.node_revert', icon = 'FILE_REFRESH')
        row.operator('dt.node_rebuild', icon = 'FULLSCREEN_EXIT')
        row.enabled = row.active = (node is not None and
            node.type == 'EMPTY' and
            node.instance_type != 'COLLECTION' and
            node.instance_collection is not None)
        return

    def draw_node_converter(self, context, layout):
        pref = get_preferences()
        props = get_local_props()
        cmp_tools_props = props.cmp.tools
        node = context.object
        box = layout.box()
        draw_custom_header(box, "Basic Converters", pref, 'basic_converter_maximized', icon = 'GEOMETRY_SET')
        if not pref.basic_converter_maximized:
            return

        row = box.row()
        row.operator('dt.materialize_nodes', icon='MESH_MONKEY', text = 'Nodes to mesh')
        row.enabled = row.active = node is not None and node.type == 'EMPTY'
        box = box.box()
        col = box.column()
        col.label(text = 'gameobj:')
        col.prop(cmp_tools_props, 'node', text = '')
        col.operator('dt.bbox_to_gameobj', text = 'Mesh BBOX to node', icon = 'CUBE')
        if cmp_tools_props.node is None:
            col.label(text = 'Choose gameobj collection!', icon = 'ERROR')
        col.enabled = col.active = node is not None and node.type == 'MESH'
        return

    def draw_init(self,context,layout):
        pref = get_preferences()
        box = layout.box()
        header = box.row()
        first_time = True
        for sc in ["COMPOSITS", "GAMEOBJ", "GEOMETRY", "TECH_STUFF"]:
            if bpy.data.scenes.get(sc) is not None:
                first_time = False
        draw_custom_header(header, "Scenes", pref, 'cmp_init_maximized',
            icon='ERROR' if first_time else 'FILE_REFRESH')

        if not pref.cmp_init_maximized:
            return
        window = context.window
        box.operator('dt.init_blend', text = 'Init scenes' if first_time else 'Refresh scenes')
        box.template_ID(window, "scene")
        return

    def draw_node_place_type(self, context,layout):
        pref = get_preferences()
        box = layout.box()
        draw_custom_header(box, "Place Type", pref, 'cmp_place_maximized', icon = 'ORIENTATION_LOCAL')
        if not pref.cmp_place_maximized:
            return
        node = context.object
        draw_place_type(box,node)
        return

    def draw_node_transforms(self,context,layout):
        pref = get_preferences()
        box = layout.box()
        draw_custom_header(box, "Transformations", pref, 'cmp_tm_maximized', icon = 'EMPTY_ARROWS')
        if not pref.cmp_tm_maximized:
            return
        node = context.object
        draw_matrix(box, node)
        return

    def draw_cmp_importer(self, context, layout):
        pref = get_preferences()
        props = get_local_props()
        cmp_import_props = props.cmp.importer
        importer = layout.box()
        draw_custom_header(importer, "Import", pref, 'cmp_imp_maximized', icon = 'IMPORT')
        if pref.cmp_imp_maximized:
            importer.label(text = 'import path:')
            importer.prop(cmp_import_props,'filepath',text='')
            toggles = importer.column(align = True)
            row = toggles.row()
            row.prop(cmp_import_props,'refresh_cache', toggle = True,
                icon = 'CHECKBOX_HLT' if cmp_import_props.refresh_cache else 'CHECKBOX_DEHLT')
            row = toggles.row()
            row.prop(cmp_import_props,'with_sub_cmp', toggle = True,
                icon = 'CHECKBOX_HLT' if cmp_import_props.with_sub_cmp else 'CHECKBOX_DEHLT')
            row = toggles.row()
            row.prop(cmp_import_props,'with_dags', toggle = True,
                icon = 'CHECKBOX_HLT' if cmp_import_props.with_dags else 'CHECKBOX_DEHLT')
            if cmp_import_props.with_dags:
                row = toggles.row()
                row.prop(cmp_import_props,'with_lods', toggle = True,
                icon = 'CHECKBOX_HLT' if cmp_import_props.with_lods else 'CHECKBOX_DEHLT')
            button = importer.row()
            button.scale_y = 2
            button.operator('dt.cmp_import', icon = 'IMPORT')
        return

    def draw_cmp_exporter(self, context, layout):
        pref = get_preferences()
        props = get_local_props()
        cmp_export_props = props.cmp.exporter
        exporter = layout.box()
        draw_custom_header(exporter, "Export", pref, 'cmp_exp_maximized', icon = 'EXPORT')
        if pref.cmp_exp_maximized:
            exporter.label(text = 'export path:')
            exporter.prop(cmp_export_props, 'dirpath',text = '')
            exporter.prop(cmp_export_props, 'collection')
            button = exporter.row()
            button.scale_y = 2
            button.operator('dt.cmp_export', icon = 'EXPORT')
        return

    def draw_cmp_tools(self, context, layout):
        pref = get_preferences()
        tools = layout.box()
        draw_custom_header(tools, "Tools", pref, 'cmp_tools_maximized', icon = 'TOOL_SETTINGS')
        if not pref.cmp_tools_maximized:
            return
        self.draw_init(context, tools)
        self.draw_node_converter(context, tools)
        self.draw_hyerarchy_editor(context, tools)
        return

    def draw_node_props(self, context, layout):
        pref = get_preferences()
        node_props = layout.box()
        draw_custom_header(node_props, "Node Properties", pref, 'cmp_props_maximized', icon = 'OPTIONS')
        if not pref.cmp_props_maximized:
            return
        column = node_props.column(align = True)
        if context.object is None:
            column.label(text = 'No active object', icon = 'INFO')
            return
        elif context.object.type != 'EMPTY':
            column.label(text = 'Active object is not Empty', icon = 'INFO')
            return
        row = column.row()
        self.draw_entity_editor(context, row)
        row = column.row()
        self.draw_node_place_type(context, row)
        return

    def draw(self,context):
        self.draw_cmp_importer(context, self.layout)
        self.draw_cmp_exporter(context, self.layout)
        self.draw_cmp_tools(context, self.layout)
        self.draw_node_props(context, self.layout)
        return
classes.append(DAGOR_PT_composits)


def register():
    for cl in classes:
        register_class(cl)
    return

def unregister():
    for cl in classes[::-1]:
        unregister_class(cl)
    return
