import bpy
import os
import re
from fnmatch    import fnmatch
from os.path    import exists, dirname, basename, isfile, join, abspath
from os         import listdir
from math                           import pi
from mathutils                      import Vector
from bpy.types                      import Operator
from bpy.props                      import BoolProperty, StringProperty
from bpy.utils                      import register_class, unregister_class
from bpy_extras.io_utils            import ImportHelper
from .                              import reader
from ..                             import datablock
from ..pyparsing                    import ParseException
from ..constants                    import *
from ..dagMath                      import mul
from ..node                         import NodeExp
from ..nodeMaterial                 import MaterialExp
from ..face                         import FaceExp
from ..mesh                         import MeshExp
from ..material                     import Material
from ..dagormat.build_node_tree     import build_dagormat_node_tree
from ..dagormat.rw_dagormat_text    import dagormat_to_string
from ..dagormat.compare_dagormats   import compare_dagormats
from ..object_properties            import object_properties
from ..helpers.props                import fix_type, dagormat_prop_add
from ..helpers.texts                import log
from ..popup.popup_functions        import show_popup
from ..helpers.version              import get_blender_version
from ..helpers.getters              import get_preferences
from ..smooth_groups.smooth_groups  import uint_to_int, int_to_uint, sg_to_sharp_edges
from ..tools.tools_panel            import fix_mat_slots, optimize_mat_slots

FACENO = 0
INDEX = 1
BEENTHERE = 2


#FUNCTIONS

#returns possible asset type based on name
def guess_dag_type(name):
    pref = get_preferences()
    if name.endswith('_destr.lod00'):
        return 'dynmodel'
    if name[:-2].endswith('.lod'):
        return 'rendinst'
    return 'prefab'

def get_col_parent(col):
    collections = bpy.data.collections
    scenes = bpy.data.scenes
    parent = None

    name = col.name
    if 'type' in col.keys():
        type = col['type']
    else:
        type = None

    if name[-6:-2] == '.lod':
        parent = collections.get(name[:-2]+'s')
    if parent is not None:
        return parent
    parent = collections.get(name[:-6])
    if parent is not None:
        if 'type' in parent.keys() and parent['type'] == type:
            return parent
        elif 'type' not in parent.keys():
            parent.name = name[:-2]+'s'
            parent['type'] = type
            return parent
    if name[-6:-2] == '.lod':
        parent = bpy.data.collections.new(name[:-2]+'s')
        parent['type'] = type
        scenes['GEOMETRY'].collection.children.link(parent)
    else:
        parent = scenes['GEOMETRY'].collection
    return parent

#returns collection to store imported asset
def get_dag_col(name,replace_existing):
    collections = bpy.data.collections
    type = guess_dag_type(name)
    col = None
#looking for existing one
    col = collections.get(name)
#checking overriden name/path
    if col is None:
        for c in collections:
            keys = c.keys()
            if 'name' in keys:
                name_parts = c['name']
                name_parts = name_parts.replace('\\','/')
                name_parts = name_parts.split('/')
                if name_parts[-1] in [name, f'{name}.dag']:
                    if 'type' in keys and c['type'] == type:
                        col = c
                        break
                    elif 'type' not in keys:
                        c["type"] = type
                        col = c
                        break

    if not col is None and not replace_existing:
        if col.children.__len__()>0 or col.objects.__len__()>0:
            col = None

    if col is None:
        col = collections.new(name = name)
        parent = get_col_parent(col)
        parent.children.link(col)
        if name.__len__()>63:
            col['name'] = name
        col["type"] = type
    else:
        objects = col.objects
        for o in objects:
            col.objects.unlink(o)
    return col

neighbors = {}

#searches for triangles of same polygons
def buildNeighbors(faces):
    global neighbors
    neighbors = {}
    for i, f in enumerate(faces):
        neighbors[tuple(f.f[0:2])] = [i, 0, 0]
        neighbors[tuple(f.f[1:3])] = [i, 1, 0]
        neighbors[tuple([f.f[2], f.f[0]])] = [i, 2, 0]
    return

#replace apex_interior prop by new name of renamed material
def correct_apex_mats(old_name,mat):
    new_name = mat.name
    for obj in bpy.data.objects:
        if obj.type not in ['MESH','CURVE']:
            continue
        if obj.data.materials.get(mat.name) is None:
            continue
        apex_mat = obj.dagorprops.get("apex_interior_material:t")
        if apex_mat is None:
            continue
        if apex_mat == old_name:
            apex_mat = new_name
            msg = f'Object "{obj.name}": updated "apex_interior_material:t" property to match new material name\n'
            log(msg)
    return

