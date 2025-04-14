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
from bpy.types          import PropertyGroup, Operator, AddonPreferences, Panel, Collection
from bpy.utils          import user_resource
from os                 import listdir
from os.path            import exists, splitext, isfile, join

from .read_config                       import read_config
from .helpers.popup                     import show_popup
from .helpers.get_preferences           import get_preferences
from .projects.draw_project_manager     import draw_project_settings
from .ui.draw_elements                  import draw_custom_header

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
        self.filepath=self.filepath.replace('"','')
    return

def upd_cmp_e_path(self,context):
    if self.dirpath.find('"') > -1:
        self.dirpath=self.dirpath.replace('"','')
    if not self.dirpath.endswith(os.sep):
        self.dirpath = self.dirpath + os.sep
    return

def upd_imp_path(self,context):
    if self.dirpath.find('"')>-1:
        self.dirpath=self.dirpath.replace('"','')
    if self.dirpath.endswith('.dag'):
        self.masks = os.path.basename(self.dirpath)  # dag name turned to mask for batch import
        self.dirpath = os.path.dirname(self.dirpath)  # dag name removed from path
    if not self.dirpath.endswith(os.sep):
        self.dirpath = self.dirpath+os.sep
    return

def upd_exp_path(self,context):
    if self.dirpath.find('"')>-1:
        self.dirpath=self.dirpath.replace('"','')
    if not self.dirpath.endswith(os.sep):
        self.dirpath+=os.sep
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
        index = self.index
        if index <0:
            return{'CANCELLED'}
        pref = get_preferences()
        if len(pref.projects) > index:
            pref.projects.remove(index)
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
        show_popup(message=f'New project added. Please, configure it')
        return {'FINISHED'}
classes.append(DAG_OT_AddProject)


#shader classes###########################################################

class DagShaderProp(PropertyGroup):
    name:   StringProperty()
    type:   StringProperty()
classes.append(DagShaderProp)


class DagShader(PropertyGroup):
    name:   bpy.props.StringProperty()
    props:  CollectionProperty(type = DagShaderProp)
classes.append(DagShader)


class DagShaderClass(PropertyGroup):
    name:    StringProperty()
    shaders: CollectionProperty(type = DagShader)
classes.append(DagShaderClass)


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
    save_textures_dirpath:  StringProperty(name = "", default = 'C:\\tmp\\', subtype = 'DIR_PATH')
    save_proxymats_dirpath: StringProperty(name = "", default = 'C:\\tmp\\', subtype = 'DIR_PATH')
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
class Dag_Import_Props(PropertyGroup):
    with_subfolders :BoolProperty  (name="Search in subfolders", default=False,
                        description = 'Search for .dags in subfolders as well')
    mopt            :BoolProperty  (name="Optimize material slots", default=True,
                        description = 'Remove unnecessary material slots')
    preserve_sg     :BoolProperty  (name="Preserve Smoothing Groups", default=False,
                        description = "Store Smoothing Groups in a integer face attribute called 'SG'")
    replace_existing:BoolProperty  (name="Reimport existing", default=False,
                        description = 'Replace dags that already exist in scene by imported versions')
    preserve_path   :BoolProperty  (name="Preserve paths", default=False,
                        description = 'Override export path for each imported dag')
    dirpath         :StringProperty(name="Path",    default='C:\\tmp\\', subtype = 'DIR_PATH',
                        description = "Where search for .dag files?",update=upd_imp_path)
    masks           :StringProperty(name="Masks",   default='',
                        description = 'name should contain at least one to be imported. Split by";"')
    excludes        :StringProperty(name="Excludes",default='',
                        description = 'name should not contain any to be imported. Split by";"')
classes.append(Dag_Import_Props)


