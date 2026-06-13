// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/dagFileRW/geomMeshHelper.h>
#include "exp_tools.h"
#include <gameRes/dag_stdGameRes.h>
#include <math/dag_boundingSphere.h>
#include <math/dag_mesh.h>
#include <util/dag_hashedKeyMap.h>
#include <osApiWrappers/dag_direct.h>
#include <libTools/dagFileRW/dagFileNode.h>
#include <gameRes/dag_collisionResource.h>
#include <math/dag_plane3.h>
#include <math/dag_geomTree.h>
#include <stdio.h>
#include <libTools/math/kdop.h>
#include <VHACD.h>
#include <math/dag_convexHullComputer.h>
#include <libTools/collision/vhacdMeshChecker.h>
#include <libTools/collision/exportCollisionNodeType.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_btagCompr.h>
#include <scene/dag_physMat.h>
#include <sceneRay/dag_sceneRay.h>
#include <util/dag_fastNameMapTS.h>
#include "getSkeleton.h"


BEGIN_DABUILD_PLUGIN_NAMESPACE(collision)

static bool def_collidable = true;
enum
{
  DEGENERATIVE_MESH_DO_ERROR = 0,
  DEGENERATIVE_MESH_DO_PASS_THROUGH,
  DEGENERATIVE_MESH_DO_REMOVE,
};
static int degenerative_mesh_strategy = DEGENERATIVE_MESH_DO_ERROR;
static bool jolt_degenerate_fail_export = false;

static float degenerate_tri_area_threshold_sq = 5e-12f;
static bool report_inverted_mesh_tm = false;

template <typename StringType>
static void remove_dm_suffix(const String &src, StringType &dst)
{
  const int len = src.length();
  if (len >= 3 && src[len - 3] == '_' && src[len - 2] == 'd' && src[len - 1] == 'm')
  {
    dst.setStr(src.str(), len - 3);
  }
  else
    dst = src;
}

static bool find_dm_parts_name(const DataBlock *blk, int name_id, String part_name)
{
  if (blk == nullptr)
    return false;
  int paramNum = -1;
  for (;;)
  {
    paramNum = blk->findParam(name_id, paramNum);
    if (paramNum < 0)
      break;
    const char *partName = blk->getStr(paramNum);
    int index = 0;
    if (strcmp(part_name, partName) == 0 ||
        (sscanf(part_name, partName, &index) == 1 && strcmp(String(32, partName, index).str(), part_name) == 0))
      return true;
  }
  return false;
}

struct CollisionObjectProps
{
  BBox3 boundingBoxs = BBox3();
  String objectName;
  String physMat;
  int objectCollisionType = -1;
  bool objectTreeCapsule = false;
  bool objectHasKdop = false;
  int objectKdopPreset = -1;
  int objectKdopSegmentsX = -1;
  int objectKdopSegmentsY = -1;
  int objectKdopRotX = 0;
  int objectKdopRotY = 0;
  int objectKdopRotZ = 0;
  float objectKdopCutOffThreshold = 0.0f;
  uint16_t behaviorFlags = 0;
};

static int get_node_idx(const Tab<GeomMeshHelperDagObject> &dag_objects_list, const char *node_name)
{
  for (int i = 0; i < dag_objects_list.size(); ++i)
  {
    SimpleString nameWithoutSuffix;
    remove_dm_suffix(dag_objects_list[i].name, nameWithoutSuffix);
    if (dag_objects_list[i].name == node_name || nameWithoutSuffix == node_name)
    {
      return i;
    }
  }
  return -1;
}

static int get_props_idx(const Tab<CollisionObjectProps> &collision_objects_props, const char *object_name)
{
  for (int i = 0; i < collision_objects_props.size(); ++i)
  {
    SimpleString nameWithoutSuffix;
    remove_dm_suffix(collision_objects_props[i].objectName, nameWithoutSuffix);
    if (collision_objects_props[i].objectName == object_name || nameWithoutSuffix == object_name)
    {
      return i;
    }
  }
  return -1;
}

static void fill_kdop_props(const DataBlock *node, CollisionObjectProps &kdop_props)
{
  kdop_props.objectCollisionType = COLLISION_NODE_TYPE_CONVEX;
  kdop_props.objectHasKdop = true;
  kdop_props.objectKdopPreset = node->getInt("kdopPreset", -1);
  kdop_props.objectKdopSegmentsX = node->getInt("kdopSegmentsX", -1);
  kdop_props.objectKdopSegmentsY = node->getInt("kdopSegmentsY", -1);
  kdop_props.objectKdopRotX = node->getInt("kdopRotX", 0);
  kdop_props.objectKdopRotY = node->getInt("kdopRotY", 0);
  kdop_props.objectKdopRotZ = node->getInt("kdopRotZ", 0);
  kdop_props.objectKdopCutOffThreshold = node->getReal("cutOffThreshold", 0.0f);
}

static void collision_object_setup(const Tab<GeomMeshHelperDagObject> &dag_objects_list,
  const Tab<CollisionObjectProps> &collision_objects_props, const DataBlock *ref_nodes, GeomMeshHelperDagObject &collision_object,
  CollisionObjectProps &collision_props)
{
  for (int i = 0; i < ref_nodes->blockCount(); ++i)
  {
    const char *refNodeName = ref_nodes->getBlock(i)->getBlockName();
    int objectIdx = get_node_idx(dag_objects_list, refNodeName);
    int propsIdx = get_props_idx(collision_objects_props, refNodeName);
    G_ASSERT(objectIdx != -1 && propsIdx != -1);
    const GeomMeshHelperDagObject &refDagObject = dag_objects_list[objectIdx];
    const CollisionObjectProps &refProps = collision_objects_props[propsIdx];
    collision_props.behaviorFlags |= refProps.behaviorFlags;
    collision_props.boundingBoxs += refProps.boundingBoxs;
    const int idxOffset = collision_object.mesh.verts.size();
    for (auto const &vert : refDagObject.mesh.verts)
    {
      collision_object.mesh.verts.push_back(vert * refDagObject.wtm);
    }
    for (auto const &face : refDagObject.mesh.faces)
    {
      collision_object.mesh.faces.push_back({face.v[0] + idxOffset, face.v[1] + idxOffset, face.v[2] + idxOffset});
    }
  }
}

static void erase_replaced(const DataBlock *ref_nodes, Tab<GeomMeshHelperDagObject> &dag_objects_list,
  Tab<CollisionObjectProps> &collision_objects_props, unsigned int &num_collision_nodes)
{
  for (int i = 0; i < ref_nodes->blockCount(); ++i)
  {
    const char *refNodeName = ref_nodes->getBlock(i)->getBlockName();
    int objectIdx = get_node_idx(dag_objects_list, refNodeName);
    int propsIdx = get_props_idx(collision_objects_props, refNodeName);
    if (objectIdx > -1 && propsIdx > -1)
    {
      --num_collision_nodes;
      erase_items(dag_objects_list, objectIdx, 1);
      erase_items(collision_objects_props, propsIdx, 1);
    }
  }
}

static void add_mesh_from_hull(const VHACD::IVHACD::ConvexHull &ch, GeomMeshHelper &mesh)
{
  for (const auto &p : ch.m_points)
  {
    mesh.verts.push_back({static_cast<float>(p.mX), static_cast<float>(p.mY), static_cast<float>(p.mZ)});
  }
  for (const auto &t : ch.m_triangles)
  {
    mesh.faces.push_back({static_cast<int>(t.mI0), static_cast<int>(t.mI2), static_cast<int>(t.mI1)});
  }
}

static void calc_vhacd(const DataBlock *node, GeomMeshHelper &mesh, Tab<GeomMeshHelperDagObject> &collision_objects,
  Tab<CollisionObjectProps> &collisions_props, unsigned int &num_collision_nodes)
{

  VHACD::IVHACD::Parameters params;
  VHACD::IVHACD *iface = VHACD::CreateVHACD();
  params.m_maxRecursionDepth = node->getInt("convexDepth", -1);
  params.m_maxConvexHulls = node->getInt("maxConvexHulls", -1);
  params.m_maxNumVerticesPerCH = node->getInt("maxConvexVerts", -1);
  params.m_resolution = node->getInt("convexResolution", -1);
  iface->Compute((float *)mesh.verts.data(), mesh.verts.size(), (uint32_t *)mesh.faces.data(), mesh.faces.size(), params);

  mesh.verts.clear();
  mesh.faces.clear();
  const int originalObjectIdx = collision_objects.size() - 1;
  const int originalPropsIdx = collisions_props.size() - 1;
  for (int i = 0; i < iface->GetNConvexHulls(); ++i)
  {
    VHACD::IVHACD::ConvexHull ch;
    iface->GetConvexHull(i, ch);
    if (i == 0)
    {
      add_mesh_from_hull(ch, mesh);
      fix_vhacd_faces(collision_objects[originalObjectIdx]);
    }
    else
    {
      GeomMeshHelperDagObject &collisionObject = collision_objects.push_back();
      CollisionObjectProps &collisionProps = collisions_props.push_back();
      const GeomMeshHelperDagObject &originalObject = collision_objects[originalObjectIdx];
      const CollisionObjectProps &originalProps = collisions_props[originalPropsIdx];
      ++num_collision_nodes;
      collisionObject.wtm = TMatrix::IDENT;
      collisionObject.name = originalObject.name + String(5, "_ch%02d", i);
      collisionProps = originalProps;
      collisionProps.objectName = collisionObject.name;
      add_mesh_from_hull(ch, collisionObject.mesh);
      fix_vhacd_faces(collisionObject);
    }
  }
  iface->Release();
}

static void calc_computer(const DataBlock *node, GeomMeshHelper &mesh)
{
  ConvexHullComputer computer;
  const float shrink = node->getReal("shrink", 0.f);
  computer.compute(reinterpret_cast<float *>(mesh.verts.data()), sizeof(Point3), mesh.verts.size(), shrink, 0.0f);
  mesh.verts.clear();
  mesh.faces.clear();
  for (const auto &vert : computer.vertices)
  {
    Point3 p = {v_extract_x(vert), v_extract_y(vert), v_extract_z(vert)};
    mesh.verts.push_back(p);
  }
  for (int i = 0; i < computer.faces.size(); ++i)
  {
    dag::Vector<int> indices;
    int edgeIdx = computer.faces[i];
    int targetVert = computer.edges[edgeIdx].getTargetVertex();
    indices.push_back(targetVert);
    const ConvexHullComputer::Edge *next = computer.edges[edgeIdx].getNextEdgeOfFace();
    while (next->getTargetVertex() != targetVert)
    {
      indices.push_back(next->getTargetVertex());
      next = next->getNextEdgeOfFace();
    }
    for (int i = 2; i < indices.size(); ++i)
    {
      mesh.faces.push_back({indices[0], indices[i], indices[i - 1]});
    }
  }
}

static bool check_collision_type(const char *collision)
{
  const ExportCollisionNodeType nodeType = get_export_type_by_name(collision);
  return nodeType >= ExportCollisionNodeType::MESH && nodeType < ExportCollisionNodeType::NODE_TYPES_COUNT;
}

