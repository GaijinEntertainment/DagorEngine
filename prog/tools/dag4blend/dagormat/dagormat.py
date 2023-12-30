import bpy, os
from bpy.props import BoolProperty, IntProperty, FloatProperty, FloatVectorProperty, StringProperty, PointerProperty, EnumProperty
from bpy.types import Operator, PropertyGroup, Panel

from .build_node_tree       import buildMaterial
from ..helpers.texts        import *
from ..helpers.basename     import basename
from .rw_dagormat_text      import dagormat_from_text, dagormat_to_text
from ..helpers.props        import fix_type
from ..helpers.popup        import show_popup
from  time                  import time

#temp
class NK_OT_do_nothing (Operator):
    bl_idname = "nk.do_nothing"
    bl_label = "dummy operator"
    bl_description = "placeholder that does nothing"
    bl_options = {'REGISTER', 'UNDO'}
    def execute(self,context):
        show_popup(message='Not implemented yet',title='Oops...',icon='INFO')
        return {'FINISHED'}

#functions
supported_extensions=['.dds','.tga','.tif']
#functions/search
def get_missing_tex(mats):
    missing_tex=[]
    for mat in mats:
        T=mat.dagormat.textures
        for tex in T.keys():
            if T[tex]!='':
                if basename(tex) not in missing_tex:
                    if not os.path.exists(bpy.data.images[basename(T[tex])].filepath):
                        if basename(T[tex]) not in missing_tex:
                            missing_tex.append(basename(T[tex]))
    return missing_tex

def find_textures(mats):
    missing_tex=get_missing_tex(mats)
    if missing_tex.__len__()==0:
        print('nothing to search')
        return{'FINISHED'}
    pref = bpy.context.preferences.addons[basename(__package__)].preferences
    i=int(pref.project_active)
    search_path = pref.projects[i].path

    for subdir,dirs,files in os.walk(search_path):
        for file in files:
            for name in missing_tex:
                for extension in supported_extensions:
                    if file.lower()==(name+extension).lower():
                        print ('Found: '+name)
                        bpy.data.images[name].source = 'FILE'
                        bpy.data.images[name].filepath = subdir + os.sep + file
    return{'FINISHED'}

def find_proxymats(mats):
    pref = bpy.context.preferences.addons[basename(__package__)].preferences
    i=int(pref.project_active)
    search_path = pref.projects[i].path
    proxymats=[]
    for mat in mats:
        DM=mat.dagormat
        if DM.is_proxy:
            if not os.path.exists(os.path.join(DM.proxy_path,(mat.name+'.proxymat.blk'))):
                if mat.name not in proxymats:
                    proxymats.append(mat.name)
    if proxymats.__len__()>0:
        for subdir,dirs,files in os.walk(search_path):
            for file in files:
                for proxy in proxymats:
                    if file.endswith('.proxymat.blk') and basename(file)==basename(proxy):
                        bpy.data.materials[proxy].dagormat.proxy_path=subdir+os.sep
                        read_proxy_blk(bpy.data.materials[proxy])

#functions/getters
def get_shader_categories(self,context):
    pref = bpy.context.preferences.addons[basename(__package__)].preferences
    items = []
    for shader_class in pref.shader_categories:
        name = shader_class.name
        item = (name, name, name)
        items.append(item)
    return items

def get_shaders(self,context):
    pref=context.preferences.addons[basename(__package__)].preferences
    items = []
    for cl in pref.shader_categories:
        if cl.name==self.shader_category_active:
            shaders=pref.shader_categories[self.shader_category_active]['shaders']
    for shader in shaders:
        name = shader['name']
        item = (name, name, name)
        items.append(item)
    current = self.shader_class
    if (current, current, current) not in items:
        items.append((current, current, current))
    return items

#functions/props
def get_props(self,context):
    DM     =context.object.active_material.dagormat
    exclude=list(DM.optional.keys())
    exclude.append('real_two_sided')#already exists in ui
    items  =[]
    pref   =context.preferences.addons[basename(__package__)].preferences
    found = False
    for shader_class in pref.shader_categories:
        for shader in shader_class['shaders']:
            if shader['name']==DM.shader_class:
                found = True
                for prop in shader['props']:
                    if prop['name'] not in exclude:
                        item=(prop['name'])
                        items.append((item,item,item))
                break
            if found:
                break
        if found:
            break
    p = self.prop_name
    if (p, p, p) not in items:
        items.append((p,p,p))
    return items

