#pragma once

#include <EditorCore/ec_IEditorCore.h>
#include <shaders/dag_overrideStates.h>

class EcRender : public IDagorRender
{
public:
  // driver
  virtual void fillD3dInterfaceTable(D3dInterfaceTable &d3dit) const;

  // driver objects
  virtual DagorCurView &curView() const;

  // wire render
  virtual void startLinesRender(bool test_z, bool write_z, bool z_func_less = false) const;
  virtual void setLinesTm(const TMatrix &tm) const;
  virtual void endLinesRender() const;

  virtual void renderLine(const Point3 &p0, const Point3 &p1, E3DCOLOR color) const;
  virtual void renderLine(const Point3 *points, int count, E3DCOLOR color) const;
  virtual void renderBox(const BBox3 &box, E3DCOLOR color) const;
  virtual void renderBox(const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color) const;
  virtual void renderSphere(const Point3 &center, real rad, E3DCOLOR col, int segs) const;
  virtual void renderCircle(const Point3 &center, const Point3 &ax1, const Point3 &ax2, real radius, E3DCOLOR col, int segs) const;
  virtual void renderXZCircle(const Point3 &center, real radius, E3DCOLOR col, int segs) const;
  virtual void renderCapsuleW(const Capsule &cap, E3DCOLOR c) const;

  virtual DebugPrimitivesVbuffer *newDebugPrimitivesVbuffer(const char *name, IMemAlloc *alloc = NULL) const;
  virtual void deleteDebugPrimitivesVbuffer(DebugPrimitivesVbuffer *&vbuf) const;
  virtual void beginDebugLinesCacheToVbuffer(DebugPrimitivesVbuffer &vbuf) const;
  virtual void endDebugLinesCacheToVbuffer(DebugPrimitivesVbuffer &vbuf) const;
  virtual void invalidateDebugPrimitivesVbuffer(DebugPrimitivesVbuffer &vbuf) const;
  virtual void renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf) const;
  virtual void renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf, E3DCOLOR c) const;
  virtual void renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf, bool z_test, bool z_write, bool z_func_less,
    Color4 color_multiplier) const;
  virtual void addLineToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p0, const Point3 &p1, E3DCOLOR c) const;
  virtual void addHatchedBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const TMatrix &box_tm, float hatching_step, E3DCOLOR color) const;
  virtual void addBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az,
    E3DCOLOR color) const;
  virtual void addSolidBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p, const Point3 &ax, const Point3 &ay,
    const Point3 &az, E3DCOLOR color) const;
  virtual void addBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const BBox3 &box, E3DCOLOR color) const;
  virtual void addTriangleToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 p[3], E3DCOLOR color) const;
  virtual bool isLinesVbufferValid(DebugPrimitivesVbuffer &vbuf) const;
  virtual void setVbufferTm(DebugPrimitivesVbuffer &vbuf, const TMatrix &tm) const;

  // DynamicShadersBuffer
  virtual DynamicShadersBuffer *newDynamicShadersBuffer(IMemAlloc *alloc) const;
  virtual DynamicShadersBuffer *newDynamicShadersBuffer(CompiledShaderChannelId *channels, int channel_count, int max_verts,
    int max_faces, IMemAlloc *alloc) const;
  virtual void deleteDynamicShadersBuffer(DynamicShadersBuffer *&buf) const;

  virtual void dynShaderBufferSetCurrentShader(DynamicShadersBuffer &buf, ShaderElement *shader) const;
  virtual void dynShaderBufferAddFaces(DynamicShadersBuffer &buf, const void *vertex_data, int num_verts, const int *indices,
    int num_faces) const;
  virtual void dynShaderBufferFlush(DynamicShadersBuffer &buf) const;

  // DynRenderBuffer
  virtual DynRenderBuffer *newDynRenderBuffer(const char *class_name, IMemAlloc *alloc) const;
  virtual void deleteDynRenderBuffer(DynRenderBuffer *&buf) const;

  virtual void dynRenderBufferClearBuf(DynRenderBuffer &buf) const;
  virtual void dynRenderBufferFlush(DynRenderBuffer &buf) const;
  virtual void dynRenderBufferFlushToBuffer(DynRenderBuffer &buf, TEXTUREID tid) const;

  virtual void dynRenderBufferDrawQuad(DynRenderBuffer &buf, const Point3 &p0, const Point3 &p1, const Point3 &p2, const Point3 &p3,
    E3DCOLOR color, float u, float v) const;
  virtual void dynRenderBufferDrawLine(DynRenderBuffer &buf, const Point3 &from, const Point3 &to, real width, E3DCOLOR color) const;
  virtual void dynRenderBufferDrawSquare(DynRenderBuffer &buf, const Point3 &p, real radius, E3DCOLOR color) const;
  virtual void dynRenderBufferDrawBox(DynRenderBuffer &buf, const TMatrix &tm, E3DCOLOR color) const;
  virtual void *dynRenderBufferDrawNetSurface(DynRenderBuffer &buf, int w, int h) const;

  // textures
  virtual TEXTUREID addManagedTexture(const char *name) const;
  virtual TEXTUREID registerManagedTex(const char *name, BaseTexture *basetex) const;
  virtual BaseTexture *acquireManagedTex(TEXTUREID id) const;
  virtual void releaseManagedTex(TEXTUREID id) const;
  virtual void releaseManagedResVerified(TEXTUREID &id, D3dResource *cmp) const;

  // misc
  virtual E3DCOLOR normalizeColor4(const Color4 &color, real &bright) const;
  virtual int getHdrMode() const;

  // stdGuiRender
  virtual void renderTextFmt(real x, real y, E3DCOLOR color, const char *format, const DagorSafeArg *arg, int anum) const;
  virtual BBox2 getTextBBox(const char *str, int len = -1) const;
  virtual int setTextFont(int font_id, int font_kern = 0) const;
};


