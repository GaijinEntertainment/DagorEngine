import mathutils

from os         import makedirs
from os.path    import exists, join, dirname

import bpy
from bpy.path                       import ensure_ext
from mathutils                      import Matrix
from shutil                         import copyfile
from bpy_extras.io_utils            import ExportHelper
from bpy.props                      import StringProperty, BoolProperty, EnumProperty, PointerProperty
from bpy.types                      import Operator
from ..object_properties            import object_properties
from .                              import writer
from ..dagMath                      import mul
from ..constants                    import *
from ..face                         import FaceExp
from ..node                         import NodeExp
from ..nodeMaterial                 import MaterialExp
from ..mesh                         import MeshExp
from ..popup.popup_functions    import show_popup
from ..helpers.names                import ensure_no_path
from ..helpers.multiline            import _label_multiline
from ..helpers.texts                import log
from ..helpers.version              import get_blender_version
from ..tools.tools_panel            import apply_modifiers, fix_mat_slots, optimize_mat_slots, preserve_sharp
from ..tools.tools_functions        import *

from ..smooth_groups.smooth_groups              import int_to_uint
from ..smooth_groups.mesh_calc_smooth_groups    import objects_calc_smooth_groups


SUPPORTED_TYPES = ('MESH', 'CURVE')  # ,'CURVE','EMPTY','TEXT','CAMERA','LAMP')

#reorders UVMap names to match three first chanels to viewport preview
def sort_uv_names(keys):
    keys.sort()
    sorted = []
    for name in ['UVMap', 'UVMap.001', 'UVMap.002']:
        if name in keys:
            sorted.append(name)
            keys.remove(name)
    sorted += keys
    return sorted

#CLASSES

class DAG_PT_export(bpy.types.Panel):
    bl_space_type = 'FILE_BROWSER'
    bl_region_type = 'TOOL_PROPS'
    bl_label = "Mode:"
    bl_parent_id = "FILE_PT_operator"

    @classmethod
    def poll(cls, context):
        sfile = context.space_data
        operator = sfile.active_operator

        return operator.bl_idname == 'EXPORT_SCENE_OT_dag'

    def draw(self, context):
        l = self.layout
        l.use_property_decorate = False  # No animation.
        S = context.scene
        P = bpy.data.scenes[0].dag4blend.exporter
        sfile = context.space_data
        operator = sfile.active_operator
        hint='Wait... What mode we in?'
        l.prop(operator, "limits")
        if operator.limits=='Visible':
            hint = 'Export all visible objects of supported types as single .dag.'
        elif operator.limits=='Sel.Joined':
            hint = 'Export all selected objects of supported types as single .dag.'
        elif operator.limits=='Sel.Separated':
            hint = 'Export all visible objects of supported types as separate .dag, named as objects itself. User defined name will be ignored.'
        elif operator.limits=='Col.Separated':
            if P.collection==None:
                if S.collection.children.__len__()==0:
                    hint = "No sub-collections in Scene Collection. Scene Collection.dag can't be exported"
                else:
                    hint = 'Export each collection without sub-collections in scene as separate .dags, named as collections itself. User defined name will be ignored.'
            elif P.collection.children.__len__()>0:
                hint = f'Export each sub-collection of {P.collection.name} without children as separate .dags, named as collections itself. User defined name will be ignored.'
            else:
                hint = f'Export {P.collection.name}.dag. User defined name will be ignored.'
            l.prop(P,"collection")
            l.prop(P,"orphans")
        elif operator.limits=='Col.Joined':
            if P.collection==None:
                if S.collection.children.__len__()==0:
                    hint = "Select collection for export. Scene Collection.dag can't be exported"
                else:
                    hint = f'Export {S.collection.name}.dag (including objects of sub-collections). User defined name will be ignored.'
            l.prop(P,"collection")
        info=l.box()
        _label_multiline(context=context, text=hint, parent=info)

