#include "dabuild_exp_plugin_chain.h"
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

class CollisionExporter : public IDagorAssetExporter
{
public:
  virtual const char *__stdcall getExporterIdStr() const { return "collision exp"; }

  virtual const char *__stdcall getAssetType() const { return "collision"; }
  virtual unsigned __stdcall getGameResClassId() const { return 0xACE50000; }
  virtual unsigned __stdcall getGameResVersion() const
  {
    static constexpr const int base_ver = 2;
    return base_ver * 12 + 5 + (def_collidable ? 1 : 0) + 2 * (!preferZstdPacking ? 0 : (allowOodlePacking ? 2 : 1 + 6)) +
           (writePrecookedFmt ? 6 : 0);
  }

  virtual void __stdcall onRegister() {}
  virtual void __stdcall onUnregister() {}

  void __stdcall gatherSrcDataFiles(const DagorAsset &a, Tab<SimpleString> &files) override { files.clear(); }

  virtual bool __stdcall isExportableAsset(DagorAsset &a) { return true; }

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

    mkbindump::BinDumpSaveCB mcwr(128 << 10, final_cwr.getTarget(), final_cwr.WRITE_BE);
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
      if (!dd_strnicmp(dagObjectsList[objectNo].name, "_Clip", i_strlen("_Clip")))
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
      cwr.writeDwString(dagNodeScriptBlk.getStr("phmat", ""));

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
      mkbindump::BinDumpSaveCB mcwr(cwr.getSize(), final_cwr.getTarget(), final_cwr.WRITE_BE);
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

