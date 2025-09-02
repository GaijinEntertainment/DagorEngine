// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EditorCore/ec_IEditorCore.h>
#include <shaders/dag_overrideStates.h>

class EcRender : public IDagorRender
{
public:
  // driver
  void fillD3dInterfaceTable(D3dInterfaceTable &d3dit) const override;

  // driver objects
  DagorCurView &curView() const override;

  // wire render
  void startLinesRender(bool test_z, bool write_z, bool z_func_less = false) const override;
  void setLinesTm(const TMatrix &tm) const override;
  void endLinesRender() const override;

  void renderLine(const Point3 &p0, const Point3 &p1, E3DCOLOR color) const override;
  void renderLine(const Point3 *points, int count, E3DCOLOR color) const override;
  void renderBox(const BBox3 &box, E3DCOLOR color) const override;
  void renderBox(const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color) const override;
  void renderSphere(const Point3 &center, real rad, E3DCOLOR col, int segs) const override;
  void renderCircle(const Point3 &center, const Point3 &ax1, const Point3 &ax2, real radius, E3DCOLOR col, int segs) const override;
  void renderXZCircle(const Point3 &center, real radius, E3DCOLOR col, int segs) const override;
  void renderCapsuleW(const Capsule &cap, E3DCOLOR c) const override;
  void renderCylinder(const TMatrix &tm, float rad, float height, E3DCOLOR c) const override;

  DebugPrimitivesVbuffer *newDebugPrimitivesVbuffer(const char *name, IMemAlloc *alloc = NULL) const override;
  void deleteDebugPrimitivesVbuffer(DebugPrimitivesVbuffer *&vbuf) const override;
  void beginDebugLinesCacheToVbuffer(DebugPrimitivesVbuffer &vbuf) const override;
  void endDebugLinesCacheToVbuffer(DebugPrimitivesVbuffer &vbuf) const override;
  void invalidateDebugPrimitivesVbuffer(DebugPrimitivesVbuffer &vbuf) const override;
  void renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf) const override;
  void renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf, E3DCOLOR c) const override;
  void renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf, bool z_test, bool z_write, bool z_func_less,
    Color4 color_multiplier) const override;
  void addLineToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p0, const Point3 &p1, E3DCOLOR c) const override;
  void addHatchedBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const TMatrix &box_tm, float hatching_step, E3DCOLOR color) const override;
  void addBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az,
    E3DCOLOR color) const override;
  void addSolidBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az,
    E3DCOLOR color) const override;
  void addBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const BBox3 &box, E3DCOLOR color) const override;
  void addTriangleToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 p[3], E3DCOLOR color) const override;
  bool isLinesVbufferValid(DebugPrimitivesVbuffer &vbuf) const override;
  void setVbufferTm(DebugPrimitivesVbuffer &vbuf, const TMatrix &tm) const override;

  // DynamicShadersBuffer
  DynamicShadersBuffer *newDynamicShadersBuffer(IMemAlloc *alloc) const override;
  DynamicShadersBuffer *newDynamicShadersBuffer(CompiledShaderChannelId *channels, int channel_count, int max_verts, int max_faces,
    IMemAlloc *alloc) const override;
  void deleteDynamicShadersBuffer(DynamicShadersBuffer *&buf) const override;

  void dynShaderBufferSetCurrentShader(DynamicShadersBuffer &buf, ShaderElement *shader) const override;
  void dynShaderBufferAddFaces(DynamicShadersBuffer &buf, const void *vertex_data, int num_verts, const int *indices,
    int num_faces) const override;
  void dynShaderBufferFlush(DynamicShadersBuffer &buf) const override;

  // DynRenderBuffer
  DynRenderBuffer *newDynRenderBuffer(const char *class_name, IMemAlloc *alloc) const override;
  void deleteDynRenderBuffer(DynRenderBuffer *&buf) const override;

  void dynRenderBufferClearBuf(DynRenderBuffer &buf) const override;
  void dynRenderBufferFlush(DynRenderBuffer &buf) const override;
  void dynRenderBufferFlushToBuffer(DynRenderBuffer &buf, TEXTUREID tid) const override;

  void dynRenderBufferDrawQuad(DynRenderBuffer &buf, const Point3 &p0, const Point3 &p1, const Point3 &p2, const Point3 &p3,
    E3DCOLOR color, float u, float v) const override;
  void dynRenderBufferDrawLine(DynRenderBuffer &buf, const Point3 &from, const Point3 &to, real width, E3DCOLOR color) const override;
  void dynRenderBufferDrawSquare(DynRenderBuffer &buf, const Point3 &p, real radius, E3DCOLOR color) const override;
  void dynRenderBufferDrawBox(DynRenderBuffer &buf, const TMatrix &tm, E3DCOLOR color) const override;
  void *dynRenderBufferDrawNetSurface(DynRenderBuffer &buf, int w, int h) const override;

  // textures
  TEXTUREID addManagedTexture(const char *name) const override;
  TEXTUREID registerManagedTex(const char *name, BaseTexture *basetex) const override;
  BaseTexture *acquireManagedTex(TEXTUREID id) const override;
  void releaseManagedTex(TEXTUREID id) const override;
  void releaseManagedResVerified(TEXTUREID &id, D3dResource *cmp) const override;

  // misc
  E3DCOLOR normalizeColor4(const Color4 &color, real &bright) const override;
  int getHdrMode() const override;

  // stdGuiRender
  void renderTextFmt(real x, real y, E3DCOLOR color, const char *format, const DagorSafeArg *arg, int anum) const override;
  BBox2 getTextBBox(const char *str, int len = -1) const override;
  int setTextFont(int font_id, int font_kern = 0) const override;
  void getFontAscentAndDescent(int &ascent, int &descent) const override;
  void drawSolidRectangle(real left, real top, real right, real bottom, E3DCOLOR color) const override;
};