#CLASSES
class Curve:
    closed: bool
    points = []
    handle_left = []
    handle_right = []


class DagImporter(Operator, ImportHelper):
    bl_idname = "import_scene.dag"
    bl_label = "Import DAG"
    bl_options = {'PRESET', 'UNDO'}

    filename_ext = ".dag"

    filter_glob: StringProperty(
        default="*.dag",
        options={'HIDDEN'})

# Batch import related
    includes_re: StringProperty(
        name = "Regular Expression",
        description = "RE for search in import forler",
        default = "",
        options = {'HIDDEN'})

    excludes_re: StringProperty(
        name = "Regular Expression",
        description = "RE for skip in import forler",
        default = "",
        options = {'HIDDEN'})

    dirpath: StringProperty(
        name = "Dirpath",
        description = "Directory that contains dag files to import",
        default = "",
        subtype = 'DIR_PATH',
        options = {'HIDDEN'})

    check_subdirs: BoolProperty(
        name = "Include subdirs",
        description = "Search in subdirectories as well",
        default = False,
        options = {'HIDDEN'})

    includes: StringProperty(
        default = "",
        options = {'HIDDEN'})

    excludes: StringProperty(
        default = "",
        options = {'HIDDEN'})

    with_lods: BoolProperty(
            name = "Import LODs",
            description = "Search for other levels of detail",
            default = False)

    with_dps: BoolProperty(
        name = "Import dps",
        description = "Search for related Damage Parts",
        default = False)

    with_dmgs: BoolProperty(
        name = "Import dmgs",
        description = "Search for Damaged versions",
        default = False)

    with_destr: BoolProperty(
        name = "Import destr",
        description = "Search for dynamic destr asset",
        default = False)

#Optimization
    mopt: BoolProperty(
        name="Optimize material slots",
        description="Remove unused materials",
        default=True)
#hidden from UI, because not implemented yet----------------------------------------------------------------------------
    fix_mat_ids: BoolProperty(
        name = "Fix material indices",
        description = "update each material index, if it's higher than amount of material_slots",
        default = True,
        options = {'HIDDEN'})

    remove_degenerates: BoolProperty(
        name = "Remove degenerates",
        description = "Remove triangles with area = 0",
        default = True,
        options = {'HIDDEN'})

    remove_loose: BoolProperty(
        name = "Remove loose geometry",
        description = "Remove vertices, that not connected to any face",
        default = True,
        options = {'HIDDEN'})
#-----------------------------------------------------------------------------------------------------------------------
#Other
    replace_existing: BoolProperty(
        name = 'Replace existing',
        description = 'Replace already existing in blend instead of duplicating',
        default = False)

    preserve_path: BoolProperty(
        name = 'Preserve path',
        description = 'Override export path for imported files',
        default = False)

    preserve_sg: BoolProperty(
        name = "Preserve Smoothing Groups",
        description = "Store Smoothing Groups in a integer face attribute called 'SG'",
        default = False)


    rootNode = None
    reader = reader.DagReader()
    end = False

    textures = []
    materials = []

    @classmethod
    def poll(self, context):
        pref = get_preferences()
        if pref.projects.__len__() == 0:
            return False
        project = pref.projects[int(pref.project_active)]
        if not exists(project.path):
            return False
        return True

    @classmethod
    def description(cls, context, properties):
        pref = get_preferences()
        if pref.projects.__len__() == 0:
            return "No active project found. Please, configure at least one project"
        return "Import DAG file(s)"

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
        return

    def buildPoly(self, faceIndex, faces, idx, res, indices, faceDict, vertices):
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
        return

    def buildExtraPoly(self, faceIndex, faces, idx):
        f = faces[faceIndex]
        for i in [idx, (idx + 1) % 3, (idx + 2) % 3]:
            if f.neighbors[i][BEENTHERE] == 2:
                f.neighbors[i][BEENTHERE]=1
                return True
        return False


    def buildMesh(self, node, tm, parent):
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

        if len(node.mesh.color_attributes):
            poly = []
            for index in indices:
                poly.append(node.mesh.color_attributes_poly[index])
            vcol_lay = me.color_attributes.new(name = "",domain = "CORNER", type = "FLOAT_COLOR")
            col = []
            for index in poly:
                col.extend(node.mesh.color_attributes[index])
                col.append(1.0)
            vcol_lay.data.foreach_set("color_srgb", tuple(col))

        me.validate()
        me.update()

        me.attributes.new("SG", 'INT', 'FACE')
        i=0
        for sg in me.attributes['SG'].data:
            sg.value=uint_to_int(SG[i])
            i+=1
        sg_to_sharp_edges(me)
        if not self.preserve_sg:
            me.attributes.remove(me.attributes.get('SG'))

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
                msg =f"{err=}\n"
                log(msg, type = 'ERROR', show = True)
        elif get_blender_version()< 4.1:
            me.calc_normals_split()
        o = bpy.data.objects.new(name=node.name, object_data=me)
        if get_blender_version() < 4.1:
            me.use_auto_smooth = True
            me.auto_smooth_angle = pi
        else:
            values = [True]*o.data.polygons.__len__()
            o.data.polygons.foreach_set('use_smooth', values)
        self.collection.objects.link(o)
        if parent is not None:
            o.parent = parent
        o.matrix_world = tm

        self.buildObjProperties(o, node.objProps)
        if o.dagorprops.get('broken_properties:t') is not None:
            msg =f'{o.name} has incorrect object properties\n'
            log(msg,type = 'WARNING', show = True)
        tm.transpose()

