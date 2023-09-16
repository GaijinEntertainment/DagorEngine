import bpy
import os
import re
import glob
from math                           import pi
from mathutils                      import Vector
from bpy.types                      import Operator
from bpy_extras.io_utils            import ImportHelper
from .                              import reader
from ..constants                    import *
from ..dagMath                      import mul
from ..node                         import NodeExp
from ..nodeMaterial                 import MaterialExp
from ..face                         import FaceExp
from ..mesh                         import MeshExp
from ..material                     import Material
from ..dagormat.build_node_tree     import buildMaterial
from ..dagormat.compare_dagormats   import compare_dagormats
from ..object_properties            import object_properties
from ..helpers.props                import fix_type
from ..helpers.texts                import get_text
from ..helpers.basename             import basename
from ..smooth_groups.smooth_groups  import uint_to_int,int_to_uint,sg_to_sharp_edges
from ..tools.tools_panel            import fix_mat_slots, optimize_mat_slots

import sys
sys.path.append('../../')
from pythonCommon                   import datablock
from pythonCommon.pyparsing         import ParseException

FACENO = 0
INDEX = 1
BEENTHERE = 2


#FUNCTIONS

#returns possible asset type based on name
def dag_type(name):
    pref = bpy.context.preferences.addons[basename(__package__)].preferences
    if not pref.guess_dag_type:
        return None
    if name.endswith('_destr.lod00'):
        return 'dynmodel'
    if name[:-2].endswith('.lod'):
        return 'rendinst'
    return 'prefab'

#returns collection to store imported asset
def get_dag_col(name,refresh_existing):
    type = dag_type(name)
    colls = bpy.data.collections
    if refresh_existing:
        for col in colls:
    #collection defined by custom parameter
            if "name" in col.keys():
                if os.path.basename(col["name"]) == name:
                    return col
    #col.name can't be longer than 63 symbols
            elif (name.__len__()<=63 and col.name == name) or col.name == name[:63]:
                if "type" in col.keys():
                    if col["type"] == type:
                        return col
                    elif type == None and col["type"] in ["dynmodel","rendinst","prefab"]:
                        return col
                else:
                    return col
    #if collection wasn't found, we should create new one:
    col = bpy.data.collections.new(name="")
    col.name = name
    if name.__len__()>63:#one or more sybols lost, orig name should be preserved
        col["name"] = name
    col["type"] = type
    #collection should be linked to scene
    if type == "prefab":
        col_parent = colls.get(f'{name}_ALL')
    else:
        col_parent = colls.get(f'{os.path.splitext(name)[0]}')
        if col_parent == col:
            col_parent = None
    if col_parent is None:
        bpy.context.scene.collection.children.link(col)
    else:
        col_parent.children.link(col)
    return col

#searches for triangles of same polygons
def buildNeighbors(faces):
    global neighbors
    neighbors = {}
    for i, f in enumerate(faces):
        neighbors[tuple(f.f[0:2])] = [i, 0, 0]
        neighbors[tuple(f.f[1:3])] = [i, 1, 0]
        neighbors[tuple([f.f[2], f.f[0]])] = [i, 2, 0]

#CLASSES
class Curve:
    closed: bool
    points = []
    handle_left = []
    handle_right = []

neighbors = {}