def upd_prop_active(self, context):
    pref=context.preferences.addons[basename(__package__)].preferences
    self.prop_name = self.prop_active
    self.prop_value = '???'
    found = False
    for shader_class in pref.shader_categories:
        for shader in shader_class['shaders']:
            if shader['name'] != self.shader_class:
                continue
            for prop in shader['props']:
                if prop['name'] == self.prop_active:
                    if prop['type'] == 'text':
                        self.prop_value='0,0,0,0'
                    elif prop['type'] == 'bool':
                        self.prop_value='yes'
                    elif prop['type'] == 'real':
                        self.prop_value='0.0'
                    elif prop['type'] == 'int':
                        self.prop_value='0'
                    found = True
                if found:
                    break
            if found:
                break
        if found:
            break
    return

def upd_prop_name(self, context):
    if self.prop_active!=self.prop_name:
        self.prop_active = self.prop_name
    return

#functions/upd
def upd_prop_selector(self,context):
    if self.prop_active == self.prop_name:
        return
    if self.use_prop_enum:
        self.prop_active = self.prop_name
    else:
        self.prop_name = self.prop_active
    return

def clear_tex_paths(mat):
    T=mat.dagormat.textures
    for tex in T.keys():
        if T[tex]!='':
            T[tex]=os.path.basename(T[tex])

def update_tex_paths(mat):
    T=mat.dagormat.textures
    for tex in T.keys():
        tex_name=basename(T[tex])#no extention needed
        if T[tex]=='':
            continue
        elif os.path.exists(T[tex]):
            continue
        img=bpy.data.images.get(tex_name)
        if img is None:
            continue
        if os.path.exists(img.filepath):
            T[tex]=img.filepath
    return

def update_backface_culling(self, context):
    #self is dagormat
    material = self.id_data#material, owner of current dagormat PropertyGroup
    material.use_backface_culling = self.sides == 0
    return

def update_material(self,context):
    material = self.id_data
    buildMaterial(material)
    return

def upd_shader_selector(self, context):
    if self.shader_class_active == self.shader_class:
        return
    if self.use_shader_enum:
        self.shader_class_active = self.shader_class
    else:
        self.shader_class = self.shader_class_active
    return

def upd_shader_category(self,context):
    if self.shader_class == self.shader_class_active:
        return
    pref = bpy.context.preferences.addons[basename(__package__)].preferences
    for shader in pref.shader_categories[self.shader_category_active]['shaders']:
        if self.shader_class == shader['name']:
            self.shader_class_active = self.shader_class
            return
    self.shader_class_active = pref.shader_categories[self.shader_category_active]['shaders'][0]['name']
    return

def upd_active_shader(self,context):
    if self.shader_class == self.shader_class_active:
        return
    self.shader_class = self.shader_class_active
    return

def update_shader_class(self, context):
    if self.shader_class_active != self.shader_class:
        self.shader_class_active = self.shader_class
    if self.use_prop_enum and not self.prop_active == self.prop_name:
            self.prop_active = self.prop_name
    material = self.id_data
    buildMaterial(material)
    return

def update_proxy_path(self,context):
    if context.active_object is None or context.active_object.active_material is None:
        return
    DM=context.object.active_material.dagormat
    if DM.proxy_path=='':
        return
    if DM.proxy_path.startswith('"') or DM.proxy_path.endswith('"'):
        DM.proxy_path=DM.proxy_path.replace('"','')
    basename=os.path.basename(DM.proxy_path)
    if basename!='' and basename.endswith('.proxymat.blk'):
        context.material.name=os.path.basename(DM.proxy_path).replace('.proxymat.blk','')
        DM.proxy_path=os.path.dirname(DM.proxy_path)
    if DM.proxy_path[-1]!=os.sep:
        DM.proxy_path=DM.proxy_path+os.sep
    return

