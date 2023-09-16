import bpy

from bpy.props      import StringProperty, BoolProperty, EnumProperty,CollectionProperty,IntProperty
from bpy.types      import PropertyGroup,Operator,AddonPreferences,Panel
from bpy.utils      import user_resource
from os             import listdir
from os.path        import exists, splitext, isfile, join

from .read_config   import read_config
from .helpers.popup import show_popup

S=bpy.types.Scene

#projects interaction#####################################################
def get_projects(self, context):
    pref=context.preferences.addons[__package__].preferences
    items = []
    i=0
    for proj in pref.projects:
        name = proj.name
        path = proj.path
        item = (str(i), name, path)
        items.append(item)
        i+=1
    return items

class DAG_OT_RemoveProject(Operator):
    bl_idname = "dt.remove_project"
    bl_label = "Remove"
    def execute(self, context):
        pref=context.preferences.addons[__package__].preferences
        if len(pref.projects) > 0:
            i=int(pref.project_active)
            pref.projects.remove(i)
        return {'FINISHED'}

class DagProject(PropertyGroup):
    name: StringProperty()
    path: StringProperty(subtype = 'DIR_PATH')

class DAG_OT_AddProject(Operator):
    bl_idname = "dt.add_project"
    bl_label = "Add"
    def execute(self, context):
        pref=context.preferences.addons[__package__].preferences
        new = pref.projects.add()
        new.name = pref.new_project_name
        new.path = pref.new_project_path
        show_popup(message=f'Project added: {new.name}')
        return {'FINISHED'}

#shader classes###########################################################

class DagShaderProp(PropertyGroup):
    name:   StringProperty()
    type:   StringProperty()

class DagShader(PropertyGroup):
    name:   bpy.props.StringProperty()
    props:  CollectionProperty(type = DagShaderProp)

class DagShaderClass(PropertyGroup):
    name:    StringProperty()
    shaders: CollectionProperty(type = DagShader)

def get_shader_classes(self,context):
    pref=context.preferences.addons[__package__].preferences
    items = []
    for shader_class in pref.shader_classes:
        name = shader_class.name
        item = (name, name, name)
        items.append(item)
    return items

def get_shaders(self,context):
    pref=context.preferences.addons[__package__].preferences
    items = []
    for cl in pref.shader_classes:
        if cl.name==pref.shader_class_active:
            shaders=pref.shader_classes[pref.shader_class_active]['shaders']
    for shader in shaders:
        name = shader['name']
        item = (name, name, name)
        items.append(item)
    return items

def get_obj_prop_presets(self, context):
    pref=bpy.context.preferences.addons[__package__].preferences
    path = user_resource('SCRIPTS') + f'\\addons\\{__package__}\\object_properties\\presets\\'
    items = []
    files = [f for f in listdir(path) if isfile(join(path,f))]
    for f in files:
        i = (splitext(f)[0])
        items.append((i,i,i))
    return items

def upd_shader_class(self,context):
    pref=context.preferences.addons[__package__].preferences
    try:
        context.object.active_material.dagormat.shader_class=pref.shader_active
    except:
        pass
def upd_shader_selector(self,context):
    pref=context.preferences.addons[__package__].preferences
    shader_active=context.object.active_material.dagormat.shader_class
    for shader_class in pref.shader_classes:
        for shader in shader_class.shaders:
            if shader['name']==shader_active:
                pref.shader_class_active=shader_class.name
                pref.shader_active=shader.name
                break

def upd_prop_selector(self,context):
    pref=context.preferences.addons[__package__].preferences
#UI#######################################################################
class DagSettings(AddonPreferences):
    bl_idname = __package__
    def_path='C:\\replace_by_correct_path\\'
    def_cfg_path='D:\\dagor2\tools\\dagor3_cdk\\plugin3dsMax-x64\\dagorShaders.cfg'
#global
    assets_path:        StringProperty(default=def_path,     subtype = 'DIR_PATH', description = 'assets location')
    #cfg_path:           StringProperty(default=def_cfg_path, subtype = 'FILE_PATH',description = 'dagorShaders.cfg location')

    new_project_name:   StringProperty(description = 'name of project')
    new_project_path:   StringProperty(description = 'where assets(dags, textures, composits) of that project stored?',
                                        subtype = 'DIR_PATH')

    projects:           CollectionProperty(type =DagProject)
    project_active:     EnumProperty      (items=get_projects)
#shaders
    shader_classes:     CollectionProperty(type =DagShaderClass)
#Addon features
    experimental_unfold:BoolProperty(default=False,description='Experimental features')
    use_cmp_editor:     BoolProperty(default=False,description='Composit editor')
    use_tex_baker:      BoolProperty(default=False,description='Baking tools for dag shaders')
    guess_dag_type:     BoolProperty(default=False,description='Guess what asset tipe .dag should be, based on name and shader types. Would be used to check shader types on import and export')
    type_unfold:        BoolProperty(default=False,description='')