class EcGeom : public IDagorGeom
{
public:
  // StaticGeometryContainer
  virtual StaticGeometryContainer *newStaticGeometryContainer(IMemAlloc *alloc);
  virtual void deleteStaticGeometryContainer(StaticGeometryContainer *&cont);
  virtual void staticGeometryContainerClear(StaticGeometryContainer &cont);
  virtual bool staticGeometryContainerLoadFromDag(StaticGeometryContainer &cont, const char *path, ILogWriter *log,
    bool use_not_found_tex, int load_flags);
  virtual bool staticGeometryContainerImportDag(StaticGeometryContainer &cont, const char *src, const char *dest) const;
  virtual void staticGeometryContainerExportDag(StaticGeometryContainer &cont, const char *path, bool make_tex_path_local) const;

  // GeomObject
  virtual GeomObject *newGeomObject(IMemAlloc *alloc);
  virtual void deleteGeomObject(GeomObject *&geom);

  virtual void geomObjectSaveToDag(GeomObject &go, const char *path) const;
  virtual bool geomObjectLoadFromDag(GeomObject &go, const char *path, ILogWriter *log, ITextureNameResolver *resolv_cb) const;
  virtual void geomObjectClear(GeomObject &go) const;
  virtual void geomObjectEraseNode(GeomObject &go, int idx) const;

  virtual StaticGeometryContainer &geomObjectGetGeometryContainer(GeomObject &go) const;
  virtual const StaticGeometryContainer &geomObjectGetGeometryContainer(const GeomObject &go) const;

  virtual void geomObjectSetDefNodeVis(GeomObject &go);
  virtual void geomObjectRecompile(GeomObject &go, const Bitarray *node_vis);
  virtual void geomObjectCreateFromGeomContainer(GeomObject &go, const StaticGeometryContainer &cont, bool do_recompile);

  virtual void geomObjectRender(GeomObject &go);
  virtual void geomObjectRenderColor(GeomObject &go, DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only) const;
  virtual void geomObjectRenderNodeColor(GeomObject &go, int idx, DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only) const;
  virtual void geomObjectRenderTrans(GeomObject &go);
  virtual void geomObjectRenderEdges(GeomObject &go, bool is_visible, E3DCOLOR color);

  virtual BBox3 geomObjectGetBoundBox(GeomObject &go, bool local_coord) const;
  virtual BBox3 geomObjectGetNodeBoundBox(const GeomObject &go, int idx, bool local_coord) const;

  virtual const char *geomObjectGetShaderName(const GeomObject &go, int idx) const;
  virtual ShaderMesh *geomObjectGetShaderMesh(const GeomObject &go, int idx) const;

