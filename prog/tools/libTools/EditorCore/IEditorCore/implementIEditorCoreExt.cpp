#include "implementIEditorCore.h"
#include <EditorCore/captureCursor.h>
#include <EditorCore/ec_ObjectEditor.h>
#include <EditorCore/ec_ObjectCreator.h>

#include <3d/dag_drv3d.h>
#include <math/dag_SHlight.h>
#include <3d/dag_render.h>

#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_DynamicShadersBuffer.h>
#include <shaders/dag_renderScene.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStates.h>

#include <coolConsole/coolConsole.h>

#include <libTools/staticGeom/staticGeometryContainer.h>
#include <libTools/staticGeom/geomObject.h>
#include <libTools/staticGeom/geomResources.h>

#include <libTools/dtx/ddsxPlugin.h>

#include <libTools/util/hash.h>
#include <libTools/util/fileUtils.h>
#include <libTools/util/fileMask.h>

#include <libTools/dagFileRW/dagUtil.h>
#include <libTools/dagFileRW/dagFileExport.h>

#include <libTools/ObjCreator3d/objCreator3d.h>

#include <sceneBuilder/nsb_StdTonemapper.h>

#include <debug/dag_debug.h>
#include <debug/dag_debug3d.h>
#include <debug/dag_fatal.h>

#include <memory/dag_memStat.h>

#include <math/dag_boundingSphere.h>

#include <scene/dag_loadLevel.h>
#include <scene/dag_renderSceneMgr.h>
#include <scene/dag_occlusionMap.h>
#include <scene/dag_frtdump.h>
#include <scene/dag_frtdumpInline.h>

#include <sceneRay/dag_sceneRay.h>

#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_progGlobals.h>

#include <image/dag_tga.h>

#include <perfMon/dag_cpuFreq.h>

#include <animChar/dag_animCharacter2.h>
#include <anim/dag_animChannels.h>

#include <startup/dag_globalSettings.h>


#define CREATE_OBJECT(type, alloc)             \
  type *ret = NULL;                            \
  ret = (alloc) ? new (alloc) type : new type; \
  G_ASSERT(ret);                               \
  return ret;


#define CREATE_OBJECT_1PARAM(type, param, alloc)             \
  type *ret = NULL;                                          \
  ret = (alloc) ? new (alloc) type(param) : new type(param); \
  G_ASSERT(ret);                                             \
  return ret;


//==================================================================================================
// EcGeom
//==================================================================================================
StaticGeometryContainer *EcGeom::newStaticGeometryContainer(IMemAlloc *alloc) { CREATE_OBJECT(StaticGeometryContainer, alloc); }


//==================================================================================================
void EcGeom::deleteStaticGeometryContainer(StaticGeometryContainer *&cont) { del_it(cont); }


//==================================================================================================
void EcGeom::staticGeometryContainerClear(StaticGeometryContainer &cont) { cont.clear(); }


//==================================================================================================
bool EcGeom::staticGeometryContainerLoadFromDag(StaticGeometryContainer &cont, const char *path, ILogWriter *log,
  bool use_not_found_tex, int load_flags)
{
  return cont.loadFromDag(path, log, use_not_found_tex, load_flags);
}


//==================================================================================================
bool EcGeom::staticGeometryContainerImportDag(StaticGeometryContainer &cont, const char *src, const char *dest) const
{
  return cont.importDag(src, dest);
}


//==================================================================================================
void EcGeom::staticGeometryContainerExportDag(StaticGeometryContainer &cont, const char *path, bool make_tex_path_local) const
{
  cont.exportDag(path, make_tex_path_local);
}


//==================================================================================================
GeomObject *EcGeom::newGeomObject(IMemAlloc *alloc) { CREATE_OBJECT(GeomObject, alloc); }


//==================================================================================================
void EcGeom::deleteGeomObject(GeomObject *&geom) { del_it(geom); }


