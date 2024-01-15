#ifndef __GAIJIN_IEDITOR_CORE__
#define __GAIJIN_IEDITOR_CORE__
#pragma once


#include <EditorCore/ec_interface.h>

#include <math/dag_e3dColor.h>
#include <math/dag_TMatrix.h>
#include <math/dag_bounds3.h>

#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <util/dag_safeArg.h>

#include <generic/dag_tab.h>
#include <generic/dag_smallTab.h>

#include <3d/dag_texMgr.h>

#include <libTools/dagFileRW/dagFileNode.h>
#include <libTools/dagFileRW/dagFileFormat.h>

#include <libTools/util/iLogWriter.h>
#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/dagFileRW/textureNameResolver.h>

#include <libTools/dtx/dtx.h>

#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_direct.h>
#include <gameRes/dag_gameResources.h>
#include <shaders/dag_overrideStateId.h>
#include <sceneRay/dag_sceneRayDecl.h>

#include <stdarg.h>


struct D3dInterfaceTable;
class StaticGeometryContainer;
class StaticGeometryNode;
class GeomObject;
class ILogWriter;
class Bitarray;
class CoolConsole;
class DataBlock;
class DynamicShadersBuffer;
class ShaderElement;
class ShaderMaterial;
class MaterialData;
class BaseTexture;
class BaseStreamingSceneHolder;
class BinaryDump;
class IBinaryDumpLoaderClient;
class ExecutionScheduler;
class ObjectEditor;
class DynRenderBuffer;
class StaticGeometryMesh;
class Mesh;
class BoxCreator;
class SphereCreator;
class PlaneCreator;
class PointCreator;
class CylinderCreator;
class PolyMeshCreator;
class StairCreator;
class SpiralStairCreator;
class SplineCreator;
class TargetCreator;
class IObjectCreator;
class ShaderMesh;
class PostFxRenderer;
class GlobalVertexData;
class FullFileSaveCB;
class FullFileLoadCB;
class LFileGeneralLoadCB;
class LFileGeneralSaveCB;
class GeomScene;
class IBinaryLTinputLoad;
class DagSaver;
class DagSpline;
class HdrRender;
class IBaseLoad;
class MemGeneralLoadCB;
class InPlaceMemLoadCB;
class IStaticGeomResourcesService;
class GeomResourcesHelper;
class DynamicMemGeneralSaveCB;
class IConsoleCmd;
class FastRtDump;
class DebugPrimitivesVbuffer;

struct CompiledShaderChannelId;
struct DagorCurView;
struct DagorCurFog;
struct Occluder;
struct Capsule;
struct IScriptObjectManager;
struct IScriptObjectManager2;
struct TexPixel8a;
struct TexImage32;
struct TexImage8;
struct DagUUID;

namespace ddsx
{
struct Buffer;
struct ConvertParams;
} // namespace ddsx
namespace StaticSceneBuilder
{
class StdTonemapper;
}
namespace AnimV20
{
class AnimData;
}

class IDagorRender
{
public:
  // driver
  virtual void fillD3dInterfaceTable(D3dInterfaceTable &d3dit) const = 0;

  // driver objects
  virtual DagorCurView &curView() const = 0;

  // wire render
  virtual void startLinesRender(bool test_z = true, bool write_z = true, bool z_func_less = false) const = 0;
  virtual void setLinesTm(const TMatrix &tm) const = 0;
  virtual void endLinesRender() const = 0;

  virtual void renderLine(const Point3 &p0, const Point3 &p1, E3DCOLOR color) const = 0;
  virtual void renderLine(const Point3 *points, int count, E3DCOLOR color) const = 0;
  virtual void renderBox(const BBox3 &box, E3DCOLOR color) const = 0;
  virtual void renderBox(const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az, E3DCOLOR color) const = 0;
  virtual void renderSphere(const Point3 &center, real rad, E3DCOLOR col, int segs = 24) const = 0;
  virtual void renderCircle(const Point3 &center, const Point3 &ax1, const Point3 &ax2, real radius, E3DCOLOR col,
    int segs = 24) const = 0;
  virtual void renderXZCircle(const Point3 &center, real radius, E3DCOLOR col, int segs = 24) const = 0;
  virtual void renderCapsuleW(const Capsule &cap, E3DCOLOR c) const = 0;