#> fixing apex if material was renamed on import
        apex_mat=o.dagorprops.get('apex_interior_material:t')
        if apex_mat is not None and o.data.materials.get(apex_mat) is None:
            for m in self.materials:
                found=False
                if m.name==apex_mat:
                    found=True
                    msg = f'{o.name}: apex_interior_material was renamed. '
                    msg+= f'"{apex_mat}" merged with "{m.mat.name}"\n'
                    log(msg)
                    o.dagorprops['apex_interior_material:t']=m.mat.name
                    break
                elif m.shader==apex_mat+':proxymat':
                    o.dagorprops['apex_interior_material:t']=m.mat.name
                    msg = f'{o.name}: apex_interior_material was renamed. '
                    msg+= f'"{apex_mat}" renamed into "{m.mat.name}" to match proxymat.blk name\n'
                    log(msg)
                    found=True
                    break
            if not found:
                msg = f'{o.name} materials does not contain "apex_interior_material:t"="{apex_mat}"\n'
                log(msg, type = 'WARNING')
#fixing mat_ids:
        fix_mat_slots(o)
        if self.mopt:
            before=o.material_slots.__len__()
            optimize_mat_slots(o)
            diff = before - o.material_slots.__len__()
            if diff>0:
                msg =f'{o.name}: removed {diff} unnecessary material slot(s)\n'
                log(msg)
        for node in node.children:
            tm2 = mul(node.tm, tm)
            tm2.transpose()
            self.buildMesh(node, tm2, o)
            self.buildCurves(node, tm2, o)
            self.buildEmpty(node, tm2, o)
        return

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
            dagorprops['broken_properties:t']=broken
        return

    def DM_set_params(_self, script):
        params = []
        type_value = script.split('\n')
        for el in type_value:
            if el.find("=") == -1:
                continue
            pair = el.split('=')
            params.append([pair[0], pair[1]])
        return params

    def build_dagormat_node_trees(self):
        for m in self.materials:
            og_mat = bpy.data.materials.get(m.name)
            if og_mat is not None:
                og_name = og_mat.name
            m.mat = bpy.data.materials.new(name=m.name)
#proxymat
            if m.shader.endswith(':proxymat'):
                proxy_name = m.shader.replace(':proxymat','')
                og_mat = bpy.data.materials.get(proxy_name)
                if og_mat is not None:
                    og_name = og_mat.name
                m.mat.name = proxy_name
                m.mat.name = proxy_name #making sure there's no ".NNN" postfix added
                m.mat.dagormat.is_proxy = True
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
                    m.mat.dagormat.sides = 1
                tex_amount = DAGTEXNUM if (m.flags & DAG_MF_16TEX) else 8
                for i in range(tex_amount):
                    tex='tex'+str(i)
                    if m.tex[i] != DAGBADMATID:
                        setattr(m.mat.dagormat.textures, tex, self.textures[m.tex[i]])
                    else:
                        setattr(m.mat.dagormat.textures, tex, "")
                blk = datablock.DataBlock()
                try:
                    m.script = m.script.replace("(", "")
                    m.script = m.script.replace(")", "")
                    m.script = m.script.replace("\r", "")
                    blk.parseString(m.script)
                    params = self.DM_set_params(m.script)
                    for param in params:
                        if param[0] == "real_two_sided":
                            if param[1].lower() in ['1', 'yes', 'true']:
                                m.mat.dagormat.sides = 2
                            continue
                        else:
                            try:
                                dagormat_prop_add(m.mat.dagormat, param[0], param[1])
                            except:
                                msg = f'\nsomething wrong happened on reading "{m.mat.name}" parameters!\n'
                                log(msg,type = 'ERROR')
                except ParseException as err:
                    print(f"{err=}: " + m.script)
