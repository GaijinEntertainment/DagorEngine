#ifndef __GAIJIN_GEOM_OBJECT__
#define __GAIJIN_GEOM_OBJECT__
#pragma once


#include <3d/dag_materialData.h>

#include <util/dag_bitArray.h>
#include <util/dag_oaHashNameMap.h>

#include <generic/dag_ptrTab.h>
#include <generic/dag_smallTab.h>

#include <shaders/dag_shaderMesh.h>
#include <scene/dag_visibility.h>
#include <sceneRay/dag_sceneRayDecl.h>

#include <libTools/staticGeom/staticGeometry.h>
#include <libTools/containers/dag_StrMap.h>


// forward declarations for external classes
class StaticGeometryContainer;
class DynRenderBuffer;
class Bitarray;
class StaticGeometryMaterial;
class CoolConsole;
class ILogWriter;
class IGenericProgressIndicator;
class ITextureNameResolver;
namespace StaticSceneBuilder
{
class StdTonemapper;
}

class GeomObject
{
public:
  static bool isEdgedFaceView;
  static E3DCOLOR edgedFaceColor;
  static bool useDynLighting;
  static bool isRenderingReflection;
  static bool isRenderingShadows;
  static bool setNodeNameHashToShaders; // when true, each node computes name hash and sets 16 lower bits to 'geom_node_hash' shader
                                        // var

  enum GeomObjRequest
  {
    REQUEST_CLASS,
  };


  GeomObject();
  ~GeomObject();

  inline const StaticGeometryContainer *getGeometryContainer() const { return geom; }
  inline StaticGeometryContainer *getGeometryContainer() { return geom; }
  inline TMatrix getTm() const { return tm; }
  inline bool getSaveAllMats() const { return (bool)saveAllMats; }
  inline bool isUseNodeVisRange() const { return (bool)useNodeVisRange; }

  // returns bounding box in global coordinates
  BBox3 getNodeBoundBox(int idx, bool local_coord = false) const;
  BBox3 getBoundBox(bool local_coord = false);
  void calcBoundBox(bool local_coord = false);
  Point3 getFarestPoint() const;
  Point3 getNearestPoint() const;

  static void getSkyLight(E3DCOLOR &color, real &bright);
  static void getEarthLight(E3DCOLOR &color, real &bright);
  static void getDirectLight(E3DCOLOR &color, real &mult);
  static void getLightAngles(real &alpha, real &beta);

  inline ShaderMesh *getShaderMesh(int idx) const;

  inline int getShaderNamesCount() const { return shaderNameIds.size(); }
  inline const char *getShaderName(int idx) const;

  inline void setTm(const TMatrix &new_tm) { tm = new_tm; }
  inline void setSaveAllMats(bool val) { saveAllMats = val ? 1 : 0; }
  inline void setUseNodeVisRange(bool use) { useNodeVisRange = use ? 1 : 0; }
  inline void notChangeVertexColors(bool not_change) { doNotChangeVertexColors = not_change ? 1 : 0; }

  static void setAmbientLight(E3DCOLOR sky_color, real sky_bright, E3DCOLOR earth_color, real earth_bright);
  static void setDirectLight(E3DCOLOR color, real mult);
  static void setLightAngles(real zenith, real azimuth);
  static void setLightAnglesDeg(real alpha, real beta);

  static void recompileAllGeomObjects(IGenericProgressIndicator *progress);
  static void getAllObjects(GeomObjRequest req, Tab<GeomObject *> &objs, const void *condition);

  static void setTonemapper(StaticSceneBuilder::StdTonemapper *map) { toneMapper = map; }
  static void initWater();

  void clear();
  void eraseNode(int idx);
  void replaceNode(int idx, StaticGeometryNode *node, bool rebuild = true);

  void saveToDag(const char *fname);
  void saveToDagAndScale(const char *fname, real scale_coef);
  bool loadFromDag(const char *fname, ILogWriter *log, ITextureNameResolver *resolv_cb = NULL);
  void connectToGeomContainer(StaticGeometryContainer *new_geom);
  void createFromGeomContainer(const StaticGeometryContainer *cont, bool do_recompile = true);

  void recompile(const Bitarray *node_vis = NULL);
  void recompileNode(int idx);

  void render();
  void renderTrans();
  void renderEdges(bool is_visible, E3DCOLOR color = E3DCOLOR(200, 200, 200, 200));
  void renderNodeColor(int idx, DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only = true);
  void renderColor(DynRenderBuffer *buffer, E3DCOLOR c, bool renderable_only = true);

  void setDefNodeVis();

  enum GeomObjRayTrayceFlags
  {
    TRACE_USE_NODEVIS = 1 << 0,
    TRACE_INVISIBLE = 1 << 1,
  };

  StaticSceneRayTracer *getRayTracer();
  bool reloadRayTracer();
  bool shadowRayHitTest(const Point3 &p, const Point3 &dir, real maxt, int trace_flags = 0);
  bool traceRay(const Point3 &p, const Point3 &dir, real &maxt, Point3 *norm);
  bool hasCollision();

protected:
  StaticGeometryContainer *geom;

  void compileRenderData();

private:
  class SceneRayTracer;
  SceneRayTracer *rayTracer;

  static StaticSceneBuilder::StdTonemapper *toneMapper;

  struct RenderObject
  {
    struct ObjectLod
    {
      bool hasLandmesh;
      dag::Span<GlobalVertexData> vdata;
      ShaderMesh *mesh;
      real lodRangeSq;
      int nodeID;
      Point3 pos_to_world_ofs, pos_to_world_scale;