#functions/proxy_rw
def read_proxy_blk(mat):
    proxymat = get_text_clear('dagormat_temp')
    DM=mat.dagormat
    file = open(os.path.join(DM.proxy_path,mat.name+'.proxymat.blk'),'r')
    lines=file.readlines()
    for line in lines:
        proxymat.write(line.replace(' ',''))
    file.close()
    bpy.ops.dt.dagormat_from_text(mat=mat.name)
    buildMaterial(mat)

def write_proxy_blk(mat):
    proxymat=get_text('dagormat_temp')
    DM=mat.dagormat
    path = DM.proxy_path
    if DM.is_proxy:
        file = open(os.path.join(DM.proxy_path,mat.name+'.proxymat.blk'),'w')
        for line in proxymat.lines:
            if line.body.__len__()>0:
                file.write(line.body+'\n')
        file.close()
#operators
#operators/upd
class DAGOR_OT_dagormats_update(Operator):#V
    bl_idname = "dt.dagormats_update"
    bl_label = "rebuild dagormats"
    bl_description = "rebuild dagormats"
    bl_options = {'UNDO'}
    mats:  StringProperty(default='')
    def execute(self,context):
        names=self.mats.split(',')
        mats=[bpy.data.materials.get(n) for n in names]
        start=time()
        for mat in mats:
            buildMaterial(mat)
        show_popup(message=f'finished in {round(time()-start,5)} sec')
        return{'FINISHED'}

#operators/proxy
class DAGOR_OT_read_proxy(Operator):#V
    bl_idname = "dt.dagormat_read_proxy"
    bl_label = "read proxymat"
    bl_description = "build dagormat from .proxymat.blk"
    bl_options = {'UNDO'}
    #TODO: replace by pointer
    mat:  StringProperty(default='Material')

    def execute(self,context):
        mat=bpy.data.materials.get(self.mat)
        if mat is None:
            show_popup(message='No material to process!',title='Error!',icon='ERROR')
            return{'CANCELLED'}
        DM=mat.dagormat
        if DM.is_proxy==False:
            show_popup(message="Material is'n proxy!",title='Error!',icon='ERROR')
            return{'CANCELLED'}
        proxy_file=os.path.join(DM.proxy_path,mat.name+'.proxymat.blk')
        if not os.path.exists(proxy_file):
            show_popup(message="Can't find "+proxy_file,title='Error!',icon='ERROR')
            return{'CANCELLED'}
        read_proxy_blk(mat)
        return{'FINISHED'}

class DAGOR_OT_write_proxy(Operator):#V
    bl_idname = "dt.dagormat_write_proxy"
    bl_label = "write proxymat"
    bl_description = "save dagormat to .proxymat.blk"
    bl_options = {'UNDO'}
    #TODO: replace by pointer
    mat:  StringProperty(default='Material')
    def execute(self,context):
        mat=bpy.data.materials.get(self.mat)
        if mat is None:
            show_popup(message='No material to process!',title='Error!',icon='ERROR')
            return{'CANCELLED'}
        DM=mat.dagormat
        if DM.is_proxy==False:
            show_popup(message="not a proxy material!",title='Error!',icon='ERROR')
            return{'CANCELLED'}
        if not os.path.exists(DM.proxy_path):
            show_popup(message="Can't find "+DM.proxy_path,title='Error!',icon='ERROR')
            return{'CANCELLED'}
        dagormat_to_text(mat,get_text_clear('dagormat_temp'))
        write_proxy_blk(mat)
        return{'FINISHED'}

#operators/search
class DAGOR_OT_FindProxymats(bpy.types.Operator):#V
    bl_idname = "dt.find_proxymats"
    bl_label = "find proxymats"
    bl_description = "search for .proxymat.blk files"
    bl_options = {'UNDO'}
    mats:  StringProperty(default='')
    def execute(self,context):
        names=self.mats.split(',')
        mats=[bpy.data.materials.get(n) for n in names]
        pref=bpy.context.preferences.addons[basename(__package__)].preferences
        if mats =='':
            show_popup(message='Nothing to process!',title='Error!',icon='ERROR')
            return {'CANCELLED'}
        start=time()
        find_proxymats(mats)
        show_popup(message=f'finished in {round(time()-start,5)} sec')
        return{'FINISHED'}

