import bpy, os

from bpy                import context as C

from bpy.props          import (StringProperty,
                                BoolProperty,
                                EnumProperty,
                                CollectionProperty,
                                IntProperty,
                                PointerProperty,
                                FloatProperty,
                                )
from bpy.types          import PropertyGroup, Operator, AddonPreferences, Panel, Collection, Object
from bpy.utils          import user_resource
from os                 import listdir
from os.path            import exists, splitext, isfile, join

from .dagormat.read_shader_config   import read_shader_config
from .popup.popup_functions         import show_popup
from .helpers.getters               import get_local_props, get_preferences, get_addon_directory
from .projects.draw_project_manager import draw_project_settings
from .ui.draw_elements              import draw_custom_header, draw_custom_toggle

classes = []

#projects interaction#####################################################
def get_projects(self, context):
    pref = get_preferences()
    items = []
    i=0
    for proj in pref.projects:
        name = proj.name
        path = proj.path
        item = (str(i), name, path)
        items.append(item)
        i+=1
    return items

def update_parent_node(self, context):
    if self.parent_node is None:
        return
    if self.parent_node.type != 'EMPTY':
        show_popup(message = f"\"{self.parent_node.type}\" objects are not allowed! Use Empty instead")
        self.parent_node = None
    return

def upd_palettes(self, context):
    palette_node = bpy.data.node_groups.get('.painting')
    if palette_node is None:
        return
    pref = get_preferences()
    index = int(pref.project_active)
    palette_global = pref.projects[index].palette_global
    palette_local  = pref.projects[index].palette_local
#global palette
    palette_0 = bpy.data.images.get('palette_0')
    if palette_0 is None:
        pass
    else:
        palette_0.filepath = palette_global
        if exists(palette_global):
            palette_0.source = 'FILE'
            palette_node.nodes['UV_global'].inputs['height, pix'].default_value = palette_0.size[1]
            palette_node.nodes['UV_global'].inputs['width, pix'].default_value =  palette_0.size[0]
        else:
            palette_0.source = 'GENERATED'
#local_palette
    palette_1 = bpy.data.images.get('palette_1')
    if palette_1 is None:
        pass
    else:
        palette_1.filepath = palette_local
        if exists(palette_local):
            palette_1.source = 'FILE'
            palette_node.nodes['UV_local'].inputs['height, pix'].default_value = palette_1.size[1]
            palette_node.nodes['UV_local'].inputs['width, pix'].default_value =  palette_1.size[0]
        else:
            palette_1.source = 'GENERATED'
    return


#Updaters
def upd_project(self,context):
    upd_palettes(self,context)
    global_settings = bpy.data.node_groups.get('global_settings')
    if global_settings is not None:
        pref = get_preferences()
        index = int(pref.project_active)
        project = pref.projects[index]
        global_settings.nodes['Group Output'].inputs['WT|Enl'].default_value = int(project.mode == 'DNG')
    return

def upd_cmp_i_path(self,context):
    if self.filepath.find('"') > -1:
        self.filepath = self.filepath.replace('"','')
    if self.filepath.find("\\") > -1:
        self.filepath = self.filepath.replace("\\","/")
    return

def upd_cmp_e_path(self,context):
    if self.dirpath.find('"') > -1:
        self.dirpath = self.dirpath.replace('"','')
    if self.dirpath.find("\\") > -1:
        self.dirpath = self.dirpath.replace("\\","/")
    if not self.dirpath.endswith("/"):
        self.dirpath = self.dirpath + "/"
    return

def upd_imp_filepath(self,context):
    if self.filepath.find("\\") > -1:
        self.filepath = self.filepath.replace("\\","/")
    if self.filepath.find('"') > -1:
        self.filepath = self.filepath.replace('"','')
    return