class EcGeom : public IDagorGeom
{
public:
  // StaticGeometryContainer
  StaticGeometryContainer *newStaticGeometryContainer(IMemAlloc *alloc) override;
  void deleteStaticGeometryContainer(StaticGeometryContainer *&cont) override;
  void staticGeometryContainerClear(StaticGeometryContainer &cont) override;
  bool staticGeometryContainerLoadFromDag(StaticGeometryContainer &cont, const char *path, ILogWriter *log, bool use_not_found_tex,
    int load_flags) override;
  bool staticGeometryContainerImportDag(StaticGeometryContainer &cont, const char *src, const char *dest) const override;
  void staticGeometryContainerExportDag(StaticGeometryContainer &cont, const char *path, bool make_tex_path_local) const override;

  // GeomObject
  GeomObject *newGeomObject(IMemAlloc *alloc) override;
  void deleteGeomObject(GeomObject *&geom) override;

  void geomObjectSaveToDag(GeomObject &go, const char *path) const override;
  bool geomObjectLoadFromDag(GeomObject &go, const char *path, ILogWriter *log, ITextureNameResolver *resolv_cb) const override;
  void geomObjectClear(GeomObject &go) const override;
  void geomObjectEraseNode(GeomObject &go, int idx) const override;

  StaticGeometryContainer &geomObjectGetGeometryContainer(GeomObject &go) const override;
  const StaticGeometryContainer &geomObjectGetGeometryContainer(const GeomObject &go) const override;

  void geomObjectSetDefNodeVis(GeomObject &go) override;
  void geomObjectRecompile(GeomObject &go, const Bitarray *node_vis) override;
  void geomObjectCreateFromGeomContainer(GeomObject &go, const StaticGeometryContainer &cont, bool do_recompile) override;

  void geomObjectRender(GeomObject &go) override;
  void geomObjectRenderColor(GeomObject &go, DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only) const override;
  void geomObjectRenderNodeColor(GeomObject &go, int idx, DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only) const override;
  void geomObjectRenderTrans(GeomObject &go) override;
  void geomObjectRenderEdges(GeomObject &go, bool is_visible, E3DCOLOR color) override;

  BBox3 geomObjectGetBoundBox(GeomObject &go, bool local_coord) const override;
  BBox3 geomObjectGetNodeBoundBox(const GeomObject &go, int idx, bool local_coord) const override;

  const char *geomObjectGetShaderName(const GeomObject &go, int idx) const override;
  ShaderMesh *geomObjectGetShaderMesh(const GeomObject &go, int idx) const override;