#fixing apex
            if og_mat is not None:
                correct_apex_mats(og_name,og_mat)
#merging_mats
            for mat in bpy.data.materials:
                is_unic=True
                if mat == m.mat:
                    continue
                if compare_dagormats(m.mat,mat):
                    log(f'"{m.mat.name}" remapped to "{mat.name}"\n')
                    just_imported = m.mat
                    m.mat=mat
                    if m.mat.name.startswith(just_imported.name):
                        m.mat.name=just_imported.name
                    bpy.data.materials.remove(just_imported)
                    is_unic=False
                    break
            if is_unic:
                build_dagormat_node_tree(m.mat)
        return

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
            self.buildEmpty(node, tm2, o)
        return

    def buildEmpty(self,node,tm,parent):
        if not (node.curves is None and node.mesh is None):
            return
        o = bpy.data.objects.new(node.name, None)
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
            self.buildEmpty(node, tm2, o)
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
        return

    def loadMesh(self, mesh):
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
                mesh.color_attributes = list()
                while count:
                    if co == 1:
                        mesh.color_attributes.append([self.reader.readFloat(), 0, 0])
                    elif co == 2:
                        mesh.color_attributes.append([self.reader.readFloat(), self.reader.readFloat(), 0])
                    else:
                        mesh.color_attributes.append(self.reader.readVector())
                    count -= 1
                count = faceNumber
                while count:
                    mesh.color_attributes_poly.append(self.reader.readUShort())
                    poly1 = self.reader.readUShort()
                    poly2 = self.reader.readUShort()
                    mesh.color_attributes_poly.append(poly2)
                    mesh.color_attributes_poly.append(poly1)
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
        return

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
                mesh.color_attributes = list()
                while count:
                    if co == 1:
                        mesh.color_attributes.append([self.reader.readFloat(), 0, 0])
                    elif co == 2:
                        mesh.color_attributes.append([self.reader.readFloat(), self.reader.readFloat(), 0])
                    else:
                        mesh.color_attributes.append(self.reader.readVector())
                    count -= 1
                count = faceNumber
                while count:
                    mesh.color_attributes_poly.append(self.reader.readDWord())
                    poly1 = self.reader.readDWord()
                    poly2 = self.reader.readDWord()
                    mesh.color_attributes_poly.append(poly2)
                    mesh.color_attributes_poly.append(poly1)
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
        return

    def loadObj(self, node, size):
        endpos = self.reader.file.tell() + size
        while self.reader.file.tell() < endpos:
            chunk = self.reader.beginChunk()
            if chunk.id == DAG_OBJ_MESH:
                mesh = node.mesh = MeshExp()
                self.loadMesh(mesh)
            elif chunk.id == DAG_OBJ_BIGMESH:
                mesh = node.mesh = MeshExp()
                self.loadBigMesh(mesh)
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
        return

    def importNodes(self, size):
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
        return

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
        self.build_dagormat_node_trees()
#searching for existing collection
        col_name = basename(filepath).replace('.dag','')
        self.collection = get_dag_col(col_name,self.replace_existing)
        if self.preserve_path:
            self.collection["name"] = filepath.replace('.dag','')

        for node in self.rootNode.children:
            node.tm.transpose()
            tmp_r = node.tm[1].copy()
            node.tm[1] = node.tm[2].copy()
            node.tm[2] = tmp_r.copy()
            self.buildMesh(node, node.tm, None)
            self.buildCurves(node, node.tm, None)
            self.buildEmpty(node, node.tm, None)
        return

    def filename_to_regex(self):
        filename = basename(self.filepath)
        dp =    self.with_dps
        lods =  self.with_lods
        dmg =   self.with_dmgs
        destr = self.with_destr
        lod_info = re.search("\\.lod\\d\\d\\.dag$", filename)  # '\d' == '[0-9]'
        if lod_info is None:  # can not find variantions for non-standard names
            return f"^{filename}$"
        split_index = lod_info.span()[0]
        base_name = filename[:split_index]
        base_lod = filename[split_index+1:-4]  # no dots or extension needed
        dp_re = "(_dp_\\d\\d|)" if dp else "()"
        dmg_re = "(_dmg|)" if dmg else "()"
        lods_re = "\\.lod\\d\\d" if lods else f"\\.{base_lod}"
        variants_re = f"^{base_name}{dp_re}{dmg_re}{lods_re}\\.dag$"
        if not destr:
            return variants_re
        destr_re = f"^{base_name}{dp_re}{dmg_re}_destr\\.lod00\\.dag$"  # destr is always ".lod00"
        combined_re = f"({variants_re}|{destr_re})"
        return combined_re