def upd_imp_dirpath(self,context):
    if self.dirpath.find('"') > -1:
        self.dirpath = self.dirpath.replace('"','')
    if self.dirpath.find("\\") > -1:
        self.dirpath = self.dirpath.replace("\\","/")
    if self.dirpath.endswith('.dag'):
        props = get_local_props()
        mode = props.importer.mode
        if mode == 'WILDCARD':
            includes = props.importer.includes
            for key in includes.keys():
                del includes[key]
            includes["0"] = os.path.basename(self.dirpath)  # dag name turned to mask for batch import
        else:  # mode == 'REGEX'
            includes = props.importer.includes_re
            for key in includes.keys():
                del includes[key]
            name_corrected = os.path.basename(self.dirpath)
            name_corrected = name_corrected.replace('.', '\.')
            includes["0"] =  f'^{name_corrected}$'  # dag name turned to mask for batch import
        self.dirpath = os.path.dirname(self.dirpath)  # dag name removed from path
    if not self.dirpath.endswith("/"):
        self.dirpath = self.dirpath+"/"
    return

def upd_exp_path(self,context):
    if self.dirpath.find('"') > -1:
        self.dirpath = self.dirpath.replace('"','')
    if not self.dirpath.endswith('/'):
        self.dirpath += '/'
    return

#Getters

def get_modes(self, context):
    modes = (
                ('UV_TO_UV', 'Self Bake', "Bake from one UV channel to another"),
                ('HP_TO_LP', 'HP to LP', "Bake from different geometry"),
            )
    return modes

def get_resolutions(self, context):
    start = 128
    items = []
    for i in range(6):
        step = str(start*pow(2,i))
        items.append((step,step,step))
    return items

class DAG_OT_RemoveProject(Operator):
    bl_idname = "dt.remove_project"
    bl_label = "Remove"
    bl_description = "Remove current project from projects list"
    index: IntProperty(default = -1)
    def execute(self, context):
        index_to_remove = self.index
        pref = get_preferences()
        projects_count = len(pref.projects)
        if index_to_remove < 0 or index_to_remove >= projects_count:
            return{'CANCELLED'}  # out of range
        active_index = int(pref.project_active)
        if active_index > index_to_remove or active_index == projects_count-1 and active_index > 0:
            pref.project_active = str(active_index - 1)
        pref.projects.remove(index_to_remove)
        return {'FINISHED'}
classes.append(DAG_OT_RemoveProject)


class DagProject(PropertyGroup):
    name:               StringProperty()
    path:               StringProperty(subtype = 'DIR_PATH',
        description = "Where assets stored. Used to search for textures, proxymats, nodes for composites")
    #Shading relatet parameters
    palette_global:     StringProperty(subtype = 'FILE_PATH', name = "Global palette",
                            description = "Used everywhere, unless it's overriden by local palette",
                            update = upd_palettes)
    palette_local:      StringProperty(subtype = 'FILE_PATH', name = "Local palette",
                            description = "Optional extra palette for specific level",
                            update = upd_palettes)
    unlock:             BoolProperty(default = True, name = "Lock/Unlock",
        description = "Lock/unlock ability to edit parameters of project")
    mode:               EnumProperty(
        items = [('WT','War Thunder-like','Mimic War Thunder specific shaders'),
                ('DNG','daNetGame-like','Mimic daNetGame specific shaders'),
                 ],
        default = 'WT')
    #UI parameters
    maximized:          BoolProperty(default = True)
    palettes_maximized: BoolProperty(default = True)
classes.append(DagProject)



class DAG_OT_AddProject(Operator):
    '''
    Create a new project
    '''
    bl_idname = "dt.add_project"
    bl_label = "Add"

    def execute(self, context):
        pref = get_preferences()
        new = pref.projects.add()
        if pref.projects.__len__() == 1:
            pref.project_active = '0'
        show_popup(message=f'New project added. Please, configure it')
        return {'FINISHED'}
classes.append(DAG_OT_AddProject)


#shader classes###########################################################

def get_obj_prop_presets(self, context):
    pref = get_preferences()
    path = pref.props_presets_path
    items = []
    files = [f for f in listdir(path) if isfile(join(path,f))]
    for f in files:
        i = (splitext(f)[0])
        items.append((i,i,i))
    return items