class DagExporter(Operator, ExportHelper):
    bl_idname = "export_scene.dag"
    bl_label = "Export DAG"
    bl_options = {'REGISTER', 'UNDO'}

    nodes = None
    writer = None
    last_index = 0

    # ImportHelper mixin class uses this
    filename_ext = ".dag"
    filter_glob = StringProperty(default="*.dag", options={'HIDDEN'})
    normals: BoolProperty(
            name="Preserve custom normals",
            description="Export custom normals when it exists",
            default=True)

    modifiers: BoolProperty(
            name="Apply modifiers",
            description="Preserve effects of modifiers (exported geometry only)",
            default=True)

    mopt: BoolProperty(
            name="Optimize materials",
            description="Merge material slots with similar materials, remove unused slots",
            default=True)

    orphans: BoolProperty(
            name="Export orphans",
            description="Export objects not in the bottom of hierarchy as separate .dags",
            default=True,
            options={'HIDDEN'})

    limits: EnumProperty(
                #(identifier, name, description)

        items = [('Visible','Visible','Export everithing as one dag'),
             ('Sel.Joined','Sel.Joined','Selected objects as one dag'),
             ('Sel.Separated','Sel.Separated','Selected objects as separate dags'),
             ('Col.Separated','Col.Separated','Chosen collection and subcollections as separate .dags'),
             ('Col.Joined','Col.Joined','Objects of chosen collection and subcollections as one .dag')],
        name = "",
        options={'HIDDEN'},
        default = 'Visible')

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.writer = writer.DagWriter()
        self.last_index = 0
        self.nodes = list()
        self.materials = {}
        self.textures = []

    def createRootNode(self):
        node = NodeExp()
        node.id = 65535
        node.parent_id = -1
        node.tm = Matrix()
        node.tm.identity()
        node.wtm = Matrix()
        node.wtm.identity()
        node.renderable = False
        self.nodes.append(node)
        return node

    def writeCurves(self, curve):
        objChunkPos = self.writer.beginChunk(DAG_NODE_OBJ)
        splineShunkPos = self.writer.beginChunk(DAG_OBJ_SPLINES)
        self.writer.writeDWord(len(curve.splines))
        for spline in curve.splines:
            closed = spline.use_cyclic_u or spline.use_cyclic_v
            self.writer.writeByte(closed)
            count = len(spline.bezier_points)
            self.writer.writeDWord(count)
            for point in spline.bezier_points:
                self.writer.writeByte(DAG_SKNOT_BEZIER)
                self.writer.writeFloat(point.co[0])
                self.writer.writeFloat(point.co[1])
                self.writer.writeFloat(point.co[2])
                self.writer.writeFloat(point.handle_left[0])
                self.writer.writeFloat(point.handle_left[1])
                self.writer.writeFloat(point.handle_left[2])
                self.writer.writeFloat(point.handle_right[0])
                self.writer.writeFloat(point.handle_right[1])
                self.writer.writeFloat(point.handle_right[2])
        self.writer.endChunk(splineShunkPos)
        self.writer.endChunk(objChunkPos)
        return

    def writeMesh(self, mesh):
        objChunkPos = self.writer.beginChunk(DAG_NODE_OBJ)
        maxUVCount = 0
        for uv in mesh.uv:
            if len(uv) > maxUVCount:
                maxUVCount = len(uv)
        if mesh.color_attributes and len(mesh.color_attributes) > maxUVCount:
            maxUVCount = len(mesh.color_attributes)
        if len(mesh.vertices) >= 0x10000 or len(mesh.faces) >= 0x10000 or maxUVCount >= 0x10000:
            bigMeshChunkPos = self.writer.beginChunk(DAG_OBJ_BIGMESH)
            self.writer.writeDWord(len(mesh.vertices))
            for v in mesh.vertices:
                self.writer.writeVertex(v)

            self.writer.writeDWord(len(mesh.faces))
            for face in mesh.faces:
                self.writer.writeBigFace(face.f, face.smgr, face.mat)

            count = len(mesh.uv)
            if mesh.color_attributes:
                self.writer.writeByte(count + 1)
                self.writer.writeDWord(len(mesh.color_attributes))
                self.writer.writeByte(3)
                self.writer.writeByte(0)
                for c in mesh.color_attributes:
                    self.writer.writeFloat(c[0])
                    self.writer.writeFloat(c[1])
                    self.writer.writeFloat(c[2])
                for cp in mesh.color_attributes_poly:
                    self.writer.writeDWord(cp)
            else:
                self.writer.writeByte(count)
            for n, uvs in enumerate(mesh.uv):
                self.writer.writeDWord(len(uvs))
                self.writer.writeByte(3 if n == 0 else 2)
                self.writer.writeByte(n + 1)

                for uv in uvs:
                    self.writer.writeFloat(uv.x)
                    self.writer.writeFloat(uv.y)
                    if n == 0:
                        self.writer.writeFloat(0.0)
                polys = mesh.uv_poly[n]
                for poly in polys:
                    self.writer.writeDWord(poly)
            self.writer.endChunk(bigMeshChunkPos)
        else:
            meshChunkPos = self.writer.beginChunk(DAG_OBJ_MESH)
            self.writer.writeUShort(len(mesh.vertices))
            for v in mesh.vertices:
                self.writer.writeVertex(v)

            self.writer.writeUShort(len(mesh.faces))
            for face in mesh.faces:
                self.writer.writeFace(face.f, face.smgr, face.mat)

            count = len(mesh.uv)
            if mesh.color_attributes:
                self.writer.writeByte(count + 1)
                self.writer.writeUShort(len(mesh.color_attributes))
                self.writer.writeByte(3)
                self.writer.writeByte(0)
                for c in mesh.color_attributes:
                    self.writer.writeFloat(c[0])
                    self.writer.writeFloat(c[1])
                    self.writer.writeFloat(c[2])
                for cp in mesh.color_attributes_poly:
                    self.writer.writeUShort(cp)
            else:
                self.writer.writeByte(count)
            n = 0
            for uvs in mesh.uv:
                self.writer.writeUShort(len(uvs))
                self.writer.writeByte(3 if n == 0 else 2)
                self.writer.writeByte(n + 1)
                for uv in uvs:
                    self.writer.writeFloat(uv.x)
                    self.writer.writeFloat(uv.y)
                    if n == 0:
                        self.writer.writeFloat(0.0)
                polys = mesh.uv_poly[n]
                for poly in polys:
                    self.writer.writeUShort(poly)
                n += 1
            self.writer.endChunk(meshChunkPos)

        faceFlagChunkPos = self.writer.beginChunk(DAG_OBJ_FACEFLG)
        for f in mesh.faces:
            if f.flg is not None:
                self.writer.writeByte(f.flg)
            else:
                self.writer.writeByte(0)
        self.writer.endChunk(faceFlagChunkPos)

        if len(mesh.normals_ver) > 0:
            normalsChunkPos = self.writer.beginChunk(DAG_OBJ_NORMALS)
            self.writer.writeDWord(len(mesh.normals_ver))
            for n in mesh.normals_ver:
                self.writer.writeFloat(n[0])
                self.writer.writeFloat(n[1])
                self.writer.writeFloat(n[2])
            for f in mesh.normals_faces:
                self.writer.writeDWord(f[0])
                self.writer.writeDWord(f[2])
                self.writer.writeDWord(f[1])
            self.writer.endChunk(normalsChunkPos)

        self.writer.endChunk(objChunkPos)

    def dumpCurve(self, o, node, scene):
        if o.type != 'CURVE':
            return
        node.curves = o.data

    def gatherObjPropScript(self, o, node):
        scripts = ''
        #>Not initialised in properties anymore
        node.objFlg |= DAG_NF_RENDERABLE
        node.objFlg |= DAG_NF_CASTSHADOW
        #<
        DP=o.dagorprops
        broken=''
        for key in DP.keys():
            if key!='broken_properties':
                if key=='renderable:b':
                    node.objFlg -= DAG_NF_RENDERABLE*int(DP[key]=='no')
                elif key=='cast_shadows:b':
                    node.objFlg -= DAG_NF_CASTSHADOW*int(DP[key]=='no')
                try:
                    value=str(DP[key].to_list())
                    value=value.replace('[','')
                    value=value.replace(']','')
                except:
                    value=str(DP[key])
                    if key.endswith(':t'):
                        value=value.replace('"','')#to avoid multiple quotes
                        value='"'+value+'"'
                line=key+'='+value+'\r\n'
                #TODO: replace with more complex validation
                if value.__len__()==0 or (key.endswith(':b') and value not in['yes','no']) or key.find(':')==-1:
                    broken+=line
                else:
                    scripts+=line
            else:
                broken+=DP[key]
        if broken.__len__()>0:
            scripts+='broken_properties='+broken
        return scripts

    def toMesh(self, o, scene):
        if o.type != 'MESH':
            try:
                me = o.to_mesh(scene, True, "PREVIEW")
            except Exception:
                me = None
        else:
            me = o.data
        return me

    def getEdgeKeys(self, o, scene):
        edge_keys = dict()
        me = self.toMesh(o, scene)
        if me is None:
            return edge_keys
        for p in me.polygons:
            for edge_key in p.edge_keys:
                edge_keys[edge_key] = edge_key
        return edge_keys

    def dumpMesh(self, obj, node, scene):
    #creating temporary object to keep originals untouched
        exp_obj=obj.copy()
        exp_obj.data=obj.data.copy()
        exp_obj['og_name']=obj.name
        me = exp_obj.data
    #optional edits
        if self.modifiers:
            apply_modifiers(exp_obj)
        if self.mopt:
            fix_mat_slots(exp_obj)
            optimize_mat_slots(exp_obj)
    #mesh checks
        if not is_mesh_smooth(me):
            log(f'Node "{exp_obj["og_name"]}": flat shaded faces, exported without smooth group\n', type = 'WARNING')
        if is_mesh_empty(me):
            log(f'Node "{exp_obj["og_name"]}": no faces\n', type = 'WARNING')
    #fixing should be optional but now it's hardcoded
        msg = mesh_verify_fix_degenerates(me)
        if msg is not None:
            log(f'Object "{exp_obj["og_name"]}": {msg}\n', type = 'ERROR')
        msg = mesh_verify_fix_loose(me)
        if msg is not None:
            log(f'Object "{exp_obj["og_name"]}": {msg}\n', type = 'ERROR')
    #necessary no matter what
        if get_blender_version()<4.1:
            preserve_sharp(exp_obj)
        fix_mat_slots(exp_obj)
        if exp_obj.data.attributes.get('SG') is None:
            objects_calc_smooth_groups([exp_obj])
    #collecting from pre-triangulated mesh to make it reversable on import
        edge_keys = self.getEdgeKeys(exp_obj, scene)
    #dag contains only triangles
        triangulate(me)

        node.objProps = self.gatherObjPropScript(exp_obj, node)
        node.materials_indices = [-1 for p in range(len(exp_obj.material_slots))]
        for i, slot in enumerate(exp_obj.material_slots):
            if slot.material.name not in self.materials:
                mat_exp = MaterialExp()
                mat_exp.index = len(self.materials)
                mat_exp.mat = slot.material
                self.materials[slot.material.name] = mat_exp

            val = self.materials.get(slot.material.name)
            if val is not None:
                node.materials_indices[i] = val.index
            else:
                node.materials_indices[i] = -1

        meshExp = MeshExp()

        me.calc_loop_triangles()
        if get_blender_version()<4.1:
            me.calc_normals_split()

        for v in me.vertices:
            meshExp.vertices.append(v.co)
            meshExp.normals.append(-v.normal)

        smooth_groups=[]
        for sg in me.attributes['SG'].data:
            smooth_groups.append(int_to_uint(sg.value))

        for tri in me.polygons:
            face = FaceExp()
            face.f.append(tri.vertices[0])
            face.f.append(tri.vertices[2])
            face.f.append(tri.vertices[1])
            face.smgr = smooth_groups[tri.index]
            face.mat = tri.material_index
            for i, tri_edge in enumerate(tri.edge_keys):
                if tri_edge in edge_keys:
                    if i == 0:
                        face.flg =face.flg | DAG_FACEFLG_EDGE2 if face.flg is not None else DAG_FACEFLG_EDGE2
                    elif i == 1:
                        face.flg =face.flg | DAG_FACEFLG_EDGE1 if face.flg is not None else DAG_FACEFLG_EDGE1
                    elif i == 2:
                        face.flg =face.flg | DAG_FACEFLG_EDGE0 if face.flg is not None else DAG_FACEFLG_EDGE0
            meshExp.faces.append(face)