#UI/props
    props_unfold:       BoolProperty(default=True)
    props_preset_unfold:BoolProperty(default=True)
    props_tools_unfold: BoolProperty(default=True)
    prop_preset:        EnumProperty(items = get_obj_prop_presets)
    prop_preset_name:   StringProperty(default = "preset_name")
    prop_name:          StringProperty(default = 'name',description='name')
    prop_value:         StringProperty(default = 'value',description='value')
#UI/ColProps
    colprops_unfold:    BoolProperty(default=True)
    colprops_all_unfold:BoolProperty(default=True)
#UI/dagormat
    mat_maximized:      BoolProperty(default=True)
    tex_maximized:      BoolProperty(default=True)
    opt_maximized:      BoolProperty(default=True)
    tools_maximized:    BoolProperty(default=True)
    proxy_maximized:    BoolProperty(default=True)

    old_shader_selector:BoolProperty(default=False, description = 'use old shader selector',update=upd_shader_selector)
    shader_class_active:EnumProperty(items=get_shader_classes,update=upd_shader_class)
    shader_active:      EnumProperty(items=get_shaders,       update=upd_shader_class)

    old_prop_selector:  BoolProperty(default=False, description = 'show as dropdown list',  update=upd_prop_selector)
#UI/dagormat/tools
    process_all:        BoolProperty(default=False, description = 'Process every element instead of active only')

#UI/View3D_tools
    matbox_maximized:   BoolProperty(default=True)
    searchbox_maximized:BoolProperty(default=True)
    savebox_maximized:  BoolProperty(default=True)
    meshbox_maximized:  BoolProperty(default=True)
    scenebox_maximized: BoolProperty(default=True)
    destrbox_maximized: BoolProperty(default=True)
    windbox_maximized:  BoolProperty(default=True)

#UI/extra_tools
    wind_defaultSG:              IntProperty       (default = 1,  description = "smooth group normal to use on hard edges")
    wind_vertexColorIndexName:   StringProperty    (default='windDir', description = "Normals vertex color index name")
    destr_materialName:          StringProperty    (default='wood', description = "physmat of destr colliders")
    destr_density:               IntProperty       (default = 100,  description = "density of colliders")

    def draw(self, context):
        pref=context.preferences.addons[__package__].preferences
        l = self.layout
        proj=l.box()
        new=proj.row()
        new.prop (self,'new_project_name',text='')
        new.prop (self,'new_project_path',text='')
        new.operator('dt.add_project',icon='ADD',text='')
        sel=proj.row()
        sel.prop(self,'project_active',   text='Active project')
        sel.operator('dt.remove_project',icon='TRASH',text='')
        exp=l.box()
        namerow = exp.row()
        namerow.prop(pref,'experimental_unfold',text='',icon = 'DOWNARROW_HLT'if pref.experimental_unfold else 'RIGHTARROW_THIN',
                emboss=False,expand=True)
        namerow.label(text = "Experimental Features")
        if pref.experimental_unfold:
            warning = exp.box()
            wrow = warning.row()
            wrow.label(text = '', icon = 'ERROR')
            wrow.label(text='Might be unstable. Use it on your own risk')
            wrow.label(text = '', icon = 'ERROR')
            exp.prop(pref,'use_cmp_editor',text='Use composit editor')
            exp.prop(pref,'use_tex_baker',text='Use baking tools')
            #exp.prop(pref,'guess_dag_type',text='Guess what asset type ".dag" should be')
            #not fully implemented yet
class DAG_OT_SetSearchFolder(Operator):
    bl_idname = "dt.set_search_path"
    bl_label = "Set search folder"
    bl_description = "Use whole project folder for search"
    def execute(self, context):
        pref=context.preferences.addons[__package__].preferences
        i=int(pref.project_active)
        context.scene.dag_import_path=pref.projects[i].path
        context.scene.dag_import_subfolders=True
        show_popup(message='Import without full name of an asset as mask will be extremely slow!',title='WARNING!', icon='INFO')
        return {'FINISHED'}

class DAG_PT_project(Panel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "scene"
    bl_label = "Project (dagor)"

    def draw(self,context):
        layout = self.layout
        pref=context.preferences.addons[__package__].preferences
        layout.prop(pref,'project_active',text='')
        if exists(bpy.utils.user_resource('SCRIPTS',path=f'addons\\{__package__}\\importer')):
            layout.operator('dt.set_search_path',text='Apply as search path',icon='ERROR')

classes = [DagProject, DAG_OT_AddProject, DAG_OT_RemoveProject, DAG_PT_project,
            DAG_OT_SetSearchFolder,
            DagShaderProp, DagShader, DagShaderClass,
            DagSettings]

def init_shader_classes():
    shader_classes=bpy.context.preferences.addons[__package__].preferences.shader_classes
    dagorShaders=read_config()
    shader_classes.clear()
    for shader_class in dagorShaders:
        new_shader_class=shader_classes.add()
        new_shader_class.name=shader_class[0].replace('-','')
        for shader in shader_class[1]:
            new_shader=new_shader_class.shaders.add()
            new_shader.name=shader[0]
            for prop in shader[1]:
                new_prop=new_shader.props.add()
                new_prop.name=prop[0]
                new_prop.type=prop[1]
def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    init_shader_classes()

def unregister():
    for cl in classes:
        bpy.utils.unregister_class(cl)