class DAGOR_OT_FindTextures(bpy.types.Operator):#V
    bl_idname = "dt.find_textures"
    bl_label = "find textures"
    bl_description = "search for dagMat textures"
    bl_options = {'UNDO'}
    mats:  StringProperty(default='')

    def execute(self,context):
        names=self.mats.split(',')
        mats=[bpy.data.materials.get(n) for n in names]
        pref=bpy.context.preferences.addons[basename(__package__)].preferences
        if mats=='':
            show_popup(message='Nothing to process!',title='Error!',icon='ERROR')
            return {'CANCELLED'}
        start=time()
        find_textures(mats)
        show_popup(message=f'finished in {round(time()-start,5)} sec')
        return{'FINISHED'}

class DAGOR_OT_SaveTextures(bpy.types.Operator):
    bl_idname = "dt.save_textures"
    bl_label = "Save textures"
    bl_description = "save existing dagMat textures to 'export folder/textures/...'"
    bl_options = {'UNDO'}
    def execute(self,context):
        print('\nStart texture searching')
        textures=[]
        for mat in bpy.data.materials:
            try:
                for tex in mat.dagormat.textures:
                    name=basename(tex)
                    if name not in textures and name!='':
                        textures.append(name)
            except:
                pass
        imgs = bpy.data.images
        e_path = context.scene.dag_export_path+'textures\\'
        if not os.path.exists(e_path):
                    os.makedirs(e_path)
        for image in textures:
            filepath=imgs[image].filepath
            if os.path.exists(filepath):
                print ('Saved: '+image)
                image = os.path.basename(filepath)
                copyfile(filepath,os.path.joyn(e_path,image))
        print('Done')
        return{'FINISHED'}

#operators/text editing
class DAGOR_OT_dagormat_to_text(Operator):
    bl_idname = "dt.dagormat_to_text"
    bl_label = "dagormat to text"
    bl_description = "Write dagormat to text"
    bl_options = {'UNDO'}
    mat:  StringProperty(default='Material')
    def execute(self,context):
        mat=bpy.data.materials.get(self.mat)
        if mat is None:
            show_popup(message='No material to process!',title='Error!',icon='ERROR')
            return{'CANCELLED'}
        text = get_text_clear('dagormat_temp')
        dagormat_to_text(mat,text)
        show_text(text)#only if it wasn't opened
        show_popup(message='Check "dagormat_temp" in text editor',title='Done',icon='INFO')
        return{'FINISHED'}

class DAGOR_OT_dagormat_from_text(Operator):
    bl_idname = "dt.dagormat_from_text"
    bl_label = "dagormat from text"
    bl_description = "Apply dagormat settings from text"
    bl_options = {'UNDO'}
    mat:  StringProperty(default='Material')
    def execute(self,context):
        mat=bpy.data.materials.get(self.mat)
        if mat is None or bpy.data.texts.get('dagormat_temp') is None:
            show_popup(message='No text to process!',title='Error!',icon='ERROR')
            return {'CANCELLED'}
        text = get_text('dagormat_temp')
        dagormat_from_text(mat,text)
        buildMaterial(mat)
        return {'FINISHED'}
#operators/parameters
class DAGOR_OT_addMaterialParam(Operator):
    bl_idname = "dt.add_mat_param"
    bl_label = "optional dagormat parameter:"
    bl_description = "Add optional parameter"
    bl_options = {'REGISTER', 'UNDO'}

    name:  StringProperty(name="",default='prop')
    value: StringProperty(name="",default='1.0')

    def execute(self, context):
        value=fix_type(self.value)
        context.active_object.active_material.dagormat.optional[self.name]=value
        return {'FINISHED'}

class DAGOR_OT_removeMaterialParam(Operator):
    bl_idname = "dt.remove_mat_param"
    bl_label = "remove"
    bl_description = "Remove parameter"
    bl_options = {'UNDO'}

    param: StringProperty(name="")

    def execute(self, context):
        del context.active_object.active_material.dagormat.optional[self.param]
        return {'FINISHED'}
#operators/textures
class DAGOR_OT_clear_tex_paths(Operator):#V
    bl_idname = "dt.clear_tex_paths"
    bl_label = "clear textures"
    bl_description = "Keep only base names, without paths"
    bl_options = {'UNDO'}

    mats:  StringProperty(default='')

    def execute(self,context):
        names=self.mats.split(',')
        mats=[bpy.data.materials.get(n) for n in names]
        start=time()
        for mat in mats:
            clear_tex_paths(mat)
        show_popup(message=f'finished in {round(time()-start,5)} sec')
        return {'FINISHED'}