  virtual bool __stdcall exportAsset(DagorAsset &a, mkbindump::BinDumpSaveCB &cwr, ILogWriter &log)
  {
    bool collapse_and_optimize = a.props.getBool("collapseAndOptimize", false);
    if (!writePrecookedFmt && !collapse_and_optimize) // legacy format
      return writeLegacyDump(a, cwr, log, preferZstdPacking);

    // modern (pre-cooked) format

    // first we write legacy format and read it back to CollisionResource object
    mkbindump::BinDumpSaveCB mcwr(128 << 10, cwr.getTarget(), cwr.WRITE_BE);
    if (!writeLegacyDump(a, mcwr, log, false))
      return false;
    MemoryLoadCB mcrd(mcwr.getRawWriter().getMem(), false);
    G_ASSERT(mcrd.readInt() == 0xACE50000);
    CollisionResource coll;
    coll.loadLegacyRawFormat(mcrd, -1, resolve_phmat);

    auto remove_degenerate_faces = [&](const char *label) {
      unsigned degenerate_meshes_cnt = 0;
      unsigned bad_tm_cnt = 0;
      for (auto &n : coll.allNodesList)
        if (n.type == COLLISION_NODE_TYPE_MESH)
        {
          MeshData m;
          if (!*label && n.tm.det() > 0) // require left matrix in initial data
          {
            if (report_inverted_mesh_tm)
              log.addMessage(log.ERROR, "%s: bad mesh node \"%s\" tm=%@", a.getName(), n.name, n.tm);
            else
              logwarn("%s: bad mesh node \"%s\" tm=%@", a.getName(), n.name, n.tm);
            bad_tm_cnt++;
          }
          m.vert.resize(n.vertices.size());
          for (unsigned i = 0; i < m.vert.size(); i++)
            m.vert[i] = n.vertices[i];
          m.face.resize(n.indices.size() / 3);
          for (unsigned i = 0; i < m.face.size(); i++)
            for (unsigned fi = 0; fi < 3; fi++)
              m.face[i].v[fi] = n.indices[i * 3 + fi];

          float weld_eps = a.props.getReal("meshVertWeldEps", 1e-3f) * safeinv(n.cachedMaxTmScale);
          unsigned zeroarea_faces_cnt = 0;
          m.kill_unused_verts(weld_eps * weld_eps);
          m.kill_bad_faces();
          if (n.cachedMaxTmScale <= 1.0f)
            for (unsigned i = 0; i < m.face.size(); i++)
              if (lengthSq((m.vert[m.face[i].v[1]] - m.vert[m.face[i].v[0]]) % (m.vert[m.face[i].v[2]] - m.vert[m.face[i].v[0]])) <
                  5e-12f)
              {
                zeroarea_faces_cnt++;
                logerr("%s: %sdegenerate tri %d,%d,%d: %@, %@, %@ (edge len: %g, %g, %g)", a.getName(), label, m.face[i].v[0],
                  m.face[i].v[1], m.face[i].v[2], m.vert[m.face[i].v[0]], m.vert[m.face[i].v[1]], m.vert[m.face[i].v[2]],
                  length(m.vert[m.face[i].v[0]] - m.vert[m.face[i].v[1]]), length(m.vert[m.face[i].v[1]] - m.vert[m.face[i].v[2]]),
                  length(m.vert[m.face[i].v[2]] - m.vert[m.face[i].v[0]]));
                m.removeFacesFast(i, 1);
                i--;
              }
          if (m.face.size() * 3 != n.indices.size())
            m.kill_unused_verts(-1);
          if (m.vert.size() != n.vertices.size() || m.face.size() * 3 != n.indices.size())
          {
            if (m.vert.size() < 3 || m.face.size() < 1)
            {
              degenerate_meshes_cnt++;
              if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_ERROR)
                log.addMessage(log.ERROR, "%s: %sdegenerate mesh node \"%s\": vert=%d->%d face=%d->%d maxTmScale=%g eps=%g bbox=%@",
                  a.getName(), label, n.name, n.vertices.size(), m.vert.size(), n.indices.size() / 3, m.face.size(),
                  n.cachedMaxTmScale, weld_eps, n.modelBBox);
              else
                logerr("%s: %sdegenerate mesh node \"%s\": vert=%d->%d face=%d->%d maxTmScale=%g eps=%g bbox=%@", a.getName(), label,
                  n.name, n.vertices.size(), m.vert.size(), n.indices.size() / 3, m.face.size(), n.cachedMaxTmScale, weld_eps,
                  n.modelBBox);
              for (unsigned i = 0; i < n.vertices.size(); i++)
                debug("  v[%3d]=%+g,%+g,%+g", i, n.vertices[i].x, n.vertices[i].y, n.vertices[i].z);
              for (unsigned i = 0; i < n.indices.size(); i += 3)
                debug("  f[%3d]=%u, %u, %u", i / 3, n.indices[i + 0], n.indices[i + 1], n.indices[i + 2]);
              for (unsigned i = 0; i < m.vert.size(); i++)
                debug("  mv[%d]=%+g,%+g,%+g", i, m.vert[i].x, m.vert[i].y, m.vert[i].z);
              if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_REMOVE)
              {
                n.resetVertices();
                n.resetIndices();
              }
              continue;
            }

            if (zeroarea_faces_cnt)
              logwarn("%s: %soptimized mesh node \"%s\": vert=%d->%d face=%d->%d, weld_eps=%g (%d zero-area faces removed)",
                a.getName(), label, n.name, n.vertices.size(), m.vert.size(), n.indices.size() / 3, m.face.size(), weld_eps,
                zeroarea_faces_cnt);
            else
              logwarn("%s: %soptimized mesh node \"%s\": vert=%d->%d face=%d->%d, weld_eps=%g", a.getName(), label, n.name,
                n.vertices.size(), m.vert.size(), n.indices.size() / 3, m.face.size(), weld_eps);

            if (n.vertices.size() != m.vert.size())
              n.resetVertices({memalloc_typed<Point3_vec4>(m.vert.size(), midmem), m.vert.size()});
            for (unsigned i = 0; i < m.vert.size(); i++)
              n.vertices[i] = m.vert[i], n.vertices[i].resv = 1.0f;
            if (n.indices.size() != m.face.size() * 3)
              n.resetIndices({memalloc_typed<uint16_t>(m.face.size() * 3, midmem), m.face.size() * 3});
            for (unsigned i = 0; i < m.face.size(); i++)
              for (unsigned fi = 0; fi < 3; fi++)
                n.indices[i * 3 + fi] = m.face[i].v[fi];
          }
        }
      if (degenerate_meshes_cnt)
      {
        if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_ERROR)
          return false;
        if (degenerative_mesh_strategy == DEGENERATIVE_MESH_DO_REMOVE)
        {
          for (int ni = coll.allNodesList.size() - 1; ni >= 0; ni--)
            if (coll.allNodesList[ni].type == COLLISION_NODE_TYPE_MESH && !coll.allNodesList[ni].vertices.size() &&
                !coll.allNodesList[ni].indices.size())
              erase_items(coll.allNodesList, ni, 1);
          coll.rebuildNodesLL();
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
      coll.collapseAndOptimize(/* build_frt */ false, /* fast= */ false);
      if (!remove_degenerate_faces("[post-collapse-pass] "))
        return false;
      if (bool build_frt = a.props.getBool("buildFRT", true))
      {
        coll.collisionFlags &= ~COLLISION_RES_FLAG_OPTIMIZED;
        coll.collapseAndOptimize(build_frt, /* fast= */ false);
      }
    }

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
      mkbindump::BinDumpSaveCB zcwr(mcwr.getSize(), cwr.getTarget(), cwr.WRITE_BE);
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