  virtual DebugPrimitivesVbuffer *newDebugPrimitivesVbuffer(const char *name, IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteDebugPrimitivesVbuffer(DebugPrimitivesVbuffer *&vbuf) const = 0;
  virtual void beginDebugLinesCacheToVbuffer(DebugPrimitivesVbuffer &vbuf) const = 0;
  virtual void endDebugLinesCacheToVbuffer(DebugPrimitivesVbuffer &vbuf) const = 0;
  virtual void invalidateDebugPrimitivesVbuffer(DebugPrimitivesVbuffer &vbuf) const = 0;
  virtual void renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf) const = 0;
  virtual void renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf, E3DCOLOR c) const = 0;
  virtual void renderLinesFromVbuffer(DebugPrimitivesVbuffer &vbuf, bool z_test, bool z_write, bool z_func_less,
    Color4 color_multiplier) const = 0;
  virtual void addLineToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p0, const Point3 &p1, E3DCOLOR c) const = 0;
  virtual void addHatchedBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const TMatrix &box_tm, float hatching_step,
    E3DCOLOR color) const = 0;
  virtual void addBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p, const Point3 &ax, const Point3 &ay, const Point3 &az,
    E3DCOLOR color) const = 0;
  virtual void addSolidBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 &p, const Point3 &ax, const Point3 &ay,
    const Point3 &az, E3DCOLOR color) const = 0;
  virtual void addBoxToVbuffer(DebugPrimitivesVbuffer &vbuf, const BBox3 &box, E3DCOLOR color) const = 0;
  virtual void addTriangleToVbuffer(DebugPrimitivesVbuffer &vbuf, const Point3 p[3], E3DCOLOR color) const = 0;
  virtual bool isLinesVbufferValid(DebugPrimitivesVbuffer &vbuf) const = 0;
  virtual void setVbufferTm(DebugPrimitivesVbuffer &vbuf, const TMatrix &tm) const = 0;

  // DynamicShadersBuffer
  virtual DynamicShadersBuffer *newDynamicShadersBuffer(IMemAlloc *alloc = NULL) const = 0;
  virtual DynamicShadersBuffer *newDynamicShadersBuffer(CompiledShaderChannelId *channels, int channel_count, int max_verts,
    int max_faces, IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteDynamicShadersBuffer(DynamicShadersBuffer *&buf) const = 0;

  virtual void dynShaderBufferSetCurrentShader(DynamicShadersBuffer &buf, ShaderElement *shader) const = 0;
  virtual void dynShaderBufferAddFaces(DynamicShadersBuffer &buf, const void *vertex_data, int num_verts, const int *indices,
    int num_faces) const = 0;
  virtual void dynShaderBufferFlush(DynamicShadersBuffer &buf) const = 0;

  // DynRenderBuffer
  virtual DynRenderBuffer *newDynRenderBuffer(const char *class_name, IMemAlloc *alloc) const = 0;
  virtual void deleteDynRenderBuffer(DynRenderBuffer *&buf) const = 0;

  virtual void dynRenderBufferClearBuf(DynRenderBuffer &buf) const = 0;
  virtual void dynRenderBufferFlush(DynRenderBuffer &buf) const = 0;
  virtual void dynRenderBufferFlushToBuffer(DynRenderBuffer &buf, TEXTUREID tid) const = 0;

  virtual void dynRenderBufferDrawQuad(DynRenderBuffer &buf, const Point3 &p0, const Point3 &p1, const Point3 &p2, const Point3 &p3,
    E3DCOLOR color, float u = 1, float v = 1) const = 0;
  virtual void dynRenderBufferDrawLine(DynRenderBuffer &buf, const Point3 &from, const Point3 &to, real width,
    E3DCOLOR color) const = 0;
  virtual void dynRenderBufferDrawSquare(DynRenderBuffer &buf, const Point3 &p, real radius, E3DCOLOR color) const = 0;
  virtual void dynRenderBufferDrawBox(DynRenderBuffer &buf, const TMatrix &tm, E3DCOLOR color) const = 0;
  virtual void *dynRenderBufferDrawNetSurface(DynRenderBuffer &buf, int w, int h) const = 0;
  // textures
  virtual TEXTUREID addManagedTexture(const char *name) const = 0;
  virtual TEXTUREID registerManagedTex(const char *name, BaseTexture *basetex) const = 0;
  virtual BaseTexture *acquireManagedTex(TEXTUREID id) const = 0;
  virtual void releaseManagedTex(TEXTUREID id) const = 0;
  virtual void releaseManagedResVerified(TEXTUREID &id, D3dResource *cmp) const;
  template <class T>
  inline void releaseManagedTexVerified(D3DRESID &id, T &tex)
  {
    releaseManagedResVerified(id, static_cast<D3dResource *>(tex));
    tex = nullptr;
  }

  // misc
  virtual E3DCOLOR normalizeColor4(const Color4 &color, real &bright) const = 0;
  virtual int getHdrMode() const = 0;

  // stdGuiRender
  virtual void renderTextFmt(real x, real y, E3DCOLOR color, const char *format, const DagorSafeArg *arg, int anum) const = 0;
  virtual BBox2 getTextBBox(const char *str, int len = -1) const = 0;
  virtual int setTextFont(int font_id, int font_kern = 0) const = 0;

#define DSA_OVERLOADS_PARAM_DECL real x, real y, E3DCOLOR color,
#define DSA_OVERLOADS_PARAM_PASS x, y, color,
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void renderText, renderTextFmt);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
};