static void collision_preprocessing(const DataBlock *nodes, Tab<GeomMeshHelperDagObject> &dag_objects_list,
  Tab<CollisionObjectProps> &collision_objects_props, unsigned int &num_collision_nodes)
{
  Tab<GeomMeshHelperDagObject> collisionObjects;
  Tab<CollisionObjectProps> collisionsProps;

  for (int i = 0; i < nodes->blockCount(); ++i)
  {
    const DataBlock *node = nodes->getBlock(i);
    const char *collision = node->getStr("collision", nullptr);
    if (collision && check_collision_type(collision))
    {
      const ExportCollisionNodeType nodeType = get_export_type_by_name(collision);
      const DataBlock *refNodes = node->getBlockByName("refNodes");
      ++num_collision_nodes;
      GeomMeshHelperDagObject &collisionObject = collisionObjects.push_back();
      CollisionObjectProps &collisionProps = collisionsProps.push_back();
      collisionObject.name = node->getBlockName();
      collisionObject.wtm = TMatrix::IDENT;
      collisionProps.objectName = collisionObject.name;
      collisionProps.physMat = node->getStr("phmat", "");

      collision_object_setup(dag_objects_list, collision_objects_props, refNodes, collisionObject, collisionProps);

      if (node->getBool("isTraceable", false))
        collisionProps.behaviorFlags |= CollisionNode::TRACEABLE;
      else
        collisionProps.behaviorFlags &= ~CollisionNode::TRACEABLE;

      if (node->getBool("isPhysCollidable", false))
        collisionProps.behaviorFlags |= CollisionNode::PHYS_COLLIDABLE;
      else
        collisionProps.behaviorFlags &= ~CollisionNode::PHYS_COLLIDABLE;

      if (nodeType == ExportCollisionNodeType::MESH)
      {
        collisionProps.objectCollisionType = COLLISION_NODE_TYPE_MESH;
      }
      else if (nodeType == ExportCollisionNodeType::BOX)
      {
        collisionProps.objectCollisionType = COLLISION_NODE_TYPE_BOX;
      }
      else if (nodeType == ExportCollisionNodeType::SPHERE)
      {
        collisionProps.objectCollisionType = COLLISION_NODE_TYPE_SPHERE;
      }
      else if (nodeType == ExportCollisionNodeType::KDOP)
      {
        fill_kdop_props(node, collisionProps);
      }
      else if (nodeType == ExportCollisionNodeType::CONVEX_COMPUTER)
      {
        collisionProps.objectCollisionType = COLLISION_NODE_TYPE_CONVEX;
        calc_computer(node, collisionObject.mesh);
      }
      else if (nodeType == ExportCollisionNodeType::CONVEX_VHACD)
      {
        collisionProps.objectCollisionType = COLLISION_NODE_TYPE_CONVEX;
        calc_vhacd(node, collisionObject.mesh, collisionObjects, collisionsProps, num_collision_nodes);
      }
    }
  }

  for (int i = 0; i < nodes->blockCount(); ++i)
  {
    const DataBlock *node = nodes->getBlock(i);
    const char *collision = node->getStr("collision", nullptr);
    if (collision && check_collision_type(collision) && node->getBool("replaceNodes", false))
    {
      const DataBlock *refNodes = node->getBlockByName("refNodes");
      erase_replaced(refNodes, dag_objects_list, collision_objects_props, num_collision_nodes);
    }
  }
  append_items(dag_objects_list, collisionObjects.size(), collisionObjects.begin());
  append_items(collision_objects_props, collisionsProps.size(), collisionsProps.begin());
}

static bool preferZstdPacking = false;
static bool allowOodlePacking = false;
static bool writePrecookedFmt = false;

// The vert21 grid frame for one node: the runtime BLAS reconstructs every vertex at a 21-bit cell center
// (node-local -> resource-local via node_tm -> round(f*32) in the 65535/extent frame Grid::buildBLAS uses
// -> cell center -> node-local via inverse node_tm). Storing exporter verts at those exact centers makes
// the geometry the exporter sees identical to what the runtime decodes, so the degeneracy the exporter
// resolves is exactly the degeneracy Jolt sees. det() and inverse(node_tm) -- plus the quantization
// constants -- depend only on (node_tm, model_box), so they are computed ONCE in the constructor and
// reused for every vertex; the old per-point helper recomputed all of them on each call (per welded
// vertex and per degenerate-edge collapse). canSnap is false when the grid cannot map cells back to
// node-local (empty model_box or a non-invertible node tm); callers then keep the original vertex.
struct Vert21Grid
{
  TMatrix nodeTm;
  TMatrix invNodeTm = TMatrix::IDENT; // only meaningful when canSnap
  Point3 boxMin, qScale, qOfs, dec;   // resource-local f -> cell: (f*qScale+qOfs)*32; cell -> ext: cell*dec
  bool canSnap = false;

  Vert21Grid(const TMatrix &node_tm, const BBox3 &model_box)
  {
    const Point3 ext = model_box.width();
    qScale = Point3(65535.f / max(ext.x, 1e-4f), 65535.f / max(ext.y, 1e-4f), 65535.f / max(ext.z, 1e-4f));
    qOfs = -mul(model_box[0], qScale);
    dec = Point3(max(ext.x, 1e-4f) / (65535.f * 32.f), max(ext.y, 1e-4f) / (65535.f * 32.f), max(ext.z, 1e-4f) / (65535.f * 32.f));
    boxMin = model_box[0];
    nodeTm = node_tm;
    const float det = node_tm.det();
    if (!model_box.isempty() && fabsf(det) > 1e-12f)
    {
      invNodeTm = inverse(node_tm, det);
      canSnap = true;
    }
  }

  // node-local point -> clamped 21-bit cell index (the same frame the weld key and Grid::buildBLAS use).
  void cellOf(const Point3 &local, int &cx, int &cy, int &cz) const
  {
    constexpr int maxCell = (1 << 21) - 1;
    const Point3 rl = nodeTm * local; // resource-local position (the space model_box spans)
    cx = min(max((int)((rl.x * qScale.x + qOfs.x) * 32.f + 0.5f), 0), maxCell);
    cy = min(max((int)((rl.y * qScale.y + qOfs.y) * 32.f + 0.5f), 0), maxCell);
    cz = min(max((int)((rl.z * qScale.z + qOfs.z) * 32.f + 0.5f), 0), maxCell);
  }

  // 21-bit cell index -> node-local cell center. Requires canSnap (uses the node-tm inverse).
  Point3 cellCenterToLocal(int cx, int cy, int cz) const
  {
    return invNodeTm * Point3(boxMin.x + cx * dec.x, boxMin.y + cy * dec.y, boxMin.z + cz * dec.z);
  }

  // full node-local -> snapped node-local cell center (returns the point unchanged when !canSnap).
  Point3 snap(const Point3 &local) const
  {
    if (!canSnap)
      return local;
    int cx, cy, cz;
    cellOf(local, cx, cy, cz);
    return cellCenterToLocal(cx, cy, cz);
  }
};

// Weld vertices to the runtime BLAS's 21-bit vert21 grid AND snap every survivor onto its cell center.
// Every mesh node is quantized into ONE whole-model 21-bit grid (the same frame Grid::buildBLAS uses,
// derived here from model_box), so verts the BLAS would collapse to a single cell are merged and the
// survivors are stored at the exact positions the runtime reconstructs (see Vert21Grid::cellCenterToLocal).
// Coincident-cell merging alone is not enough -- a near-collinear sliver whose three verts land in three
// distinct cells survives the merge, yet the cell-center snap flattens it to exactly collinear; the
// caller's degeneracy pass (edge collapse) then resolves it on identical-to-runtime geometry. The cell ->
// new-vertex-index map is a HashedKeyMap keyed by the packed cell (x[20:0] | y[41:21] | z[62:42]); it is
// insert-only. Returns true if anything changed (verts merged and/or snapped).
static bool weld_verts_to_vert21_grid(MeshData &m, const TMatrix &node_tm, const BBox3 &model_box)
{
  const uint32_t vcount = m.vert.size();
  if (vcount == 0)
    return false;
  const Vert21Grid grid(node_tm, model_box);
  // 3 * 21 = 63 bits used, so bit 63 is always clear and ~0ull can never be a real cell -> safe empty key.
  HashedKeyMap<uint64_t, uint32_t, ~uint64_t(0), oa_hashmap_util::MumStepHash<uint64_t>> cellToVert;
  cellToVert.reserve(vcount);
  Tab<int> remap(tmpmem);
  remap.resize(vcount);
  Tab<Point3> welded(tmpmem);
  welded.reserve(vcount);
  bool snappedMoved = false;
  for (int i = 0; i < vcount; i++)
  {
    int cx, cy, cz;
    grid.cellOf(m.vert[i], cx, cy, cz);
    const uint64_t cell = uint64_t(unsigned(cx)) | (uint64_t(unsigned(cy)) << 21) | (uint64_t(unsigned(cz)) << 42);
    auto added = cellToVert.emplace_if_missing(cell);
    if (added.second)
    {
      // exact runtime cell center, or the original vert when the grid can't map cells back (non-invertible tm)
      const Point3 rep = grid.canSnap ? grid.cellCenterToLocal(cx, cy, cz) : m.vert[i];
      snappedMoved |= rep != m.vert[i];
      *added.first = (uint32_t)welded.size();
      welded.push_back(rep);
    }
    remap[i] = (int)*added.first;
  }
  const bool merged = (int)welded.size() != (int)vcount;
  if (!merged && !snappedMoved)
    return false; // no coincident verts and every vert already sits on its cell center
  for (int fi = 0, fe = (int)m.face.size(); fi < fe; fi++)
    for (int k = 0; k < 3; k++)
      m.face[fi].v[k] = (uint32_t)remap[m.face[fi].v[k]];
  m.vert = welded;
  return true;
}

class CollisionExporter : public IDagorAssetExporter
{
public:
  const char *__stdcall getExporterIdStr() const override { return "collision exp"; }