class DAGOR_OT_upd_tex_paths(Operator):#V
    bl_idname = "dt.update_tex_paths"
    bl_label = "update texture paths"
    bl_description = "Replace unexisting texture paths by paths from loaded images"
    bl_options = {'UNDO'}

    mats:  StringProperty(default='')

    def execute(self,context):
        names=self.mats.split(',')
        mats=[bpy.data.materials.get(n) for n in names]
        start=time()
        for mat in mats:
            update_tex_paths(mat)
        show_popup(message=f'finished in {round(time()-start,5)} sec')
        return {'FINISHED'}

class DAGOR_OT_set_mat_sides(Operator):
    bl_idname = "dt.set_mat_sides"
    bl_label = ""
    bl_description = "How backfaces should be processed?"
    bl_options = {'UNDO'}
# 0 = single_sided, 1 = two_sided, 2 = real_two_sided
    sides: IntProperty(default = 0, min = 0, max = 2)
    @classmethod
    def poll(self, context):
        return context.active_object.active_material is not None

    def execute(self, context):
        DM = context.DM #set in UI right before call
        DM.sides = self.sides
        return {'FINISHED'}
#properties
class dagor_optional(PropertyGroup):
    pass
class dagor_textures(PropertyGroup):
    tex0:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex1:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex2:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex3:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex4:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex5:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex6:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex7:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex8:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex9:  StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)
    tex10: StringProperty(default='',subtype='FILE_PATH',description='global path or image name',update=update_material)

class dagormat(PropertyGroup):
#single properties
    ambient:        FloatVectorProperty(default=(0.5,0.5,0.5),max=1.0,min=0.0,subtype='COLOR')
    specular:       FloatVectorProperty(default=(0.0,0.0,0.0),max=1.0,min=0.0,subtype='COLOR')
    diffuse:        FloatVectorProperty(default=(1.0,1.0,1.0),max=1.0,min=0.0,subtype='COLOR')
    emissive:       FloatVectorProperty(default=(0.0,0.0,0.0),max=1.0,min=0.0,subtype='COLOR')
    power:          FloatProperty(default=0.0)
# 0 - single_sided, 1 - two_sided, 2 - real_two_sided
    sides:          IntProperty(default = 0, min = 0, max = 2, update = update_backface_culling)
#shader_class related stuff
    shader_class:           StringProperty(default='rendinst_simple',update = update_shader_class)
    use_shader_enum:        BoolProperty(default=False, description = 'use old shader selector',
                                            update = upd_shader_selector)
    shader_category_active: EnumProperty(items=get_shader_categories, update = upd_shader_category)
    shader_class_active:    EnumProperty(items=get_shaders, update = upd_active_shader)
#property groups
    textures:   PointerProperty(type=dagor_textures)
    optional:   PointerProperty(type=dagor_optional)
#temporary props for UI
    prop_name:          StringProperty (default='atest', update = upd_prop_name)
    prop_value:         StringProperty (default='127')
    prop_active:        EnumProperty   (items = get_props, update=upd_prop_active)
    use_prop_enum:  BoolProperty(default=False, description = 'show as dropdown list',  update=upd_prop_selector)
#proxymat related properties
    is_proxy:   BoolProperty   (default=False,description='is it proxymat?')
    proxy_path: StringProperty (default='', subtype = 'FILE_PATH', description='\\<Material.name>.proxymat.blk',
                                update=update_proxy_path)