class IDagorGeom
{
public:
  // StaticGeometryContainer
  virtual StaticGeometryContainer *newStaticGeometryContainer(IMemAlloc *alloc = NULL) = 0;
  virtual void deleteStaticGeometryContainer(StaticGeometryContainer *&cont) = 0;
  virtual void staticGeometryContainerClear(StaticGeometryContainer &cont) = 0;
  virtual bool staticGeometryContainerLoadFromDag(StaticGeometryContainer &cont, const char *path, ILogWriter *log,
    bool use_not_found_tex = false, int load_flags = LASF_MATNAMES | LASF_NULLMATS) = 0;
  virtual bool staticGeometryContainerImportDag(StaticGeometryContainer &cont, const char *src, const char *dest) const = 0;
  virtual void staticGeometryContainerExportDag(StaticGeometryContainer &cont, const char *path,
    bool make_tex_path_local = true) const = 0;

  // GeomObject
  virtual GeomObject *newGeomObject(IMemAlloc *alloc = NULL) = 0;
  virtual void deleteGeomObject(GeomObject *&geom) = 0;

  virtual void geomObjectSaveToDag(GeomObject &go, const char *path) const = 0;
  virtual bool geomObjectLoadFromDag(GeomObject &go, const char *path, ILogWriter *log,
    ITextureNameResolver *resolv_cb = NULL) const = 0;
  virtual void geomObjectClear(GeomObject &go) const = 0;
  virtual void geomObjectEraseNode(GeomObject &go, int idx) const = 0;

  virtual StaticGeometryContainer &geomObjectGetGeometryContainer(GeomObject &go) const = 0;
  virtual const StaticGeometryContainer &geomObjectGetGeometryContainer(const GeomObject &go) const = 0;

  virtual void geomObjectSetDefNodeVis(GeomObject &go) = 0;
  virtual void geomObjectRecompile(GeomObject &go, const Bitarray *node_vis = NULL) = 0;
  virtual void geomObjectCreateFromGeomContainer(GeomObject &go, const StaticGeometryContainer &cont, bool do_recompile = true) = 0;