#TOOLS
class Dag_Tools_Props(PropertyGroup):
    #SAVE TOOLS
    save_textures_dirpath:  StringProperty(name = "", default = 'C:/tmp/', subtype = 'DIR_PATH')
    save_proxymats_dirpath: StringProperty(name = "", default = 'C:/tmp/', subtype = 'DIR_PATH')
    #DESTR SETUP
    destr_materialName:     StringProperty(default = 'wood', description = "physmat of destr colliders")
    destr_density:          IntProperty   (default = 100, description = "density of colliders")
    #DEFORM SETUP
    configure_shaders:      BoolProperty(default = False, name = "Configure Mats",
        description = "write default values of 'dynamic_deformed' shader(s)")
    preview_deformation:    BoolProperty(default = False, name = "Preview Deformation",
        description = "Apply Geometry Nodes to preview VCol deformation")
    #MESH TOOLS
    vert_threshold:         FloatProperty(default = 1, min = 0, max = 180)
    vert_cleanup_mode:      EnumProperty(
            #(identifier, name, description)
        items = [('SELECT','Select','Select unnecessary vertices'),
                 ('DISSOLVE','Dissolve','Dissolve unnecessary vertices'),
                 ],
        default = 'SELECT')

    tri_legacy_mode:        BoolProperty(default = True, name = "Legacy Mode",
        description = "Use the same search algorithm as dabuild")
    tri_destructive:        BoolProperty(default = True, name = "Destructive",
        description = "Triangulate polygons that produce degenerates on export")
    tri_area_threshold:     FloatProperty(default = 1e-3, min = 1e-4, name = "Area",
        description = "Area less than")
    tri_ratio_threshold:    FloatProperty(default = 1e-3, min = 1e-4, name = "Ratio",
        description = "Area/Perimeter less than")
classes.append(Dag_Tools_Props)


#IMPORTER
class Empty_Group(PropertyGroup):
    pass
classes.append(Empty_Group)

class Dag_Import_Props(PropertyGroup):
    mode            :EnumProperty(
            #(identifier, name, description)
        items = [('SIMPLE','Simple','Import single asset and its related files'),
                 ('WILDCARD','Wildcard','Import multiple assets using fnmatch search'),
                 ('REGEX','Regex','Import multiple assets using regular expressions for search')],
        name = "Importer Mode",
        default = 'WILDCARD')
# Shared parameters
    check_subdirs   :BoolProperty  (name = "Search in subfolders", default = False,
                        description = 'Search for .dags in subfolders as well')
    mopt            :BoolProperty  (name = "Optimize material slots", default = True,
                        description = 'Remove unnecessary material slots')
    preserve_sg     :BoolProperty  (name = "Preserve Smoothing Groups", default = False,
                        description = "Store Smoothing Groups in a integer face attribute called 'SG'")
    replace_existing:BoolProperty  (name = "Reimport existing", default = False,
                        description = 'Replace dags that already exist in scene by imported versions')
    preserve_path   :BoolProperty  (name = "Preserve paths", default = False,
                        description = 'Override export path for each imported dag')
# Simple mode
    with_lods       :BoolProperty(name = "Import LODs", default = True,
                        description = "Search for other levels of detail")
    with_dps        :BoolProperty(name = "Import DPs", default = True,
                        description = "Search for related Damage Parts")
    with_dmgs       :BoolProperty(name = "Import DMGs", default = True,
                        description = "Search for Damaged versions")
    with_destr      :BoolProperty(name = "Import destr", default = True,
                        description = "Search for dynamic destr asset")
    filepath        :StringProperty(name = "File Path", default = "", subtype = 'FILE_PATH',
                        description = "Path to file that should be imported", update = upd_imp_filepath)
# Advanced mode
    dirpath         :StringProperty(name = "Dirpath", default = "C:/tmp/", subtype = 'DIR_PATH',
                        description = "Where search for .dag files?", update = upd_imp_dirpath)
    includes        :PointerProperty(type = Empty_Group)  # Used only as parent for dynamic properties
    excludes        :PointerProperty(type = Empty_Group)  # Used only as parent for dynamic properties
    includes_re     :PointerProperty(type = Empty_Group)  # Used only as parent for dynamic properties
    excludes_re     :PointerProperty(type = Empty_Group)  # Used only as parent for dynamic properties
#UI
    show_help       :BoolProperty(default = False, name = "Help",
        description = "Show short description for current importer mode")
    help_maximized          :BoolProperty(default = True)
    includes_maximized      :BoolProperty(default = True)
    includes_re_maximized   :BoolProperty(default = True)
    excludes_maximized      :BoolProperty(default = True)
    excludes_re_maximized   :BoolProperty(default = True)
classes.append(Dag_Import_Props)