  const char *__stdcall getAssetType() const override { return "collision"; }
  unsigned __stdcall getGameResClassId() const override { return 0xACE50000; }
  unsigned __stdcall getGameResVersion() const override
  {
    static constexpr const int base_ver = 2;
    return base_ver * 12 + 5 + (def_collidable ? 1 : 0) + 2 * (!preferZstdPacking ? 0 : (allowOodlePacking ? 2 : 1 + 6)) +
           (writePrecookedFmt ? 6 : 0);
  }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = a.getTargetFilePath();
  }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  void readDataFromBlk(Tab<GeomMeshHelperDagObject> &dagObjectsList, const DataBlock &blk) { read_data_from_blk(dagObjectsList, blk); }
  bool writeLegacyDump(DagorAsset &a, mkbindump::BinDumpSaveCB &final_cwr, ILogWriter &log, bool do_pack)
  {
    Tab<GeomMeshHelperDagObject> dagObjectsList(tmpmem);
    String fpath(a.getTargetFilePath());
    AScene dagScene;
    if (stricmp(::dd_get_fname_ext(fpath), ".dag") == 0)
    {
      if (!import_geom_mesh_helpers_dag(fpath, dagObjectsList))
      {
        log.addMessage(log.ERROR, "%s: cannot read geometry from %s", a.getName(), fpath);
        return false;
      }

      load_ascene(fpath, dagScene, LASF_NOMATS);
    }
    else
      readDataFromBlk(dagObjectsList, a.props);

    bool forceBoxCollision = a.props.getBool("forceBoxCollision", false);

    GeomNodeTree nodeTree;
    if (const char *skeletonName = a.props.getStr("ref_skeleton", nullptr))
    {
      if (getSkeleton(nodeTree, a.getMgr(), skeletonName, log))
      {
        nodeTree.invalidateWtm();
        nodeTree.calcWtm();
      }
    }

    // Calculate model bounding sphere.

    Tab<Point3> pointsList(tmpmem);
    for (unsigned int objectNo = 0; objectNo < dagObjectsList.size(); objectNo++)
    {
      for (unsigned int vertexNo = 0; vertexNo < dagObjectsList[objectNo].mesh.verts.size(); vertexNo++)
      {
        pointsList.push_back(dagObjectsList[objectNo].mesh.verts[vertexNo] * dagObjectsList[objectNo].wtm);
      }
    }
    BSphere3 boundingSphere = mesh_bounding_sphere(pointsList.data(), pointsList.size());
    if (lengthSq(boundingSphere.c) > sqr(1e9f) || boundingSphere.r2 > sqr(1e9f))
    {
      log.addMessage(log.ERROR, "%s: has invalid geometry loaded from %s", a.getName(), fpath);
      log.addMessage(log.ERROR, "Calculated from vertices bounding sphere: c=" FMT_P3 " r=%f", P3D(boundingSphere.c),
        boundingSphere.r);
      return false;
    }

    if (boundingSphere.isempty())
    {
      boundingSphere.c.zero();
      boundingSphere.r = boundingSphere.r2 = -1;
    }

    final_cwr.writeInt32e(0xACE50000);
    int versionPos = final_cwr.tell();
    final_cwr.writeInt32e(0x20180510);

    final_cwr.beginBlock();

    final_cwr.write32ex(&boundingSphere, sizeof(BSphere3));

    final_cwr.endBlock();

    final_cwr.beginBlock();

    bool blkExist = false;
    int nameId = -1;
    if (a.props.blockCount() > 0)
    {
      blkExist = true;
      nameId = a.props.getNameId("part");
    }

    mkbindump::BinDumpSaveCB mcwr(128 << 10, final_cwr);
    mkbindump::BinDumpSaveCB &cwr = do_pack ? mcwr : final_cwr;

    unsigned int numCollisionNodes = 0;
    bool hasConvexes = false;
    bool collapseConvexes = a.props.getBool("collapseConvexes", false);
    bool nodesHaveFlags = false;
    bool def_coll = a.props.getBool("defCollidable", def_collidable);
    Tab<CollisionObjectProps> collisionObjectsProps;
    for (unsigned int objectNo = 0; objectNo < dagObjectsList.size(); objectNo++)
    {
      uint8_t haveHolesFlags = 0; // check all holes flags (noHoles>>detachablePart>>thinPart>>noBullets)
      DataBlock dagNodeScriptBlk;
      Node *dagNode = NULL;
      if (dagScene.root)
      {
        dagNode = dagScene.root->find_node(dagObjectsList[objectNo].name);
        if (dagNode)
        {
          dblk::load_text(dagNodeScriptBlk, make_span_const(dagNode->script), dblk::ReadFlag::ROBUST, fpath);
          if (!dagNodeScriptBlk.getBool("collidable", def_coll))
            continue;
          nodesHaveFlags |= !dagNodeScriptBlk.getBool("isTraceable", true) || !dagNodeScriptBlk.getBool("isPhysCollidable", true) ||
                            dagNodeScriptBlk.getBool("solid", false);
        }
      }

      CollisionObjectProps &collisionObjectProps = collisionObjectsProps.push_back();
      collisionObjectProps.objectName = dagObjectsList[objectNo].name;
      collisionObjectProps.behaviorFlags = dagNodeScriptBlk.getBool("isTraceable", true) ? CollisionNode::TRACEABLE : 0;
      collisionObjectProps.behaviorFlags |= dagNodeScriptBlk.getBool("isPhysCollidable", true) ? CollisionNode::PHYS_COLLIDABLE : 0;
      collisionObjectProps.behaviorFlags |= dagNodeScriptBlk.getBool("solid", false) ? CollisionNode::SOLID : 0;
      collisionObjectProps.behaviorFlags |= dagNodeScriptBlk.getBool("isTransparent", false) ? CollisionNode::FLAG_TRANSPARENT : 0;
      haveHolesFlags |= dagNodeScriptBlk.paramExists("noOverlapHoles") ? CollisionNode::FLAG_ALLOW_HOLE : 0;
      haveHolesFlags |= dagNodeScriptBlk.paramExists("noOverlapHolesIfNoDamage") ? CollisionNode::FLAG_DAMAGE_REQUIRED : 0;
      haveHolesFlags |= dagNodeScriptBlk.paramExists("noOverlapHolesIfNoCut") ? CollisionNode::FLAG_CUT_REQUIRED : 0;
      haveHolesFlags |= dagNodeScriptBlk.paramExists("noOverlapEdgeHoles") ? CollisionNode::FLAG_CHECK_SIDE : 0;
      haveHolesFlags |= dagNodeScriptBlk.paramExists("noOverlapSplahDamage") ? CollisionNode::FLAG_ALLOW_SPLASH_HOLE : 0;
      haveHolesFlags |= dagNodeScriptBlk.paramExists("noBullets") ? CollisionNode::FLAG_ALLOW_BULLET_DECAL : 0;
      haveHolesFlags |=
        dagNodeScriptBlk.paramExists("noOverlapHolesForSurrounding") ? CollisionNode::FLAG_CHECK_SURROUNDING_PART_FOR_EXCLUSION : 0;

      // Get node collision type.

      int type = COLLISION_NODE_TYPE_MESH;
      bool tree_capsule = false;
      if (!dd_strnicmp(dagObjectsList[objectNo].name, "_Clip", (int)strlen("_Clip")))
      {
        type = COLLISION_NODE_TYPE_POINTS;
      }
      else if (forceBoxCollision)
      {
        type = COLLISION_NODE_TYPE_BOX;
      }
      else if (dagNode)
      {
        const char *collision = dagNodeScriptBlk.getStr("collision", NULL);
        if (collision)
        {
          if (!stricmp(collision, "box"))
            type = COLLISION_NODE_TYPE_BOX;
          else if (!stricmp(collision, "capsule"))
            type = COLLISION_NODE_TYPE_CAPSULE;
          else if (!stricmp(collision, "tree_capsule"))
            type = COLLISION_NODE_TYPE_CAPSULE, tree_capsule = true;
          else if (!stricmp(collision, "sphere"))
            type = COLLISION_NODE_TYPE_SPHERE;
          else if (!stricmp(collision, "convex"))
          {
            type = COLLISION_NODE_TYPE_CONVEX;
            hasConvexes = true;
          }
        }
      }
      collisionObjectProps.objectCollisionType = type;
      collisionObjectProps.objectTreeCapsule = tree_capsule;

      if (a.props.getBlockByNameEx("nodes")->blockExists(dagObjectsList[objectNo].name))
      {
        DataBlock *props = a.props.getBlockByName("nodes")->getBlockByName(dagObjectsList[objectNo].name);
        const char *collision = props->getStr("collision", NULL);
        if (collision)
        {
          if (!strcmp(collision, "kdop"))
          {
            collisionObjectProps.objectCollisionType = COLLISION_NODE_TYPE_CONVEX;
            collisionObjectProps.objectHasKdop = true;
            collisionObjectProps.objectKdopPreset = props->getInt("kdopPreset", -1);
            collisionObjectProps.objectKdopSegmentsX = props->getInt("kdopSegmentsX", -1);
            collisionObjectProps.objectKdopSegmentsY = props->getInt("kdopSegmentsY", -1);
            collisionObjectProps.objectKdopRotX = props->getInt("kdopRotX", 0);
            collisionObjectProps.objectKdopRotY = props->getInt("kdopRotY", 0);
            collisionObjectProps.objectKdopRotZ = props->getInt("kdopRotZ", 0);
            collisionObjectProps.objectKdopCutOffThreshold = props->getReal("cutOffThreshold", 0.0f);
          }
        }
      }

      TMatrix wtm = dagObjectsList[objectNo].wtm;

      Tab<Point3> &verts = dagObjectsList[objectNo].mesh.verts;

      // Calculate bounding box.
      BBox3 bbox;
      for (unsigned int vertexNo = 0; vertexNo < verts.size(); vertexNo++)
      {
        if (type != COLLISION_NODE_TYPE_CAPSULE || tree_capsule)
          bbox += verts[vertexNo] * wtm;
        else
          bbox += verts[vertexNo];
      }

      // if bbox is a cube, capsule should collapse into a sphere, so override it (to avoid invalid capsule)
      if (type == COLLISION_NODE_TYPE_CAPSULE &&
          (abs(bbox.width().z - bbox.width().x) <= 0.01f || abs(bbox.width().z - bbox.width().y) <= 0.01f))
        collisionObjectProps.objectCollisionType = type = COLLISION_NODE_TYPE_SPHERE;
      if (tree_capsule)
      {
        float tree_rad = dagNodeScriptBlk.getReal("tree_radius", 0.1);
        bbox[0].x = -tree_rad;
        bbox[0].z = -tree_rad;
        bbox[1].x = tree_rad;
        bbox[1].z = tree_rad;
      }
      collisionObjectProps.boundingBoxs = bbox;

      if (haveHolesFlags & CollisionNode::FLAG_ALLOW_HOLE) // no holes
      {
        if (!dagNodeScriptBlk.getBool("noOverlapHoles", false))
          collisionObjectProps.behaviorFlags |= CollisionNode::FLAG_ALLOW_HOLE;
      }
      else if (blkExist)
      {
        const DataBlock *excludePartsBlk = a.props.getBlockByNameEx("excludeParts");
        if (!find_dm_parts_name(excludePartsBlk, nameId, dagObjectsList[objectNo].name))
          collisionObjectProps.behaviorFlags |= CollisionNode::FLAG_ALLOW_HOLE;
      }
      if (haveHolesFlags & CollisionNode::FLAG_DAMAGE_REQUIRED) // Damaged part
      {
        collisionObjectProps.behaviorFlags |=
          dagNodeScriptBlk.getBool("noOverlapHolesIfNoDamage", true) ? CollisionNode::FLAG_DAMAGE_REQUIRED : 0;
      }
      else if (blkExist)
      {
        const DataBlock *excludeNonDamagePartsBlk = a.props.getBlockByNameEx("excludeNonDamageParts");
        collisionObjectProps.behaviorFlags |= find_dm_parts_name(excludeNonDamagePartsBlk, nameId, dagObjectsList[objectNo].name)
                                                ? CollisionNode::FLAG_DAMAGE_REQUIRED
                                                : 0;
      }
      if (haveHolesFlags & CollisionNode::FLAG_CUT_REQUIRED) // deteachable part
      {
        collisionObjectProps.behaviorFlags |=
          dagNodeScriptBlk.getBool("noOverlapHolesIfNoCut", false) ? CollisionNode::FLAG_CUT_REQUIRED : 0;
      }
      else if (blkExist)
      {
        const DataBlock *excludeNonCutPartsBlk = a.props.getBlockByNameEx("excludeNonCutParts");
        collisionObjectProps.behaviorFlags |=
          find_dm_parts_name(excludeNonCutPartsBlk, nameId, dagObjectsList[objectNo].name) ? CollisionNode::FLAG_CUT_REQUIRED : 0;
      }
      if (haveHolesFlags & CollisionNode::FLAG_CHECK_SIDE) // thin part
      {
        collisionObjectProps.behaviorFlags |=
          dagNodeScriptBlk.getBool("noOverlapEdgeHoles", false) ? CollisionNode::FLAG_CHECK_SIDE : 0;
      }
      else if (blkExist)
      {
        const DataBlock *checkPartsSideSizeBlk = a.props.getBlockByNameEx("checkPartsSideSize");
        collisionObjectProps.behaviorFlags |=
          find_dm_parts_name(checkPartsSideSizeBlk, nameId, dagObjectsList[objectNo].name) ? CollisionNode::FLAG_CHECK_SIDE : 0;
      }
      if (haveHolesFlags & CollisionNode::FLAG_ALLOW_BULLET_DECAL)
      {
        collisionObjectProps.behaviorFlags |=
          !dagNodeScriptBlk.getBool("noBullets", false) ? CollisionNode::FLAG_ALLOW_BULLET_DECAL : 0;
      }
      else if (blkExist)
      {
        const DataBlock *checkPartsSideSizeBlk = a.props.getBlockByNameEx("excludeFromBulletDecal");
        collisionObjectProps.behaviorFlags |= !find_dm_parts_name(checkPartsSideSizeBlk, nameId, dagObjectsList[objectNo].name)
                                                ? CollisionNode::FLAG_ALLOW_BULLET_DECAL
                                                : 0;
      }
      if (haveHolesFlags & CollisionNode::FLAG_ALLOW_SPLASH_HOLE) // no internal part
      {
        collisionObjectProps.behaviorFlags |=
          dagNodeScriptBlk.getBool("noOverlapSplahDamage", false) ? CollisionNode::FLAG_ALLOW_SPLASH_HOLE : 0;
      }
      else if (blkExist)
      {
        const DataBlock *checkPartsSideSizeBlk = a.props.getBlockByNameEx("excludeFromSplashDecal");
        collisionObjectProps.behaviorFlags |= !find_dm_parts_name(checkPartsSideSizeBlk, nameId, dagObjectsList[objectNo].name)
                                                ? CollisionNode::FLAG_ALLOW_SPLASH_HOLE
                                                : 0;
      }
      if (haveHolesFlags & CollisionNode::FLAG_CHECK_SURROUNDING_PART_FOR_EXCLUSION) // no internal part
      {
        collisionObjectProps.behaviorFlags |= dagNodeScriptBlk.getBool("noOverlapHolesForSurrounding", false)
                                                ? CollisionNode::FLAG_CHECK_SURROUNDING_PART_FOR_EXCLUSION
                                                : 0;
      }
      else if (blkExist)
      {
        const DataBlock *checkPartsSideSizeBlk = a.props.getBlockByNameEx("excludePartsThatAreInContact");
        collisionObjectProps.behaviorFlags |= !find_dm_parts_name(checkPartsSideSizeBlk, nameId, dagObjectsList[objectNo].name)
                                                ? CollisionNode::FLAG_CHECK_SURROUNDING_PART_FOR_EXCLUSION
                                                : 0;
      }
      nodesHaveFlags |= (collisionObjectProps.behaviorFlags &
                          ~(CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE | CollisionNode::SOLID)) != 0;
      numCollisionNodes++;
    }

    uint32_t collisionFlags = 0;
    collisionFlags |= collapseConvexes ? COLLISION_RES_FLAG_COLLAPSE_CONVEXES : 0;
    collisionFlags |= nodesHaveFlags ? COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS : 0;
    collisionFlags |= (nodeTree.nodeCount() > 0) ? COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID : 0;
    if (collisionFlags)
      cwr.writeInt32e(collisionFlags);

    collision_preprocessing(a.props.getBlockByNameEx("nodes"), dagObjectsList, collisionObjectsProps, numCollisionNodes);

    cwr.writeInt32e(numCollisionNodes);
    unsigned int numExportedNodes = 0;
    for (unsigned int objectNo = 0; objectNo < dagObjectsList.size(); objectNo++)
    {
      // Skip non-collision nodes.

      DataBlock dagNodeScriptBlk;
      Node *dagNode = NULL;
      if (dagScene.root)
      {
        dagNode = dagScene.root->find_node(dagObjectsList[objectNo].name);
        if (dagNode)
        {
          dblk::load_text(dagNodeScriptBlk, make_span_const(dagNode->script), dblk::ReadFlag::ROBUST, fpath);
          if (!dagNodeScriptBlk.getBool("collidable", def_coll))
            continue;
        }
      }
      numExportedNodes++;

      CollisionObjectProps &collisionObjectProps = collisionObjectsProps[numExportedNodes - 1];

      int type = collisionObjectProps.objectCollisionType;
      bool tree_capsule = collisionObjectProps.objectTreeCapsule;
      bool hasKdop = collisionObjectProps.objectHasKdop;
      int kdopPreset = collisionObjectProps.objectKdopPreset;
      int kdopSegmentsX = collisionObjectProps.objectKdopSegmentsX;
      int kdopSegmentsY = collisionObjectProps.objectKdopSegmentsY;
      int kdopRotX = collisionObjectProps.objectKdopRotX;
      int kdopRotY = collisionObjectProps.objectKdopRotY;
      int kdopRotZ = collisionObjectProps.objectKdopRotZ;
      float kdopCutOffThreshold = collisionObjectProps.objectKdopCutOffThreshold;
      // Write collision node header.

      if (a.props.getBool("removeDMSuffix", false))
      {
        SimpleString nameWithoutSuffix;
        remove_dm_suffix(dagObjectsList[objectNo].name, nameWithoutSuffix);
        cwr.writeDwString(nameWithoutSuffix);
      }
      else
        cwr.writeDwString(dagObjectsList[objectNo].name);

      if (collisionObjectProps.physMat.empty())
        cwr.writeDwString(dagNodeScriptBlk.getStr("phmat", ""));
      else
        cwr.writeDwString(collisionObjectProps.physMat.str());

      TMatrix wtm = dagObjectsList[objectNo].wtm;

      Tab<Point3> &verts = dagObjectsList[objectNo].mesh.verts;

      // Calculate  node bounding sphere and bounding box.

      BSphere3 boundingSphere;
      BBox3 bbox = collisionObjectProps.boundingBoxs;
      for (unsigned int vertexNo = 0; vertexNo < verts.size(); vertexNo++)
      {
        if (type == COLLISION_NODE_TYPE_SPHERE)
          boundingSphere += verts[vertexNo] * wtm;
        else
          boundingSphere += verts[vertexNo];
      }
      if (boundingSphere.isempty())
      {
        boundingSphere.c.zero();
        boundingSphere.r = boundingSphere.r2 = -1;
      }

      // Check in Contact with excludedPart
      if (collisionObjectProps.behaviorFlags & CollisionNode::FLAG_ALLOW_HOLE)
      {
        if (a.props.blockCount() > 0)
        {
          const DataBlock *excludePartsInContactWithExcludedPartBlk =
            a.props.getBlockByNameEx("excludePartsInContactWithExcludedPart");
          if (find_dm_parts_name(excludePartsInContactWithExcludedPartBlk, nameId, dagObjectsList[objectNo].name))
          {
            for (int j = 0; j < collisionObjectsProps.size(); ++j)
            {
              if (!(collisionObjectsProps[j].behaviorFlags & CollisionNode::FLAG_CHECK_SURROUNDING_PART_FOR_EXCLUSION)) // excludedPart
                if (collisionObjectsProps[j].boundingBoxs & bbox)
                {
                  collisionObjectProps.behaviorFlags &= ~CollisionNode::FLAG_ALLOW_HOLE;
                  break;
                }
            }
          }
        }
      }

      cwr.writeInt32e(type | (collisionObjectProps.behaviorFlags & 0xFF00));
      if ((collisionFlags & COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS) == COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS)
      {
        uint8_t behFlagFirstByte = collisionObjectProps.behaviorFlags & 0xFF;
        cwr.writeRaw(&collisionObjectProps.behaviorFlags, sizeof(behFlagFirstByte));
      }

      cwr.write32ex(tree_capsule ? &TMatrix::IDENT : &wtm, sizeof(TMatrix));
      if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
      {
        TMatrix relGeomNodeTm = TMatrix::IDENT;
        String nodeName = dagObjectsList[objectNo].name;
        if (a.props.getBool("removeDMSuffix", false))
          remove_dm_suffix(dagObjectsList[objectNo].name, nodeName);

        if (auto geomNodeIdx = nodeTree.findINodeIndex(nodeName.str()))
        {
          TMatrix geomNodeWtm;
          nodeTree.getNodeWtmRelScalar(geomNodeIdx, geomNodeWtm);
          relGeomNodeTm = inverse(geomNodeWtm) * wtm;
        }
        cwr.write32ex(&relGeomNodeTm, sizeof(TMatrix));
      }

      // write bouding sphere and box

      cwr.write32ex(&boundingSphere, sizeof(BSphere3));
      cwr.write32ex(&bbox, sizeof(BBox3));

      Kdop kdop;
      if (hasKdop)
      {
        kdop.setPreset(static_cast<KdopPreset>(kdopPreset), kdopCutOffThreshold, kdopSegmentsX, kdopSegmentsY);
        kdop.setRotation(Point3(kdopRotX, kdopRotY, kdopRotZ));
        kdop.calcKdop(verts, TMatrix::IDENT);
      }

      // Write vertices and indices.
      if (type == COLLISION_NODE_TYPE_CONVEX && !hasKdop)
      {
        // First recreate planes
        Tab<Plane3> convexPlanes(tmpmem);
        for (int i = 0; i < dagObjectsList[objectNo].mesh.faces.size(); ++i)
        {
          const GeomMeshHelper::Face &face = dagObjectsList[objectNo].mesh.faces[i];
          Plane3 plane(verts[face.v[0]], verts[face.v[2]], verts[face.v[1]]);
          plane.normalize();
          for (unsigned int vertexNo = 0; vertexNo < verts.size(); vertexNo++)
          {
            float dist = plane.distance(verts[vertexNo]);
            float distEps = max(boundingSphere.r * 1e-3f, 1e-2f);
            if (dist > distEps)
              log.addMessage(log.ERROR, "%s: not a convex %s in node '%s', dist %f", a.getName(), fpath, dagObjectsList[objectNo].name,
                dist);
          }
          convexPlanes.push_back(plane);
        }
        // Now we can save them
        // TODO: think about throwing off any nonconvex planes. It'll take some performance, but it could reduce number of errors as
        // well
        cwr.writeInt32e(convexPlanes.size());
        cwr.writeTabData32ex(convexPlanes);
      }
      else if (type == COLLISION_NODE_TYPE_CONVEX && hasKdop)
      {
        Tab<Plane3> convexPlanes(tmpmem);
        for (int i = 0; i < kdop.planeDefinitions.size(); ++i)
        {
          if (kdop.planeDefinitions[i].vertices.size() > 0)
          {
            Plane3 plane(kdop.planeDefinitions[i].planeNormal, kdop.planeDefinitions[i].vertices[0]);
            convexPlanes.push_back(plane);
          }
        }
        cwr.writeInt32e(convexPlanes.size());
        cwr.writeTabData32ex(convexPlanes);
      }

      if ((type == COLLISION_NODE_TYPE_MESH || type == COLLISION_NODE_TYPE_CONVEX) && !hasKdop)
      {
        if (!dagObjectsList[objectNo].mesh.faces.size())
          log.addMessage(log.WARNING, "%s: node <%s> with type=%s contains empty mesh (face.count=%d vert.count=%d)", a.getName(),
            dagObjectsList[objectNo].name, (type == COLLISION_NODE_TYPE_MESH) ? "mesh" : "convex",
            dagObjectsList[objectNo].mesh.faces.size(), verts.size());
        cwr.writeInt32e(verts.size());
        cwr.writeTabData32ex(verts);
        cwr.writeInt32e(dagObjectsList[objectNo].mesh.faces.size() * 3);
        cwr.write32ex(dagObjectsList[objectNo].mesh.faces.data(), (int)(dagObjectsList[objectNo].mesh.faces.size() * sizeof(int) * 3));
      }
      else if ((type == COLLISION_NODE_TYPE_MESH || type == COLLISION_NODE_TYPE_CONVEX) && hasKdop)
      {
        Tab<Point3> kdopVerts(tmpmem);
        Tab<int> kdopFaces(tmpmem);
        for (int i = 0; i < kdop.vertices.size(); ++i)
        {
          kdopVerts.push_back(kdop.vertices[i]);
        }
        for (int i = 0; i < kdop.indices.size(); ++i)
        {
          kdopFaces.push_back(kdop.indices[i]);
        }
        cwr.writeInt32e(kdopVerts.size());
        cwr.writeTabData32ex(kdopVerts);
        cwr.writeInt32e(kdopFaces.size());
        cwr.writeTabData32ex(kdopFaces);
      }
      else
      {
        cwr.writeInt32e(0);
        cwr.writeInt32e(0);
      }
    }

    G_ASSERT(numExportedNodes == numCollisionNodes);

    if (!do_pack)
      final_cwr.endBlock(btag_compr::NONE);
    else if (cwr.getSize() < 512) // no sence in compressing
    {
      cwr.copyDataTo(final_cwr.getRawWriter());
      final_cwr.endBlock(btag_compr::NONE);
    }
    else
    {
      mkbindump::BinDumpSaveCB mcwr(cwr.getSize(), final_cwr);
      MemoryLoadCB mcrd(cwr.getRawWriter().getMem(), false);

      if (allowOodlePacking)
      {
        mcwr.writeInt32e(cwr.getSize());
        oodle_compress_data(mcwr.getRawWriter(), mcrd, cwr.getSize());
      }
      else
        zstd_compress_data(mcwr.getRawWriter(), mcrd, cwr.getSize(), 256 << 10, 19);

      if (mcwr.getSize() < cwr.getSize() * 8 / 10 && mcwr.getSize() + 256 < cwr.getSize()) // enough profit in compression
      {
        mcwr.copyDataTo(final_cwr.getRawWriter());
        final_cwr.endBlock(allowOodlePacking ? btag_compr::OODLE : btag_compr::ZSTD);
      }
      else
      {
        cwr.copyDataTo(final_cwr.getRawWriter());
        final_cwr.endBlock(btag_compr::NONE);
      }
    }

    if (!hasConvexes || !collisionFlags)
    {
      final_cwr.seekto(versionPos);
      final_cwr.writeInt32e(collisionFlags ? 0x20180510 : (hasConvexes ? 0x20160120 : 0x20150115));
      final_cwr.seekToEnd();
    }

    return true;
  }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    bool collapse_and_optimize = a.props.getBool("collapseAndOptimize", false);
    if (!writePrecookedFmt && !collapse_and_optimize) // legacy format
      return writeLegacyDump(a, cwr, log, preferZstdPacking);

    // modern (pre-cooked) format

    // first we write legacy format and read it back to CollisionResource object
    mkbindump::BinDumpSaveCB mcwr(128 << 10, cwr);
    if (!writeLegacyDump(a, mcwr, log, false))
      return false;
    MemoryLoadCB mcrd(mcwr.getRawWriter().getMem(), false);
    G_ASSERT(mcrd.readInt() == 0xACE50000);
    CollisionResource coll;
    coll.loadLegacyRawFormat(mcrd, -1, resolve_phmat);

    // Set by remove_degenerate_faces when it rewrites a modelBBox (REPLACE) or erases a node (DROP) -- i.e.
    // when node sort order / containment can change relative to the last sortNodesList. collapseAndOptimize
    // ends with its own sortNodesList, so a sort there clears this again. Gates the pre-serialization
    // containment rebuild below: re-sorting when nothing changed would, since stlsort is not stable, risk
    // reordering equal-key (same size + name) nodes and emitting a binary diff where there should be none.
    bool containmentDirty = false;

    auto remove_degenerate_faces = [&](const char *label) {
      unsigned degenerate_meshes_cnt = 0;
      unsigned bad_tm_cnt = 0;
      // Per-mesh-node decision: KEEP existing slice, REPLACE with welded MeshData, or DROP entirely.
      // We stage decisions, then rebuild ownVertices/ownIndices in a single pass after the loop.
      enum class NodeAction : uint8_t
      {
        KEEP,
        REPLACE,
        DROP
      };
      dag::Vector<NodeAction> actions(coll.allNodesList.size(), NodeAction::KEEP);
      dag::Vector<MeshData> meshes(coll.allNodesList.size());

      // The per-node degeneracy pipeline: distance-weld, bad/degenerate face removal, KEEP/REPLACE/DROP
      // decision. Both the initial pass and the post-vert21 re-process below funnel through this single
      // lambda so the logic lives in one place. The decision is always relative to the ORIGINAL source
      // slice (coll is not rebuilt until the end), so a node that vert21 later shrinks turns KEEP->REPLACE.
      // vert21_box != nullptr in the post-weld pass: a degenerate triangle's edge collapse snaps the merged
      // vertex back onto that vert21 grid so the runtime reconstructs it exactly. nullptr in the pre-weld
      // pass (no grid yet) -> collapse to the plain midpoint.
      auto process_mesh_node = [&](const CollisionNode &n, MeshData &m, const BBox3 *vert21_box) -> NodeAction {
        auto srcVerts = coll.getNodeVertices(n.nodeIndex);
        auto srcIndices = coll.getNodeIndices(n.nodeIndex);
        const TMatrix &nTm = coll.getNodeTm(n.nodeIndex);
        const float maxTmScale = coll.getNodeMaxTmScale(n.nodeIndex);
        const float weld_eps = a.props.getReal("meshVertWeldEps", 1e-3f) * safeinv(maxTmScale);
        unsigned zeroarea_faces_cnt = 0;
        m.kill_unused_verts(weld_eps * weld_eps);
        // Strip only TOPOLOGICAL degenerates (duplicate-index faces, e.g. from a coincident-cell weld merge):
        // those have a zero-length edge the collapse loop cannot repair and must go. Pass threshold 0 so
        // GEOMETRIC zero-area-but-distinct faces survive to the edge-collapse loop below and get repaired
        // (merge two verts) instead of deleted outright -- deletion would skip the watertight-preserving path.
        m.kill_bad_faces(0.f);
        // Resolve degenerate triangles by EDGE COLLAPSE (merge two verts) rather than face deletion, so a
        // watertight mesh stays watertight where deleting the triangle would punch a hole. Two criteria:
        //   1. Jolt-degenerate: (2*area)^2 <= 1e-12 (Jolt Vec3::IsNearZero default). Such a triangle fails
        //      MeshShape creation and fatals the load, so it is resolved for every node (no maxTmScale gate).
        //      Judged in resource-local space (node tm applied): a collapseAndOptimize asset bakes the node
        //      tm to identity, so that is the only space the runtime ever feeds Jolt. A LEGACY (un-collapsed)
        //      asset is not baked at export and can reach Jolt in EITHER space -- the client bakes the tm at
        //      load (rendinst optimize_collres_on_load -> resource-local), the dedicated server does not
        //      (allowOptimizeCollResOnLoad=false -> node-local verts with the tm applied as a separate shape
        //      transform). So a legacy triangle is ALSO rejected when degenerate in NODE-LOCAL space, matching
        //      validateVerticesForJolt (node-local) and the unbaked path. The node-local arm is legacy-only:
        //      on a collapsed asset it would wrongly drop a tiny-but-fine-once-baked triangle in a >1 node.
        //   2. Sliver cleanup: the configurable, looser degenerativeTriAreaThresholdSq (default 5e-12),
        //      kept gated to un-scaled nodes -- a quality tunable, not a Jolt requirement.
        // Each collapse merges the shortest edge's two verts to their midpoint, snapped back onto the vert21
        // grid (post-weld pass) so the runtime reproduces the result exactly. The collapsed triangle and its
        // neighbour across that edge gain a duplicate index; kill_bad_faces drops them. Iterate: moving a
        // vert can flatten another triangle.
        constexpr float jolt_degenerate_cross_sq = 1e-12f; // matches Jolt/Geometry/IndexedTriangle.h IsDegenerate
        // Precompute the vert21 grid frame once per node: a collapse below snaps the merged vertex back onto
        // it, and det()+inverse(node tm) are node-constant, so they must not be recomputed per collapse.
        const Vert21Grid vert21Grid(nTm, vert21_box ? *vert21_box : BBox3());
        for (bool more = true; more;)
        {
          more = false;
          for (unsigned i = 0; i < m.face.size(); i++)
          {
            const uint32_t ia = m.face[i].v[0], ib = m.face[i].v[1], ic = m.face[i].v[2];
            if (ia == ib || ib == ic || ia == ic)
              continue; // duplicate-index face from a prior collapse; kill_bad_faces will drop it
            const Point3 w0 = nTm * m.vert[ia], w1 = nTm * m.vert[ib], w2 = nTm * m.vert[ic];
            const float crossSqRl = lengthSq((w1 - w0) % (w2 - w0));
            const float crossSqLocal = lengthSq((m.vert[ib] - m.vert[ia]) % (m.vert[ic] - m.vert[ia]));
            // resource-local always; node-local too for a legacy asset (it can ship to Jolt unbaked -- see above)
            const bool joltDegen =
              crossSqRl <= jolt_degenerate_cross_sq || (!collapse_and_optimize && crossSqLocal <= jolt_degenerate_cross_sq);
            const bool sliver = maxTmScale <= 1.0f && crossSqLocal < degenerate_tri_area_threshold_sq;
            if (!joltDegen && !sliver)
              continue;
            const uint32_t ends[3][2] = {{ia, ib}, {ib, ic}, {ic, ia}};
            const float elen[3] = {
              lengthSq(m.vert[ia] - m.vert[ib]), lengthSq(m.vert[ib] - m.vert[ic]), lengthSq(m.vert[ic] - m.vert[ia])};
            const int se = (elen[0] <= elen[1] && elen[0] <= elen[2]) ? 0 : (elen[1] <= elen[2] ? 1 : 2);
            const uint32_t keep = min(ends[se][0], ends[se][1]), drop = max(ends[se][0], ends[se][1]);
            Point3 merged = (m.vert[keep] + m.vert[drop]) * 0.5f;
            if (vert21_box)
              merged = vert21Grid.snap(merged);
            m.vert[keep] = merged;
            for (auto &ff : m.face)
              for (int k = 0; k < 3; k++)
                if (ff.v[k] == drop)
                  ff.v[k] = keep;
            zeroarea_faces_cnt++;
            more = true;
            //  logwarn("%s: %sedge-collapse degenerate tri %u,%u,%u node \"%s\": v%u+v%u -> %@ (crossSq.rl=%g local=%g %s)",
            //  a.getName(), label, ia, ib, ic, coll.getNodeName(n.nodeIndex), keep, drop, nTm * merged, crossSqRl, crossSqLocal,
            //  joltDegen ? "JOLT-REJECT" : "sliver");
          }
          if (more)
            m.kill_bad_faces(0.f);
        }
        if (m.face.size() * 3 != srcIndices.size())
          m.kill_unused_verts(-1);
        if (m.vert.size() == srcVerts.size() && m.face.size() * 3 == srcIndices.size())
          return NodeAction::KEEP;
        if (m.vert.size() < 3 || m.face.size() < 1)
        {
          degenerate_meshes_cnt++;
          if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_ERROR)
            log.addMessage(log.ERROR, "%s: %sdegenerate mesh node \"%s\": vert=%d->%d face=%d->%d maxTmScale=%g eps=%g bbox=%@",
              a.getName(), label, coll.getNodeName(n.nodeIndex), (int)srcVerts.size(), m.vert.size(), (int)srcIndices.size() / 3,
              m.face.size(), maxTmScale, weld_eps, coll.getNodeBBox(n.nodeIndex));
          else
            logwarn("%s: %sdegenerate mesh node \"%s\": vert=%d->%d face=%d->%d maxTmScale=%g eps=%g bbox=%@", a.getName(), label,
              coll.getNodeName(n.nodeIndex), (int)srcVerts.size(), m.vert.size(), (int)srcIndices.size() / 3, m.face.size(),
              maxTmScale, weld_eps, coll.getNodeBBox(n.nodeIndex));
          for (unsigned i = 0; i < srcVerts.size(); i++)
            debug("  v[%3d]=%+g,%+g,%+g", i, srcVerts[i].x, srcVerts[i].y, srcVerts[i].z);
          for (unsigned i = 0; i < srcIndices.size(); i += 3)
            debug("  f[%3d]=%u, %u, %u", i / 3, srcIndices[i + 0], srcIndices[i + 1], srcIndices[i + 2]);
          for (unsigned i = 0; i < m.vert.size(); i++)
            debug("  mv[%d]=%+g,%+g,%+g", i, m.vert[i].x, m.vert[i].y, m.vert[i].z);
          return (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_REMOVE) ? NodeAction::DROP : NodeAction::KEEP;
        }
        if (zeroarea_faces_cnt)
          logwarn("%s: %soptimized mesh node \"%s\": vert=%d->%d face=%d->%d, weld_eps=%g (%d degenerate tris edge-collapsed)",
            a.getName(), label, coll.getNodeName(n.nodeIndex), (int)srcVerts.size(), m.vert.size(), (int)srcIndices.size() / 3,
            m.face.size(), weld_eps, zeroarea_faces_cnt);
        else
          logwarn("%s: %soptimized mesh node \"%s\": vert=%d->%d face=%d->%d, weld_eps=%g", a.getName(), label,
            coll.getNodeName(n.nodeIndex), (int)srcVerts.size(), m.vert.size(), (int)srcIndices.size() / 3, m.face.size(), weld_eps);
        return NodeAction::REPLACE;
      };

      // Pass 1: process every mesh node WITHOUT the vert21 weld (exactly as the export did before the weld
      // was added), filling meshes[] and the KEEP/REPLACE/DROP decision. The per-behavior weld boxes are
      // built from the survivors AFTER this pass (below), so verts removed/collapsed here -- and DROP'd
      // nodes -- never widen the grid the runtime reconstructs from.
      for (auto &n : coll.allNodesList)
        if (n.type == COLLISION_NODE_TYPE_MESH)
        {
          const TMatrix &nTm = coll.getNodeTm(n.nodeIndex);
          if (!*label && nTm.det() > 0) // require left matrix in initial data
          {
            if (report_inverted_mesh_tm)
              log.addMessage(log.ERROR, "%s: bad mesh node \"%s\" tm=%@", a.getName(), coll.getNodeName(n.nodeIndex), nTm);
            else
              logwarn("%s: bad mesh node \"%s\" tm=%@", a.getName(), coll.getNodeName(n.nodeIndex), nTm);
            bad_tm_cnt++;
          }
          MeshData &m = meshes[n.nodeIndex];
          auto srcVerts = coll.getNodeVertices(n.nodeIndex);
          auto srcIndices = coll.getNodeIndices(n.nodeIndex);
          m.vert.resize(srcVerts.size());
          for (unsigned i = 0; i < m.vert.size(); i++)
            m.vert[i] = srcVerts[i];
          m.face.resize(srcIndices.size() / 3);
          for (unsigned i = 0; i < m.face.size(); i++)
            for (unsigned fi = 0; fi < 3; fi++)
              m.face[i].v[fi] = srcIndices[i * 3 + fi];
          actions[n.nodeIndex] = process_mesh_node(n, m, /*vert21_box*/ nullptr);
        }

      // Snap the verts onto the runtime's vert21 grid PER BEHAVIOR GRID, matching CollisionResource::Grid::
      // buildBLAS: the runtime builds a separate BLAS for TRACEABLE and (when the node sets differ)
      // PHYS_COLLIDABLE, each quantizing against the bbox of ITS eligible nodes. A node must weld against the
      // same box the runtime reconstructs it from, or the snapped verts land off the runtime's cell centers
      // and the degeneracy the exporter validated is not the degeneracy Jolt sees. The box spans the SURVIVING
      // (non-DROP) nodes' post-pass-1 verts -- removed/collapsed verts (and DROP'd nodes) are excluded, since
      // the runtime's box never sees them. PHYS_COLLIDABLE is the grid that feeds Jolt, so it is authoritative
      // for a node; a trace-only node uses the TRACEABLE box. (Equal sets -> REUSE_TRACE_FRT -> one runtime
      // grid -> the boxes coincide.) The runtime builds this quantizing BLAS for EITHER asset form: a
      // collapse_and_optimize asset is baked + BLAS-built here at export; a legacy precooked asset
      // (writePrecookedFmt, nodes still carry their tm on disk) is baked + BLAS-built at CLIENT load
      // (rendinst optimize_collres_on_load) against the same per-behavior union box. So both must ship verts
      // already sitting on that grid -- the snap loop below runs unconditionally in this modern path. Dedicated
      // keeps the legacy nodes unbaked and feeds Jolt the raw verts, but the sub-mm snap to cell centers is
      // harmless there and the node-local degeneracy arm above still guards that unbaked path.
      // A SOLID mesh node makes buildBLAS skip the BLAS for THAT behavior grid only (the behavior filter at
      // collisionGameResLoad.cpp:1046 runs before the SOLID return at :1048), so the veto is per behavior: a
      // trace-only SOLID node empties the trace box but must NOT empty the collidable box (Jolt still quantizes it).
      // Exclude already-DROP'd SOLID nodes: a DROP'd node is erased before serialization, so the runtime never
      // sees it and DOES build the grid for that behavior -- counting it would wrongly empty the box and make the
      // survivors skip their snap, shipping the un-snapped slivers this pass exists to remove. A node can DROP in
      // the post-collapse pass (sliver check, ungated once collapseAndOptimize bakes a >1 node scale to IDENT)
      // after surviving the pre-collapse pass, so this filter matters on the final, serialized snap.
      bool anySolidTraceable = false, anySolidCollidable = false;
      for (const auto &n : coll.allNodesList)
        if (
          n.type == COLLISION_NODE_TYPE_MESH && actions[n.nodeIndex] != NodeAction::DROP && n.checkBehaviorFlags(CollisionNode::SOLID))
        {
          anySolidTraceable |= n.checkBehaviorFlags(CollisionNode::TRACEABLE);
          anySolidCollidable |= n.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE);
        }

      // The snap itself can still turn a node into DROP (process_mesh_node below, when the vert21 grid flattens
      // a thin sliver to <3 verts). A DROP'd node is not serialized, so it must not widen the box the survivors
      // snap to -- yet the box is built from the survivor set, which the snap may shrink. So iterate: build the
      // boxes from the current survivors, snap, and if any node drops, shrink the box and re-snap. Dropping a
      // node only shrinks the box, which makes the rest LESS likely to drop, so this converges fast (a single
      // round whenever the snap drops nothing). The weld mutates in place, so re-snapping restarts each
      // survivor from its post-pass-1 state (snapPass1).
      const dag::Vector<MeshData> snapPass1 = meshes;
      // Always snap in the modern path: the runtime quantizes this geometry into the per-behavior BLAS either
      // way -- a collapse_and_optimize asset is baked + BLAS-built here at export, a legacy precooked asset at
      // client load (rendinst optimize_collres_on_load). (The pure-legacy writeLegacyDump path returned at the
      // top of exportAsset and never reaches here, so reaching this loop already implies the runtime quantizes.)
      for (bool survivorsStable = false, firstRound = true; !survivorsStable; firstRound = false)
      {
        survivorsStable = true;
        BBox3 boxTraceable, boxCollidable;
        for (const auto &n : coll.allNodesList)
          if (n.type == COLLISION_NODE_TYPE_MESH && actions[n.nodeIndex] != NodeAction::DROP && snapPass1[n.nodeIndex].face.size() > 0)
          {
            const TMatrix &nTm = coll.getNodeTm(n.nodeIndex);
            BBox3 nodeBox;
            for (const Point3 &v : snapPass1[n.nodeIndex].vert)
              nodeBox += nTm * v;
            if (!anySolidTraceable && n.checkBehaviorFlags(CollisionNode::TRACEABLE))
              boxTraceable += nodeBox;
            if (!anySolidCollidable && n.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
              boxCollidable += nodeBox;
          }

        for (auto &n : coll.allNodesList)
          if (n.type == COLLISION_NODE_TYPE_MESH && actions[n.nodeIndex] != NodeAction::DROP)
          {
            if (!firstRound) // round 1 still holds the post-pass-1 mesh; later rounds restart from it
              meshes[n.nodeIndex] = snapPass1[n.nodeIndex];
            MeshData &m = meshes[n.nodeIndex];
            if (m.vert.size() < 3 || m.face.size() < 1)
              continue; // degenerate node kept by a non-REMOVE strategy: nothing to weld
            // PHYS_COLLIDABLE feeds Jolt, so its grid wins; a trace-only node uses the trace grid; a node in
            // neither (SOLID resource -> empty boxes, or no matching behavior) isn't BLAS-resident -> no snap.
            const BBox3 *snapBox = n.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE) ? &boxCollidable
                                   : n.checkBehaviorFlags(CollisionNode::TRACEABLE)     ? &boxTraceable
                                                                                        : nullptr;
            if (!snapBox || snapBox->isempty())
              continue;
            if (weld_verts_to_vert21_grid(m, coll.getNodeTm(n.nodeIndex), *snapBox))
            {
              NodeAction act = process_mesh_node(n, m, /*vert21_box*/ snapBox);
              // The weld moved verts onto their cell centers (and/or merged some), so the snapped mesh differs
              // from the source slice even when vert/face counts are unchanged -- and in that case
              // process_mesh_node returns KEEP, whose rebuild path copies the stale source slice and drops the
              // snap. Promote a healthy KEEP to REPLACE so the snapped meshes[n] is what gets emitted; leave a
              // degenerate KEEP (vert<3/face<1, kept by a non-REMOVE strategy) and DROP untouched.
              if (act == NodeAction::KEEP && m.vert.size() >= 3 && m.face.size() >= 1)
                act = NodeAction::REPLACE;
              if (act == NodeAction::DROP && actions[n.nodeIndex] != NodeAction::DROP)
                survivorsStable = false; // a survivor dropped -> shrink the boxes and re-snap the rest
              actions[n.nodeIndex] = act;
            }
          }
      }

      // Rebuild ownVertices/ownIndices from per-node decisions. Walking allNodesList preserves
      // the existing node order; offsets are stamped fresh.
      dag::Vector<Point3_vec4> newOwnVerts;
      dag::Vector<uint16_t> newOwnIdx;
      newOwnVerts.reserve(coll.ownVertices.size());
      newOwnIdx.reserve(coll.ownIndices.size());
      for (CollisionNode &n : coll.allNodesList)
      {
        if (n.type != COLLISION_NODE_TYPE_MESH && n.type != COLLISION_NODE_TYPE_CONVEX)
          continue;
        if (n.type == COLLISION_NODE_TYPE_MESH && actions[n.nodeIndex] == NodeAction::DROP)
        {
          n.verticesOfs = 0;
          n.verticesCount = 0;
          n.indicesOfs = 0;
          n.indicesCount = 0;
          continue;
        }
        if (n.type == COLLISION_NODE_TYPE_MESH && actions[n.nodeIndex] == NodeAction::REPLACE)
        {
          containmentDirty = true; // modelBBox is rewritten below -> sort order / containment may change
          const MeshData &m = meshes[n.nodeIndex];
          n.verticesOfs = (uint32_t)newOwnVerts.size();
          n.verticesCount = (uint16_t)(m.vert.size() - 1); // count-minus-one encoding
          n.indicesOfs = (uint32_t)newOwnIdx.size();
          n.indicesCount = (uint32_t)(m.face.size() * 3);
          newOwnVerts.reserve(newOwnVerts.size() + m.vert.size());
          // Refresh this node's bounds from the rewritten verts: the weld snapped survivors onto vert21 cell
          // centers (and edge-collapse moved the merged vert), so a vert can land up to ~half a cell outside
          // the source modelBBox. collapseAndOptimize later refreshes only non-IDENT (baked) and traceable
          // bucket-target nodes, so an IDENT phys-collidable-only node would otherwise serialize a box that no
          // longer encloses its mesh. m.vert is in the node's local space -- the same space modelBBox/
          // boundingSphere use. The resource-level bbox/sphere are refreshed after collapse in
          // recomputeResourceBounds(): the same half-cell snap nudge can poke a survivor just past the stale
          // resource sphere, and the trace/inclusion top-level early reject keys off vBoundingSphere.
          BBox3 nodeBox;
          for (const Point3 &v : m.vert)
          {
            Point3_vec4 vv;
            vv.x = v.x;
            vv.y = v.y;
            vv.z = v.z;
            vv.resv = 1.0f;
            newOwnVerts.push_back(vv);
            nodeBox += v;
          }
          n.modelBBox = nodeBox;
          n.boundingSphere.c = nodeBox.center();
          float r2 = 0.f;
          for (const Point3 &v : m.vert)
            r2 = max(r2, lengthSq(v - n.boundingSphere.c));
          n.boundingSphere.r = sqrtf(r2);
          for (const auto &f : m.face)
            for (int fi = 0; fi < 3; fi++)
              newOwnIdx.push_back((uint16_t)f.v[fi]);
          continue;
        }
        // KEEP: copy the existing slice into the new pool and re-stamp offsets.
        if (n.indicesCount) // gate on indicesCount: verticesCount uses count-minus-one encoding
        {
          const Point3_vec4 *srcV = coll.ownVertices.data() + n.verticesOfs;
          const uint16_t *srcI = coll.ownIndices.data() + n.indicesOfs;
          uint32_t newVOfs = (uint32_t)newOwnVerts.size();
          uint32_t newIOfs = (uint32_t)newOwnIdx.size();
          newOwnVerts.insert(newOwnVerts.end(), srcV, srcV + (uint32_t)n.verticesCount + 1u);
          newOwnIdx.insert(newOwnIdx.end(), srcI, srcI + n.indicesCount);
          n.verticesOfs = newVOfs;
          n.indicesOfs = newIOfs;
        }
      }
      coll.ownVertices = eastl::move(newOwnVerts);
      coll.ownIndices = eastl::move(newOwnIdx);
      coll.meshVertsBase = coll.ownVertices.data();
      coll.meshIndicesBase = coll.ownIndices.data();

      if (degenerate_meshes_cnt)
      {
        if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_ERROR)
          return false;
        if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_REMOVE)
        {
          // relGeomNodeTms is parallel to allNodesList (the last sortNodesList in loadLegacyRawFormat /
          // collapseAndOptimize reordered it to match -- entry i <-> node i), so a node erase must drop its
          // entry in lockstep. Otherwise the array keeps the old size, both writeCollisionData's size assert
          // and the sortNodesList() before serialization fail, and the runtime BVH reads the wrong per-node
          // transform (getRelGeomNodeTms keys off nodeIndex == array position).
          const bool hasRelGeomNodeTms = (coll.collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID) != 0;
          for (int ni = coll.allNodesList.size() - 1; ni >= 0; ni--)
            if (coll.allNodesList[ni].type == COLLISION_NODE_TYPE_MESH && !coll.allNodesList[ni].indicesCount)
            {
              erase_items(coll.allNodesList, ni, 1);
              if (hasRelGeomNodeTms)
                erase_items(coll.relGeomNodeTms, ni, 1);
            }
          coll.rebuildNodesLL();
          containmentDirty = true; // erasing nodes shifts allNodesList positions -> insideOfNode indices stale
          logwarn("%s: %sremoved %d nodes with degenerative meshes, %d nodes remain", //
            a.getName(), label, degenerate_meshes_cnt, coll.allNodesList.size());
        }
        if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_PASS_THROUGH)
          logwarn("%s: %spassing through %d nodes with degenerative meshes", a.getName(), label, degenerate_meshes_cnt);
      }
      if (bad_tm_cnt)
      {
        if (report_inverted_mesh_tm)
        {
          log.addMessage(log.ERROR, "%s: found %d mesh nodes with bad tm, src=%s", a.getName(), bad_tm_cnt, a.getTargetFilePath());
          return false;
        }
        else
          logwarn("%s: found %d mesh nodes with bad tm, src=%s", a.getName(), bad_tm_cnt, a.getTargetFilePath());
      }
      return true;
    };

    if (!remove_degenerate_faces(""))
      return false;

    // optimize collision data and build FRT if requested
    if (collapse_and_optimize)
    {
      // collapseAndOptimize ends with sortNodesList() (rebuilding containment on the post-collapse geometry)
      // unless it early-returns with no mesh nodes. Capture that BEFORE the call (it merges/drops nodes), so a
      // sort here clears the dirty flag the pass above set; if it no-ops, the flag stays so the rebuild below
      // still fixes containment (e.g. all mesh nodes were dropped, shifting the surviving non-mesh nodes).
      const bool collapseSorted = coll.getMeshNodeCount() > 0;
      coll.collapseAndOptimize(a.getName(), /* build_frt */ false, /* fast= */ false);
      if (collapseSorted)
        containmentDirty = false;
      if (!remove_degenerate_faces("[post-collapse-pass] "))
        return false;
      if (bool build_frt = a.props.getBool("buildFRT", true))
      {
        coll.collisionFlags &= ~COLLISION_RES_FLAG_OPTIMIZED;
        const bool collapseSortedFrt = coll.getMeshNodeCount() > 0;
        coll.collapseAndOptimize(a.getName(), build_frt, /* fast= */ false);
        if (collapseSortedFrt)
          containmentDirty = false;
      }
    }

    // do last verification after collapse, because Jolt uses quantization
    bool skipJoltValidation = a.props.getBool("skipJoltValidation", false);
    if (!skipJoltValidation && !coll.validateVerticesForJolt(a.getName()))
    {
      if (jolt_degenerate_fail_export)
      {
        log.addMessage(log.ERROR, "%s: build failed due to huge degenerative triangles (joltDegenerativeTriFailExport=true)",
          a.getName());
        return false;
      }
    }

    // Rebuild node containment (insideOfNode) from the FINAL geometry before serialization -- but ONLY when a
    // pass above actually changed it (containmentDirty): rewrote a modelBBox (REPLACE) or erased a node (DROP)
    // since the last sortNodesList. stlsort is not stable, so an unconditional re-sort could reorder equal-key
    // (same size + name) nodes and emit a binary diff where nothing changed; and in the common buildFRT path
    // collapseAndOptimize's final sortNodesList already reflects the emitted geometry (so the flag is clean and
    // we skip). insideOfNode is a positional allNodesList index the precooked runtime load reads straight off
    // disk (it calls only rebuildNodesLL, never sortNodesList) and then indexes UNCHECKED in testIntersection's
    // boxOutside[] -- a stale/out-of-range value silently mis-culls or overruns. sortNodesList only SETS
    // insideOfNode (it never resets), so clear it first -- matching collapseAndOptimize's pre-sort reset
    // (collisionGameResLoad.cpp). The re-sort also reorders relGeomNodeTms in lockstep.
    if (containmentDirty)
    {
      for (CollisionNode &n : coll.allNodesList)
        n.insideOfNode = CollisionNode::INVALID_IDX;
      coll.sortNodesList();
      coll.rebuildNodesLL();
    }

    // The vert21 weld/snap, edge-collapse, and DROP changed the geometry, so the resource-level bounds
    // carried over from the pre-snap legacy load are stale. Refresh before serialization: the precooked
    // runtime load reads them straight off disk, and the trace/inclusion early reject uses vBoundingSphere.
    recomputeResourceBounds(coll);

    // write back uncompressed data in modern format
    mcwr.reset(128 << 10);
    writeCollisionData(coll, mcwr);

    // finally write data with optional compression
    cwr.writeInt32e(0xACE50000 | 0x0001);
    cwr.beginBlock();
    if (!preferZstdPacking || mcwr.getSize() < 512) // no sence in compressing
    {
      mcwr.copyDataTo(cwr.getRawWriter());
      cwr.endBlock(btag_compr::NONE);
    }
    else
    {
      mkbindump::BinDumpSaveCB zcwr(mcwr.getSize(), cwr);
      mcrd.setMem(mcwr.getRawWriter().getMem(), false);

      if (allowOodlePacking)
      {
        zcwr.writeInt32e(mcwr.getSize());
        oodle_compress_data(zcwr.getRawWriter(), mcrd, mcwr.getSize());
      }
      else
        zstd_compress_data(zcwr.getRawWriter(), mcrd, mcwr.getSize(), 256 << 10, 19);

      if (zcwr.getSize() < mcwr.getSize() * 8 / 10 && zcwr.getSize() + 256 < mcwr.getSize()) // enough profit in compression
      {
        zcwr.copyDataTo(cwr.getRawWriter());
        cwr.endBlock(allowOodlePacking ? btag_compr::OODLE : btag_compr::ZSTD);
      }
      else
      {
        mcwr.copyDataTo(cwr.getRawWriter());
        cwr.endBlock(btag_compr::NONE);
      }
    }
    return true;
  }

  // Recompute the resource-level bounds (vFullBBox / boundingBox / vBoundingSphere / boundingSphereRad)
  // from the final emitted geometry, right before serialization. The vert21 weld/snap moves survivors onto
  // cell centers, edge-collapse moves merged verts, and DROP removes whole mesh nodes, so the bounds
  // loadLegacyRawFormat derived from the pre-snap geometry no longer enclose what we emit. The precooked
  // runtime load reads these straight off disk (collisionGameResLoad.cpp load() -- no recompute), and the
  // trace/inclusion top-level early reject keys off vBoundingSphere, so a vert outside a stale sphere would
  // be culled before per-node tests. Called at the very end (after both degeneracy passes and
  // collapseAndOptimize), so it captures the final geometry regardless of the collapse / buildFRT flags.
  // The sphere is the cheap enclosing sphere (bbox center + farthest point) -- the same form the per-node
  // boundingSphere refresh above uses; ~2% looser than a minimal sphere, immaterial for a coarse early reject.
  static void recomputeResourceBounds(CollisionResource &c)
  {
    // Every emitted resource-local geometry point: mesh/convex verts exactly; capsule/box/sphere via their
    // modelBBox corners (no vert buffer). Dropped/empty mesh nodes (indicesCount == 0) contribute nothing.
    auto forEachResourcePoint = [&](auto &&pt) {
      for (const CollisionNode &n : c.allNodesList)
      {
        if (n.type == COLLISION_NODE_TYPE_MESH || n.type == COLLISION_NODE_TYPE_CONVEX)
        {
          if (n.indicesCount == 0)
            continue;
          const TMatrix &tm = c.getNodeTm(n.nodeIndex);
          for (const Point3_vec4 &v : c.getNodeVertices(n.nodeIndex))
            pt(tm * Point3(v.x, v.y, v.z));
        }
        else if (!n.modelBBox.isempty())
        {
          // box/sphere modelBBox is already resource-local; capsule modelBBox is node-local (transform it).
          const bool nodeLocal = n.type == COLLISION_NODE_TYPE_CAPSULE;
          const TMatrix &tm = c.getNodeTm(n.nodeIndex);
          for (int k = 0; k < 8; k++)
            pt(nodeLocal ? tm * n.modelBBox.point(k) : n.modelBBox.point(k));
        }
      }
    };

    BBox3 total;
    forEachResourcePoint([&](const Point3 &p) { total += p; });
    if (total.isempty())
      return; // point-only / fully-dropped resource: keep the loaded bounds

    c.boundingBox = total;
    v_bbox3_init(c.vFullBBox, v_ldu(&total[0].x));
    v_bbox3_add_pt(c.vFullBBox, v_ldu(&total[1].x));

    const Point3 center = total.center();
    float r2 = 0.f;
    forEachResourcePoint([&](const Point3 &p) { r2 = max(r2, lengthSq(p - center)); });
    c.vBoundingSphere = v_make_vec4f(center.x, center.y, center.z, r2);
    c.boundingSphereRad = sqrtf(r2);
  }

  static void writeCollisionData(const CollisionResource &c, mkbindump::BinDumpSaveCB &cwr)
  {
    cwr.write32ex(&c.vFullBBox, sizeof(c.vFullBBox));
    cwr.write32ex(&c.vBoundingSphere, sizeof(c.vBoundingSphere));
    cwr.write32ex(&c.boundingBox, sizeof(c.boundingBox));
    cwr.writeFloat32e(c.boundingSphereRad);
    // Strip legacy FRT-presence bits before writing: the runtime no longer builds or uses FRT, so
    // new exports carry no FRT blocks. Old assets with these bits set stay readable (the loader skips
    // the legacy FRT bytes), but we don't re-emit them. COLLISION_RES_FLAG_BLAS_TWO_SIDED is NOT
    // stripped: it is the only on-disk signal the loader uses to restore the rebuilt BLAS cull mode
    // (collapseAndOptimize set it from need_frt above), replacing the cleared HAS_*_FRT bits the cull
    // mode used to be derived from.
    cwr.writeInt32e(c.collisionFlags & ~(COLLISION_RES_FLAG_HAS_TRACE_FRT | COLLISION_RES_FLAG_HAS_COLL_FRT));

    cwr.writeInt32e(c.allNodesList.size());
    for (const auto &n : c.allNodesList)
    {
      cwr.writeDwString(c.getNodeName(n.nodeIndex));
      cwr.writeDwString(n.physMatId != PHYSMAT_INVALID ? phmatNames.getName(n.physMatId) : "");
      BBox3 nodeBBox = c.getNodeBBox(n.nodeIndex);
      BSphere3 nodeBSphere = c.getNodeBSphere(n.nodeIndex);
      cwr.write32ex(&nodeBBox, sizeof(nodeBBox));
      cwr.write32ex(&nodeBSphere, sizeof(nodeBSphere));
      cwr.writeInt16e(n.behaviorFlags);
      cwr.writeInt8e(n.flags);
      cwr.writeInt8e(n.type);
      cwr.writeFloat32e(c.getNodeMaxTmScale(n.nodeIndex));
      const TMatrix &sTm = c.getNodeTm(n.nodeIndex);
      cwr.write32ex(&sTm, sizeof(sTm));
      cwr.writeInt16e(n.insideOfNode);

      cwr.writeInt16e(c.getNodeConvexPlanes(n.nodeIndex).size());
      cwr.write32ex(c.getNodeConvexPlanes(n.nodeIndex).data(), data_size(c.getNodeConvexPlanes(n.nodeIndex)));

      auto verts = c.getNodeVertices(n.nodeIndex);
      cwr.writeInt32e((int)verts.size());
      cwr.write32ex(verts.data(), data_size(verts));

      auto idxs = c.getNodeIndices(n.nodeIndex);
      cwr.writeInt32e((int)idxs.size());
      cwr.write16ex(idxs.data(), data_size(idxs));
    }

    if (c.collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    {
      G_ASSERTF(c.relGeomNodeTms.size() == c.allNodesList.size(), //
        "allNodesList=%d relGeomNodeTms=%d", c.relGeomNodeTms.size(), c.allNodesList.size());
      cwr.write32ex(c.relGeomNodeTms.data(), data_size(c.relGeomNodeTms));
    }
  }
  static FastNameMapTS<false> phmatNames;
  static int resolve_phmat(const char *nm) { return phmatNames.addNameId(nm); }
};
FastNameMapTS<false> CollisionExporter::phmatNames;