//==================================================================================================
void EcGeom::geomObjectSaveToDag(GeomObject &go, const char *path) const { go.saveToDag(path); }


//==================================================================================================
bool EcGeom::geomObjectLoadFromDag(GeomObject &go, const char *path, ILogWriter *log, ITextureNameResolver *resolv_cb) const
{
  return go.loadFromDag(path, log, resolv_cb);
}


//==================================================================================================
void EcGeom::geomObjectClear(GeomObject &go) const { go.clear(); }


//==================================================================================================
void EcGeom::geomObjectEraseNode(GeomObject &go, int idx) const { go.eraseNode(idx); }


//==================================================================================================
StaticGeometryContainer &EcGeom::geomObjectGetGeometryContainer(GeomObject &go) const
{
  StaticGeometryContainer *ret = go.getGeometryContainer();
  G_ASSERT(ret);

  return *ret;
}


//==================================================================================================
const StaticGeometryContainer &EcGeom::geomObjectGetGeometryContainer(const GeomObject &go) const
{
  const StaticGeometryContainer *ret = go.getGeometryContainer();
  G_ASSERT(ret);

  return *ret;
}


//==================================================================================================
void EcGeom::geomObjectSetDefNodeVis(GeomObject &go) { go.setDefNodeVis(); }


//==================================================================================================
void EcGeom::geomObjectRecompile(GeomObject &go, const Bitarray *node_vis) { go.recompile(node_vis); }


//==================================================================================================
void EcGeom::geomObjectCreateFromGeomContainer(GeomObject &go, const StaticGeometryContainer &cont, bool do_recompile)
{
  go.createFromGeomContainer(&cont, do_recompile);
}


//==================================================================================================
void EcGeom::geomObjectRender(GeomObject &go) { go.render(); }


//==================================================================================================
void EcGeom::geomObjectRenderColor(GeomObject &go, DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only) const
{
  go.renderColor(buffer, c, renderable_only);
}


//==================================================================================================
void EcGeom::geomObjectRenderNodeColor(GeomObject &go, int idx, DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only) const
{
  go.renderNodeColor(idx, buffer, c, renderable_only);
}


//==================================================================================================
void EcGeom::geomObjectRenderTrans(GeomObject &go) { go.renderTrans(); }


//==================================================================================================
void EcGeom::geomObjectRenderEdges(GeomObject &go, bool is_visible, E3DCOLOR color) { go.renderEdges(is_visible, color); }


//==================================================================================================
BBox3 EcGeom::geomObjectGetBoundBox(GeomObject &go, bool local_coord) const { return go.getBoundBox(local_coord); }


//==================================================================================================
BBox3 EcGeom::geomObjectGetNodeBoundBox(const GeomObject &go, int idx, bool local_coord) const
{
  return go.getNodeBoundBox(idx, local_coord);
}


//==================================================================================================
const char *EcGeom::geomObjectGetShaderName(const GeomObject &go, int idx) const { return go.getShaderName(idx); }


//==================================================================================================
ShaderMesh *EcGeom::geomObjectGetShaderMesh(const GeomObject &go, int idx) const { return go.getShaderMesh(idx); }


//==================================================================================================
bool EcGeom::geomObjectShadowRayHitTest(GeomObject &go, const Point3 &p, const Point3 &dir, real maxt, int trace_flags) const
{
  return go.shadowRayHitTest(p, dir, maxt, trace_flags);
}