#Custom Normals
        for tri in me.loop_triangles:
            if self.normals and me.has_custom_normals:
                norm = []
                useSmooth = tri.use_smooth
                if useSmooth:
                    for normal in tri.split_normals:
                        norm.append(mathutils.Vector((normal[0], normal[1], normal[2])))
                else:
                    norm.append(tri.normal)
                    norm.append(tri.normal)
                    norm.append(tri.normal)

                norm_face_ind = []
                for nv in norm:
                    norm_face_ind.append(meshExp.addNormal(nv))
                meshExp.normals_faces.append(norm_face_ind)
#UVMaps
        meshExp.uv = [dict() for p in range(len(me.uv_layers))]
        meshExp.uv_poly = [list() for p in range(len(me.uv_layers))]
        uv_amount = me.uv_layers.__len__()
        if uv_amount == 0:
            msg = f'Node "{exp_obj["og_name"]}" has no UVs\n'
            log(msg)
        elif uv_amount > 3:
            msg = f'Node "{exp_obj["og_name"]}" has {uv_amount} UV layers! Are you sure all of them are necessary?\n'
            log(msg, type = 'WARNING')
        uv_index = 0
        UV_names = sort_uv_names(me.uv_layers.keys())
        for layer_name in UV_names:
            layer = me.uv_layers[layer_name]
            if len(layer.data) == 0:
                break#empty layer?
            for tri in me.polygons:
                meshExp.uv_poly[uv_index].append(meshExp.addUV(layer.data[tri.loop_indices[0]].uv, uv_index))
                meshExp.uv_poly[uv_index].append(meshExp.addUV(layer.data[tri.loop_indices[2]].uv, uv_index))
                meshExp.uv_poly[uv_index].append(meshExp.addUV(layer.data[tri.loop_indices[1]].uv, uv_index))
            uv_index += 1