#panel
class DAGOR_PT_dagormat(Panel):
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "material"
    bl_label = "dagormat"
    bl_options = {'DEFAULT_CLOSED'}

    @classmethod
    def poll(self, context):
        return context.active_object.active_material is not None

    def draw_sides_switcher(self,context, layout):
        DM = bpy.context.object.active_material.dagormat
        sides = DM.sides
        box = layout.box()
        col = box.column(align = True)
        pref = bpy.context.preferences.addons[basename(__package__)].preferences
        bool = pref.backfacing_maximized
        labels = ['single_sided', 'two_sided', 'real_two_sided']
        row = col.row(align = True)
        row.prop(pref, 'backfacing_maximized', text = "", emboss = False,
            icon = 'DOWNARROW_HLT' if bool else 'RIGHTARROW')
        row.prop(pref, 'backfacing_maximized', emboss = False,
            text = "Backface mode:" if bool else f"Backface mode: {labels[sides]}")
        if not bool:
            return
        col.context_pointer_set(name = "DM", data = DM)#called in operator

        row = col.row(align = True)
        row.operator('dt.set_mat_sides', text = "single_sided",
            icon ='RADIOBUT_ON' if sides == 0 else 'RADIOBUT_OFF').sides = 0

        row = col.row(align = True)
        row.operator('dt.set_mat_sides', text = "two_sided",
            icon ='RADIOBUT_ON' if sides == 1 else 'RADIOBUT_OFF').sides = 1

        row = col.row(align = True)
        row.operator('dt.set_mat_sides', text = "real_two_sided",
            icon ='RADIOBUT_ON' if sides == 2 else 'RADIOBUT_OFF').sides = 2
        return

    def draw(self,context):
        layout = self.layout
        pref=bpy.context.preferences.addons[basename(__package__)].preferences
        if bpy.context.object:
            if bpy.context.object.active_material:
                act_mat=bpy.context.object.active_material
                DM=act_mat.dagormat
#Material
                layout.prop(DM,'is_proxy',toggle=True, text='is proxymat',
                            icon = 'CHECKBOX_HLT'if DM.is_proxy else 'CHECKBOX_DEHLT')
                if not DM.is_proxy:
                    mat=layout.box()
                    header=mat.row()
                    header.prop(pref, 'mat_maximized',icon = 'DOWNARROW_HLT'if pref.mat_maximized else 'RIGHTARROW_THIN',
                        emboss=False,text='Main',expand=True)
                    header.label(text='',icon='MATERIAL')
                    if pref.mat_maximized:
                        self.draw_sides_switcher(context, mat)
                        colors=mat.row()
                        colors.prop(DM,  'ambient', text='')
                        colors.prop(DM,  'specular',text='')
                        colors.prop(DM,  'diffuse', text='')
                        colors.prop(DM,  'emissive',text='')
                        mat.prop   (DM,  'power')
                        cl=mat.row ()

                        if DM.use_shader_enum:
                            cl.prop (DM,  'shader_class_active',text='shader')
                            cl.prop (DM,'use_shader_enum', icon='MENU_PANEL', text='')
                            cl=mat.row()
                            cl.prop(DM,'shader_category_active', text='category')
                        else:
                            cl.prop (DM,  'shader_class',text='shader')
                            cl.prop (DM,'use_shader_enum', icon='MENU_PANEL', text='')
#Material/Textures
                    tex=layout.box()
                    header=tex.row()
                    header.prop(pref, 'tex_maximized',icon = 'DOWNARROW_HLT'if pref.tex_maximized else 'RIGHTARROW_THIN',
                        emboss=False,text='Textures')
                    header.label(text='',icon='IMAGE')
                    if pref.tex_maximized:
                        T=DM.textures
                        tex.prop(T, 'tex0', text='')
                        tex.prop(T, 'tex1', text='')
                        tex.prop(T, 'tex2', text='')
                        tex.prop(T, 'tex3', text='')
                        tex.prop(T, 'tex4', text='')
                        tex.prop(T, 'tex5', text='')
                        tex.prop(T, 'tex6', text='')
                        tex.prop(T, 'tex7', text='')
                        tex.prop(T, 'tex8', text='')
                        tex.prop(T, 'tex9', text='')
                        tex.prop(T, 'tex10',text='')