  virtual bool geomObjectShadowRayHitTest(GeomObject &go, const Point3 &p, const Point3 &dir, real maxt, int trace_flags) const;
  virtual bool geomObjectTraceRay(GeomObject &go, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const;
  virtual bool geomObjectReloadRayTracer(GeomObject &go) const;
  virtual StaticSceneRayTracer *geomObjectGetRayTracer(GeomObject &go) const;

  // StaticGeometryNode
  virtual StaticGeometryNode *newStaticGeometryNode(IMemAlloc *alloc);
  virtual StaticGeometryNode *newStaticGeometryNode(const StaticGeometryNode &from, IMemAlloc *alloc);
  virtual void deleteStaticGeometryNode(StaticGeometryNode *&node);
  virtual bool staticGeometryNodeHaveBillboardMat(const StaticGeometryNode &node) const;
  virtual void staticGeometryNodeCalcBoundBox(StaticGeometryNode &node) const;
  virtual void staticGeometryNodeCalcBoundSphere(StaticGeometryNode &node) const;
  virtual void staticGeometryNodeSetMaterialLighting(const StaticGeometryNode &node, StaticGeometryMaterial &mat) const;

  virtual const char *staticGeometryNodeLightingToStr(StaticGeometryNode::Lighting light) const;
  virtual StaticGeometryNode::Lighting staticGeometryNodeStrToLighting(const char *light) const;

  virtual shaders::OverrideStateId create(const shaders::OverrideState &os) const { return shaders::overrides::create(os); }
  virtual bool destroy(shaders::OverrideStateId &override_id) const { return shaders::overrides::destroy(override_id); }
  virtual bool set(shaders::OverrideStateId override_id) const { return shaders::overrides::set(override_id); }
  virtual bool reset_override() const { return shaders::overrides::reset(); }

  // StaticGeometryMesh
  virtual StaticGeometryMesh *newStaticGeometryMesh(IMemAlloc *alloc) const;
  virtual void deleteStaticGeometryMesh(StaticGeometryMesh *&mesh) const;

  // GeomResourcesHelper
  virtual GeomResourcesHelper *newGeomResourcesHelper(IStaticGeomResourcesService *svc, IMemAlloc *alloc) const;
  virtual void deleteGeomResourcesHelper(GeomResourcesHelper *&helper) const;

  virtual void geomResourcesHelperSetResourcesService(GeomResourcesHelper &helper, IStaticGeomResourcesService *svc) const;
  virtual void geomResourcesHelperCreateResources(GeomResourcesHelper &helper, const void *obj_id, const TMatrix &tm,
    const StaticGeometryContainer &cont, ILogWriter *log) const;
  virtual void geomResourcesHelperFreeResources(GeomResourcesHelper &helper, const void *obj_id) const;
  virtual void geomResourcesHelperSetResourcesTm(GeomResourcesHelper &helper, const void *obj_id, const TMatrix &tm) const;
  virtual void geomResourcesHelperRemapResources(GeomResourcesHelper &helper, const void *obj_id_old, const void *obj_id_new) const;
  virtual int geomResourcesHelperCompact(GeomResourcesHelper &helper) const;

  // Shaders
  virtual ShaderMaterial *newShaderMaterial(MaterialData &m) const;

  virtual int getShaderVariableId(const char *name) const;
  virtual const char *getShaderVariableName(int id) const;

  virtual bool shaderGlobalSetInt(int id, int val) const;
  virtual bool shaderGlobalSetReal(int id, real val) const;
  virtual bool shaderGlobalSetColor4(int id, const Color4 &val) const;
  virtual bool shaderGlobalSetTexture(int id, TEXTUREID val) const;

  virtual int shaderGlobalGetInt(int id) const;
  virtual real shaderGlobalGetReal(int id) const;
  virtual Color4 shaderGlobalGetColor4(int id) const;
  virtual TEXTUREID shaderGlobalGetTexture(int id) const;

  virtual void shaderGlobalSetVarsFromBlk(const DataBlock &blk) const;

  virtual int shaderGlobalGetBlockId(const char *name) const;
  virtual void shaderGlobalSetBlock(int id, int layer = -1) const;

  virtual void shaderElemInvalidateCachedStateBlock() const;

  // Mesh
  virtual Mesh *newMesh(IMemAlloc *alloc) const;
  virtual void deleteMesh(Mesh *&mesh) const;

  virtual int meshOptimizeTverts(Mesh &mesh) const;
  virtual int meshCalcNgr(Mesh &mesh) const;
  virtual int meshCalcVertnorms(Mesh &mesh) const;
  virtual void meshFlipNormals(Mesh &mesh, int f0, int nf) const;

  virtual PostFxRenderer *newPostFxRenderer() const;
  virtual void deletePostFxRenderer(PostFxRenderer *&) const;
  virtual void postFxRendererInit(PostFxRenderer &p, const char *shader_name) const;
  virtual void postFxRendererRender(PostFxRenderer &p) const;

  // ObjCreator3d
  virtual bool generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom) const;
  virtual bool generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom, StaticGeometryMaterial *material) const;

  virtual bool generateBox(const IPoint3 &segments, StaticGeometryContainer &geom) const;

  virtual bool generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom) const;

  virtual bool generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom) const;

  virtual bool generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom) const;
  virtual bool generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom) const;
  virtual bool generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom) const;

  virtual bool generateBoxStair(int steps, StaticGeometryContainer &geom) const;
  virtual bool generateClosedStair(int steps, StaticGeometryContainer &geom) const;
  virtual bool generateOpenStair(int steps, real h, StaticGeometryContainer &geom) const;

  virtual bool objCreator3dAddNode(const char *name, Mesh *mesh, MaterialDataList *material, StaticGeometryContainer &geom) const;

  // Creators
  virtual BoxCreator *newBoxCreator(IMemAlloc *alloc) const;
  virtual void deleteBoxCreator(BoxCreator *&creator) const;

  virtual SphereCreator *newSphereCreator(IMemAlloc *alloc) const;
  virtual void deleteSphereCreator(SphereCreator *&creator) const;

  virtual PlaneCreator *newPlaneCreator(IMemAlloc *alloc) const;
  virtual void deletePlaneCreator(PlaneCreator *&creator) const;

  virtual PointCreator *newPointCreator(IMemAlloc *alloc = NULL) const;
  virtual void deletePointCreator(PointCreator *&creator) const;

  virtual CylinderCreator *newCylinderCreator(IMemAlloc *alloc) const;
  virtual void deleteCylinderCreator(CylinderCreator *&creator) const;

  virtual PolyMeshCreator *newPolyMeshCreator(IMemAlloc *alloc) const;
  virtual void deletePolyMeshCreator(PolyMeshCreator *&creator) const;

  virtual StairCreator *newStairCreator(IMemAlloc *alloc) const;
  virtual void deleteStairCreator(StairCreator *&creator) const;

  virtual SpiralStairCreator *newSpiralStairCreator(IMemAlloc *alloc) const;
  virtual void deleteSpiralStairCreator(SpiralStairCreator *&creator) const;

  virtual SplineCreator *newSplineCreator(IMemAlloc *alloc) const;
  virtual void deleteSplineCreator(SplineCreator *&creator) const;

  virtual TargetCreator *newTargetCreator(IMemAlloc *alloc) const;
  virtual void deleteTargetCreator(TargetCreator *&creator) const;

  virtual void deleteIObjectCreator(IObjectCreator *&creator) const;

  // DagSaver
  virtual DagSaver *newDagSaver(IMemAlloc *alloc) const;
  virtual void deleteDagSaver(DagSaver *&saver) const;

  virtual bool dagSaverStartSaveDag(DagSaver &saver, const char *path) const;
  virtual bool dagSaverEndSaveDag(DagSaver &saver) const;

  virtual bool dagSaverStartSaveNodes(DagSaver &saver) const;
  virtual bool dagSaverEndSaveNodes(DagSaver &saver) const;

  virtual bool dagSaverStartSaveNode(DagSaver &saver, const char *name, const TMatrix &wtm, int flg, int children) const;
  virtual bool dagSaverEndSaveNode(DagSaver &saver) const;

  virtual bool dagSaverSaveDagSpline(DagSaver &saver, DagSpline **spline, int cnt) const;

  // AScene
  virtual AScene *newAScene(IMemAlloc *alloc) const;
  virtual void deleteAScene(AScene *&scene) const;

  virtual int loadAscene(const char *fn, AScene &sc, int flg, bool fatal_on_error) const;

  // Node
  virtual void nodeCalcWtm(Node &node) const;
  virtual bool nodeEnumNodes(Node &node, Node::NodeEnumCB &cb, Node **res) const;
};