class DagImporter(Operator, ImportHelper):
    bl_idname = "import_scene.dag"
    bl_label = "Import DAG"
    bl_options = {'PRESET','REGISTER', 'UNDO'}

    filename_ext = ".dag"
    filter_glob: bpy.props.StringProperty(default="*.dag", options={'HIDDEN'})

    import_mopt: bpy.props.BoolProperty(
            name="Optimize material slots",
            description="Remove unused materials",
            default=True)

    import_lods: bpy.props.BoolProperty(
            name="Import LODs",
            description="Import all LODs",
            default=False)

    import_dps: bpy.props.BoolProperty(
        name="Import dps",
        description="Import all dps",
        default=False)

    import_dmgs: bpy.props.BoolProperty(
        name="Import dmgs",
        description="Import all dmgs",
        default=False)

    import_refresh: bpy.props.BoolProperty(
        name = 'Replace existing',
        description = 'Replace already existing in blend instead of duplicating',
        default = False)

    import_preserve_path: bpy.props.BoolProperty(
        name = 'Preserve path',
        description = 'Override export path for imported files',
        default = False)

    rootNode = None
    reader = reader.DagReader()
    end = False

    textures = []
    materials = []

    @staticmethod
    def findNeighbors(faces):
        for i, f in enumerate(faces):
            if f.flg is None:#no face flags - no neighbours
                continue
            if f.flg & DAG_FACEFLG_EDGE2 == 0:
                edge = [f.f[2], f.f[0]]
                res = neighbors.get(tuple(edge[::-1]))
                if res:
                    f.neighbors[2] = res
                    faces[f.neighbors[2][FACENO]].neighbors[f.neighbors[2][INDEX]] = [i, 2, 0]
            if f.flg & DAG_FACEFLG_EDGE1 == 0:
                edge = f.f[1:3]
                res = neighbors.get(tuple(edge[::-1]))
                if res:
                    f.neighbors[1] = res
                    faces[f.neighbors[1][FACENO]].neighbors[f.neighbors[1][INDEX]] = [i, 1, 0]
            if f.flg & DAG_FACEFLG_EDGE0 == 0:
                edge = f.f[0:2]
                res = neighbors.get(tuple(edge[::-1]))
                if res:
                    f.neighbors[0] = res
                    faces[f.neighbors[0][FACENO]].neighbors[f.neighbors[0][INDEX]] = [i, 0, 0]

    def buildPoly(self, faceIndex, faces, idx, res, indices, faceDict, vertices):
        log=get_text('log')
        f = faces[faceIndex]
        for i in [idx, (idx + 1) % 3, (idx + 2) % 3]:
            if f.neighbors[i][BEENTHERE] > 0:
                continue
            f.neighbors[i][BEENTHERE] = 1
            if f.neighbors[i][FACENO] == -1:
                # Workaround for Blender not correctly handling holes in polygons with one split with the same vertices
                if faceDict.get(f.f[i]) is not None:
                    f.neighbors[i][BEENTHERE] = 2
                else:
                    faceDict[f.f[i]] = f.f[i]
                    res.append(f.f[i])
                    indices.append(faceIndex * 3 + i)
            else:
                faces[f.neighbors[i][FACENO]].neighbors[f.neighbors[i][INDEX]][BEENTHERE] = 1
                self.buildPoly(f.neighbors[i][FACENO], faces, f.neighbors[i][INDEX], res, indices, faceDict, vertices)

    def buildExtraPoly(self, faceIndex, faces, idx):
        log=get_text('log')
        f = faces[faceIndex]
        for i in [idx, (idx + 1) % 3, (idx + 2) % 3]:
            if f.neighbors[i][BEENTHERE] == 2:
                f.neighbors[i][BEENTHERE]=1
                return True
        return False


    def buildMesh(self, node, tm, parent):
        log=get_text('log')
        if node.mesh is None:
            return
        loops_vert_idx = list()
        faces_loop_start = []
        faces_loop_total = []
        indices = []
        material_indices = []
        buildNeighbors(node.mesh.faces)
        self.findNeighbors(node.mesh.faces)
        i=0
        for f in node.mesh.faces:
            i+=1
        SG = []
        for i in range(len(node.mesh.faces)):
            faceDict = {}
            start = len(loops_vert_idx)
            self.buildPoly(i, node.mesh.faces, 0, loops_vert_idx, indices, faceDict, node.mesh.vertices)
            end = len(loops_vert_idx)
            diff = end - start
            if diff > 0:
                faces_loop_start.append(start)
                faces_loop_total.append(diff)
                material_indices.append(node.mesh.faces[i].mat)
                sm = node.mesh.faces[i].smgr
                SG.append(sm)

            if self.buildExtraPoly(i, node.mesh.faces, 0):
                for v in range(3):
                    indices.append(i * 3 + v)
                    loops_vert_idx.append(node.mesh.faces[i].f[v])
                faces_loop_start.append(end)
                faces_loop_total.append(3)
                material_indices.append(node.mesh.faces[i].mat)
                sm = node.mesh.faces[i].smgr
                SG.append(sm)

        me = bpy.data.meshes.new(name=node.name)

        for index in node.subMat:
            me.materials.append(self.materials[index].mat)

        me.loops.add(len(loops_vert_idx))
        me.polygons.add(len(faces_loop_total))
        me.polygons.foreach_set('material_index', material_indices)

        me.vertices.add(len(node.mesh.vertices))
        me.vertices.foreach_set("co", [a for v in node.mesh.vertices for a in (v[0], v[1], v[2])])
        me.loops.foreach_set("vertex_index", loops_vert_idx)

        me.polygons.foreach_set("loop_start", faces_loop_start)
        me.polygons.foreach_set("loop_total", faces_loop_total)
        if len(node.mesh.uv):
            for i, uvs in enumerate(node.mesh.uv):
                uvMap = []
                if len(node.mesh.uv_poly[i]) == 0:
                    continue
                for index in indices:
                    uvMap.append(uvs[node.mesh.uv_poly[i][index] * 2])
                    uvMap.append(uvs[node.mesh.uv_poly[i][index] * 2 + 1])
                me.uv_layers.new(do_init=False)
                me.uv_layers[len(me.uv_layers) - 1].data.foreach_set("uv", tuple(uvMap))

        if len(node.mesh.vertex_colors):
            poly = []
            for index in indices:
                poly.append(node.mesh.vertex_colors_poly[index])
            vcol_lay = me.vertex_colors.new()
            col = []
            for index in poly:
                col.extend(node.mesh.vertex_colors[index])
                col.append(1.0)
            vcol_lay.data.foreach_set("color", tuple(col))

        me.validate()
        me.update()

        me.polygon_layers_int.new(name='SG')
        i=0
        for sg in me.polygon_layers_int['SG'].data:
            sg.value=uint_to_int(SG[i])
            i+=1
        sg_to_sharp_edges(me)

        if len(node.mesh.normals_ver_list):
            normals = []
            for f in me.polygons:
                f.use_smooth = True
            for i in indices:
                normals.append(node.mesh.normals_ver_list[node.mesh.normals_faces[i]])
            loops_nor = tuple(normals)
            try:
                me.normals_split_custom_set(loops_nor)
            except RuntimeError as err:
                print(f"{err=}")
                log.write(f"{err=}\n")
        else:
            me.calc_normals()
        me.use_auto_smooth = True
        me.auto_smooth_angle = pi
        o = bpy.data.objects.new(name=node.name, object_data=me)
        self.collection.objects.link(o)
        if parent is not None:
            o.parent = parent
        o.matrix_world = tm

        self.buildObjProperties(o, node.objProps)
        if o.dagorprops.get('broken_properties') is not None:
            log.write(f'WARNING! {o.name}: incorrect object properties\n')
        tm.transpose()

        #> fixing apex if material was renamed on import
        apex_mat=o.dagorprops.get('apex_interior_material:t')
        if apex_mat is not None and o.data.materials.get(apex_mat) is None:
            for m in self.materials:
                found=False
                if m.name==apex_mat:
                    found=True
                    log.write(f'{o.name}: apex_interior_material was renamed. ')
                    log.write(f'"{apex_mat}" merged with "{m.mat.name}"\n')
                    o.dagorprops['apex_interior_material:t']=m.mat.name
                    break
                elif m.shader==apex_mat+':proxymat':
                    o.dagorprops['apex_interior_material:t']=m.mat.name
                    log.write(f'{o.name}: apex_interior_material was renamed. ')
                    log.write(f'"{apex_mat}" renamed into "{m.mat.name}" to match proxymat.blk name\n')
                    found=True
                    break
            if not found:
                log.write(f'WARNING! {o.name} materials does not contain "apex_interior_material:t"="{apex_mat}"\n')
        #fixin mat_ids:
        was_broken=fix_mat_slots(o)
        if was_broken:
            log.write(f'WARNING! {o.name} had incorrect material indices, fixed\n')
        if self.import_mopt:
            before=o.material_slots.__len__()
            optimize_mat_slots(o)
            diff = before - o.material_slots.__len__()
            if diff>0:
                log.write(f'{o.name}: removed {diff} unnecessary material slot(s)\n')
        #<
        for node in node.children:
            tm2 = mul(node.tm, tm)
            tm2.transpose()
            self.buildMesh(node, tm2, o)
            self.buildCurves(node, tm2, o)

    def buildObjProperties(self, o, objProps):
        blk = datablock.DataBlock()
        try:
            blk.parseString(objProps)
        except ParseException as err:
            print(f"{err=}: " + objProps)
            return
        dagorprops = o.dagorprops
        lines=objProps.split('\r\n')
        broken=''
        for line in lines:
            if line!='':
                prop=line.split('=')
                if prop.__len__()!=2:
                    broken+=line+';'
                elif prop[0].__len__()==0 or prop[1].__len__()==0 or prop[0].find(':')==-1:
                    broken+=line+';'
                else:
                    dagorprops[prop[0]]=fix_type(prop[1])
        if broken.__len__()>0:
            dagorprops['broken_properties']=broken

    def buildMaterials(self):
        for m in self.materials:
            m.mat = bpy.data.materials.new(name=m.name)