#Material/Optional
                    opt=layout.box()
                    header=opt.row()
                    header.prop(pref, 'opt_maximized',icon = 'DOWNARROW_HLT'if pref.opt_maximized else 'RIGHTARROW_THIN',
                        emboss=False,text='Optional')
                    header.label(text='',icon='THREE_DOTS')
                    if pref.opt_maximized:
                        addbox=opt.box()
                        add=addbox.operator("dt.add_mat_param",text="ADD",icon="ADD")
                        add.name=DM.prop_name
                        add.value=DM.prop_value
                        prew=addbox.row()
                        if DM.use_prop_enum:
                            prew.prop(DM,'prop_active',text='')
                        else:
                            prew.prop(DM,'prop_name',text='')
                        prew.label(text='',icon='FORWARD')
                        prew.prop(DM,'prop_value',text='')
                        prew.prop(DM,'use_prop_enum', icon='MENU_PANEL', text='')
                        for key in DM.optional.keys():
                            prop=opt.box()
                            rem=prop.row()
                            rem.label(text=key)
                            rem.operator('dt.remove_mat_param',text='',icon='TRASH').param=key
                            value=prop.row()
                            value.prop(DM.optional, '["'+key+'"]',text='')
#Proxymat
                else:
                    proxy=layout.box()
                    header=proxy.row()
                    header.prop(pref, 'proxy_maximized',icon = 'DOWNARROW_HLT'if pref.proxy_maximized else 'RIGHTARROW_THIN',
                        emboss=False,text='Proxy')
                    header.label(text='',icon='MATERIAL')
                    if pref.proxy_maximized:
                        proxy.label(text='Proxymat folder:')
                        proxy.prop(DM,'proxy_path',text='')
                        proxy.operator('dt.dagormat_read_proxy', text='(Re)load from file',icon='RECOVER_LAST').mat=act_mat.name
                        proxy.operator('dt.dagormat_write_proxy',text='Save proxymat',icon='FILE_TICK').mat=act_mat.name
#Tools
                tools=layout.box()
                header=tools.row()
                header.prop(pref, 'tools_maximized',icon = 'DOWNARROW_HLT'if pref.tools_maximized else 'RIGHTARROW_THIN',
                    emboss=False,text='Tools')
                header.label(text='',icon='TOOL_SETTINGS')
                if pref.tools_maximized:
                    text=tools.box()
#Tools/Text Editing
                    text.label(text='', icon='TEXT')
                    text_ed=text.row()
                    text_ed.operator('dt.dagormat_to_text',text='Open as text').mat=act_mat.name
                    text_ed.operator('dt.dagormat_from_text',text='Apply from text').mat=act_mat.name
#Tools/Search
                    mats=','.join([m.name for m in bpy.data.materials]) if pref.process_all else act_mat.name

                    shared=tools.box()
                    shared.label(text='', icon='VIEWZOOM')
                    mode=shared.box()
                    mode.prop(pref, 'process_all',text='Process all materials' if pref.process_all else 'Process active material only', emboss=False)

                    shared.operator('dt.find_textures',text='Find missing textures',icon='TEXTURE').mats=mats
                    if DM.is_proxy or pref.process_all:
                        shared.operator('dt.find_proxymats',text='Find missing proxymat', icon='MATERIAL').mats=mats
#Tools/Operators
                    shared.label(text='',icon='TOOL_SETTINGS')
                    shared.operator('dt.dagormats_update',text='Rebuild materials' if pref.process_all else 'Rebuild material',
                    icon='MATERIAL').mats=mats
                    shared.operator('dt.update_tex_paths',text='Update texture paths', icon='TEXTURE').mats=mats
                    shared.operator('dt.clear_tex_paths',text='Clear texture paths',  icon='TEXTURE').mats=mats

classes = [dagor_optional,dagor_textures,dagormat,
            DAGOR_OT_set_mat_sides,
            DAGOR_OT_removeMaterialParam, DAGOR_OT_addMaterialParam,
            DAGOR_OT_clear_tex_paths, DAGOR_OT_upd_tex_paths,
            DAGOR_OT_dagormat_to_text, DAGOR_OT_dagormat_from_text,
            DAGOR_OT_FindProxymats, DAGOR_OT_FindTextures,
            DAGOR_OT_dagormats_update, DAGOR_OT_read_proxy, DAGOR_OT_write_proxy,
            DAGOR_PT_dagormat]

def register():
    for cl in classes:
        bpy.utils.register_class(cl)
    bpy.types.Material.dagormat=PointerProperty(type=dagormat)


def unregister():
    for cl in classes:
        bpy.utils.unregister_class(cl)
    del bpy.types.Material.dagormat
if __name__ == "__main__":
    register()