//==================================================================================================
bool EcGeom::geomObjectTraceRay(GeomObject &go, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const
{
  return go.traceRay(p, dir, maxt, norm);
}


//==================================================================================================
bool EcGeom::geomObjectReloadRayTracer(GeomObject &go) const { return go.reloadRayTracer(); }

StaticSceneRayTracer *EcGeom::geomObjectGetRayTracer(GeomObject &go) const { return go.getRayTracer(); }


//==================================================================================================
StaticGeometryNode *EcGeom::newStaticGeometryNode(IMemAlloc *alloc) { CREATE_OBJECT(StaticGeometryNode, alloc); }


//==================================================================================================
StaticGeometryNode *EcGeom::newStaticGeometryNode(const StaticGeometryNode &from, IMemAlloc *alloc)
{
  StaticGeometryNode *ret = NULL;

  ret = (alloc) ? new (alloc) StaticGeometryNode(from) : new StaticGeometryNode(from);
  G_ASSERT(ret);

  return ret;
}


//==================================================================================================
void EcGeom::deleteStaticGeometryNode(StaticGeometryNode *&node) { del_it(node); }


//==================================================================================================
bool EcGeom::staticGeometryNodeHaveBillboardMat(const StaticGeometryNode &node) const { return node.haveBillboardMat(); }


//==================================================================================================
void EcGeom::staticGeometryNodeCalcBoundBox(StaticGeometryNode &node) const { node.calcBoundBox(); }


//==================================================================================================
void EcGeom::staticGeometryNodeCalcBoundSphere(StaticGeometryNode &node) const { node.calcBoundSphere(); }


//==================================================================================================
const char *EcGeom::staticGeometryNodeLightingToStr(StaticGeometryNode::Lighting light) const
{
  return StaticGeometryNode::lightingToStr(light);
}


//==================================================================================================
StaticGeometryNode::Lighting EcGeom::staticGeometryNodeStrToLighting(const char *light) const
{
  return StaticGeometryNode::strToLighting(light);
}


//==================================================================================================
void EcGeom::staticGeometryNodeSetMaterialLighting(const StaticGeometryNode &node, StaticGeometryMaterial &mat) const
{
  node.setMaterialLighting(mat);
}


//==================================================================================================
StaticGeometryMesh *EcGeom::newStaticGeometryMesh(IMemAlloc *alloc) const { CREATE_OBJECT(StaticGeometryMesh, alloc); }


//==================================================================================================
void EcGeom::deleteStaticGeometryMesh(StaticGeometryMesh *&mesh) const
{
  if (mesh)
  {
    bool doNULL = false;

    if (mesh->getRefCount() == 1)
      doNULL = true;

    mesh->delRef();

    if (doNULL)
      mesh = NULL;
  }
}


//==================================================================================================
GeomResourcesHelper *EcGeom::newGeomResourcesHelper(IStaticGeomResourcesService *svc, IMemAlloc *alloc) const
{
  CREATE_OBJECT_1PARAM(GeomResourcesHelper, svc, alloc);
}


//==================================================================================================
void EcGeom::deleteGeomResourcesHelper(GeomResourcesHelper *&helper) const { del_it(helper); }


//==================================================================================================
void EcGeom::geomResourcesHelperSetResourcesService(GeomResourcesHelper &helper, IStaticGeomResourcesService *svc) const
{
  helper.setResourcesService(svc);
}


//==================================================================================================
void EcGeom::geomResourcesHelperCreateResources(GeomResourcesHelper &helper, const void *obj_id, const TMatrix &tm,
  const StaticGeometryContainer &cont, ILogWriter *log) const
{
  helper.createResources(obj_id, tm, cont, log);
}


//==================================================================================================
void EcGeom::geomResourcesHelperFreeResources(GeomResourcesHelper &helper, const void *obj_id) const { helper.freeResources(obj_id); }


//==================================================================================================
void EcGeom::geomResourcesHelperSetResourcesTm(GeomResourcesHelper &helper, const void *obj_id, const TMatrix &tm) const
{
  helper.setResourcesTm(obj_id, tm);
}


//==================================================================================================
void EcGeom::geomResourcesHelperRemapResources(GeomResourcesHelper &helper, const void *obj_id_old, const void *obj_id_new) const
{
  helper.remapResources(obj_id_old, obj_id_new);
}


//==================================================================================================
int EcGeom::geomResourcesHelperCompact(GeomResourcesHelper &helper) const { return helper.compact(); }


//==================================================================================================
ShaderMaterial *EcGeom::newShaderMaterial(MaterialData &m) const { return ::new_shader_material(m); }


//==================================================================================================
int EcGeom::getShaderVariableId(const char *name) const { return ::get_shader_variable_id(name, true); }


//==================================================================================================
const char *EcGeom::getShaderVariableName(int id) const { return VariableMap::getVariableName(id); }


//==================================================================================================
bool EcGeom::shaderGlobalSetInt(int id, int val) const { return ShaderGlobal::set_int(id, val); }


//==================================================================================================
bool EcGeom::shaderGlobalSetReal(int id, real val) const { return ShaderGlobal::set_real(id, val); }


//==================================================================================================
bool EcGeom::shaderGlobalSetColor4(int id, const Color4 &val) const { return ShaderGlobal::set_color4(id, val); }


//==================================================================================================
bool EcGeom::shaderGlobalSetTexture(int id, TEXTUREID val) const { return ShaderGlobal::set_texture(id, val); }

int EcGeom::shaderGlobalGetInt(int id) const { return ShaderGlobal::get_int_fast(id); }
real EcGeom::shaderGlobalGetReal(int id) const { return ShaderGlobal::get_real_fast(id); }
Color4 EcGeom::shaderGlobalGetColor4(int id) const { return ShaderGlobal::get_color4_fast(id); }
TEXTUREID EcGeom::shaderGlobalGetTexture(int id) const { return ShaderGlobal::get_tex_fast(id); }

//==================================================================================================
void EcGeom::shaderGlobalSetVarsFromBlk(const DataBlock &blk) const { return ShaderGlobal::set_vars_from_blk(blk); }

int EcGeom::shaderGlobalGetBlockId(const char *name) const { return ShaderGlobal::getBlockId(name); }

void EcGeom::shaderGlobalSetBlock(int id, int layer) const { ShaderGlobal::setBlock(id, layer); }

void EcGeom::shaderElemInvalidateCachedStateBlock() const { return ShaderElement::invalidate_cached_state_block(); }

//==================================================================================================
Mesh *EcGeom::newMesh(IMemAlloc *alloc) const { CREATE_OBJECT(Mesh, alloc); }


//==================================================================================================
void EcGeom::deleteMesh(Mesh *&mesh) const { del_it(mesh); }


//==================================================================================================
int EcGeom::meshOptimizeTverts(Mesh &mesh) const
{
  mesh.optimize_tverts();
  return 1;
}


//==================================================================================================
int EcGeom::meshCalcNgr(Mesh &mesh) const { return mesh.calc_ngr(); }


//==================================================================================================
int EcGeom::meshCalcVertnorms(Mesh &mesh) const { return mesh.calc_vertnorms(); }


//==================================================================================================
void EcGeom::meshFlipNormals(Mesh &mesh, int f0, int nf) const { mesh.flip_normals(f0, nf); }

PostFxRenderer *EcGeom::newPostFxRenderer() const { return new PostFxRenderer; }
void EcGeom::deletePostFxRenderer(PostFxRenderer *&p) const { del_it(p); }
void EcGeom::postFxRendererInit(PostFxRenderer &p, const char *shader_name) const { p.init(shader_name); }
void EcGeom::postFxRendererRender(PostFxRenderer &p) const { p.render(); }

//==================================================================================================
bool EcGeom::generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::generatePlane(cell_size, geom);
}