  static void writeCollisionData(const CollisionResource &c, mkbindump::BinDumpSaveCB &cwr)
  {
    cwr.write32ex(&c.vFullBBox, sizeof(c.vFullBBox));
    cwr.write32ex(&c.vBoundingSphere, sizeof(c.vBoundingSphere));
    cwr.write32ex(&c.boundingBox, sizeof(c.boundingBox));
    cwr.writeFloat32e(c.boundingSphereRad);
    cwr.writeInt32e(c.collisionFlags);

    if (c.collisionFlags & COLLISION_RES_FLAG_HAS_TRACE_FRT)
      c.gridForTraceable.tracer->serialize(cwr.getRawWriter(), cwr.WRITE_BE, nullptr, false);
    if ((c.collisionFlags & COLLISION_RES_FLAG_HAS_COLL_FRT) && !(c.collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT))
      c.gridForCollidable.tracer->serialize(cwr.getRawWriter(), cwr.WRITE_BE, nullptr, false);

    cwr.writeInt32e(c.allNodesList.size());
    for (const auto &n : c.allNodesList)
    {
      cwr.writeDwString(n.name);
      cwr.writeDwString(n.physMatId != PHYSMAT_INVALID ? phmatNames.getName(n.physMatId) : "");
      cwr.write32ex(&n.modelBBox, sizeof(n.modelBBox));
      cwr.write32ex(&n.boundingSphere, sizeof(n.boundingSphere));
      cwr.writeInt16e(n.behaviorFlags);
      cwr.writeInt8e(n.flags);
      cwr.writeInt8e(n.type);
      cwr.writeFloat32e(n.cachedMaxTmScale);
      cwr.write32ex(&n.tm, sizeof(n.tm));
      cwr.writeInt16e(n.insideOfNode);

      cwr.writeInt16e(n.convexPlanes.size());
      cwr.write32ex(n.convexPlanes.data(), data_size(n.convexPlanes));

      cwr.writeInt32e(n.vertices.size());
      if ((n.flags & n.FLAG_VERTICES_ARE_REFS) && n.vertices.size())
      {
#define CHECK_VERT_IN_FRT(G) \
  (n.vertices.data() >= &c.G.tracer->verts(0) && n.vertices.data() < &c.G.tracer->verts(0) + c.G.tracer->getVertsCount())
#define GET_VERT_OFS_IN_FRT(G) (n.vertices.data() - &c.G.tracer->verts(0))
        if (CHECK_VERT_IN_FRT(gridForTraceable))
          cwr.writeInt32e(GET_VERT_OFS_IN_FRT(gridForTraceable));
        else if (CHECK_VERT_IN_FRT(gridForCollidable))
          cwr.writeInt32e(GET_VERT_OFS_IN_FRT(gridForCollidable) | 0x40000000);
        else
          G_ASSERTF(0, "n.vertices=%p,%d gridForTraceable=%p,%d gridForCollidable=%p,%d", n.vertices.data(), n.vertices.size(),
            &c.gridForTraceable.tracer->verts(0), c.gridForTraceable.tracer->getVertsCount(), //
            &c.gridForCollidable.tracer->verts(0), c.gridForCollidable.tracer->getVertsCount());
#undef CHECK_VERT_IN_FRT
#undef GET_VERT_OFS_IN_FRT
      }
      else
        cwr.write32ex(n.vertices.data(), data_size(n.vertices));

      cwr.writeInt32e(n.indices.size());
      if ((n.flags & n.FLAG_INDICES_ARE_REFS) && n.indices.size())
      {
#define CHECK_IND_IN_FRT(G) \
  (n.indices.data() >= c.G.tracer->faces(0).v && n.indices.data() < (&c.G.tracer->faces(0) + c.G.tracer->getFacesCount())->v)
#define GET_IND_OFS_IN_FRT(G) (n.indices.data() - c.G.tracer->faces(0).v)
        if (CHECK_IND_IN_FRT(gridForTraceable))
          cwr.writeInt32e(GET_IND_OFS_IN_FRT(gridForTraceable));
        else if (CHECK_IND_IN_FRT(gridForCollidable))
          cwr.writeInt32e(GET_IND_OFS_IN_FRT(gridForCollidable) | 0x40000000);
        else
          G_ASSERTF(0, "n.indices=%p,%d gridForTraceable=%p,%d gridForCollidable=%p,%d", n.indices.data(), n.indices.size(),
            &c.gridForTraceable.tracer->faces(0), c.gridForTraceable.tracer->getFacesCount(), //
            &c.gridForCollidable.tracer->faces(0), c.gridForCollidable.tracer->getFacesCount());
#undef CHECK_IND_IN_FRT
#undef GET_IND_OFS_IN_FRT
      }
      else
        cwr.write16ex(n.indices.data(), data_size(n.indices));
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
  virtual bool __stdcall init(const DataBlock &appblk)
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
    report_inverted_mesh_tm = collisionBlk->getBool("errorOnInvertedMesh", false);
    return true;
  }
  virtual void __stdcall destroy() { delete this; }

  virtual int __stdcall getExpCount() { return 1; }
  virtual const char *__stdcall getExpType(int idx)
  {
    switch (idx)
    {
      case 0: return "collision";
      default: return NULL;
    }
  }
  virtual IDagorAssetExporter *__stdcall getExp(int idx)
  {
    switch (idx)
    {
      case 0: return &expCollision;
      default: return NULL;
    }
  }

  virtual int __stdcall getRefProvCount() { return 0; }
  virtual const char *__stdcall getRefProvType(int idx) { return NULL; }
  virtual IDagorAssetRefProvider *__stdcall getRefProv(int idx) { return NULL; }

protected:
  CollisionExporter expCollision;
};

DABUILD_PLUGIN_API IDaBuildPlugin *__stdcall get_dabuild_plugin() { return new (midmem) CollisionExporterPlugin; }
END_DABUILD_PLUGIN_NAMESPACE(collision)
REGISTER_DABUILD_PLUGIN(collision, nullptr)
