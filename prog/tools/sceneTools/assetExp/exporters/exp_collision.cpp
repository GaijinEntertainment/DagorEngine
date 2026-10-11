// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <assets/daBuildExpPluginChain.h>
#include <assets/assetPlugin.h>
#include <assets/assetExporter.h>
#include <assets/assetRefs.h>
#include <assets/assetMgr.h>
#include <assets/asset.h>
#include <libTools/util/makeBindump.h>
#include <libTools/util/iLogWriter.h>
#include <libTools/util/appDirRelativePath.h>
#include <libTools/dagFileRW/geomMeshHelper.h>
#include "exp_tools.h"
#include <gameRes/dag_stdGameRes.h>
#include <math/dag_boundingSphere.h>
#include <math/dag_mesh.h>
#include <util/dag_hashedKeyMap.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_localConv.h>
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
#include <util/dag_fastNameMapTS.h>
#include <ioSys/dag_chainedMemIo.h>
#include "getSkeleton.h"
#include <gameRes/collisionResourceBuilder.h>
#include <gameRes/collResStream.h>


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
// Resolved at plugin init. Listed as a source dep of every collision asset: an isTransparent
// node is checked against this file, so a physmat edit must re-run the export.
static String physmat_path;

// isTransparent on a node is only a request: occluder feeders read PhysMat::lightTransparent,
// so the node's phmat must carry it or the glass keeps culling what is seen through it.
static bool check_transparent_node_phmat(const DagorAsset &a, const char *node_name, const char *phmat, ILogWriter &log)
{
  if (PhysMat::physMatCount() == 0)
  {
    log.addMessage(log.ERROR, "%s: node '%s' isTransparent cannot be checked: no physmat.blk (application.blk game{ physmat:t })",
      a.getName(), node_name);
    return false;
  }
  const PhysMat::MaterialData &mat = PhysMat::getMaterial(PhysMat::getMaterialId(phmat));
  if (dd_stricmp(mat.name, phmat) != 0)
    log.addMessage(log.ERROR, "%s: node '%s' isTransparent but phmat '%s' is not in physmat.blk", a.getName(), node_name, phmat);
  else if (!mat.lightTransparent)
    log.addMessage(log.ERROR, "%s: node '%s' isTransparent but phmat '%s' has no lightTransparent", a.getName(), node_name, phmat);
  else
    return true;
  return false;
}

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
  bool isTransparent = false; // authored request, checked against the phmat at write time
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
    collision_props.isTransparent |= refProps.isTransparent;
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
  // Run VHACD single-threaded: dabuild already exports assets concurrently, so the per-VHACD worker
  // pool only oversubscribes, and serialized hull/vertex order must not depend on thread scheduling.
  params.m_asyncACD = false;
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

// The vert21 grid frame for one node: the runtime BLAS reconstructs every vertex at a 21-bit cell center
// (node-local -> resource-local via node_tm -> round(f*32) in the 65535/extent frame packVert21 uses
// -> cell center -> node-local via inverse node_tm). Storing exporter verts at those exact centers makes
// the geometry the exporter sees identical to what the runtime decodes, so the degeneracy the exporter
// resolves is exactly the degeneracy Jolt sees. det() and inverse(node_tm) -- plus the quantization
// constants -- depend only on (node_tm, model_box), so they are computed once in the constructor and
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

  // node-local point -> clamped 21-bit cell index (the same frame the weld key and packVert21 use).
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