//==================================================================================================
bool EcGeom::generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom, StaticGeometryMaterial *material) const
{
  return ObjCreator3d::generatePlane(cell_size, geom, material);
}


//==================================================================================================
bool EcGeom::generateBox(const IPoint3 &segments, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::generateBox(segments, geom);
}


//==================================================================================================
bool EcGeom::generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::generateCylinder(sides, height_segments, cap_segments, geom);
}


//==================================================================================================
bool EcGeom::generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::generatePolyMesh(points, height_segments, geom);
}


//==================================================================================================
bool EcGeom::generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::generateBoxSpiralStair(steps, w, arc, geom);
}


//==================================================================================================
bool EcGeom::generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::generateClosedSpiralStair(steps, w, arc, geom);
}


//==================================================================================================
bool EcGeom::generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::generateOpenSpiralStair(steps, w, h, arc, geom);
}


//==================================================================================================
bool EcGeom::generateBoxStair(int steps, StaticGeometryContainer &geom) const { return ObjCreator3d::generateBoxStair(steps, geom); }


//==================================================================================================
bool EcGeom::generateClosedStair(int steps, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::generateClosedStair(steps, geom);
}


//==================================================================================================
bool EcGeom::generateOpenStair(int steps, real h, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::generateOpenStair(steps, h, geom);
}