#EXPORTER
class Dag_Export_Props(PropertyGroup):
    filename        :StringProperty(name = "Name", default = 'Filename', description = 'Name for exported .dag file')
    dirpath         :StringProperty(name = "Path", default = 'C:/tmp/', subtype = 'DIR_PATH',
                        description = "Where your .dag files should be saved?",update = upd_exp_path)
    modifiers       :BoolProperty(name = "Apply Modifiers", default = True,
                        description = 'Export meshes with effects of modifiers')
    mopt            :BoolProperty(name = "Optimize Materials", default = True,
                        description = 'Remove unused slots; merge similar materials')
    vnorm           :BoolProperty(name = "vNormals", default = True, description = 'Export of custom vertex normals')
    orphans         :BoolProperty(name = "Export Orphans", default = False,
                        description = 'Export of objects not in the bottom of hierarchy as separate .dags')
    collection      :PointerProperty(type = Collection, name = '',
        description = 'Drag&Drop collection here or select it from list. Keep it empty to use Scene Collection instead')
    cleanup_names   :BoolProperty(name = "Cleanup Names", default = False,
                        description = "Remove indices and node types from names. Use only for cmp palettes!")
    limit_by        :EnumProperty(
            #(identifier, name, description)
        items = [('Visible','Visible','Export everithing as one dag'),
                 ('Sel.Joined','Sel.Joined','Selected objects as one dag'),
                 ('Sel.Separated','Sel.Separated','Selected objects as separate dags'),
                 ('Col.Separated','Col.Separated','Chosen collection and subcollections as separate .dags'),
                 ('Col.Joined','Col.Joined','Objects from chosen collection and subcollections as one .dag')],
        name = "Limit by",
        default = 'Col.Separated')
classes.append(Dag_Export_Props)


#TEXTURE BAKER
class Dag_Baking_Props(PropertyGroup):
#outputs
    bake_width:         EnumProperty(items = get_resolutions)
    bake_height:        EnumProperty(items = get_resolutions)
    tex_d:              BoolProperty(default = False, description = 'RGB: Diffuse; A:Height Map')
    tex_n:              BoolProperty(default = False, description = 'RG: Normal Map; B: Metallness; A: Roughness')
    tex_dirpath:        StringProperty(subtype='DIR_PATH', default = 'C:/tmp/',
                            description = "Directory for baked textures")
    proxymat:           BoolProperty(default = False, name = "Create Proxymat File",
                            description = "Save rendinst_simple proxymat with baked textures")
    apply_proxymat:     BoolProperty(default = False, name = "Apply Proxymat",
                            description = "Apply saved proxymat to the model")
    proxymat_dirpath:   StringProperty(subtype='DIR_PATH', default = 'C:/tmp/',
                            description = "Directory for proxymats")
#mode related
    mode:               EnumProperty(items =get_modes)
    lp_collection:      PointerProperty(type = Collection)
    hp_collection:      PointerProperty(type = Collection)
    recursive:          BoolProperty(default = False, name = "Recursive",
                            description = "Include objects from children collections")
    reveal_result:      BoolProperty(default = True, name = "Reveal Result",
                            description = "Open directory with textures when baking is finished")
#ui stuff
    settings_maximized: BoolProperty(default = False)
    render_maximized:   BoolProperty(default = True)
    output_maximized:   BoolProperty(default = True)
    input_maximized:    BoolProperty(default = True)
    operators_maximized:BoolProperty(default = True)
    node_maximized:     BoolProperty(default = False)
classes.append(Dag_Baking_Props)


#COMPOSITS
class Dag_CMP_Import_Props(PropertyGroup):
    filepath:       StringProperty  (name="cmp import path", default = 'C:/tmp/', subtype = 'FILE_PATH',
                        description = "Path to composit", update=upd_cmp_i_path)
    with_sub_cmp:   BoolProperty    (name="recursive", default = False, description = 'Search for sub-composits as well')
    with_dags:      BoolProperty    (name="with dags", default = False,
                        description = 'Import dags (!MIGHT BE REALLY SLOW!)')
    refresh_cache:  BoolProperty    (name="refresh cache", default = True,
                        description = 'Collect paths to all resources or use previously collected?')
    with_lods:      BoolProperty    (name="with lods", default = False,
                        description = 'Import dags (!MIGHT BE EVEN SLOWER!)')