  virtual void geomObjectRender(GeomObject &go) = 0;
  virtual void geomObjectRenderColor(GeomObject &go, DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only = true) const = 0;
  virtual void geomObjectRenderNodeColor(GeomObject &go, int idx, DynRenderBuffer *buffer, E3DCOLOR c,
    bool renderable_only = true) const = 0;
  virtual void geomObjectRenderTrans(GeomObject &go) = 0;
  virtual void geomObjectRenderEdges(GeomObject &go, bool is_visible, E3DCOLOR color = E3DCOLOR(200, 200, 200, 200)) = 0;

  virtual BBox3 geomObjectGetBoundBox(GeomObject &go, bool local_coord = false) const = 0;
  virtual BBox3 geomObjectGetNodeBoundBox(const GeomObject &go, int idx, bool local_coord = false) const = 0;

  virtual const char *geomObjectGetShaderName(const GeomObject &go, int idx) const = 0;
  virtual ShaderMesh *geomObjectGetShaderMesh(const GeomObject &go, int idx) const = 0;

  virtual bool geomObjectShadowRayHitTest(GeomObject &go, const Point3 &p, const Point3 &dir, real maxt,
    int trace_flags = 0) const = 0;
  virtual bool geomObjectTraceRay(GeomObject &go, const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm) const = 0;
  virtual bool geomObjectReloadRayTracer(GeomObject &go) const = 0;
  virtual StaticSceneRayTracer *geomObjectGetRayTracer(GeomObject &go) const = 0;


  // StaticGeometryNode
  virtual StaticGeometryNode *newStaticGeometryNode(IMemAlloc *alloc = NULL) = 0;
  virtual StaticGeometryNode *newStaticGeometryNode(const StaticGeometryNode &from, IMemAlloc *alloc = NULL) = 0;
  virtual void deleteStaticGeometryNode(StaticGeometryNode *&node) = 0;
  virtual bool staticGeometryNodeHaveBillboardMat(const StaticGeometryNode &node) const = 0;
  virtual void staticGeometryNodeCalcBoundBox(StaticGeometryNode &node) const = 0;
  virtual void staticGeometryNodeCalcBoundSphere(StaticGeometryNode &node) const = 0;
  virtual void staticGeometryNodeSetMaterialLighting(const StaticGeometryNode &node, StaticGeometryMaterial &mat) const = 0;

  virtual const char *staticGeometryNodeLightingToStr(StaticGeometryNode::Lighting light) const = 0;
  virtual StaticGeometryNode::Lighting staticGeometryNodeStrToLighting(const char *light) const = 0;