//==================================================================================================
bool EcGeom::objCreator3dAddNode(const char *name, Mesh *mesh, MaterialDataList *material, StaticGeometryContainer &geom) const
{
  return ObjCreator3d::addNode(name, mesh, material, geom);
}


//==================================================================================================
BoxCreator *EcGeom::newBoxCreator(IMemAlloc *alloc) const { CREATE_OBJECT(BoxCreator, alloc); }


//==================================================================================================
void EcGeom::deleteBoxCreator(BoxCreator *&creator) const { del_it(creator); }


//==================================================================================================
SphereCreator *EcGeom::newSphereCreator(IMemAlloc *alloc) const { CREATE_OBJECT(SphereCreator, alloc); }


//==================================================================================================
void EcGeom::deleteSphereCreator(SphereCreator *&creator) const { del_it(creator); }


//==================================================================================================
PlaneCreator *EcGeom::newPlaneCreator(IMemAlloc *alloc) const { CREATE_OBJECT(PlaneCreator, alloc); }


//==================================================================================================
void EcGeom::deletePlaneCreator(PlaneCreator *&creator) const { del_it(creator); }

//==================================================================================================
PointCreator *EcGeom::newPointCreator(IMemAlloc *alloc) const { CREATE_OBJECT(PointCreator, alloc); }


//==================================================================================================
void EcGeom::deletePointCreator(PointCreator *&creator) const { del_it(creator); }

//==================================================================================================
CylinderCreator *EcGeom::newCylinderCreator(IMemAlloc *alloc) const { CREATE_OBJECT(CylinderCreator, alloc); }


//==================================================================================================
void EcGeom::deleteCylinderCreator(CylinderCreator *&creator) const { del_it(creator); }


//==================================================================================================
PolyMeshCreator *EcGeom::newPolyMeshCreator(IMemAlloc *alloc) const { CREATE_OBJECT(PolyMeshCreator, alloc); }


//==================================================================================================
void EcGeom::deletePolyMeshCreator(PolyMeshCreator *&creator) const { del_it(creator); }


//==================================================================================================
StairCreator *EcGeom::newStairCreator(IMemAlloc *alloc) const { CREATE_OBJECT(StairCreator, alloc); }


//==================================================================================================
void EcGeom::deleteStairCreator(StairCreator *&creator) const { del_it(creator); }


//==================================================================================================
SpiralStairCreator *EcGeom::newSpiralStairCreator(IMemAlloc *alloc) const { CREATE_OBJECT(SpiralStairCreator, alloc); }


//==================================================================================================
void EcGeom::deleteSpiralStairCreator(SpiralStairCreator *&creator) const { del_it(creator); }


//==================================================================================================
SplineCreator *EcGeom::newSplineCreator(IMemAlloc *alloc) const { CREATE_OBJECT(SplineCreator, alloc); }


//==================================================================================================
void EcGeom::deleteSplineCreator(SplineCreator *&creator) const { del_it(creator); }