classes.append(Dag_CMP_Import_Props)


class Dag_CMP_Export_Props(PropertyGroup):
    collection: PointerProperty (type = Collection, name = '',
                    description = 'Drag&Drop collection here or select it from list')
    dirpath:    StringProperty  (name="cmp export path", default = 'C:/tmp/', subtype = 'DIR_PATH',
                    description = "Path to save your composit",  update=upd_cmp_e_path)
classes.append(Dag_CMP_Export_Props)


class Dag_CMP_Tools_Props(PropertyGroup):
    node:       PointerProperty (type = Collection, name = 'Node')
    collection: PointerProperty(type = Collection, name = 'Collection')
#split_nodes
    recursive:      BoolProperty(default = False)
    destructive:    BoolProperty(default = False)
#nodes_to_asset
    naming_mode:    EnumProperty(
            #(identifier, name, description)
        items = [('COLLECTION','Collection','Specify collection directly'),
                 ('NAME','Name','Specify a name for new collection')],
        name = "Naming mode",
        default = 'COLLECTION')
    asset_name:         StringProperty(default = "new_asset", description = "Name for the new asset")
    parent_node:        PointerProperty(type = Object, name = "Parent", update = update_parent_node)
    parent_collection:  PointerProperty(type = Collection, name = "Collection")
    cleanup_parents:    BoolProperty(default = False, name = "Cleanup",
        description = "Remove parent nodes with no entities")
#composite_to_rendinst
    collection_to_convert:  PointerProperty(type = Collection, description = "Where to put new nodes?")
classes.append(Dag_CMP_Tools_Props)


#CMP JOINED
class Dag_CMP_Props(PropertyGroup):
    importer:   PointerProperty(type = Dag_CMP_Import_Props)
    exporter:   PointerProperty(type = Dag_CMP_Export_Props)
    tools:      PointerProperty(type = Dag_CMP_Tools_Props)
classes.append(Dag_CMP_Props)


#ALL JOINED
class dag4blend_props(PropertyGroup):
    tools:      PointerProperty(type = Dag_Tools_Props)
    importer:   PointerProperty(type = Dag_Import_Props)
    exporter:   PointerProperty(type = Dag_Export_Props)
    baker:      PointerProperty(type = Dag_Baking_Props)
    cmp:        PointerProperty(type = Dag_CMP_Props)
classes.append(dag4blend_props)

#UI#######################################################################
class DagSettings(AddonPreferences):
    bl_idname = __package__
    default_config_path = get_addon_directory() + '/dagorShaders.blk'
#global
    shader_config_path:     StringProperty(name = "dagorShaders.blk location",
                            default = default_config_path, subtype = 'FILE_PATH',
                            description = 'path to custom shader config location')

    projects:           CollectionProperty(type = DagProject)
    project_active:     EnumProperty      (items = get_projects, update = upd_project)
#shaders
    shaders:            PointerProperty(type = Empty_Group)
    shader_categories:  PointerProperty(type = Empty_Group)  # used to get shader names for category faster
#Addon features
    projects_maximized:     BoolProperty(default = False, description = 'Projects')
    experimental_maximized: BoolProperty(default = False, description = 'Experimental features')
    use_cmp_editor:         BoolProperty(default = False, name = "Composite Editor", description = 'Composit editor')
    guess_dag_type:         BoolProperty(default = False,
        description = 'Guess asset type .dag based on name and shader types. Would be used to check shader types on import and export')
    type_maximized:         BoolProperty(default = False, description = '')
    hide_dagormat_legacy:   BoolProperty(default = True, description = "Hide dagormat parameters that are no longer used")
#UI/smoothing groups
    sg_live_refresh:        BoolProperty(default = False, description = 'Convert to sharp edges on each change (might be slow!)')
    sg_set_maximized:       BoolProperty(default = False, description = 'Assign smoothing group to selected polygons')
    sg_select_maximized:    BoolProperty(default = False, description = 'Select polygonsby smoothing group')