class CollisionExporterPlugin : public IDaBuildPlugin
{
public:
  bool __stdcall init(const DataBlock &appblk) override
  {
    const DataBlock *collisionBlk = appblk.getBlockByNameEx("assets")->getBlockByNameEx("build")->getBlockByNameEx("collision");

    def_collidable = collisionBlk->getBool("defCollidable", def_collidable);

    preferZstdPacking = collisionBlk->getBool("preferZSTD", false);
    if (preferZstdPacking)
      debug("collision prefers ZSTD");

    allowOodlePacking = collisionBlk->getBool("allowOODLE", false);
    if (allowOodlePacking)
      debug("collision allows OODLE");

    writePrecookedFmt = collisionBlk->getBool("writePrecookedFmt", false);

    const char *degen_strategy_str = collisionBlk->getStr("degenerativeMeshStrategy", "remove");
    if (strcmp(degen_strategy_str, "remove") == 0)
      degenerative_mesh_strategy = DEGENERATIVE_MESH_DO_REMOVE;
    else if (strcmp(degen_strategy_str, "ignore") == 0)
      degenerative_mesh_strategy = DEGENERATIVE_MESH_DO_PASS_THROUGH;
    else if (strcmp(degen_strategy_str, "error") == 0)
      degenerative_mesh_strategy = DEGENERATIVE_MESH_DO_ERROR;
    else
      logerr("bad assets{ build{ collision{ %s:t=%s, leaving degenerative_mesh_strategy=%d", "degenerativeMeshStrategy",
        degen_strategy_str, degenerative_mesh_strategy);
    degenerate_tri_area_threshold_sq = collisionBlk->getReal("degenerativeTriAreaThresholdSq", 5e-12f);
    report_inverted_mesh_tm = collisionBlk->getBool("errorOnInvertedMesh", false);
    jolt_degenerate_fail_export = collisionBlk->getBool("joltDegenerativeTriFailExport", false);
    return true;
  }
  void __stdcall destroy() override { delete this; }

  int __stdcall getExpCount() override { return 1; }
  const char *__stdcall getExpType(int idx) override
  {
    switch (idx)
    {
      case 0: return "collision";
      default: return NULL;
    }
  }
  IDagorAssetExporter *__stdcall getExp(int idx) override
  {
    switch (idx)
    {
      case 0: return &expCollision;
      default: return NULL;
    }
  }

  int __stdcall getRefProvCount() override { return 0; }
  const char *__stdcall getRefProvType(int idx) override { return NULL; }
  IDagorAssetRefProvider *__stdcall getRefProv(int idx) override { return NULL; }

protected:
  CollisionExporter expCollision;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) CollisionExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(collision)
REGISTER_DABUILD_PLUGIN(collision, nullptr)