//==================================================================================================
TargetCreator *EcGeom::newTargetCreator(IMemAlloc *alloc) const { CREATE_OBJECT(TargetCreator, alloc); }


//==================================================================================================
void EcGeom::deleteTargetCreator(TargetCreator *&creator) const { del_it(creator); }


//==================================================================================================
void EcGeom::deleteIObjectCreator(IObjectCreator *&creator) const { del_it(creator); }


//==================================================================================================
DagSaver *EcGeom::newDagSaver(IMemAlloc *alloc) const { CREATE_OBJECT(DagSaver, alloc); }


//==================================================================================================
void EcGeom::deleteDagSaver(DagSaver *&saver) const { del_it(saver); }


//==================================================================================================
bool EcGeom::dagSaverStartSaveDag(DagSaver &saver, const char *path) const { return saver.start_save_dag(path); }


//==================================================================================================
bool EcGeom::dagSaverEndSaveDag(DagSaver &saver) const { return saver.end_save_dag(); }


//==================================================================================================
bool EcGeom::dagSaverStartSaveNodes(DagSaver &saver) const { return saver.start_save_nodes(); }


//==================================================================================================
bool EcGeom::dagSaverEndSaveNodes(DagSaver &saver) const { return saver.end_save_nodes(); }


//==================================================================================================
bool EcGeom::dagSaverStartSaveNode(DagSaver &saver, const char *name, const TMatrix &wtm, int flg, int children) const
{
  return saver.start_save_node(name, wtm, flg, children);
}


//==================================================================================================
bool EcGeom::dagSaverEndSaveNode(DagSaver &saver) const { return saver.end_save_node(); }


//==================================================================================================
bool EcGeom::dagSaverSaveDagSpline(DagSaver &saver, DagSpline **spline, int cnt) const { return saver.save_dag_spline(spline, cnt); }


//==================================================================================================
AScene *EcGeom::newAScene(IMemAlloc *alloc) const { CREATE_OBJECT(AScene, alloc); }


//==================================================================================================
void EcGeom::deleteAScene(AScene *&scene) const { del_it(scene); }


//==================================================================================================
int EcGeom::loadAscene(const char *fn, AScene &sc, int flg, bool fatal_on_error) const
{
  return ::load_ascene(fn, sc, flg, fatal_on_error);
}


//==================================================================================================
void EcGeom::nodeCalcWtm(Node &node) const { node.calc_wtm(); }


//==================================================================================================
bool EcGeom::nodeEnumNodes(Node &node, Node::NodeEnumCB &cb, Node **res) const { return node.enum_nodes(cb, res); }


//==================================================================================================
// EcTools
//==================================================================================================


//==================================================================================================
bool EcTools::ddsConvertImage(ddstexture::Converter &converter, IGenSave &cb, TexPixel32 *pixels, int width, int height,
  int stride) const
{
  return converter.convertImage(cb, pixels, width, height, stride);
}


//==================================================================================================
bool EcTools::loadmaskloadMaskFromFile(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h) const
{
  return loadmask::loadMaskFromFile(filename, hmap, w, h);
}


//==================================================================================================
AnimV20::AnimData *EcTools::newAnimData(IMemAlloc *alloc) const { CREATE_OBJECT(AnimV20::AnimData, alloc); }


//==================================================================================================
bool EcTools::animDataLoad(AnimV20::AnimData &anim, IGenLoad &cb, IMemAlloc *alloc) const { return anim.load(cb, alloc); }


//==================================================================================================
unsigned EcTools::getSimpleHash(const char *s, unsigned int init_val) const { return ::get_hash(s, init_val); }


//==================================================================================================
void *EcTools::win32GetMainWnd() const { return ::win32_get_main_wnd(); }


//==================================================================================================
int EcTools::getTimeMsec() const { return ::get_time_msec(); }


//==================================================================================================
__int64 EcTools::refTimeUsec() const { return ::ref_time_ticks(); }