// Weld vertices to the runtime BLAS's 21-bit vert21 grid and snap every survivor onto its cell center.
// Every mesh node is quantized into one whole-model 21-bit lattice (the same encoding packVert21 uses,
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
    // 6: the v3 stream; 11 keeps the version the precooked products had
    return 6 * 12 + 11 + (def_collidable ? 1 : 0) + 2 * (!preferZstdPacking ? 0 : (allowOodlePacking ? 2 : 1 + 6));
  }

  void __stdcall onRegister() override {}
  void __stdcall onUnregister() override {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override
  {
    files.clear();
    files.push_back() = a.getTargetFilePath();
    if (!physmat_path.empty())
      files.push_back() = physmat_path;
  }

  bool __stdcall isExportableAsset(DagorAsset &a) override { return true; }

  void readDataFromBlk(Tab<GeomMeshHelperDagObject> &dagObjectsList, const DataBlock &blk) { read_data_from_blk(dagObjectsList, blk); }
  bool addDagNodes(DagorAsset &a, CollisionResourceBuilder &coll, ILogWriter &log)
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
      if (GeomNodeTreeUniquePtr t = getSkeleton(a.getMgr(), skeletonName, log))
      {
        nodeTree.replaceContentFrom(*t);
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
    // no points (an empty DAG: no collision) is a zero sphere, not the NaN a fit over nothing yields
    BSphere3 boundingSphere =
      pointsList.empty() ? BSphere3(Point3(0, 0, 0), 0.f) : mesh_bounding_sphere(pointsList.data(), pointsList.size());
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

    bool blkExist = false;
    int nameId = -1;
    if (a.props.blockCount() > 0)
    {
      blkExist = true;
      nameId = a.props.getNameId("part");
    }

    unsigned int numCollisionNodes = 0;
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
      collisionObjectProps.isTransparent = dagNodeScriptBlk.getBool("isTransparent", false);
      collisionObjectProps.behaviorFlags |= dagNodeScriptBlk.getBool("solid", false) ? CollisionNode::SOLID : 0;
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
            type = COLLISION_NODE_TYPE_CONVEX;
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
    coll.beginRawNodes(collisionFlags, boundingSphere);

    collision_preprocessing(a.props.getBlockByNameEx("nodes"), dagObjectsList, collisionObjectsProps, numCollisionNodes);

    unsigned int numExportedNodes = 0;
    bool transparentContractOk = true;
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

      CollisionResourceBuilder::RawNode raw;
      SimpleString nameWithoutSuffix;
      if (a.props.getBool("removeDMSuffix", false))
      {
        remove_dm_suffix(dagObjectsList[objectNo].name, nameWithoutSuffix);
        raw.name = nameWithoutSuffix.str();
      }
      else
        raw.name = dagObjectsList[objectNo].name.str();

      const char *phmat =
        collisionObjectProps.physMat.empty() ? dagNodeScriptBlk.getStr("phmat", "") : collisionObjectProps.physMat.str();
      raw.physMat = phmat;
      // props, not the DAG script: a generated (refNodes) node inherits the request from its sources
      if (collisionObjectProps.isTransparent && !check_transparent_node_phmat(a, dagObjectsList[objectNo].name, phmat, log))
        transparentContractOk = false;

      TMatrix wtm = dagObjectsList[objectNo].wtm;

      Tab<Point3> &verts = dagObjectsList[objectNo].mesh.verts;

      // Built before the sphere, because a kdop node ships THESE verts and the sphere must bound
      // what it ships: a k-DOP hull's corners lie outside the source points it was fit through.
      Kdop kdop;
      if (hasKdop)
      {
        kdop.setPreset(static_cast<KdopPreset>(kdopPreset), kdopCutOffThreshold, kdopSegmentsX, kdopSegmentsY);
        kdop.setRotation(Point3(kdopRotX, kdopRotY, kdopRotZ));
        kdop.calcKdop(verts, TMatrix::IDENT);
      }

      BBox3 bbox = collisionObjectProps.boundingBoxs;
      // The sphere is the center of its own vert box and the farthest vert from it, so the builder's
      // fold keeps that radius. The FRAME is the one the built node keeps its radius in, which is
      // world for a SPHERE (the builder un-bakes the authored placement out of it) and node-local
      // for every other type, whose radius the runtime scales by the authored tm itself.
      const bool fitInWorld = type == COLLISION_NODE_TYPE_SPHERE;
      const dag::ConstSpan<Point3> fitVerts = hasKdop ? dag::ConstSpan<Point3>(kdop.vertices.data(), kdop.vertices.size())
                                                      : dag::ConstSpan<Point3>(verts.data(), verts.size());
      BSphere3 boundingSphere(Point3(0, 0, 0), -1.f);
      boundingSphere.r2 = -1.f; // the zero-vert marker is r = r2 = -1, as every reader tests
      if (!fitVerts.empty())
      {
        BBox3 fitBox;
        for (const Point3 &v : fitVerts)
          fitBox += fitInWorld ? v * wtm : v;
        boundingSphere.c = fitBox.center();
        boundingSphere.r2 = 0.f;
        for (const Point3 &v : fitVerts)
          inplace_max(boundingSphere.r2, lengthSq((fitInWorld ? v * wtm : v) - boundingSphere.c));
        boundingSphere.r = sqrtf(boundingSphere.r2);
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

      raw.type = (CollisionResourceNodeType)type;
      raw.behaviorFlags = collisionObjectProps.behaviorFlags;
      raw.tm = tree_capsule ? TMatrix::IDENT : wtm;
      if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
      {
        String nodeName = dagObjectsList[objectNo].name;
        if (a.props.getBool("removeDMSuffix", false))
          remove_dm_suffix(dagObjectsList[objectNo].name, nodeName);

        if (auto geomNodeIdx = nodeTree.findINodeIndex(nodeName.str()))
        {
          TMatrix geomNodeWtm;
          nodeTree.getNodeWtmRelScalar(geomNodeIdx, geomNodeWtm);
          raw.relGeomTm = inverse(geomNodeWtm) * wtm;
        }
      }
      raw.sphere = boundingSphere;
      raw.box = bbox;

      Tab<Plane3> convexPlanes(tmpmem);
      if (type == COLLISION_NODE_TYPE_CONVEX && !hasKdop)
      {
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
      }
      else if (type == COLLISION_NODE_TYPE_CONVEX && hasKdop)
      {
        for (int i = 0; i < kdop.planeDefinitions.size(); ++i)
          if (kdop.planeDefinitions[i].vertices.size() > 0)
            convexPlanes.push_back(Plane3(kdop.planeDefinitions[i].planeNormal, kdop.planeDefinitions[i].vertices[0]));
      }
      raw.planes = make_span_const(convexPlanes);

      if (type == COLLISION_NODE_TYPE_MESH || type == COLLISION_NODE_TYPE_CONVEX)
      {
        const Tab<GeomMeshHelper::Face> &faces = dagObjectsList[objectNo].mesh.faces;
        if (!hasKdop && !faces.size())
          log.addMessage(log.WARNING, "%s: node <%s> with type=%s contains empty mesh (face.count=%d vert.count=%d)", a.getName(),
            dagObjectsList[objectNo].name, (type == COLLISION_NODE_TYPE_MESH) ? "mesh" : "convex", faces.size(), verts.size());
        raw.verts = fitVerts;
        raw.indices =
          hasKdop ? make_span_const(kdop.indices) : make_span_const(faces.empty() ? nullptr : faces[0].v, faces.size() * 3);
      }
      coll.addRawNode(raw, a.getName(), resolve_phmat);
    }

    G_ASSERT(numExportedNodes == numCollisionNodes);
    coll.endRawNodes();
    return transparentContractOk;
  }

  bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log) override
  {
    bool collapse_nodes = a.props.getBool("collapseNodes", a.props.getBool("collapseAndOptimize", false));
    phmatNames.reset(); // this asset's material ids start at 0

    CollisionResourceBuilder coll;
    if (!addDagNodes(a, coll, log))
      return false;

    // Gates the pre-serialization sort: stlsort is not stable, so re-sorting when nothing changed
    // could reorder equal-key (same size + name) nodes and emit a binary diff.
    bool containmentDirty = false;

    auto remove_degenerate_faces = [&](const char *label) {
      unsigned degenerate_meshes_cnt = 0;
      unsigned bad_tm_cnt = 0;
      // Per-mesh-node decision: KEEP existing slice, REPLACE with welded MeshData, or DROP entirely.
      // We stage decisions, then apply them to the builder in a single pass after the loop.
      enum class NodeAction : uint8_t
      {
        KEEP,
        REPLACE,
        DROP
      };
      dag::Vector<NodeAction> actions(coll.nodes.size(), NodeAction::KEEP);
      dag::Vector<MeshData> meshes(coll.nodes.size());

      // The per-node degeneracy pipeline: distance-weld, bad/degenerate face removal, KEEP/REPLACE/DROP
      // decision. Both the initial pass and the post-vert21 re-process below funnel through this single
      // lambda so the logic lives in one place. The decision is always relative to the original source
      // slice (coll is not rewritten until the end), so a node that vert21 later shrinks turns KEEP->REPLACE.
      // vert21_box != nullptr in the post-weld pass: a degenerate triangle's edge collapse snaps the merged
      // vertex back onto that vert21 grid so the runtime reconstructs it exactly. nullptr in the pre-weld
      // pass (no grid yet) -> collapse to the plain midpoint.
      // grid_tm overrides the frame the vert21 snap maps through: nullptr = the node's tm (the
      // resource-space behavior-union grid); IDENT = the node-local per-node ownVerts21 frame.
      auto process_mesh_node = [&](const CollisionNode &n, MeshData &m, const BBox3 *vert21_box,
                                 const TMatrix *grid_tm) -> NodeAction {
        dag::ConstSpan<Point3_vec4> srcVerts = coll.nodeVerts(n.nodeIndex);
        dag::ConstSpan<uint32_t> srcIndices = coll.nodeIndices(n.nodeIndex);
        const TMatrix &nTm = coll.authoredTm[n.nodeIndex];
        const float maxTmScale = coll.nodeMaxTmScale(n.nodeIndex);
        const float weld_eps = a.props.getReal("meshVertWeldEps", 1e-3f) * safeinv(maxTmScale);
        unsigned zeroarea_faces_cnt = 0;
        m.kill_unused_verts(weld_eps * weld_eps);
        // Strip only topological degenerates (duplicate-index faces, e.g. from a coincident-cell weld merge):
        // those have a zero-length edge the collapse loop cannot repair and must go. Pass threshold 0 so
        // Geometric zero-area-but-distinct faces survive to the edge-collapse loop below and get repaired
        // (merge two verts) instead of deleted outright -- deletion would skip the watertight-preserving path.
        m.kill_bad_faces(0.f);
        // Resolve degenerate triangles by edge collapse (merge two verts) rather than face deletion, so a
        // watertight mesh stays watertight where deleting the triangle would punch a hole. Two criteria:
        //   1. Jolt-degenerate: (2*area)^2 <= 1e-12 (Jolt Vec3::IsNearZero default). Such a triangle fails
        //      MeshShape creation and fatals the load, so it is resolved for every node (no maxTmScale gate).
        //      Judged in the frame the shipped chunk holds: a collapseNodes asset bakes the node tm
        //      into the verts (resource-local), an un-collapsed asset ships node-local verts with the tm as
        //      a separate shape transform, and no process rebuilds either at load.
        //   2. Sliver cleanup: the configurable, looser degenerativeTriAreaThresholdSq (default 5e-12),
        //      kept gated to un-scaled nodes -- a quality tunable, not a Jolt requirement.
        // Each collapse merges the shortest edge's two verts to their midpoint, snapped back onto the vert21
        // grid (post-weld pass) so the runtime reproduces the result exactly. The collapsed triangle and its
        // neighbour across that edge gain a duplicate index; kill_bad_faces drops them. Iterate: moving a
        // vert can flatten another triangle.
        constexpr float jolt_degenerate_cross_sq = 1e-12f; // matches Jolt/Geometry/IndexedTriangle.h IsDegenerate
        // Precompute the vert21 grid frame once per node: a collapse below snaps the merged vertex back onto
        // it, and det()+inverse(node tm) are node-constant, so they must not be recomputed per collapse.
        const Vert21Grid vert21Grid(grid_tm ? *grid_tm : nTm, vert21_box ? *vert21_box : BBox3());
        for (bool more = true; more;)
        {
          more = false;
          for (unsigned i = 0; i < m.face.size(); i++)
          {
            const uint32_t ia = m.face[i].v[0], ib = m.face[i].v[1], ic = m.face[i].v[2];
            if (ia == ib || ib == ic || ia == ic)
              continue; // duplicate-index face from a prior collapse; kill_bad_faces will drop it
            const float crossSqLocal = lengthSq((m.vert[ib] - m.vert[ia]) % (m.vert[ic] - m.vert[ia]));
            // Only a collapsed asset judges resource-local, so only it pays the three tm products.
            float crossSqJolt = crossSqLocal;
            if (collapse_nodes)
            {
              const Point3 w0 = nTm * m.vert[ia], w1 = nTm * m.vert[ib], w2 = nTm * m.vert[ic];
              crossSqJolt = lengthSq((w1 - w0) % (w2 - w0));
            }
            const bool joltDegen = crossSqJolt <= jolt_degenerate_cross_sq;
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
              a.getName(), label, coll.nodeName(n.nodeIndex), (int)srcVerts.size(), m.vert.size(), (int)srcIndices.size() / 3,
              m.face.size(), maxTmScale, weld_eps, coll.nodeBBox(n.nodeIndex));
          else
            logwarn("%s: %sdegenerate mesh node \"%s\": vert=%d->%d face=%d->%d maxTmScale=%g eps=%g bbox=%@", a.getName(), label,
              coll.nodeName(n.nodeIndex), (int)srcVerts.size(), m.vert.size(), (int)srcIndices.size() / 3, m.face.size(), maxTmScale,
              weld_eps, coll.nodeBBox(n.nodeIndex));
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
            a.getName(), label, coll.nodeName(n.nodeIndex), (int)srcVerts.size(), m.vert.size(), (int)srcIndices.size() / 3,
            m.face.size(), weld_eps, zeroarea_faces_cnt);
        else
          logwarn("%s: %soptimized mesh node \"%s\": vert=%d->%d face=%d->%d, weld_eps=%g", a.getName(), label,
            coll.nodeName(n.nodeIndex), (int)srcVerts.size(), m.vert.size(), (int)srcIndices.size() / 3, m.face.size(), weld_eps);
        return NodeAction::REPLACE;
      };

      // Pass 1: process every mesh node without the vert21 weld (exactly as the export did before the weld
      // was added), filling meshes[] and the KEEP/REPLACE/DROP decision. The per-behavior weld boxes are
      // built from the survivors after this pass (below), so verts removed/collapsed here -- and DROP'd
      // nodes -- never widen the grid the runtime reconstructs from.
      for (auto &n : coll.nodes)
        if (n.type == COLLISION_NODE_TYPE_MESH)
        {
          // Degenerate-dropped nodes stay in the list with no geometry and no vertex slice.
          if (!n.hasGeometry())
            continue;
          const TMatrix &nTm = coll.authoredTm[n.nodeIndex];
          if (!*label && nTm.det() > 0) // require left matrix in initial data
          {
            if (report_inverted_mesh_tm)
              log.addMessage(log.ERROR, "%s: bad mesh node \"%s\" tm=%@", a.getName(), coll.nodeName(n.nodeIndex), nTm);
            else
              logwarn("%s: bad mesh node \"%s\" tm=%@", a.getName(), coll.nodeName(n.nodeIndex), nTm);
            bad_tm_cnt++;
          }
          MeshData &m = meshes[n.nodeIndex];
          dag::ConstSpan<Point3_vec4> srcVerts = coll.nodeVerts(n.nodeIndex);
          dag::ConstSpan<uint32_t> srcIndices = coll.nodeIndices(n.nodeIndex);
          m.vert.resize(srcVerts.size());
          for (unsigned i = 0; i < m.vert.size(); i++)
            m.vert[i] = srcVerts[i];
          m.face.resize(srcIndices.size() / 3);
          for (unsigned i = 0; i < m.face.size(); i++)
            for (unsigned fi = 0; fi < 3; fi++)
              m.face[i].v[fi] = srcIndices[i * 3 + fi];
          actions[n.nodeIndex] = process_mesh_node(n, m, /*vert21_box*/ nullptr, /*grid_tm*/ nullptr);
        }

      // Snap the verts onto the retired combined-grid union frame, per behavior. The runtime grids
      // this weld matched are gone: every node now decodes from its own per-node chunk frame, which
      // the second snap stage below covers, so this stage only pre-perturbs verts onto a lattice
      // nothing decodes any more (under half a union cell). It is retained so serialized assets stay
      // byte-stable until the exporter follow-up retires it; the box math is unchanged from the grid
      // era: the union spans the surviving (non-DROP) nodes' post-pass-1 verts, PHYS_COLLIDABLE wins
      // for a dual node, a trace-only node uses the TRACEABLE box, and equal sets coincide.
      // The grid-era SOLID veto is kept per behavior: a trace-only SOLID node empties the trace box
      // but must not empty the collidable box (Jolt still quantizes it).
      // The snap itself can still turn a node into DROP (process_mesh_node below, when the vert21 grid flattens
      // a thin sliver to <3 verts). A DROP'd node is not serialized, so it must not widen the box the survivors
      // snap to -- yet the box is built from the survivor set, which the snap may shrink. So iterate: build the
      // boxes from the current survivors, snap, and if any node drops, shrink the box and re-snap. Dropping a
      // node only shrinks the box, which makes the rest less likely to drop, so this converges fast (a single
      // round whenever the snap drops nothing). The weld mutates in place, so re-snapping restarts each
      // survivor from its post-pass-1 state (snapPass1).
      const dag::Vector<MeshData> snapPass1 = meshes;
      // Always snap: the per-node chunks quantize this geometry at export, collapsed or not.
      for (bool survivorsStable = false, firstRound = true; !survivorsStable; firstRound = false)
      {
        survivorsStable = true;
        // Recompute the SOLID veto each round from current actions (grid-era rule, kept for
        // byte-stability): a SOLID node emptied that behavior's union box, so survivors of that
        // behavior need no snap. But the snap below can DROP a SOLID node, and a DROP'd node is
        // erased before serialization, so the veto no longer applies and the survivors must snap.
        // A stale (pre-loop) veto would keep the box empty across
        // rounds and ship the un-snapped slivers this pass exists to remove. (A trace-only SOLID node empties only
        // the trace box; Jolt still quantizes the collidable box.)
        bool anySolidTraceable = false, anySolidCollidable = false;
        for (const auto &n : coll.nodes)
          if (n.type == COLLISION_NODE_TYPE_MESH && actions[n.nodeIndex] != NodeAction::DROP &&
              n.checkBehaviorFlags(CollisionNode::SOLID))
          {
            anySolidTraceable |= n.checkBehaviorFlags(CollisionNode::TRACEABLE);
            anySolidCollidable |= n.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE);
          }
        BBox3 boxTraceable, boxCollidable;
        for (const auto &n : coll.nodes)
          if (n.type == COLLISION_NODE_TYPE_MESH && actions[n.nodeIndex] != NodeAction::DROP && snapPass1[n.nodeIndex].face.size() > 0)
          {
            const TMatrix &nTm = coll.authoredTm[n.nodeIndex];
            BBox3 nodeBox;
            for (const Point3 &v : snapPass1[n.nodeIndex].vert)
              nodeBox += nTm * v;
            if (!anySolidTraceable && n.checkBehaviorFlags(CollisionNode::TRACEABLE))
              boxTraceable += nodeBox;
            if (!anySolidCollidable && n.checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
              boxCollidable += nodeBox;
          }

        for (auto &n : coll.nodes)
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
            if (weld_verts_to_vert21_grid(m, coll.authoredTm[n.nodeIndex], *snapBox))
            {
              NodeAction act = process_mesh_node(n, m, /*vert21_box*/ snapBox, /*grid_tm*/ nullptr);
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

        // Second snap stage, per-node ownVerts21 frame -- the frame the runtime decodes: every node
        // re-quantizes into its per-node BLAS chunk on its own slice-bbox frame, and iterateNodeVerts
        // feeds Jolt those decoded verts. The retired union snap above only guarantees the coarser
        // union lattice: a sliver legal on union cell centers can still flatten on the finer per-node
        // round trip, so snap the survivors onto the per-node frame and resolve what flattens.
        // Iterate per node: a collapse can remove a frame-defining extreme vert, which changes the
        // next pack's frame.
        for (auto &n : coll.nodes)
          if (n.type == COLLISION_NODE_TYPE_MESH && actions[n.nodeIndex] != NodeAction::DROP)
          {
            // This stage must stay last so serialized verts sit on the frame the runtime decodes.
            // The retired union stage above placed verts on distinct union cells, and this per-node
            // refinement moves each vert under half a per-node cell (<< half a union cell), so
            // union-cell membership survives and serialized assets stay byte-stable until the
            // exporter follow-up retires the union stage. Running this stage first, or re-snapping
            // the union stage against the moved verts, would lose that bound.
            MeshData &m = meshes[n.nodeIndex];
            constexpr int MAX_PER_NODE_SNAP_ROUNDS = 4;
            int round = 0;
            bool converged = false;
            for (; round < MAX_PER_NODE_SNAP_ROUNDS && m.vert.size() >= 3 && m.face.size() >= 1; ++round)
            {
              BBox3 sliceBox;
              for (const Point3 &v : m.vert)
                sliceBox += v;
              if (!weld_verts_to_vert21_grid(m, TMatrix::IDENT, sliceBox))
              {
                converged = true; // weld is a no-op: verts already sit on the final per-node cells
                break;
              }
              NodeAction act = process_mesh_node(n, m, /*vert21_box*/ &sliceBox, /*grid_tm*/ &TMatrix::IDENT);
              if (act == NodeAction::KEEP && m.vert.size() >= 3 && m.face.size() >= 1)
                act = NodeAction::REPLACE; // same KEEP promotion as the union stage: the snap moved verts
              // Only a DROP restarts the outer loop: it shrinks the survivor set, so the boxes (rebuilt from
              // the fixed snapPass1 survivors) change. A REPLACE leaves the set intact -- those boxes are
              // unchanged, so restarting on it would re-derive identical boxes and never terminate.
              if (act == NodeAction::DROP && actions[n.nodeIndex] != NodeAction::DROP)
                survivorsStable = false;
              actions[n.nodeIndex] = act;
              if (act != NodeAction::REPLACE)
              {
                converged = true; // KEEP/DROP settles the node: no further geometry change
                break;
              }
            }
            // Ran the full round budget while still moving verts: not a verified per-node fixed point. The
            // final Jolt validation still gates real degeneracy, so warn rather than fail the export.
            if (!converged && m.vert.size() >= 3 && m.face.size() >= 1)
              logwarn("%s: node \"%s\" per-node vert21 snap did not converge in %d rounds", a.getName(), coll.nodeName(n.nodeIndex),
                MAX_PER_NODE_SNAP_ROUNDS);
          }
      }

      // Apply the per-node decisions; a rewritten node's box follows its verts (the weld snapped
      // survivors onto vert21 cell centers, up to half a cell outside the source box).
      for (const CollisionNode &n : coll.nodes)
      {
        if (n.type != COLLISION_NODE_TYPE_MESH || actions[n.nodeIndex] == NodeAction::KEEP)
          continue;
        containmentDirty = true; // the box or the node list changes -> sort order / containment may change
        if (actions[n.nodeIndex] == NodeAction::DROP)
        {
          coll.replaceNodeGeometry(n.nodeIndex, {}, {});
          continue;
        }
        const MeshData &m = meshes[n.nodeIndex];
        dag::Vector<uint32_t, framemem_allocator> faces;
        faces.reserve(m.face.size() * 3);
        for (const auto &f : m.face)
          for (int fi = 0; fi < 3; fi++)
            faces.push_back(f.v[fi]);
        coll.replaceNodeGeometry(n.nodeIndex, make_span_const(m.vert), make_span_const(faces));
      }

      if (degenerate_meshes_cnt)
      {
        if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_ERROR)
          return false;
        if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_REMOVE)
        {
          for (int ni = (int)coll.nodes.size() - 1; ni >= 0; ni--)
            if (coll.nodes[ni].type == COLLISION_NODE_TYPE_MESH && !coll.nodes[ni].hasGeometry())
              coll.eraseNode(ni);
          logwarn("%s: %sremoved %d nodes with degenerative meshes, %d nodes remain", //
            a.getName(), label, degenerate_meshes_cnt, (int)coll.nodes.size());
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

    // the collapse, and the two-sided marker of its FRT branch
    if (collapse_nodes)
    {
      // The collapse re-sorts only when it merged or baked something; a no-change collapse keeps the
      // earlier sort output. The dirty flag therefore stays as the passes left it, and the sort below is
      // the one authority on containment (a re-sort after a collapse that did sort is idempotent).
      coll.collapse(a.getName());
      if (!remove_degenerate_faces("[post-collapse-pass] "))
        return false;
      // the marker of a two-sided BLAS, from the buildFRT prop as every cook so far stamped it;
      // a resource without a mesh node has no BLAS and keeps the bit clear
      bool hasMeshNode = false;
      for (const CollisionNode &n : coll.nodes)
        hasMeshNode |= collres_is_mesh_list_node(n.type);
      if (hasMeshNode && a.props.getBool("buildFRT", true))
        coll.collisionFlags |= COLLISION_RES_FLAG_BLAS_TWO_SIDED;
      else
        coll.collisionFlags &= ~COLLISION_RES_FLAG_BLAS_TWO_SIDED;
    }

    // insideOfNode is a positional node index read straight off disk and indexed unchecked in
    // testIntersection's boxOutside[], and no load re-derives it.
    if (containmentDirty)
      coll.sortNodes();

    // The vert21 weld/snap, edge-collapse, and DROP changed the geometry, so the resource-level bounds
    // of the DAG walk are stale. Refresh before serialization: the runtime load reads them straight
    // off disk, and the trace/inclusion early reject uses vBoundingSphere.
    coll.recomputeBounds();

    const unsigned label = 0xACE50000 | COLLRES_STREAM_VERSION;
    mkbindump::BinDumpSaveCB mcwr(128 << 10, cwr);
    if (mcwr.WRITE_BE)
    {
      log.addMessage(log.ERROR, "%s: the collision stream is little-endian only", a.getName());
      return false;
    }
    if (!coll.write(mcwr.getRawWriter(), a.getName(), mat_name))
    {
      log.addMessage(log.ERROR, "%s: the collision stream cannot be written", a.getName());
      return false;
    }

    // The shipped bytes back through the loader: the stream must be a fixpoint of load + write (anything
    // the loader loses and the writer needs is a byte diff), and Jolt gets the shipped chunks in the shipped
    // frame (node-local un-collapsed, resource-local collapsed), so the degenerate check runs on that twin.
    {
      mkbindump::BinDumpSaveCB acwr(mcwr.getSize() + 16, mcwr);
      acwr.writeInt32e(label);
      acwr.beginBlock();
      mcwr.copyDataTo(acwr.getRawWriter());
      acwr.endBlock(btag_compr::NONE);
      MemoryLoadCB acrd(acwr.getRawWriter().getMem(), false);
      CollisionResource back(acrd, -1, a.getName(), resolve_phmat);
      // a refused landing leaves the twin empty, and an empty twin passes the Jolt check below
      if (back.getAllNodes().empty() && !coll.nodes.empty())
      {
        log.addMessage(log.ERROR, "%s: the written collision stream loads as an empty resource", a.getName());
        return false;
      }
      mkbindump::BinDumpSaveCB bcwr(mcwr.getSize() + 16, mcwr);
      if (!back.write(bcwr.getRawWriter(), mat_name) || bcwr.getSize() != mcwr.getSize() ||
          !mcwr.getRawWriter().getMem()->cmpEq(bcwr.getRawWriter().getMem()))
      {
        log.addMessage(log.ERROR, "%s: the collision stream is not a load/write fixpoint", a.getName());
        return false;
      }
      if (!a.props.getBool("skipJoltValidation", false) && !back.validateVerticesForJolt(a.getName()) && jolt_degenerate_fail_export)
      {
        log.addMessage(log.ERROR, "%s: build failed due to huge degenerative triangles (joltDegenerativeTriFailExport=true)",
          a.getName());
        return false;
      }
    }

    // finally write data with optional compression
    cwr.writeInt32e(label);
    cwr.beginBlock();
    if (!preferZstdPacking || mcwr.getSize() < 512) // no sence in compressing
    {
      mcwr.copyDataTo(cwr.getRawWriter());
      cwr.endBlock(btag_compr::NONE);
    }
    else
    {
      mkbindump::BinDumpSaveCB zcwr(mcwr.getSize(), cwr);
      MemoryLoadCB mcrd(mcwr.getRawWriter().getMem(), false);

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

  static FastNameMapTS<false> phmatNames;
  static int resolve_phmat(const char *nm) { return phmatNames.addNameId(nm); }
  static const char *mat_name(int id) { return phmatNames.getName(id); }
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

    physmat_path = make_eff_app_relative_path(appblk.getBlockByNameEx("game")->getStr("physmat", "game/config/physmat.blk"));
    if (!dd_file_exist(physmat_path))
      physmat_path.clear();
    // a hosting tool (AssetViewer) may own PhysMat already; never release it here
    else if (PhysMat::physMatCount() == 0)
      PhysMat::init(physmat_path);
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