class EcConsole : public IDagorConsole
{
public:
  virtual void startProgress(CoolConsole &con) const;
  virtual void endProgress(CoolConsole &con) const;

  virtual void addMessageFmt(CoolConsole &con, ILogWriter::MessageType type, const char *msg, const DagorSafeArg *arg, int anum) const;

  virtual void showConsole(CoolConsole &con, bool activate) const;
  virtual void hideConsole(const CoolConsole &con) const;

  virtual bool registerCommand(CoolConsole &con, const char *cmd, IConsoleCmd *handler) const;
  virtual bool unregisterCommand(CoolConsole &con, const char *cmd, IConsoleCmd *handler) const;
};


class EcTools : public IDagorTools
{
public:
  // ddstexture::Converter
  virtual bool ddsConvertImage(ddstexture::Converter &converter, IGenSave &cb, TexPixel32 *pixels, int width, int height,
    int stride) const;

  // loadmask::
  virtual bool loadmaskloadMaskFromFile(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h) const;

  // AnimV20::AnimData
  virtual AnimV20::AnimData *newAnimData(IMemAlloc *alloc) const;
  virtual bool animDataLoad(AnimV20::AnimData &anim, IGenLoad &cb, IMemAlloc *alloc) const;

  // other
  virtual unsigned getSimpleHash(const char *s, unsigned int init_val) const;