#EXPORTER
class Dag_Export_Props(PropertyGroup):
    filename        :StringProperty(name="Name", default='Filename', description = 'Name for exported .dag file')
    dirpath         :StringProperty(name="Path", default='C:\\tmp\\', subtype = 'DIR_PATH',
                        description = "Where your .dag files should be saved?",update=upd_exp_path)
    modifiers       :BoolProperty(name="applyMods", default=True,
                        description = 'Export meshes with effects of modifiers')
    mopt            :BoolProperty(name="Optimize Materials", default=True,
                        description = 'Remove unused slots; merge similar materials')
    vnorm           :BoolProperty(name="vNormals", default=True, description = 'Export of custom vertex normals')
    orphans         :BoolProperty(name="exportOrphans", default=False,
                        description = 'Export of objects not in the bottom of hierarchy as separate .dags')
    collection      :PointerProperty(type = Collection, name = '',
        description = 'Drag&Drop collection here or select it from list. Keep it empty to use Scene Collection instead')
    cleanup_names   :BoolProperty(name="Cleanup Names", default=False,
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
    tex_dirpath:        StringProperty(subtype='DIR_PATH', default = 'C:\\tmp\\',
                            description = "Directory for baked textures")
    proxymat:           BoolProperty(default = False, name = "Create Proxymat File",
                            description = "Save rendinst_simple proxymat with baked textures")
    apply_proxymat:     BoolProperty(default = False, name = "Apply Proxymat",
                            description = "Apply saved proxymat to the model")
    proxymat_dirpath:   StringProperty(subtype='DIR_PATH', default = 'C:\\tmp\\',
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
    filepath:       StringProperty  (name="cmp import path", default='C:\\tmp\\', subtype = 'FILE_PATH',
                        description = "Path to composit", update=upd_cmp_i_path)
    with_sub_cmp:   BoolProperty    (name="recursive", default=False, description = 'Search for sub-composits as well')
    with_dags:      BoolProperty    (name="with dags", default=False,
                        description = 'Import dags (!MIGHT BE REALLY SLOW!)')
    refresh_cache:  BoolProperty    (name="refresh cache", default=True,
                        description = 'Collect paths to all resources or use previously collected?')
    with_lods:      BoolProperty    (name="with lods", default=False,
                        description = 'Import dags (!MIGHT BE EVEN SLOWER!)')
classes.append(Dag_CMP_Import_Props)


class Dag_CMP_Export_Props(PropertyGroup):
    collection: PointerProperty (type = bpy.types.Collection, name = '',
                    description = 'Drag&Drop collection here or select it from list')
    dirpath:    StringProperty  (name="cmp export path", default='C:\\tmp\\', subtype = 'DIR_PATH',
                    description = "Path to save your composit",  update=upd_cmp_e_path)
classes.append(Dag_CMP_Export_Props)


class Dag_CMP_Tools_Props(PropertyGroup):
    node:       PointerProperty (type = bpy.types.Collection, name = 'Node')
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
    def_path='C:\\replace_by_correct_path\\'
    def_cfg_path=bpy.utils.user_resource('SCRIPTS') + f"\\addons\\{__package__}\\dagorShaders.cfg"
#global
    assets_path:        StringProperty(default=def_path,     subtype = 'DIR_PATH',
                            description = 'assets location')
    cfg_path:           StringProperty(default=def_cfg_path, subtype = 'FILE_PATH',
                            description = 'dagorShaders.cfg location')

    new_project_name:   StringProperty(description = 'name of project')
    new_project_path:   StringProperty(description = 'where assets(dags, textures, composits) of that project stored?',
                                        subtype = 'DIR_PATH')

    projects:           CollectionProperty(type =DagProject)
    project_active:     EnumProperty      (items=get_projects, update = upd_project)
#shaders
    shader_categories:     CollectionProperty(type =DagShaderClass)
#Addon features
    projects_maximized:     BoolProperty(default=False,description='Projects')
    experimental_maximized: BoolProperty(default=False,description='Experimental features')
    use_cmp_editor:         BoolProperty(default=False,name = "Composite Editor", description='Composit editor')
    guess_dag_type:         BoolProperty(default=False,description='Guess what asset tipe .dag should be, based on name and shader types. Would be used to check shader types on import and export')
    type_maximized:         BoolProperty(default=False,description='')
#UI/smoothing groups
    sg_live_refresh:        BoolProperty(default=False,description='Convert to sharp edges on each change (might be slow!)')
    sg_set_maximized:       BoolProperty(default=False,description='Assign smoothing group to selected polygons')
    sg_select_maximized:    BoolProperty(default=False,description='Select polygonsby smoothing group')
#UI/props
    props_presets_path: StringProperty(subtype = 'DIR_PATH',
            default = bpy.utils.user_resource('SCRIPTS') + f"\\addons\\{__package__}\\object_properties\\presets")
    props_path_editing:     BoolProperty(default = False, description = "Change props presets directory")
    props_maximized:        BoolProperty(default=True)
    props_preset_maximized: BoolProperty(default=True)
    props_tools_maximized:  BoolProperty(default=True)
    prop_preset:            EnumProperty(items = get_obj_prop_presets)
    prop_preset_name:       StringProperty(default = "preset_name")
    prop_name:              StringProperty(default = 'name',description='name')
    prop_value:             StringProperty(default = 'value',description='value')
#UI/ColProps
    colprops_maximized:     BoolProperty(default=True)
    colprops_all_maximized: BoolProperty(default=True)
#UI/dagormat
    mat_maximized:          BoolProperty(default=True)
    backfacing_maximized:   BoolProperty(default=True)
    tex_maximized:          BoolProperty(default=True)
    opt_maximized:          BoolProperty(default=True)
    tools_maximized:        BoolProperty(default=True)
    proxy_maximized:        BoolProperty(default=True)


#UI/dagormat/tools
    process_materials:      EnumProperty(
            #(identifier, name, description)
        items = [('ACTIVE','Active material','Active material only'),
                 ('ALL','All materials','All supported materials'),
                 ],
        default = 'ACTIVE')

#UI/View3D_tools
    matbox_maximized:       BoolProperty(default = True)
    searchbox_maximized:    BoolProperty(default = True)
    savebox_maximized:      BoolProperty(default = True)
    meshbox_maximized:      BoolProperty(default = True)
    scenebox_maximized:     BoolProperty(default = True)
    destrbox_maximized:     BoolProperty(default = True)
    destrsetup_maximized:   BoolProperty(default = True)
    destrdeform_maximized:  BoolProperty(default = True)

#UI/View3D_mesh_tools
    cleanup_maximized:      BoolProperty(default = True)
    vert_cleanup_maximized: BoolProperty(default = True)
    tris_cleanup_maximized: BoolProperty(default = True)


#UI/Composits
    cmp_imp_maximized:      BoolProperty(default=True)
    cmp_exp_maximized:      BoolProperty(default=True)
    cmp_entities_maximized: BoolProperty(default=True)
    cmp_tools_maximized:    BoolProperty(default=True)
#UI/Composits/Tools
    cmp_init_maximized:     BoolProperty(default=True)
    cmp_node_prop_maximized:BoolProperty(default=True)
    cmp_converter_maximized:BoolProperty(default=True)
#UI/Project
    palettes_maximized:     BoolProperty(default=True)

    def draw(self, context):
        l = self.layout
        paths = l.box()
        paths.prop(self, 'props_presets_path', text = "ObjProps presets")
        paths.prop(self, 'cfg_path', text = "dagorShaders.cfg")
        projects = l.box()
        draw_custom_header(projects, "Projects", self, 'projects_maximized', control_value = self.projects_maximized)
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
        exp=l.box()
        exp.prop(self,'experimental_maximized',text='Experimental Features',
                icon = 'DOWNARROW_HLT'if self.experimental_maximized else 'RIGHTARROW_THIN',
                emboss=False,expand=True)
        if self.experimental_maximized:
            warning = exp.box()
            wrow = warning.row()
            wrow.label(text = '', icon = 'ERROR')
            wrow.label(text='Might be unstable. Use it on your own risk')
            wrow.label(text = '', icon = 'ERROR')
            exp.prop(self, 'use_cmp_editor', toggle = True,
            icon = 'CHECKBOX_HLT' if self.use_cmp_editor else 'CHECKBOX_DEHLT')
classes.append(DagSettings)


def init_shader_categories():
    pref = get_preferences()
    shader_categories = pref.shader_categories
    dagorShaders = read_config()
    shader_categories.clear()
    for shader_class in dagorShaders:
        new_shader_class = shader_categories.add()
        new_shader_class.name = shader_class[0].replace('-','')
        for shader in shader_class[1]:
            new_shader = new_shader_class.shaders.add()
            new_shader.name = shader[0]
            for prop in shader[1]:
                new_prop = new_shader.props.add()
                new_prop.name = prop[0]
                new_prop.type = prop[1]
    return


def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    init_shader_categories()
    bpy.types.Scene.dag4blend = PointerProperty(type = dag4blend_props)
    return


def unregister():
    for cl in classes:
        bpy.utils.unregister_class(cl)
    del bpy.types.Scene.dag4blend
    return