  // StaticGeometryMesh
  virtual StaticGeometryMesh *newStaticGeometryMesh(IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteStaticGeometryMesh(StaticGeometryMesh *&mesh) const = 0;

  // GeomResourcesHelper
  virtual GeomResourcesHelper *newGeomResourcesHelper(IStaticGeomResourcesService *svc, IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteGeomResourcesHelper(GeomResourcesHelper *&helper) const = 0;

  virtual void geomResourcesHelperSetResourcesService(GeomResourcesHelper &helper, IStaticGeomResourcesService *svc) const = 0;
  virtual void geomResourcesHelperCreateResources(GeomResourcesHelper &helper, const void *obj_id, const TMatrix &tm,
    const StaticGeometryContainer &cont, ILogWriter *log = NULL) const = 0;
  virtual void geomResourcesHelperFreeResources(GeomResourcesHelper &helper, const void *obj_id) const = 0;
  virtual void geomResourcesHelperSetResourcesTm(GeomResourcesHelper &helper, const void *obj_id, const TMatrix &tm) const = 0;
  virtual void geomResourcesHelperRemapResources(GeomResourcesHelper &helper, const void *obj_id_old,
    const void *obj_id_new) const = 0;
  virtual int geomResourcesHelperCompact(GeomResourcesHelper &helper) const = 0;

  // Shaders
  virtual ShaderMaterial *newShaderMaterial(MaterialData &m) const = 0;

  virtual int getShaderVariableId(const char *name) const = 0;
  virtual const char *getShaderVariableName(int id) const = 0;

  virtual bool shaderGlobalSetInt(int id, int val) const = 0;
  virtual bool shaderGlobalSetReal(int id, real val) const = 0;
  virtual bool shaderGlobalSetColor4(int id, const Color4 &val) const = 0;
  virtual bool shaderGlobalSetTexture(int id, TEXTUREID val) const = 0;

  virtual int shaderGlobalGetInt(int id) const = 0;
  virtual real shaderGlobalGetReal(int id) const = 0;
  virtual Color4 shaderGlobalGetColor4(int id) const = 0;
  virtual TEXTUREID shaderGlobalGetTexture(int id) const = 0;

  virtual void shaderGlobalSetVarsFromBlk(const DataBlock &blk) const = 0;

  virtual int shaderGlobalGetBlockId(const char *name) const = 0;
  virtual void shaderGlobalSetBlock(int id, int layer = -1) const = 0;

  virtual void shaderElemInvalidateCachedStateBlock() const = 0;

  virtual shaders::OverrideStateId create(const shaders::OverrideState &) const = 0;
  virtual bool destroy(shaders::OverrideStateId &override_id) const = 0;
  virtual bool set(shaders::OverrideStateId override_id) const = 0;
  virtual bool reset_override() const = 0;

  // Mesh
  virtual Mesh *newMesh(IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteMesh(Mesh *&mesh) const = 0;

  virtual int meshOptimizeTverts(Mesh &mesh) const = 0;
  virtual int meshCalcNgr(Mesh &mesh) const = 0;
  virtual int meshCalcVertnorms(Mesh &mesh) const = 0;
  virtual void meshFlipNormals(Mesh &mesh, int f0 = 0, int nf = -1) const = 0;

  virtual PostFxRenderer *newPostFxRenderer() const = 0;
  virtual void deletePostFxRenderer(PostFxRenderer *&) const = 0;
  virtual void postFxRendererInit(PostFxRenderer &p, const char *shader_name) const = 0;
  virtual void postFxRendererRender(PostFxRenderer &p) const = 0;

  // ObjCreator3d
  virtual bool generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom) const = 0;
  virtual bool generatePlane(const Point2 &cell_size, StaticGeometryContainer &geom, StaticGeometryMaterial *material) const = 0;

  virtual bool generateBox(const IPoint3 &segments, StaticGeometryContainer &geom) const = 0;

  virtual bool generateCylinder(int sides, int height_segments, int cap_segments, StaticGeometryContainer &geom) const = 0;

  virtual bool generatePolyMesh(const Tab<Point2> &points, int height_segments, StaticGeometryContainer &geom) const = 0;

  virtual bool generateBoxSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom) const = 0;
  virtual bool generateClosedSpiralStair(int steps, real w, real arc, StaticGeometryContainer &geom) const = 0;
  virtual bool generateOpenSpiralStair(int steps, real w, real h, real arc, StaticGeometryContainer &geom) const = 0;

  virtual bool generateBoxStair(int steps, StaticGeometryContainer &geom) const = 0;
  virtual bool generateClosedStair(int steps, StaticGeometryContainer &geom) const = 0;
  virtual bool generateOpenStair(int steps, real h, StaticGeometryContainer &geom) const = 0;

  // TODO: remove it from here after remove its usage from Splines plugin
  virtual bool objCreator3dAddNode(const char *name, Mesh *mesh, MaterialDataList *material, StaticGeometryContainer &geom) const = 0;

  // Creators
  virtual BoxCreator *newBoxCreator(IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteBoxCreator(BoxCreator *&creator) const = 0;

  virtual SphereCreator *newSphereCreator(IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteSphereCreator(SphereCreator *&creator) const = 0;

  virtual PlaneCreator *newPlaneCreator(IMemAlloc *alloc = NULL) const = 0;
  virtual void deletePlaneCreator(PlaneCreator *&creator) const = 0;

  virtual PointCreator *newPointCreator(IMemAlloc *alloc = NULL) const = 0;
  virtual void deletePointCreator(PointCreator *&creator) const = 0;

  virtual CylinderCreator *newCylinderCreator(IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteCylinderCreator(CylinderCreator *&creator) const = 0;

  virtual PolyMeshCreator *newPolyMeshCreator(IMemAlloc *alloc = NULL) const = 0;
  virtual void deletePolyMeshCreator(PolyMeshCreator *&creator) const = 0;

  virtual StairCreator *newStairCreator(IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteStairCreator(StairCreator *&creator) const = 0;

  virtual SpiralStairCreator *newSpiralStairCreator(IMemAlloc *alloc) const = 0;
  virtual void deleteSpiralStairCreator(SpiralStairCreator *&creator) const = 0;

  virtual SplineCreator *newSplineCreator(IMemAlloc *alloc) const = 0;
  virtual void deleteSplineCreator(SplineCreator *&creator) const = 0;

  virtual TargetCreator *newTargetCreator(IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteTargetCreator(TargetCreator *&creator) const = 0;

  virtual void deleteIObjectCreator(IObjectCreator *&creator) const = 0;

  // DagSaver
  virtual DagSaver *newDagSaver(IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteDagSaver(DagSaver *&saver) const = 0;

  virtual bool dagSaverStartSaveDag(DagSaver &saver, const char *path) const = 0;
  virtual bool dagSaverEndSaveDag(DagSaver &saver) const = 0;

  virtual bool dagSaverStartSaveNodes(DagSaver &saver) const = 0;
  virtual bool dagSaverEndSaveNodes(DagSaver &saver) const = 0;

  virtual bool dagSaverStartSaveNode(DagSaver &saver, const char *name, const TMatrix &wtm,
    int flg = DAG_NF_RENDERABLE | DAG_NF_CASTSHADOW | DAG_NF_RCVSHADOW, int children = 0) const = 0;
  virtual bool dagSaverEndSaveNode(DagSaver &saver) const = 0;

  virtual bool dagSaverSaveDagSpline(DagSaver &saver, DagSpline **spline, int cnt) const = 0;

  // AScene
  virtual AScene *newAScene(IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteAScene(AScene *&scene) const = 0;

  virtual int loadAscene(const char *fn, AScene &sc, int flg, bool fatal_on_error) const = 0;

  // Node
  virtual void nodeCalcWtm(Node &node) const = 0;
  virtual bool nodeEnumNodes(Node &node, Node::NodeEnumCB &cb, Node **res = NULL) const = 0;
};


class IDagorConsole
{
public:
  virtual void startProgress(CoolConsole &con) const = 0;
  virtual void endProgress(CoolConsole &con) const = 0;

  virtual void addMessageFmt(CoolConsole &con, ILogWriter::MessageType t, const char *msg, const DagorSafeArg *arg,
    int anum) const = 0;

  virtual void showConsole(CoolConsole &con, bool activate = false) const = 0;
  virtual void hideConsole(const CoolConsole &con) const = 0;

  virtual bool registerCommand(CoolConsole &con, const char *cmd, IConsoleCmd *handler) const = 0;
  virtual bool unregisterCommand(CoolConsole &con, const char *cmd, IConsoleCmd *handler) const = 0;

/// void addMessage(CoolConsole& con, MessageType type, const char *fmt, typesafe-var-args...)
#define DSA_OVERLOADS_PARAM_DECL CoolConsole &con, ILogWriter::MessageType t,
#define DSA_OVERLOADS_PARAM_PASS con, t,
  DECLARE_DSA_OVERLOADS_FAMILY_LT(inline void addMessage, addMessageFmt);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS
};


class IDagorTools
{
public:
  // ddstexture::Converter
  virtual bool ddsConvertImage(ddstexture::Converter &converter, IGenSave &cb, TexPixel32 *pixels, int width, int height,
    int stride) const = 0;

  // loadmask::
  virtual bool loadmaskloadMaskFromFile(const char *filename, SmallTab<float, TmpmemAlloc> &hmap, int &w, int &h) const = 0;

  // AnimV20::AnimData
  virtual AnimV20::AnimData *newAnimData(IMemAlloc *alloc = NULL) const = 0;
  virtual bool animDataLoad(AnimV20::AnimData &anim, IGenLoad &cb, IMemAlloc *alloc = midmem) const = 0;

  // other
  virtual unsigned getSimpleHash(const char *s, unsigned int init_val) const = 0;

  virtual void *win32GetMainWnd() const = 0;

  // time functions
  virtual int getTimeMsec() const = 0;
  virtual __int64 refTimeUsec() const = 0;

  // ddsx functions
  virtual void ddsxShutdownPlugins() = 0;
  virtual bool ddsxConvertDds(unsigned targ_code, ddsx::Buffer &dest, void *dds_data, int dds_len, const ddsx::ConvertParams &p) = 0;
  virtual const char *ddsxGetLastErrorText() = 0;
  virtual void ddsxFreeBuffer(ddsx::Buffer &b) = 0;

  // files
  virtual bool copyFile(const char *src, const char *dest, bool overwrite = true) const = 0;
  virtual bool compareFile(const char *path1, const char *path2) const = 0;
};


class IDagorScene
{
public:
  // StaticSceneRayTracer
  virtual int staticSceneRayTracerTraceRay(StaticSceneRayTracer &rt, const Point3 &p, const Point3 &wdir2, real &mint2,
    int from_face = -1) const = 0;

  // FastRtDump
  virtual int fastRtDumpTraceRay(FastRtDump &frt, int custom, const Point3 &p, const Point3 &dir, real &t, int &out_pmid) const = 0;

  // BuildableStaticSceneRayTracer
  virtual BuildableStaticSceneRayTracer *createBuildableStaticmeshsceneRaytracer(const Point3 &lsz, int lev) const = 0;
  virtual bool buildableStaticSceneRayTracerAddmesh(BuildableStaticSceneRayTracer &rt, const Point3 *vert, int vcount,
    const unsigned *face, unsigned stride, int fn, const unsigned *face_flags, bool rebuild = true) const = 0;
  virtual bool buildableStaticSceneRayTracerReserve(BuildableStaticSceneRayTracer &rt, int face_count, int vert_count) const = 0;
  virtual bool buildableStaticSceneRayTracerRebuild(BuildableStaticSceneRayTracer &rt) const = 0;

  // StaticSceneBuilder::StdTonemapper
  virtual StaticSceneBuilder::StdTonemapper *newStdTonemapper(IMemAlloc *alloc = NULL) const = 0;
  virtual StaticSceneBuilder::StdTonemapper *newStdTonemapper(const StaticSceneBuilder::StdTonemapper &from,
    IMemAlloc *alloc = NULL) const = 0;
  virtual void deleteStdTonemapper(StaticSceneBuilder::StdTonemapper *&mapper) const = 0;

  virtual void stdTonemapperRecalc(StaticSceneBuilder::StdTonemapper &mapper) const = 0;
  virtual void stdTonemapperSave(StaticSceneBuilder::StdTonemapper &mapper, DataBlock &blk) const = 0;
  virtual void stdTonemapperLoad(StaticSceneBuilder::StdTonemapper &mapper, const DataBlock &blk) const = 0;

  // resources
  virtual GameResource *getGameResource(GameResHandle handle, bool no_factory_fatal = true) const = 0;
  virtual void releaseGameResource(GameResource *resource) const = 0;
};


class IEditorCore
{
public:
  virtual IDagorRender *getRender() = 0;
  virtual IDagorGeom *getGeom() = 0;
  virtual IDagorConsole *getConsole() = 0;
  virtual IDagorTools *getTools() = 0;
  virtual IDagorScene *getScene() = 0;
  virtual const char *getExePath() = 0;

  static IEditorCore &make_instance();
};


#endif //__GAIJIN_IEDITOR_CORE__