  bool geomObjectShadowRayHitTest(GeomObject &go, const Point3 &p, const Point3 &dir, real maxt, int trace_flags) const override;
  bool geomObjectTraceRay(GeomObject &go, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const override;
  bool geomObjectReloadRayTracer(GeomObject &go) const override;
  StaticSceneRayTracer *geomObjectGetRayTracer(GeomObject &go) const override;

  // StaticGeometryNode
  StaticGeometryNode *newStaticGeometryNode(IMemAlloc *alloc) override;
  StaticGeometryNode *newStaticGeometryNode(const StaticGeometryNode &from, IMemAlloc *alloc) override;
  void deleteStaticGeometryNode(StaticGeometryNode *&node) override;
  bool staticGeometryNodeHaveBillboardMat(const StaticGeometryNode &node) const override;
  void staticGeometryNodeCalcBoundBox(StaticGeometryNode &node) const override;
  void staticGeometryNodeCalcBoundSphere(StaticGeometryNode &node) const override;
  void staticGeometryNodeSetMaterialLighting(const StaticGeometryNode &node, StaticGeometryMaterial &mat) const override;

  const char *staticGeometryNodeLightingToStr(StaticGeometryNode::Lighting light) const override;
  StaticGeometryNode::Lighting staticGeometryNodeStrToLighting(const char *light) const override;

  shaders::OverrideStateId create(const shaders::OverrideState &os) const override { return shaders::overrides::create(os); }
  bool destroy(shaders::OverrideStateId &override_id) const override { return shaders::overrides::destroy(override_id); }
  bool set(shaders::OverrideStateId override_id) const override { return shaders::overrides::set(override_id); }
  bool reset_override() const override { return shaders::overrides::reset(); }

  // StaticGeometryMesh
  StaticGeometryMesh *newStaticGeometryMesh(IMemAlloc *alloc) const override;
  void deleteStaticGeometryMesh(StaticGeometryMesh *&mesh) const override;

  // GeomResourcesHelper
  GeomResourcesHelper *newGeomResourcesHelper(IStaticGeomResourcesService *svc, IMemAlloc *alloc) const override;
  void deleteGeomResourcesHelper(GeomResourcesHelper *&helper) const override;

  void geomResourcesHelperSetResourcesService(GeomResourcesHelper &helper, IStaticGeomResourcesService *svc) const override;
  void geomResourcesHelperCreateResources(GeomResourcesHelper &helper, const void *obj_id, const TMatrix &tm,
    const StaticGeometryContainer &cont, ILogWriter *log) const override;
  void geomResourcesHelperFreeResources(GeomResourcesHelper &helper, const void *obj_id) const override;
  void geomResourcesHelperSetResourcesTm(GeomResourcesHelper &helper, const void *obj_id, const TMatrix &tm) const override;
  void geomResourcesHelperRemapResources(GeomResourcesHelper &helper, const void *obj_id_old, const void *obj_id_new) const override;
  int geomResourcesHelperCompact(GeomResourcesHelper &helper) const override;

  // Shaders
  ShaderMaterial *newShaderMaterial(MaterialData &m) const override;

  int getShaderVariableId(const char *name) const override;
  const char *getShaderVariableName(int id) const override;

  bool shaderGlobalSetInt(int id, int val) const override;
  bool shaderGlobalSetReal(int id, real val) const override;
  bool shaderGlobalSetColor4(int id, const Color4 &val) const override;
  bool shaderGlobalSetTexture(int id, TEXTUREID val) const override;
  bool shaderGlobalSetSampler(int id, d3d::SamplerHandle val) const override;
  d3d::SamplerHandle getSeparateSampler(TEXTUREID val) const override;

  int shaderGlobalGetInt(int id) const override;
  real shaderGlobalGetReal(int id) const override;
  Color4 shaderGlobalGetColor4(int id) const override;
  TEXTUREID shaderGlobalGetTexture(int id) const override;

  void shaderGlobalSetVarsFromBlk(const DataBlock &blk) const override;

  int shaderGlobalGetBlockId(const char *name) const override;
  void shaderGlobalSetBlock(int id, int layer = -1) const override;

  void shaderElemInvalidateCachedStateBlock() const override;

  // Mesh
  Mesh *newMesh(IMemAlloc *alloc) const override;
  void deleteMesh(Mesh *&mesh) const override;

  int meshOptimizeTverts(Mesh &mesh) const override;
  int meshCalcNgr(Mesh &mesh) const override;
  int meshCalcVertnorms(Mesh &mesh) const override;
  void meshFlipNormals(Mesh &mesh, int f0, int nf) const override;

  PostFxRenderer *newPostFxRenderer() const override;
  void deletePostFxRenderer(PostFxRenderer *&) const override;
  void postFxRendererInit(PostFxRenderer &p, const char *shader_name) const override;
  void postFxRendererRender(PostFxRenderer &p) const override;

  // ObjCreator3d
  bool generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom) const override;
  bool generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom, StaticGeometryMaterial *material) const override;

  bool generateBox(const IPoint3 &segments, StaticGeometryContainer &geom) const override;

  bool generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom) const override;

  bool generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom) const override;

  bool generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom) const override;
  bool generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom) const override;
  bool generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom) const override;

  bool generateBoxStair(int steps, StaticGeometryContainer &geom) const override;
  bool generateClosedStair(int steps, StaticGeometryContainer &geom) const override;
  bool generateOpenStair(int steps, real h, StaticGeometryContainer &geom) const override;

  bool objCreator3dAddNode(const char *name, Mesh *mesh, MaterialDataList *material, StaticGeometryContainer &geom) const override;

  // Creators
  BoxCreator *newBoxCreator(IMemAlloc *alloc) const override;
  void deleteBoxCreator(BoxCreator *&creator) const override;

  SphereCreator *newSphereCreator(IMemAlloc *alloc) const override;
  void deleteSphereCreator(SphereCreator *&creator) const override;

  PlaneCreator *newPlaneCreator(IMemAlloc *alloc) const override;
  void deletePlaneCreator(PlaneCreator *&creator) const override;

  PointCreator *newPointCreator(IMemAlloc *alloc = NULL) const override;
  void deletePointCreator(PointCreator *&creator) const override;

  CylinderCreator *newCylinderCreator(IMemAlloc *alloc) const override;
  void deleteCylinderCreator(CylinderCreator *&creator) const override;

  PolyMeshCreator *newPolyMeshCreator(IMemAlloc *alloc) const override;
  void deletePolyMeshCreator(PolyMeshCreator *&creator) const override;

  StairCreator *newStairCreator(IMemAlloc *alloc) const override;
  void deleteStairCreator(StairCreator *&creator) const override;

  SpiralStairCreator *newSpiralStairCreator(IMemAlloc *alloc) const override;
  void deleteSpiralStairCreator(SpiralStairCreator *&creator) const override;

  SplineCreator *newSplineCreator(IMemAlloc *alloc) const override;
  void deleteSplineCreator(SplineCreator *&creator) const override;

  TargetCreator *newTargetCreator(IMemAlloc *alloc) const override;
  void deleteTargetCreator(TargetCreator *&creator) const override;

  PolygoneZoneCreator *newPolygonZoneCreator(IMemAlloc *alloc) const override;
  void deletePolyZoneCreator(PolygoneZoneCreator *&creator) const override;

  void deleteIObjectCreator(IObjectCreator *&creator) const override;

  // DagSaver
  DagSaver *newDagSaver(IMemAlloc *alloc) const override;
  void deleteDagSaver(DagSaver *&saver) const override;

  bool dagSaverStartSaveDag(DagSaver &saver, const char *path) const override;
  bool dagSaverEndSaveDag(DagSaver &saver) const override;

  bool dagSaverStartSaveNodes(DagSaver &saver) const override;
  bool dagSaverEndSaveNodes(DagSaver &saver) const override;

  bool dagSaverStartSaveNode(DagSaver &saver, const char *name, const TMatrix &wtm, int flg, int children) const override;
  bool dagSaverEndSaveNode(DagSaver &saver) const override;

  bool dagSaverSaveDagSpline(DagSaver &saver, DagSpline **spline, int cnt) const override;

  // AScene
  AScene *newAScene(IMemAlloc *alloc) const override;
  void deleteAScene(AScene *&scene) const override;

  int loadAscene(const char *fn, AScene &sc, int flg, bool fatal_on_error) const override;

  // Node
  void nodeCalcWtm(Node &node) const override;
  bool nodeEnumNodes(Node &node, Node::NodeEnumCB &cb, Node **res) const override;
};