  virtual void *win32GetMainWnd() const;

  // time functions
  virtual int getTimeMsec() const;
  virtual __int64 refTimeUsec() const;

  // ddsx functions
  virtual void ddsxShutdownPlugins();
  virtual bool ddsxConvertDds(unsigned targ_code, ddsx::Buffer &dest, void *dds_data, int dds_len, const ddsx::ConvertParams &p);
  virtual const char *ddsxGetLastErrorText();
  virtual void ddsxFreeBuffer(ddsx::Buffer &b);

  // files
  virtual bool copyFile(const char *src, const char *dest, bool overwrite) const;
  virtual bool compareFile(const char *path1, const char *path2) const;
};


class EcScene : public IDagorScene
{
public:
  // StaticSceneRayTracer
  virtual int staticSceneRayTracerTraceRay(StaticSceneRayTracer &rt, const Point3 &p, const Point3 &wdir2, real &mint2,
    int from_face) const;

  // FastRtDump
  virtual int fastRtDumpTraceRay(FastRtDump &frt, int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid) const;

  // BuildableStaticSceneRayTracer
  virtual BuildableStaticSceneRayTracer *createBuildableStaticmeshsceneRaytracer(const Point3 &lsz, int lev) const;
  virtual bool buildableStaticSceneRayTracerAddmesh(BuildableStaticSceneRayTracer &rt, const Point3 *vert, int vcount,
    const unsigned *face, unsigned stride, int fn, const unsigned *face_flags, bool rebuild) const;
  virtual bool buildableStaticSceneRayTracerReserve(BuildableStaticSceneRayTracer &rt, int face_count, int vert_count) const;
  virtual bool buildableStaticSceneRayTracerRebuild(BuildableStaticSceneRayTracer &rt) const;

  // StaticSceneBuilder::StdTonemapper
  virtual StaticSceneBuilder::StdTonemapper *newStdTonemapper(IMemAlloc *alloc) const;
  virtual StaticSceneBuilder::StdTonemapper *newStdTonemapper(const StaticSceneBuilder::StdTonemapper &from, IMemAlloc *alloc) const;
  virtual void deleteStdTonemapper(StaticSceneBuilder::StdTonemapper *&mapper) const;

  virtual void stdTonemapperRecalc(StaticSceneBuilder::StdTonemapper &mapper) const;
  virtual void stdTonemapperSave(StaticSceneBuilder::StdTonemapper &mapper, DataBlock &blk) const;
  virtual void stdTonemapperLoad(StaticSceneBuilder::StdTonemapper &mapper, const DataBlock &blk) const;

  // resources
  virtual GameResource *getGameResource(GameResHandle handle, bool no_factory_fatal = true) const;
  virtual void releaseGameResource(GameResource *resource) const;
};


class EditorCoreImpl : public IEditorCore
{
public:
  IDagorRender *getRender() override { return &render; }
  IDagorGeom *getGeom() override { return &geom; }
  IDagorConsole *getConsole() override { return &console; }
  IDagorTools *getTools() override { return &tools; }
  IDagorScene *getScene() override { return &scene; }
  const char *getExePath() override;

private:
  EcRender render;
  EcGeom geom;
  EcConsole console;
  EcTools tools;
  EcScene scene;
};