#proxymat
            if m.shader.endswith(':proxymat'):
                m.mat.name=m.shader.replace(':proxymat','')
                m.mat.dagormat.is_proxy=True
#both
            m.mat.dagormat.ambient   = m.ambient
            m.mat.dagormat.diffuse   = m.diffuse
            m.mat.dagormat.specular  = m.specular
            m.mat.dagormat.emissive  = m.emissive
            m.mat.dagormat.power     = m.specular_power
            m.mat.dagormat.shader_class=m.shader
#non-proxymat
            if not m.mat.dagormat.is_proxy:
                if (m.flags & DAG_MF_2SIDED) == DAG_MF_2SIDED:
                    m.mat.dagormat.sides = '1'
                for i in range(DAGTEXNUM):
                    tex='tex'+str(i)
                    if m.tex[i] != DAGBADMATID:
                        attr_name='tex'+str(i)
                        if hasattr(m.mat.dagormat.textures, attr_name):
                            setattr(m.mat.dagormat.textures, tex, self.textures[m.tex[i]])
                blk = datablock.DataBlock()
                try:
                    m.script = m.script.replace("(", "")
                    m.script = m.script.replace(")", "")
                    blk.parseString(m.script)
                    for param in blk.getParams():
                        if param[0] == "real_two_sided":
                            if param[2]=='yes' or param[2]==1:
                                m.mat.dagormat.sides = '2'
                            continue
                        else:
                            m.mat.dagormat.optional[param[0]] = param[2]
                            try:
                                if param[0] in ['atest',
                                                'micro_detail_layer',
                                                'painting_line',
                                                'use_painting',
                                                'palette_index']:
                                    m.mat.dagormat.optional[param[0]] = int(param[2])
                            except:
                                pass
                except ParseException as err:
                    print(f"{err=}: " + m.script)
            for mat in bpy.data.materials:
                is_unic=True
                if mat!=m.mat:
                    if compare_dagormats(m.mat,mat):
                        just_imported=m.mat
                        m.mat=mat
                        if m.mat.name.startswith(just_imported.name):
                            m.mat.name=just_imported.name
                        bpy.data.materials.remove(just_imported)
                        is_unic=False
                        break
            if is_unic:
                buildMaterial(m.mat)

    def buildCurves(self, node, tm, parent):
        if node.curves is None:
            return
        curve = bpy.data.curves.new(name=node.name, type='CURVE')
        curve.splines.clear()
        for c in node.curves:
            polyline = curve.splines.new(type='BEZIER')
            polyline.bezier_points.add(len(c.points) - 1)
            for num in range(len(c.points)):
                polyline.bezier_points[num].co = Vector(c.points[num])
                polyline.bezier_points[num].handle_left = Vector(c.handle_left[num])
                polyline.bezier_points[num].handle_right = Vector(c.handle_right[num])
            if c.closed:
                polyline.use_cyclic_u = True
                polyline.use_cyclic_v = True
            else:
                polyline.use_cyclic_u = False
                polyline.use_cyclic_v = False
        o = bpy.data.objects.new(name=node.name, object_data=curve)
        self.collection.objects.link(o)
        if parent is not None:
            o.parent = parent
        o.matrix_world = tm
        tm.transpose()
        for node in node.children:
            tm2 = mul(node.tm, tm)
            tm2.transpose()
            self.buildMesh(node, tm2, o)
            self.buildCurves(node, tm2, o)

    def loadBones(self):
        log=get_text('log')
        count = self.reader.readDWord()
        while count:
            count-=1
            temp_tm = self.reader.readMatrix4x3()
            tm= temp_tm.transpose()
            log.write(f'{tm}\n\n')
        return

    def loadSplines(self, curves):
        count = self.reader.readDWord()
        while count:
            curve = Curve()
            curves.append(curve)
            curve.closed = bool(self.reader.readByte())
            pointCount = self.reader.readDWord()
            curve.points = []
            curve.handle_left = []
            curve.handle_right = []
            while pointCount:
                self.reader.readByte()  # DAG_SKNOT_BEZIER
                curve.points.append(self.reader.readVertex())
                curve.handle_left.append(self.reader.readVertex())
                curve.handle_right.append(self.reader.readVertex())
                pointCount -= 1
            count -= 1

    def loadMesh(self, mesh):
        log=get_text('log')
        count = self.reader.readUShort()
        while count:
            mesh.vertices.append(self.reader.readVertex())
            count -= 1
        faceNumber = count = self.reader.readUShort()
        while count:
            face = self.reader.readFace()
            tmp = face.f[2]
            face.f[2] = face.f[1]
            face.f[1] = tmp
            mesh.faces.append(face)
            count -= 1
        chn = self.reader.readByte()
        while chn:
            tn = self.reader.readUShort()
            co = self.reader.readByte()
            chid = self.reader.readByte()
            ch = chid - 1
            if ch == -1:
                count = tn
                mesh.vertex_colors = list()
                while count:
                    if co == 1:
                        mesh.vertex_colors.append([self.reader.readFloat(), 0, 0])
                    elif co == 2:
                        mesh.vertex_colors.append([self.reader.readFloat(), self.reader.readFloat(), 0])
                    else:
                        mesh.vertex_colors.append(self.reader.readVector())
                    count -= 1
                count = faceNumber
                while count:
                    mesh.vertex_colors_poly.append(self.reader.readUShort())
                    poly1 = self.reader.readUShort()
                    poly2 = self.reader.readUShort()
                    mesh.vertex_colors_poly.append(poly2)
                    mesh.vertex_colors_poly.append(poly1)
                    count -= 1
            elif ch >= 0 and ch < NUMMESHTCH:
                count = tn
                mesh.uv.extend([list() for _ in range(len(mesh.uv), ch + 1)])
                while count:
                    if co == 1:
                        mesh.uv[ch].append(self.reader.readFloat())
                        mesh.uv[ch].append(0)
                    else:
                        mesh.uv[ch].append(self.reader.readFloat())
                        mesh.uv[ch].append(self.reader.readFloat())
                        if co == 3:
                            self.reader.readFloat()
                    count -= 1
                count = faceNumber
                while len(mesh.uv_poly) <= ch:
                    mesh.uv_poly.append(list())
                while count:
                    mesh.uv_poly[ch].append(self.reader.readUShort())
                    poly1 = self.reader.readUShort()
                    poly2 = self.reader.readUShort()
                    mesh.uv_poly[ch].append(poly2)
                    mesh.uv_poly[ch].append(poly1)
                    count -= 1
            else:
                self.reader.endChunk(co * 4 * tn)
                self.reader.endChunk(faceNumber * 2 * 3)
            chn -= 1

    def loadBigMesh(self, mesh):
        count = self.reader.readDWord()
        while count:
            mesh.vertices.append(self.reader.readVertex())
            count -= 1
        faceNumber = count = self.reader.readDWord()
        while count:
            face = self.reader.readBigFace()
            tmp = face.f[2]
            face.f[2] = face.f[1]
            face.f[1] = tmp
            mesh.faces.append(face)
            count -= 1
        chn = self.reader.readByte()
        while chn:
            tn = self.reader.readDWord()
            co = self.reader.readByte()
            chid = self.reader.readByte()
            ch = chid - 1
            if ch == -1:
                count = tn
                mesh.vertex_colors = list()
                while count:
                    if co == 1:
                        mesh.vertex_colors.append([self.reader.readFloat(), 0, 0])
                    elif co == 2:
                        mesh.vertex_colors.append([self.reader.readFloat(), self.reader.readFloat(), 0])
                    else:
                        mesh.vertex_colors.append(self.reader.readVector())
                    count -= 1
                count = faceNumber
                while count:
                    mesh.vertex_colors_poly.append(self.reader.readDWord())
                    poly1 = self.reader.readDWord()
                    poly2 = self.reader.readDWord()
                    mesh.vertex_colors_poly.append(poly2)
                    mesh.vertex_colors_poly.append(poly1)
                    count -= 1
            elif ch >= 0 and ch < NUMMESHTCH:
                count = tn
                mesh.uv.extend([list() for _ in range(len(mesh.uv), ch + 1)])
                while count:
                    if co == 1:
                        mesh.uv[ch].append(self.reader.readFloat())
                        mesh.uv[ch].append(0)
                    if co == 2:
                        mesh.uv[ch].append(self.reader.readFloat())
                        mesh.uv[ch].append(self.reader.readFloat())
                    else:
                        mesh.uv[ch].append(self.reader.readFloat())
                        mesh.uv[ch].append(self.reader.readFloat())
                        if co == 3:
                            self.reader.readFloat()
                    count -= 1
                count = faceNumber
                while len(mesh.uv_poly) <= ch:
                    mesh.uv_poly.append(list())
                while count:
                    mesh.uv_poly[ch].append(self.reader.readDWord())
                    poly1 = self.reader.readDWord()
                    poly2 = self.reader.readDWord()
                    mesh.uv_poly[ch].append(poly2)
                    mesh.uv_poly[ch].append(poly1)
                    count -= 1
            else:
                self.reader.endChunk(co * 4 * tn)
                self.reader.endChunk(faceNumber * 4 * 3)
            chn -= 1

    def loadObj(self, node, size):
        log=get_text('log')
        endpos = self.reader.file.tell() + size
        while self.reader.file.tell() < endpos:
            chunk = self.reader.beginChunk()
            if chunk.id == DAG_OBJ_MESH:
                mesh = node.mesh = MeshExp()
                self.loadMesh(mesh)
            elif chunk.id == DAG_OBJ_BIGMESH:
                mesh = node.mesh = MeshExp()
                self.loadBigMesh(mesh)
            #elif chunk.id == DAG_OBJ_BONES:
                #self.loadBones()
                #pass
                #id = self.reader.readUShort()
                #bone_tm=self.reader.readMatrix4x3()
                #log.write(f'bone_{id}={bone_tm}\n')
            elif chunk.id == DAG_OBJ_SPLINES:
                node.curves = []
                self.loadSplines(node.curves)
            elif chunk.id == DAG_OBJ_FACEFLG:
                for f in mesh.faces:
                    f.flg = self.reader.readByte()
                    faceFlag0 = f.flg & DAG_FACEFLG_EDGE2
                    faceFlag1 = f.flg & DAG_FACEFLG_EDGE1
                    faceFlag2 = f.flg & DAG_FACEFLG_EDGE0
                    f.flg = (faceFlag0 >> 2) | (faceFlag1) | (faceFlag2 << 2)
            elif chunk.id == DAG_OBJ_NORMALS:
                count = self.reader.readDWord()
                while count:
                    count -= 1
                    v = self.reader.readVector()

                    mesh.normals_ver_list.append(v)
                count = len(mesh.faces)
                while count:
                    count -= 1
                    indices = self.reader.readIndices()
                    mesh.normals_faces.append(indices[0])
                    mesh.normals_faces.append(indices[2])
                    mesh.normals_faces.append(indices[1])
            else:
                self.reader.endChunk(chunk.size)

    def importNodes(self, size):
        log=get_text('log')
        tm=objProps=None
        subMat=[]
        node = None
        pos = self.reader.file.tell()
        chunk = self.reader.beginChunk()
        while True:
            if chunk.id == DAG_END:
                self.end = True
                break
            elif chunk.id == DAG_NODE_CHILDREN:
                endpos = self.reader.file.tell() + chunk.size
                chunk = self.reader.beginChunk()
                while True:
                    if chunk.id == DAG_NODE:
                        newNode = self.importNodes(chunk.size)
                        node.children.append(newNode)
                    if self.reader.file.tell() >= endpos:
                        break
                    chunk = self.reader.beginChunk()
                    if chunk is None:
                        self.end = True
                        break
            elif chunk.id == DAG_NODE_DATA:
                node = self.reader.readDagNodeData(chunk.size)
                if tm is not None:
                    node.tm=tm
                    tm=None
                if objProps is not None:
                    node.objProps=objProps
                    objProps=None
                if subMat!=[]:
                    node.subMat=subMat
                    subMat=[]
            elif chunk.id == DAG_NODE_MATER:
                len = int(chunk.size / 2)
                while len:
                    if node is None:
                        subMat.append(self.reader.readUShort())
                    else:
                        node.subMat.append(self.reader.readUShort())
                    len -= 1
            elif chunk.id == DAG_NODE_TM:
                if node is None or node.tm!=None:
                    tm = self.reader.readMatrix4x3()
                else:
                    node.tm = self.reader.readMatrix4x3()
            elif chunk.id == DAG_NODE_SCRIPT:
                if node is None:
                    objProps = self.reader.readStr(chunk.size)
                else:
                    node.objProps = self.reader.readStr(chunk.size)
            elif chunk.id == DAG_NODE_OBJ:
                self.loadObj(node, chunk.size)
            else:
                self.reader.endChunk(chunk.size)
            if self.reader.file.tell() >= pos + size:
                break
            chunk = self.reader.beginChunk()
        if chunk is None:
            self.end = True
        return node

    def importTextures(self):
        count = self.reader.readUShort()
        while count:
            texturePath = self.reader.readDAGString()
            self.textures.append(texturePath)
            count -= 1

    def importMaterial(self, size):
        mat = Material()
        pos = self.reader.file.tell()
        mat.name = self.reader.readDAGString()
        mat.ambient = self.reader.readColor()
        mat.diffuse = self.reader.readColor()
        mat.specular = self.reader.readColor()
        mat.emissive = self.reader.readColor()
        mat.specular_power = self.reader.readFloat()
        # flags
        mat.flags = self.reader.readDWord()
        mat.tex = []
        for i in range(DAGTEXNUM):
            mat.tex.append(self.reader.readUShort())
        # _resv
        self.reader.readUShort()
        self.reader.readUShort()
        self.reader.readUShort()
        self.reader.readUShort()
        self.reader.readUShort()
        self.reader.readUShort()
        self.reader.readUShort()
        self.reader.readUShort()
        # classname
        mat.shader = self.reader.readDAGString()
        mat.script = self.reader.readStr(size - (self.reader.file.tell() - pos))
        self.materials.append(mat)

    def load(self, filepath):
        print("\nReading DAG file:\n{}".format(filepath))

        self.textures = []
        self.materials = []
        self.end = False
        self.reader.open(filepath)

        header = self.reader.readDWord()
        if header != DAG_ID:
            print("Not a DAG file")
            return

        chunk = self.reader.beginChunk()
        if chunk.id != DAG_ID:
            print("Wrong DAG file")
            return
        chunk = self.reader.beginChunk()
        while chunk is not None:
            if self.end or chunk.id == DAG_END:
                break
            elif chunk.id == DAG_NODE:
                self.rootNode = self.importNodes(chunk.size)
            elif chunk.id == DAG_TEXTURES:
                self.importTextures()
            elif chunk.id == DAG_MATER:
                self.importMaterial(chunk.size)
            else:
                self.reader.endChunk(chunk.size)
            chunk = self.reader.beginChunk()
        self.reader.close()
        self.buildMaterials()