class EcConsole : public IDagorConsole
{
public:
  void startProgress(CoolConsole &con) const override;
  void endProgress(CoolConsole &con) const override;

  void addMessageFmt(CoolConsole &con, ILogWriter::MessageType type, const char *msg, const DagorSafeArg *arg,
    int anum) const override;

  void showConsole(CoolConsole &con, bool activate) const override;
  void hideConsole(const CoolConsole &con) const override;

  bool registerCommand(CoolConsole &con, const char *cmd, IConsoleCmd *handler) const override;
  bool unregisterCommand(CoolConsole &con, const char *cmd, IConsoleCmd *handler) const override;
};


class EcInput : public IDagorInput
{
public:
  // input functions to use from DLLs (see ec_input.h for non-DLL usage)
  bool isKeyDown(ImGuiKey key) const override;
  bool isAltKeyDown() const override;
  bool isCtrlKeyDown() const override;
  bool isShiftKeyDown() const override;
};


class EcTools : public IDagorTools
{
public:
  // ddstexture::Converter
  bool ddsConvertImage(ddstexture::Converter &converter, IGenSave &cb, TexPixel32 *pixels, int width, int height,
    int stride) const override;

  // loadmask::
  bool loadmaskloadMaskFromFile(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h) const override;

  // AnimV20::AnimData
  AnimV20::AnimData *newAnimData(IMemAlloc *alloc) const override;
  bool animDataLoad(AnimV20::AnimData &anim, IGenLoad &cb, IMemAlloc *alloc) const override;