#UI/props
    props_presets_path: StringProperty(subtype = 'DIR_PATH',
            default = bpy.utils.user_resource('SCRIPTS') + f"/addons/{__package__}/object_properties/presets")
    props_path_editing:     BoolProperty(default = False, description = "Change props presets directory")
    props_maximized:        BoolProperty(default = False)
    props_preset_maximized: BoolProperty(default = False)
    props_tools_maximized:  BoolProperty(default = False)
    prop_preset:            EnumProperty(items = get_obj_prop_presets)
    prop_preset_name:       StringProperty(default = "preset_name")
    prop_name:              StringProperty(default = 'name',description = 'name')
    prop_value:             StringProperty(default = 'value',description = 'value')
#UI/ColProps
    colprops_maximized:     BoolProperty(default = False)
    colprops_all_maximized: BoolProperty(default = False)
#UI/dagormat
    dagormat_main_maximized:BoolProperty(default = True)
    legacy_maximized:       BoolProperty(default = False)
    tex_maximized:          BoolProperty(default = False)
    optional_maximized:     BoolProperty(default = False)
    tools_maximized:        BoolProperty(default = False)
    proxy_maximized:        BoolProperty(default = False)

#UI/dagormat/tools
    process_all_materials:  BoolProperty(default = False)

#UI/View3D_tools
    matbox_maximized:       BoolProperty(default = False)
    searchbox_maximized:    BoolProperty(default = False)
    savebox_maximized:      BoolProperty(default = False)
    meshbox_maximized:      BoolProperty(default = False)
    scenebox_maximized:     BoolProperty(default = False)
    destrbox_maximized:     BoolProperty(default = False)
    destrsetup_maximized:   BoolProperty(default = False)
    destrdeform_maximized:  BoolProperty(default = False)

#UI/View3D_mesh_tools
    cleanup_maximized:      BoolProperty(default = False)
    vert_cleanup_maximized: BoolProperty(default = False)
    tris_cleanup_maximized: BoolProperty(default = False)

#UI/Composits
    cmp_imp_maximized:      BoolProperty(default = False)
    cmp_exp_maximized:      BoolProperty(default = False)
    cmp_entities_maximized: BoolProperty(default = False)
    cmp_tools_maximized:    BoolProperty(default = False)
    cmp_props_maximized:    BoolProperty(default = False)
    cmp_place_maximized:    BoolProperty(default = False)
#UI/Composits/Tools
    cmp_init_maximized:         BoolProperty(default = False)
    nodes_to_asset_maximized:   BoolProperty(default = False)
    node_to_mesh_maximized:     BoolProperty(default = False)
    basic_converter_maximized:  BoolProperty(default = False)
    cmp_tm_maximized:           BoolProperty(default = False)
    cmp_hyerarchy_maximized:    BoolProperty(default = False)
#UI/Project
    palettes_maximized:     BoolProperty(default = False)

#UI/Dag Importer Panel
    imp_props_maximized:    BoolProperty(default = False)
    # advanced mode (fnmatch)
    imp_includes_maximized: BoolProperty(default = False)
    imp_excludes_maximized: BoolProperty(default = False)
    # expert mode (regex)
    imp_includes_re_maximized:  BoolProperty(default = False)
    imp_excludes_re_maximized:  BoolProperty(default = False)

    def draw(self, context):
        layout = self.layout
        paths = layout.box()
        paths.prop(self, 'props_presets_path', text = "ObjProps presets")
        paths.prop(self, 'shader_config_path', text = "dagorShaders.blk")
        projects = layout.box()
        draw_custom_header(projects, "Projects", self, 'projects_maximized')
        if self.projects_maximized:
            if self.projects.__len__()>0:
                projects.prop(self, 'project_active', text = "Active")
            index=0
            for project in self.projects:
                draw_project_settings(projects, project, index)
                index+=1
            add_button = projects.row()
            add_button.scale_y = 2
            add_button.operator('dt.add_project',icon='ADD',text='ADD Project')
        experimental = layout.box()
        draw_custom_header(experimental, "Experimental Features", self, 'experimental_maximized')
        if self.experimental_maximized:
            draw_custom_toggle(experimental, self, 'hide_dagormat_legacy')
            draw_custom_toggle(experimental, self, 'use_cmp_editor')
classes.append(DagSettings)


def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    read_shader_config()
    bpy.types.Scene.dag4blend = PointerProperty(type = dag4blend_props)
    return


def unregister():
    for cl in classes:
        bpy.utils.unregister_class(cl)
    del bpy.types.Scene.dag4blend
    return