#searching for existing collection
        col_name = os.path.basename(filepath).replace('.dag','')
        self.collection = get_dag_col(col_name,self.import_refresh)
        if self.import_preserve_path:
            self.collection["name"] = filepath.replace('.dag','')

        for node in self.rootNode.children:
            node.tm.transpose()
            tmp_r = node.tm[1].copy()
            node.tm[1] = node.tm[2].copy()
            node.tm[2] = tmp_r.copy()
            self.buildMesh(node, node.tm, None)
            self.buildCurves(node, node.tm, None)

    def loadVariations(self, filepath):
        res = re.findall("_dmg", filepath)
        dmgs = ("", "_dmg") if self.import_dmgs else (res[0] if len(res) else "",)
        for dmg in dmgs:
            filepath2 = os.path.basename(filepath)
            if self.import_lods is False and self.import_dps is False:
                if re.search("[.]lod[0-9][0-9]", filepath2) is not None:
                    filepath2 = re.sub("_dmg[.]lod", ".lod", filepath2)
                    filepath2 = re.sub("[.]lod", dmg + ".lod", filepath2)
                else:
                    filepath2 = re.sub("_dmg[.]dag$", ".dag", filepath2)
                    filepath2 = re.sub("[.]dag$", dmg + ".dag", filepath2)
                paths = glob.glob(os.path.join(os.path.dirname(filepath), filepath2))
                for path in paths:
                    self.load(path)
                continue
            filepath2 = filepath2.replace(".dag", "")
            lod = ""
            dp = ""
            if self.import_lods:
                lod = ".lod??"
            else:
                res = re.findall("[.]lod[0-9][0-9]", filepath2)
                lod = res[0] if len(res) else ""
            filepath2 = re.sub("_dmg[.]lod[0-9][0-9]", "", filepath2)
            filepath2 = re.sub("[.]lod[0-9][0-9]", "", filepath2)

            if self.import_dps:
                dp = "_dp_??"
            else:
                res = re.findall("_dp_[0-9][0-9]", filepath2)
                dp = res[0] if len(res) else ""

            filepath2 = re.sub("_dp_[0-9][0-9]_dmg", "", filepath2)
            filepath2 = re.sub("_dp_[0-9][0-9]", "", filepath2)

            if self.import_lods or lod != "":
                for path in glob.glob(os.path.join(os.path.dirname(filepath), filepath2) + dmg + lod + ".dag"):
                    self.load(path)
            if self.import_dps or dp != "":
                for path in glob.glob(os.path.join(os.path.dirname(filepath), filepath2) + dp + dmg + lod + ".dag"):
                    self.load(path)

    def execute(self, context):
        log=get_text('log')
        try:
            bpy.ops.object.mode_set(mode='OBJECT')
        except Exception:
            pass

        filepath = '{:s}'.format(self.filepath)
        log.write(f'IMPORTING {filepath}\n')
        if self.import_dmgs is False and self.import_dps is False and self.import_lods is False:
            self.load(filepath)
        else:
            self.loadVariations(filepath)
        log.write('FINISHED\n\n')
        return {'FINISHED'}


def menu_func_import(self, context):
    self.layout.operator(DagImporter.bl_idname, text='Dagor Engine (.dag)')


def register():
    bpy.utils.register_class(DagImporter)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)
    bpy.utils.unregister_class(DagImporter)