#base mode
    def file_to_filepaths(self):
        filepath = abspath(self.filepath)
        filepaths = [filepath]
        regex = self.filename_to_regex()
        regex = re.compile(regex)
        dirpath = dirname(filepath)
        if self.check_subdirs:
            print("checking subdirs")
            for subdir, dirs, files in os.walk(dirpath):
                for file in files:
                    filepath = join(subdir, file)
                    if re.search(regex, file):
                        filepaths.append(filepath)
        else:
            filenames = [file for file in listdir(dirpath) if isfile(join(dirpath,file))]
            original_filename = basename(self.filepath)
            for filename in filenames:
                if filename == original_filename:
                    continue
                if re.search(regex, filename):
                    filepaths.append(join(dirpath, filename))
        return list(set(filepaths))
#advanced mode (wildcard search)
    def includes_to_filepaths(self):
        filepaths = []
        includes = self.includes.split(';')
        excludes = self.excludes.split(';')
        if self.check_subdirs:
            for subdir, dirs, files in os.walk(self.dirpath):
                for file in files:
                    has_match = False
                    for include in includes:
                        if include == "":
                            continue
                        if fnmatch(file, include) or fnmatch(file[:-4], include):
                            has_match = True
                            break
                    if not has_match:
                        continue
                    for exclude in excludes:
                        if exclude == "":
                            continue
                        if fnmatch(file, exclude) or fnmatch(file[:-4], exclude):
                            has_match = False
                            break
                    if has_match:
                        filepaths.append(join(subdir, file))
        else:
            files = [file for file in listdir(self.dirpath) if isfile(join(self.dirpath,file))]
            for file in files:
                has_match = False
                for include in includes:
                    if include == "":
                        continue
                    if fnmatch(file, include):
                        has_match = True
                        break
                if not has_match:
                    continue
                for exclude in excludes:
                    if exclude == "":
                        continue
                    if fnmatch(file, exclude):
                        has_match = False
                        break
                if has_match:
                    filepaths.append(join(self.dirpath, file))
        return filepaths
#expert mode (regex search)
    def regex_to_filepaths(self):
        regex_include = re.compile(self.includes_re)
        regex_exclude = re.compile(self.excludes_re)
        filepaths = []
        if self.check_subdirs:
            for subdir, dirs, files in os.walk(self.dirpath):
                for file in files:
                    name = file.lower()
                    if not name.endswith('.dag'):
                        continue
                    if re.search(regex_include, file) and not re.search(regex_exclude, file):
                        filepaths.append(join(subdir, file))
        else:
            files = [file for file in listdir(self.dirpath) if isfile(join(self.dirpath,file))]
            for file in files:
                name = file.lower()
                if not name.endswith('.dag'):
                    continue
                if re.search(regex_include, file) and not re.search(regex_exclude, file):
                    filepaths.append(join(self.dirpath, file))
        return filepaths

    def execute(self, context):
        if bpy.context.mode != 'OBJECT':
            try:
                bpy.ops.object.mode_set(mode='OBJECT')
            except Exception:
                pass
        bpy.ops.dt.init_blend()
        if self.filepath != "":
            filepaths = self.file_to_filepaths()
        elif self.includes_re != "" and self.dirpath != "":
            filepaths = self.regex_to_filepaths()
        elif self.includes != "":
            filepaths = self.includes_to_filepaths()
        else:
            filepaths = []  # placeholer, will be replaced by error message for unknown import mode
            log('Which mode we are in???', type = 'ERROR', show = True)
        for filepath in filepaths:
            self.load(filepath)
        context.window.scene = bpy.data.scenes['GEOMETRY']
        return {'FINISHED'}


def menu_func_import(self, context):
    self.layout.operator(DagImporter.bl_idname, text='Dagor Engine (.dag)')
    return


def register():
    register_class(DagImporter)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)
    return


def unregister():
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)
    unregister_class(DagImporter)
    return