      ObjectLod() = default;
      ~ObjectLod()
      {
        memfree(vdata.data(), midmem);
        vdata.reset();
      }

      void delVdata();
    };

    GeomObject *geom;
    String name;
    unsigned char curLod;
    bool hasReflectionRefraction;


    Tab<ObjectLod> lods;

    RenderObject() : curLod(0), lods(tmpmem) {}
    RenderObject(const RenderObject &) = delete;
    ~RenderObject()
    {
      for (int i = 0; i < lods.size(); ++i)
      {
        del_it(lods[i].mesh);
        lods[i].delVdata();
      }
    }

    void beforeRenderCheck();
    void render();
    void renderTrans();

    void setname(const char *n) { name = n; }
  };

  Tab<RenderObject> shMesh;
  Tab<int> shaderNameIds;
  Bitarray nodeVis;

  TMatrix tm;
  BBox3 bounds;

  unsigned externalGeomContainer : 1;
  unsigned useNodeVisRange : 1;
  unsigned doNotChangeVertexColors : 1;
  // If you want to use recompileNode method, set this flag to true. Devault value is false
  unsigned saveAllMats : 1;
  unsigned localBoundBox : 1;
  unsigned useDynLight : 1;
  unsigned hasReflection : 1;

  static NameMap shadersMap;

  static Point3 skyColor;
  static Point3 earthColor;
  static Point3 directColor;

  static real skyBright;
  static real earthBright;
  static real directMult;

  static E3DCOLOR skyCol;
  static E3DCOLOR earthCol;
  static E3DCOLOR directCol;

  static Point2 lightingDirAngles;
  static Point3 lightingDir;

  static int hdrLightmapScaleId;
  static int worldViewPosId;

  // geom objects array
  static Tab<GeomObject *> *createdObjects;

  real sampleSize;

  // All nodes's materials. If saveAllMats == 0 this tab clears every
  // time after compileRenderData()
  PtrTab<MaterialData> allmats;

  struct DefMaterialParams
  {
    StaticGeometryMaterial *mat;
    String lighting;
    int flags;
  };

  static Tab<String> fatalMessages;

  void eraseAllMats(bool clear);

  void compileLight(MaterialData &mat_null, StaticGeometryNode::Lighting light);
  void createVHarmLightmap(Mesh &mesh, int idx, bool none_lighting);

  void setNodeVis(const Bitarray &from);

  void renderWireFrame(const Mesh &mesh, E3DCOLOR color);

  static void getObjectsByClass(Tab<GeomObject *> &objs, const char *class_name);

  static int lodCmp(const GeomObject::RenderObject::ObjectLod *a, const GeomObject::RenderObject::ObjectLod *b);

  real getDefSampleSize(int idx) const;
  ShaderMesh *getMesh(int node_id) const;
  bool isNodeInVisRange(int idx) const;

  void delRayTracer();
  void initRayTracer();

  static bool fatalHandler(const char *msg, const char *stack_msg, const char *file, int line);
  static void initStaticMembers();
};
DAG_DECLARE_RELOCATABLE(GeomObject::RenderObject);
DAG_DECLARE_RELOCATABLE(GeomObject::RenderObject::ObjectLod);


inline void compress_land_position(const Point3 &cellOfs, const Point3 &cellScale, Mesh &cellm, bool has_transparency)
{
  if (!cellm.vert.size())
    return;
  int compressId = cellm.add_extra_channel(MeshData::CHT_FLOAT4, SCUSAGE_EXTRA, 7); // fixme: we can utilize 4-th component for vertex
                                                                                    // opacity in combined meshes
  MeshData::ExtraChannel &compressedPos = (MeshData::ExtraChannel &)cellm.getExtra(compressId);
  compressedPos.resize_verts(cellm.vert.size());
  for (unsigned int faceNo = 0; faceNo < cellm.face.size(); faceNo++)
    for (unsigned int cornerNo = 0; cornerNo < 3; cornerNo++)
      compressedPos.fc[faceNo].t[cornerNo] = cellm.getFace()[faceNo].v[cornerNo];
  Point4 *compressedVert = (Point4 *)compressedPos.vt.data();

  has_transparency &= (cellm.tface[1].size() && cellm.tvert[1].size());
  VertToFaceVertMap map;
  if (has_transparency)
    cellm.buildVertToFaceVertMap(map);
  for (int i = 0; i < cellm.vert.size(); ++i)
  {
    Point3 v = cellm.vert[i] - cellOfs;
    v.x *= cellScale.x;
    v.y *= cellScale.y;
    v.z *= cellScale.z;
    v.x = clamp(v.x, -1.f, 1.f);
    v.y = clamp(v.y, -1.f, 1.f);
    v.z = clamp(v.z, -1.f, 1.f);
    float transparency = 1.0f;
    if (has_transparency)
    {
      if (map.getNumFaces(i))
      {
        int fvid = map.getVertFaceIndex(i, 0);
        int tid = cellm.tface[1][fvid / 3].t[fvid % 3];
        transparency = cellm.tvert[1][tid].x;
      }
    }
    compressedVert[i] = Point4::xyzV(v, transparency);
  }
}


//==============================================================================
inline ShaderMesh *GeomObject::getShaderMesh(int idx) const
{
  if (idx >= 0 /*&& idx < shMesh.size()*/)
    return getMesh(idx);

  return NULL;
}


//==============================================================================
inline const char *GeomObject::getShaderName(int idx) const
{
  if (idx >= 0 && idx < shaderNameIds.size())
    return shadersMap.getName(shaderNameIds[idx]);

  return NULL;
}


#endif //__GAIJIN_GEOM_OBJECT__