void EcTools::ddsxShutdownPlugins() { ddsx::shutdown_plugins(); }
bool EcTools::ddsxConvertDds(unsigned targ_code, ddsx::Buffer &dest, void *dds_data, int dds_len, const ddsx::ConvertParams &p)
{
  return ddsx::convert_dds(targ_code, dest, dds_data, dds_len, p);
}
const char *EcTools::ddsxGetLastErrorText() { return ddsx::get_last_error_text(); }
void EcTools::ddsxFreeBuffer(ddsx::Buffer &b) { b.free(); }

//==================================================================================================
bool EcTools::copyFile(const char *src, const char *dest, bool overwrite) const { return ::dag_copy_file(src, dest, overwrite); }


//==================================================================================================
bool EcTools::compareFile(const char *path1, const char *path2) const { return ::dag_file_compare(path1, path2); }


//==================================================================================================
int EcScene::staticSceneRayTracerTraceRay(StaticSceneRayTracer &rt, const Point3 &p, const Point3 &wdir2, real &mint2,
  int from_face) const
{
  return rt.traceray(p, wdir2, mint2, from_face);
}


//==================================================================================================
int EcScene::fastRtDumpTraceRay(FastRtDump &frt, int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid) const
{
  return frt.traceray(custom, p, dir, t, out_pmid);
}


//==================================================================================================
BuildableStaticSceneRayTracer *EcScene::createBuildableStaticmeshsceneRaytracer(const Point3 &lsz, int lev) const
{
  return ::create_buildable_staticmeshscene_raytracer(lsz, lev);
}


//==================================================================================================
bool EcScene::buildableStaticSceneRayTracerAddmesh(BuildableStaticSceneRayTracer &rt, const Point3 *vert, int vcount,
  const unsigned *face, unsigned stride, int fn, const unsigned *face_flags, bool rebuild) const
{
  return rt.addmesh(vert, vcount, face, stride, fn, face_flags, rebuild);
}


//==================================================================================================
bool EcScene::buildableStaticSceneRayTracerReserve(BuildableStaticSceneRayTracer &rt, int face_count, int vert_count) const
{
  return rt.reserve(face_count, vert_count);
}


//==================================================================================================
bool EcScene::buildableStaticSceneRayTracerRebuild(BuildableStaticSceneRayTracer &rt) const { return rt.rebuild(); }


//==================================================================================================
StaticSceneBuilder::StdTonemapper *EcScene::newStdTonemapper(IMemAlloc *alloc) const
{
  CREATE_OBJECT(StaticSceneBuilder::StdTonemapper, alloc);
}


//==================================================================================================
StaticSceneBuilder::StdTonemapper *EcScene::newStdTonemapper(const StaticSceneBuilder::StdTonemapper &from, IMemAlloc *alloc) const
{
  CREATE_OBJECT_1PARAM(StaticSceneBuilder::StdTonemapper, from, alloc);
}


//==================================================================================================
void EcScene::deleteStdTonemapper(StaticSceneBuilder::StdTonemapper *&mapper) const { del_it(mapper); }


//==================================================================================================
void EcScene::stdTonemapperRecalc(StaticSceneBuilder::StdTonemapper &mapper) const { mapper.recalc(); }


//==================================================================================================
void EcScene::stdTonemapperSave(StaticSceneBuilder::StdTonemapper &mapper, DataBlock &blk) const { mapper.save(blk); }


//==================================================================================================
void EcScene::stdTonemapperLoad(StaticSceneBuilder::StdTonemapper &mapper, const DataBlock &blk) const { mapper.load(blk); }


//==================================================================================================
GameResource *EcScene::getGameResource(GameResHandle handle, bool no_factory_fatal) const
{
  return ::get_game_resource(handle, no_factory_fatal);
}

//==================================================================================================
void EcScene::releaseGameResource(GameResource *resource) const { ::release_game_resource(resource); }