#Vertex Colors
        if me.color_attributes.__len__()>0:
            color_attributes_fix_domain(exp_obj)  # making sure that vcol stored in face corners and not somewhere else
            color_attributes = me.color_attributes[0]
            for tri in me.polygons:
                d = color_attributes.data[tri.loop_indices[0]]
                if d is not None:
                    meshExp.color_attributes_poly.append(meshExp.addVertexColor(d.color_srgb))
                else:
                    meshExp.color_attributes_poly.append(meshExp.addVertexColor([1.0, 1.0, 1.0]))
                d = color_attributes.data[tri.loop_indices[2]]
                if d is not None:
                    meshExp.color_attributes_poly.append(meshExp.addVertexColor(d.color_srgb))
                else:
                    meshExp.color_attributes_poly.append(meshExp.addVertexColor([1.0, 1.0, 1.0]))
                d = color_attributes.data[tri.loop_indices[1]]
                if d is not None:
                    meshExp.color_attributes_poly.append(meshExp.addVertexColor(d.color_srgb))
                else:
                    meshExp.color_attributes_poly.append(meshExp.addVertexColor([1.0, 1.0, 1.0]))
        node.mesh = meshExp

    def generateMatScript(self, mat):
        DM = mat.dagormat
        optional = DM.optional
        dagormat_scripts = []
        if DM.sides == 2:
            dagormat_scripts.append("real_two_sided=yes")
        for param in optional.keys():
            try:
                values = optional[param].to_list()
                value = ''
                for el in values:
                    value+=f'{round(el,7)}, '
                value = value[:-2]
            except:
                try:
                    value = f'{round(optional[param],7)}'
                except:
                    value=f'{optional[param]}'
            dagormat_scripts.append(param+ "=" + value)
        return "\r\n".join(dagormat_scripts)

    def saveMesh(self, node):
        # serialize
        nodeChunkPos = self.writer.beginChunk(DAG_NODE)
        nodeDataChunkPos = self.writer.beginChunk(DAG_NODE_DATA)
        node.objFlg |= DAG_NF_RCVSHADOW
        self.writer.writeDagNodeData(node.id, len(node.children), node.objFlg)
        if node.name.startswith('occluder_box'):
            self.writer.writeStr('occluder_box')
        #removing indecies and types of converted cmp nodes
        elif bpy.data.scenes[0].dag4blend.exporter.cleanup_names:
            name = node.name
            #-index
            if name[-4]=='.' and name[-3:].isnumeric():
                name = name[:-4]
            if name[-6:-2]=='.lod' and name[-2:].isnumeric():
                name = name[:-6]
            #-type
            if name.find(':')>-1:
                name = name[:name.find(':')]
            err = self.writer.writeStr(name)
            if err is not None:
                log(f'{err}\n', type = 'ERROR', show = True)
        else:
            err = self.writer.writeStr(node.name)
            if err is not None:
                log(f'{err}\n', type = 'ERROR', show = True)
        self.writer.endChunk(nodeDataChunkPos)

        if len(node.objProps):
            scriptChunkPos = self.writer.beginChunk(DAG_NODE_SCRIPT)
            self.writer.writeStr(node.objProps)
            self.writer.endChunk(scriptChunkPos)

        materialChunkPos = self.writer.beginChunk(DAG_NODE_MATER)
        for mat_ind in node.materials_indices:
            self.writer.writeUShort(mat_ind)
        self.writer.endChunk(materialChunkPos)

        tmChunkPos = self.writer.beginChunk(DAG_NODE_TM)
        self.writer.writeMatrix4x3(node.tm)
        self.writer.endChunk(tmChunkPos)

        self.writeMesh(node.mesh)

        self.saveChildren(node)
        self.writer.endChunk(nodeChunkPos)

    def saveChildren(self, node):
        if len(node.children):
            childNodeChunkPos = self.writer.beginChunk(DAG_NODE_CHILDREN)
            for node in node.children:
                self.saveNode(node)
            self.writer.endChunk(childNodeChunkPos)

    def saveCurve(self, node):
        nodeChunkPos = self.writer.beginChunk(DAG_NODE)
        nodeDataChunkPos = self.writer.beginChunk(DAG_NODE_DATA)
        flg = 0  # TODO::
        self.writer.writeDagNodeData(node.id, len(node.children), flg)
        self.writer.writeStr(node.name)
        self.writer.endChunk(nodeDataChunkPos)

        tmChunkPos = self.writer.beginChunk(DAG_NODE_TM)
        self.writer.writeMatrix4x3(node.tm)
        self.writer.endChunk(tmChunkPos)

        self.writeCurves(node.curves)

        self.saveChildren(node)
        self.writer.endChunk(nodeChunkPos)

    def saveNode(self, node):
        print("Saving object %s" % node.name)
        if node.mesh:
            self.saveMesh(node)
        elif node.curves:
            self.saveCurve(node)

    def saveNodes(self, parent):
        nodeChunkPos = self.writer.beginChunk(DAG_NODE)
        nodeDataChunkPos = self.writer.beginChunk(DAG_NODE_DATA)
        self.writer.writeDagNodeData(0xFFFF, len(parent.children), 0)
        self.writer.endChunk(nodeDataChunkPos)

        self.saveChildren(parent)
        self.writer.endChunk(nodeChunkPos)

    def saveTextures(self):
        texChunkPos = self.writer.beginChunk(DAG_TEXTURES)
        print(list(self.materials))
        for m in self.materials:
            T = self.materials[m].mat.dagormat.textures
            for i in range(DAGTEXNUM):
                attr_name = "tex" + str(i)
                if hasattr(T, attr_name):
                    texture_path = getattr(T, attr_name)
                    if texture_path and texture_path not in self.textures:
                        self.textures.append(texture_path)
        self.writer.writeUShort(len(self.textures))
        for tex in self.textures:
            if tex.startswith('//'):
                abs_tex = bpy.path.abspath(tex)
                if abs_tex[1]!=':':#still doesn't start with drive letter
                    abs_tex = ensure_no_path(tex)
                self.writer.writeDAGString(abs_tex)
            else:
                self.writer.writeDAGString(tex)
        self.writer.endChunk(texChunkPos)

    def saveMaterials(self):
        for m in self.materials.values():
            mat = m.mat
            DM=mat.dagormat
            matChunkPos = self.writer.beginChunk(DAG_MATER)
            self.writer.writeDAGString(mat.name)
            self.writer.writeColor(DM.ambient)
            self.writer.writeColor(DM.diffuse)
            self.writer.writeColor(DM.specular)
            self.writer.writeColor(DM.emissive)
            self.writer.writeFloat(DM.power)
            # flags
            flags = DAG_MF_16TEX  # should be always used
            if DM.sides == 1:
                flags |= DAG_MF_2SIDED
            self.writer.writeDWord(flags)
            for i in range(DAGTEXNUM):
                if DM.is_proxy:
                        self.writer.writeUShort(DAGBADMATID)  # proxy uses textures from blk, not from dagormat itself
                else:
                    attr_name = "tex" + str(i)
                    if hasattr(mat.dagormat.textures, attr_name):
                        texture_path = getattr(mat.dagormat.textures, attr_name)
                        if not texture_path:
                            self.writer.writeUShort(DAGBADMATID)
                        q = 0
                        for tex in self.textures:
                            if texture_path == tex:
                                self.writer.writeUShort(q)
                                break
                            q += 1
                    else:
                        self.writer.writeUShort(DAGBADMATID)
            # _resv
            self.writer.writeUShort(0)
            self.writer.writeUShort(0)
            self.writer.writeUShort(0)
            self.writer.writeUShort(0)
            self.writer.writeUShort(0)
            self.writer.writeUShort(0)
            self.writer.writeUShort(0)
            self.writer.writeUShort(0)
            # classname
            if mat.dagormat.is_proxy:
                self.writer.writeDAGString(mat.name+':proxymat')
            else:
                self.writer.writeDAGString(mat.dagormat.shader_class)
                self.writer.writeStr(self.generateMatScript(mat))
            self.writer.endChunk(matChunkPos)

    def enumerate(self, whole_list, objs, parent, scene):
        objects = objs
        for o in objects:
            self.last_index += 1

            node = NodeExp()
            node.name = o.name
            node.id = self.last_index
            node.parent_id = parent.id
            if (o.parent is None) or (o.parent.type not in SUPPORTED_TYPES) or (o.parent not in whole_list):
                node.tm = o.matrix_world.copy()
            else:
                node.tm = o.matrix_local.copy()
            ok_matrix = is_matrix_ok(o.matrix_local)
            if ok_matrix is not True:
                msg = f"node {o.name} has incorrect matrix: {ok_matrix}\n"
                log(msg, type = 'ERROR')

            if parent.parent_id == -1:
                tmp_r = node.tm[1].copy()
                node.tm[1] = node.tm[2].copy()
                node.tm[2] = tmp_r.copy()
            node.tm.transpose()

            parent.children.append(node)
            self.nodes.append(node)
            if o.type == 'MESH':
                self.dumpMesh(o, node, scene)
            elif o.type == 'CURVE':
                self.dumpCurve(o, node, scene)
            if node.mesh is None and node.curves is None:
                return -1
            child_list=list(ob for ob in o.children if ob in whole_list)
            self.enumerate(whole_list, child_list, node, scene)
        return 0

    def export_dag(self,whole_list,filepath):
        existing_objects = list(ob  for ob  in bpy.data.objects)
        existing_mats = list(mat for mat in bpy.data.materials)
        filename = ensure_no_path(filepath)
        dirpath_temp = join(bpy.app.tempdir, "dag4blend")
        if not exists(dirpath_temp):
            makedirs(dirpath_temp)
        filepath_temp = join(dirpath_temp, filename)
        filepath = ensure_ext(filepath, self.filename_ext)
        log(f'EXPORTING {filepath}\n')
        objs=list(ob for ob in whole_list if ob.parent is None or ob.parent not in whole_list)
        if objs.__len__() == 0:
            err = 'No objects to export in ' + ensure_no_path(filepath)
            show_popup(message=err,icon='ERROR',title='Error!')
            log(f'{err}\n', type = 'ERROR', show = True)
            return{'CANCELLED'}
        S = bpy.context.scene
        print("Blender Version {}.{}.{}".format(bpy.app.version[0], bpy.app.version[1], bpy.app.version[2]))
        print("Filepath: {}".format(filepath))
        print("Writing DAG file...")
        self.writer.open(filepath_temp)
        self.writer.writeHeader(DAG_ID)
        node = self.createRootNode()
        dagChunkPos = self.writer.beginChunk(DAG_ID)

        if self.enumerate(whole_list,objs, node, S) == -1:
            log("Failed to enumerate objects\n", type = 'ERROR', show = True)
            self.writer.close()
            return {'CANCELLED'}

        self.saveTextures()
        self.saveMaterials()
        self.saveNodes(node)
        self.writer.endChunk(dagChunkPos)
        dagEndChunkPos = self.writer.beginChunk(DAG_END)
        self.writer.endChunk(dagEndChunkPos)
        self.writer.close()
        dirpath = dirname(filepath)
        if not exists(dirpath):
            makedirs(dirpath)
        copyfile(filepath_temp,filepath)
        #no need to delete temporary dag, it will be auto-removed with bpy.app.tempdir on blender being closed/crushed
        #cleanup part. Let's remove temporary stuff
        filename = ensure_no_path(filepath)
        occluders = 0
        for obj in bpy.data.objects:
            if obj not in existing_objects:
                if obj.name.startswith('occluder_box'):
                    occluders+=1
                bpy.data.meshes.remove(obj.data)
        if occluders>2:# one copy for export preparation, one for triangulation data
            log(f'{filename}: {occluders/2} occluders in one dag\n', type = 'WARNING', show = True)
        for mat in bpy.data.materials:
            if mat not in existing_mats:
                bpy.data.materials.remove(mat)
        print(filename+' exported')
        self.materials = {}#we don't need this for next dag.
        self.textures = []
        log(f'{filename}: export FINISHED\n')

    def export_collection(self,col,dirpath):
        #object outside of collections without subcollections
        orphans=self.orphans
        if col.children.__len__()>0:
            if orphans:
                for obj in col.objects:
                    if obj.type in SUPPORTED_TYPES:
                        filepath=join(dirpath,obj.name+'.dag')
                        self.export_dag([obj],filepath)
            for child in col.children:
                self.export_collection(child,dirpath)
        #collection(s) without children, but with objects
        elif col.objects.__len__()>0:
            if "name" in col.keys():
                col["name"] = col["name"].replace('\\','/')
                #separator in the beginning makes blender save file one directory above export path, we don't need that
                while col["name"].startswith('/'):
                    col["name"] = col["name"][1:]
                subpath = col["name"].replace('*',col.name)
                if subpath[1]==':':#drive
                    filepath = subpath
                else:
                    filepath = join(dirpath,subpath)
            else:
                filepath=join(dirpath,col.name)
            whole_list=list(ob for ob in col.objects if ob.type in SUPPORTED_TYPES)
            filepath = bpy.path.ensure_ext(filepath,'.dag')
            self.export_dag(whole_list,filepath)

    def execute(self, context):
        mode = self.limits
        filepath = self.filepath
        dirpath=dirname(filepath)
        P = bpy.data.scenes[0].dag4blend.exporter
        try:
            bpy.ops.object.mode_set(mode='OBJECT')
        except Exception:
            pass
        VL=context.view_layer
        if   mode=='Visible':
            whole_list = list(ob for ob in VL.objects if ob.visible_get() and ob.type in SUPPORTED_TYPES)
            self.export_dag(whole_list,filepath)
        elif mode=='Sel.Joined':
            whole_list = list(ob for ob in VL.objects if ob.select_get() and ob.type in SUPPORTED_TYPES)
            self.export_dag(whole_list,filepath)
        elif mode=='Sel.Separated':
            for obj in VL.objects:
                if obj.select_get() and obj.type in SUPPORTED_TYPES:
                    filepath=join(dirpath,obj.name)
                    self.export_dag([obj],filepath)
        elif mode=='Col.Separated':
            dag_col=P.collection
            if dag_col==None:
                if context.scene.collection.children.__len__()==0:
                    show_popup(message="No sub-collections found. 'Scene Collection.dag' not exported")
                else:
                    for child in context.scene.collection.children:
                        self.export_collection(child,dirpath)
            else:
                self.export_collection(dag_col,dirpath)
        elif mode =='Col.Joined':
            dag_col=P.collection
            filepath=join(self.filepath,dag_col.name)
            if dag_col==None:
                show_popup(message="Doesn't work for Scenre Collection",icon='ERROR')
                return{'CANCELLED'}
            whole_list=list(ob for ob in dag_col.objects if ob.type in SUPPORTED_TYPES)
            for col in dag_col.children_recursive:
                for obj in col.objects:
                    if obj.type in SUPPORTED_TYPES and obj not in whole_list:
                        whole_list.append(obj)
            self.export_dag(whole_list,filepath)
        return {'FINISHED'}

def menu_func_import(self, context):
    self.layout.operator(DagExporter.bl_idname, text='Dagor Engine (.dag)')

def register():
    bpy.utils.register_class(DagExporter)
    bpy.utils.register_class(DAG_PT_export)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_import)

def unregister():
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_import)
    bpy.utils.unregister_class(DAG_PT_export)
    bpy.utils.unregister_class(DagExporter)