  // other
  unsigned getSimpleHash(const char *s, unsigned int init_val) const override;

  void *win32GetMainWnd() const override;

  // time functions
  int getTimeMsec() const override;
  __int64 refTimeUsec() const override;

  // ddsx functions
  void ddsxShutdownPlugins() override;
  bool ddsxConvertDds(unsigned targ_code, ddsx::Buffer &dest, void *dds_data, int dds_len, const ddsx::ConvertParams &p) override;
  const char *ddsxGetLastErrorText() override;
  void ddsxFreeBuffer(ddsx::Buffer &b) override;

  // files
  bool copyFile(const char *src, const char *dest, bool overwrite) const override;
  bool compareFile(const char *path1, const char *path2) const override;
};


class EcScene : public IDagorScene
{
public:
  // StaticSceneRayTracer
  int staticSceneRayTracerTraceRay(StaticSceneRayTracer &rt, const Point3 &p, const Point3 &wdir2, real &mint2,
    int from_face) const override;

  // FastRtDump
  int fastRtDumpTraceRay(FastRtDump &frt, int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid) const override;

  // BuildableStaticSceneRayTracer
  BuildableStaticSceneRayTracer *createBuildableStaticmeshsceneRaytracer(const Point3 &lsz, int lev) const override;
  bool buildableStaticSceneRayTracerAddmesh(BuildableStaticSceneRayTracer &rt, const Point3 *vert, int vcount, const unsigned *face,
    unsigned stride, int fn, const unsigned *face_flags, bool rebuild) const override;
  bool buildableStaticSceneRayTracerReserve(BuildableStaticSceneRayTracer &rt, int face_count, int vert_count) const override;
  bool buildableStaticSceneRayTracerRebuild(BuildableStaticSceneRayTracer &rt) const override;

  // StaticSceneBuilder::StdTonemapper
  StaticSceneBuilder::StdTonemapper *newStdTonemapper(IMemAlloc *alloc) const override;
  StaticSceneBuilder::StdTonemapper *newStdTonemapper(const StaticSceneBuilder::StdTonemapper &from, IMemAlloc *alloc) const override;
  void deleteStdTonemapper(StaticSceneBuilder::StdTonemapper *&mapper) const override;

  void stdTonemapperRecalc(StaticSceneBuilder::StdTonemapper &mapper) const override;
  void stdTonemapperSave(StaticSceneBuilder::StdTonemapper &mapper, DataBlock &blk) const override;
  void stdTonemapperLoad(StaticSceneBuilder::StdTonemapper &mapper, const DataBlock &blk) const override;

  // resources
  GameResource *getGameResource(GameResHandle handle, bool no_factory_fatal = true) const override;
  void releaseGameResource(GameResource *resource) const override;
};


class EditorCoreImpl : public IEditorCore
{
public:
  IDagorRender *getRender() override { return &render; }
  IDagorGeom *getGeom() override { return &geom; }
  IDagorConsole *getConsole() override { return &console; }
  IDagorInput *getInput() override { return &input; }
  IDagorTools *getTools() override { return &tools; }
  IDagorScene *getScene() override { return &scene; }
  const char *getExePath() override;

private:
  EcRender render;
  EcGeom geom;
  EcConsole console;
  EcInput input;
  EcTools tools;
  EcScene scene;
};
