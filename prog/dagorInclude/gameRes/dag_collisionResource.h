//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <gameRes/dag_collResDecl.h>
#include <generic/dag_DObject.h>
#include <gameRes/dag_stdGameResId.h>
#include <util/dag_simpleString.h>
#include <util/dag_index16.h>
#include <util/dag_globDef.h>
#include <math/dag_TMatrix.h>
#include <ioSys/dag_genIo.h>
#include <math/dag_bounds3.h>
#include <vecmath/dag_vecMathDecl.h>
#include <vecmath/dag_vecMath_const.h>
#include <math/dag_capsule.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>
#include <math/dag_e3dColor.h>
#include <EASTL/fixed_function.h>
#include <EASTL/bitvector.h>
#include <dag/dag_vector.h>
#include <daBVH/dag_swBLAS_ray.h> // RayData::unpackVert21 -- inlined into iterateNodeVerts/FacesVerts for BLAS-resident nodes

class GeomNodeTree;
class Bitarray;
class CollisionExporter;
namespace dabuildExp_collision
{
class CollisionExporter;
}

struct CollisionTrace
{
  vec3f vFrom;         // in
  vec3f vDir;          // in
  vec3f vTo;           // internal
  Point3 norm;         // out
  float t;             // in/out
  float capsuleRadius; // in
  int outMatId;        // out
  int outNodeId;       // out
  bool isectBounding;  // internal
  bool isHit;          // out
};

inline CollisionTrace make_collision_trace(const Point3 &from, const Point3 &dir, float t, float radius = 0.f)
{
  CollisionTrace tr{};
  tr.vFrom = v_ldu(&from.x);
  tr.vDir = v_ldu(&dir.x);
  tr.t = t;
  tr.capsuleRadius = radius;
  return tr;
}

struct IntersectedNode
{
  Point3 normal = Point3(0, 0, 0);
  union
  {
    float intersectionT;
    int sortKey = 0; // Note: signed to correctly handle -0.0 (which is mapped to INT_MIN)
  };
  Point3 intersectionPos = Point3(0, 0, 0);
  // Self-describing hit identifier -- the single source of truth for node identity and (when
  // applicable) per-node face index. Consumers must read it via the tri_ref:: accessors
  // (nodeIndex / hasTri / faceIndex / subTri) so the encoding can evolve under them. The
  // getCollisionNodeId() helper exists for the daScript binding (which exposes it as a
  // property under the historical "collisionNodeId" name) -- C++ callers should prefer the
  // tri_ref:: free functions.
  tri_ref_t triRef = tri_ref::invalid();
  unsigned int getCollisionNodeId() const { return tri_ref::nodeIndex(triRef); }
  bool operator<(const IntersectedNode &other) const { return sortKey < other.sortKey; }
};

struct MultirayIntersectedNode : IntersectedNode
{
  int rayId;
  bool operator<(const MultirayIntersectedNode &other) const
  {
    return rayId != other.rayId ? (rayId < other.rayId) : (sortKey < other.sortKey);
  }
};

struct DegenerativeNodeData
{
  DegenerativeNodeData(const CollisionNode *node) : node(node) {}
  const CollisionNode *node;
  dag::Vector<uint16_t> indices;
};

typedef dag::RelocatableFixedVector<vec4f, 8, true, framemem_allocator> all_collres_nodes_t;
typedef dag::RelocatableFixedVector<int, 8, true, framemem_allocator> all_collres_tri_indices_t;
typedef dag::RelocatableFixedVector<int, 32, /*bEnableOverflow*/ true, framemem_allocator> CollResHitNodesType;

enum CollisionResourceNodeType : uint8_t
{
  COLLISION_NODE_TYPE_MESH,
  COLLISION_NODE_TYPE_POINTS,
  COLLISION_NODE_TYPE_BOX,
  COLLISION_NODE_TYPE_SPHERE,
  COLLISION_NODE_TYPE_CAPSULE,
  COLLISION_NODE_TYPE_CONVEX,

  NUM_COLLISION_NODE_TYPES
};

enum CollisionResourceFlags : uint32_t
{
  COLLISION_RES_FLAG_COLLAPSE_CONVEXES = 1 << 0,
  COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS = 1 << 1,
  COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID = 1 << 2,
  COLLISION_RES_FLAG_OPTIMIZED = 1 << 3,
  // Bit 4: TRACEABLE and PHYS_COLLIDABLE share one combined BLAS in gridForTraceable. Originally
  // named for FRT-grid reuse; now means BLAS reuse (same bit value, same on-disk semantics).
  COLLISION_RES_FLAG_REUSE_TRACE_FRT = 1 << 4,
  // Bits 5/6: legacy FRT presence markers. Runtime no longer uses FRT but the load path reads them
  // to skip past old exporters' FRT blocks; new exports clear them.
  COLLISION_RES_FLAG_HAS_TRACE_FRT = 1 << 5,
  COLLISION_RES_FLAG_HAS_COLL_FRT = 1 << 6,
  // Bit 7: persisted BLAS cull mode. Set when the resource would have built a two-sided CULL_BOTH
  // FRT (so the BLAS is built two-sided); clear means backface-cull CCW. The BLAS is rebuilt at load,
  // not serialized, so this bit is the only on-disk cull signal. Legacy assets predating it fall back
  // to the HAS_*_FRT bits (CULL_BOTH iff those were set).
  COLLISION_RES_FLAG_BLAS_TWO_SIDED = 1 << 7,
};

struct CollisionNode
{
  // Sentinel for nextNode / CollisionResource::*NodesHead (no node).
  static constexpr uint16_t INVALID_IDX = 0xffff;

  // Be careful using that flags since entity tm might be scaled too and in most cases will be faster to assume that instance_tm*nodeTm
  // is always scaled See also cachedMaxTmScale member if you anyway want use it
  enum NodeFlag : uint8_t
  {
    NONE = 0,

    // TransformType
    IDENT = 1,           // Identity tm with zero offset
    TRANSLATE = 2,       // Identity tm with offset
    ORTHONORMALIZED = 4, // Unscaled
    ORTHOUNIFORM = 8,    // With uniform scale

    // Backend marker (not a transform type). Set by compactOwnVertices on mesh nodes that went
    // into a per-behavior BLAS. When set: the ownVertices slice is compacted out (meshVertsBase +
    // verticesOfs is invalid); verticesOfs is reinterpreted as a vert21-array index (8 B/vert) into
    // the grid chosen by getBlasGridForResidentNode(); verticesCount = count-1 over the post-dup
    // block; ownIndices is dropped (getNodeIndices() returns EMPTY). Only MESH ever goes resident --
    // CONVEX keeps ownIndices (AssetViewer plane<->face correlation; BVH DFS leaf order would not
    // preserve it). Read verts/faces via iterateNodeVerts / iterateNodeFaces(Verts) / getNodeFaceVerts,
    // each of which dispatches on this flag; raw meshVertsBase/meshIndicesBase access is unsupported.
    BLAS_RESIDENT = 16,
  };

  enum BehaviorFlag : uint16_t
  {
    TRACEABLE = 1 << 0,
    PHYS_COLLIDABLE = 1 << 1,
    SOLID = 1 << 2, // Trace without culling required

    FLAG_ALLOW_HOLE = 1 << 3,
    FLAG_DAMAGE_REQUIRED = 1 << 4,
    FLAG_CUT_REQUIRED = 1 << 5,
    FLAG_CHECK_SIDE = 1 << 6,
    FLAG_ALLOW_BULLET_DECAL = 1 << 7,
    FLAG_TRANSPARENT = 1 << 8,

    FLAG_CHECK_SURROUNDING_PART_FOR_EXCLUSION = 1 << 14,
    FLAG_ALLOW_SPLASH_HOLE = 1 << 15
  };

  uint16_t behaviorFlags = TRACEABLE | PHYS_COLLIDABLE | FLAG_ALLOW_HOLE | FLAG_DAMAGE_REQUIRED;
  eastl::underlying_type_t<NodeFlag> flags = NodeFlag::NONE;
  CollisionResourceNodeType type = COLLISION_NODE_TYPE_MESH;
  dag::Index16 geomNodeId;
  int16_t physMatId = -1;

protected:
  TMatrix tm = TMatrix::IDENT;
  BBox3 modelBBox;
  struct
  {
    Point3 c;
    float r = 0.f;
  } boundingSphere;
  float cachedMaxTmScale = 1.f;
  friend class CollisionResource;
  friend class CollisionResourceBVH;
  friend class CollisionGeometryFeeder;
  friend class TestMeshNodeMeshNodesIntersectionAlgo;
  friend class TestMeshNodeBoxNodesIntersectionAlgo;
  friend class TestMeshNodeSphereNodesIntersectionAlgo;
  friend class TestBoxNodeBoxNodesIntersectionAlgo;
  friend struct CollisionResourceUnittest;
  friend CollisionExporter;
  friend dabuildExp_collision::CollisionExporter;

public:
  uint16_t nodeIndex = 0; // In allNodesList.
  // Index into allNodesList in final post-sort order (set in sortNodesList; stays valid because
  // rebuildNodesLL re-stamps nodeIndex = position). 0xffff = not contained by any other node.
  uint16_t insideOfNode = 0xffff;

protected:
  // Per-type intrusive linked list. Walk via CollisionResource::forEachMeshNode / forEachBoxNode /
  // forEachSphereNode / forEachCapsuleNode (or visitCollisionNodes) -- this field is internal.
  // Stored as an index into CollisionResource::allNodesList; INVALID_IDX terminates the list.
  uint16_t nextNode = INVALID_IDX;
  // capsuleIndex valid only for COLLISION_NODE_TYPE_CAPSULE; planesOfs valid only for
  // COLLISION_NODE_TYPE_CONVEX. The two types are mutually exclusive, so they share storage.
  union
  {
    uint16_t capsuleIndex = 0; // index into CollisionResource::capsules
    uint16_t planesOfs;        // start in CollisionResource::convexPlanes
  };
  uint16_t planesCount = 0; // number of planes for this node (0 unless CONVEX)

  // Count-minus-one encoding so 65536 fits in uint16_t: stored 0..65535 = actual 1..65536.
  // Only meaningful when indicesCount > 0; for non-mesh nodes / dropped meshes the field is
  // ignored, so its raw 0-default does NOT mean "1 vertex" -- always gate on indicesCount.
  uint16_t verticesCount = 0;

  // Mesh data lives in CollisionResource::meshVertsBase / meshIndicesBase (which point either
  // at CollisionResource::ownVertices/ownIndices or at the FRT vertex/face arrays).
  uint32_t verticesOfs = 0;
  uint32_t indicesOfs = 0;
  uint32_t indicesCount = 0;

  uint32_t nameOfs = 0; // offset into CollisionResource::names; 0 means empty

public:
  int getNodeIdAsInt() const { return (int)geomNodeId; }

  // If you got crash here with (this == nullptr), it's compiler error. Try to make node iterator simpler.
  bool checkBehaviorFlags(uint16_t f) const { return (behaviorFlags & f) == f; }
  bool isBehaviorFlagsInFilter(uint16_t f) const { return (behaviorFlags | f) == f; }

  TMatrix getInverseTmFlags() const
  {
    TMatrix ret;
    if (DAGOR_LIKELY(flags & (ORTHONORMALIZED | ORTHOUNIFORM)))
    {
      ret = orthonormalized_inverse(tm);
      if (DAGOR_UNLIKELY((flags & ORTHONORMALIZED) == 0))
        ret *= 1.f / lengthSq(tm.getcol(0));
    }
    else
      ret = inverse(tm);
    return ret;
  }
};
DAG_DECLARE_RELOCATABLE(CollisionNode);

class GeomNodeTree;

enum CollisionResourceDrawDebugBits
{
  CRDD_NODES = 1,
  CRDD_NON_GEOM_TREE_NODES = 2,
  CRDD_BSPHERE = 4,
  CRDD_ALL = (unsigned short)~(unsigned short)0u
};

using TraceCollisionResourceStats = dag::Vector<int, framemem_allocator>;

decl_dclass_and_id(CollisionResource, DObject, CollisionGameResClassId)
public:
  bbox3f vFullBBox = {};      // all nodes, including box
  vec4f vBoundingSphere = {}; // center|r^2 in w
  alignas(16) BBox3 boundingBox = {};
  float boundingSphereRad = 0;
  uint32_t collisionFlags = 0;

  // Per-type linked-list heads. Each is an index into allNodesList (CollisionNode::INVALID_IDX
  // when the list is empty). nodeLists[] aliases the named heads via the union.
  union
  {
    struct
    {
      uint16_t meshNodesHead;
      uint16_t pointsNodesHead;
      uint16_t boxNodesHead;
      uint16_t sphereNodesHead;
      uint16_t capsuleNodesHead;
    };
    uint16_t nodeLists[NUM_COLLISION_NODE_TYPES];
  };
  uint16_t numMeshNodes = 0, numBoxNodes = 0, numCapsuleNodes = 0;
  dag::Index16 bsphereCenterNode;

  CollisionResource() { memset(nodeLists, 0xff, sizeof(nodeLists)); } // CollisionNode::INVALID_IDX in every slot
  CollisionResource *deepCopy(void *inplace_mem_ptr = nullptr) const;

  dag::Span<CollisionNode> getAllNodes() { return make_span(allNodesList); }
  dag::ConstSpan<CollisionNode> getAllNodes() const { return allNodesList; }

  static CollisionResource *loadResource(IGenLoad & crd, int res_id);
  // Create a single-node MESH resource. Vertex and index data are copied into the resource's
  // own dense storage.
  static CollisionResource *createSingleMesh(dag::ConstSpan<Point3_vec4> vertices, dag::ConstSpan<uint16_t> indices, const BBox3 &bbox,
    const BSphere3 &bsphere, uint32_t node_flags);
  // Create a single-node MESH resource that references externally-owned vertex/index data.
  // Caller must keep the supplied buffers alive for the lifetime of the resource.
  static CollisionResource *createSingleMeshNonOwning(dag::ConstSpan<Point3_vec4> vertices, dag::ConstSpan<uint16_t> indices,
    const BBox3 &bbox, const BSphere3 &bsphere);
  void load(IGenLoad & cb, int res_id);

  // Add typed collision nodes. Returns nodeIndex. Sets type, modelBBox, boundingSphere, capsule.
  int addSphereNode(const char *name, int16_t phys_mat_id, const BSphere3 &bsphere);
  int addBoxNode(const char *name, int16_t phys_mat_id, const BBox3 &bbox);
  int addCapsuleNode(const char *name, int16_t phys_mat_id, const Point3 &p0, const Point3 &p1, float radius);
  // Add a MESH or CONVEX node fully populated in one call. Mirrors the addSphereNode /
  // addBoxNode / addCapsuleNode "create at once" pattern. Verts/indices are appended to the
  // resource's owning storage (ownVertices/ownIndices); meshVertsBase/meshIndicesBase are
  // updated to point at the new dense buffers. Multiple calls accumulate into a single
  // resource. (Provided for API parity with the BVH-backed CollisionResource so the same
  // unittest source compiles against both implementations.)
  // Default behavior_flags mirrors CollisionNode's member default so omitting the arg preserves
  // standard hole/damage participation; pass an explicit value to opt out.
  int addMeshNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox, const BSphere3 &bsphere,
    dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint16_t> indices,
    uint16_t behavior_flags =
      CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE | CollisionNode::FLAG_ALLOW_HOLE | CollisionNode::FLAG_DAMAGE_REQUIRED,
    uint8_t flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED);
  int addConvexNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox, const BSphere3 &bsphere,
    dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint16_t> indices, dag::ConstSpan<plane3f> convex_planes,
    uint16_t behavior_flags =
      CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE | CollisionNode::FLAG_ALLOW_HOLE | CollisionNode::FLAG_DAMAGE_REQUIRED,
    uint8_t flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED);

  void collapseAndOptimize(const char *res_name, bool need_frt = false, bool frt_build_fast = true);

  // Recompute COLLISION_RES_FLAG_REUSE_TRACE_FRT from the current node behavior flags (set when every
  // mesh node's TRACEABLE and PHYS_COLLIDABLE bits match, cleared otherwise). This is the authoritative
  // gate for whether gridForCollidable is built separately; the persisted on-disk bit is not trusted
  // because a stale value set when the sets differ would route getBlasGrid(PHYS_COLLIDABLE) to
  // gridForTraceable and skip gridForCollidable, dropping collidable-only IDENT mesh nodes from
  // collision. Called from both collapseAndOptimize and the direct-load OPTIMIZED path before buildBLAS.
  void recomputeTraceReuseFlagFromNodeSets();

  CollisionNode *getNode(uint32_t index);
  const CollisionNode *getNode(uint32_t index) const;
  int getNodeIndexByName(const char *name) const;
  CollisionNode *getNodeByName(const char *name);
  const CollisionNode *getNodeByName(const char *name) const;
  const char *getNodeName(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? getNodeNameStr(*n) : "";
  }
  const char *getNodeNameStr(const CollisionNode &n) const { return names.empty() ? "" : names.data() + n.nameOfs; }
  const TMatrix &getNodeTm(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? n->tm : TMatrix::IDENT;
  }
  void setNodeTm(int node_id, const TMatrix &new_tm)
  {
    CollisionNode *n = getNode(node_id);
    if (n)
      n->tm = new_tm;
  }

  bool traceRay(const TMatrix &instance_tm, const Point3 &from, const Point3 &dir, float &in_out_t, Point3 *out_normal,
    int &out_mat_id) const
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f tm;
    v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
    return traceRay(tm, from, dir, in_out_t, out_normal, out_mat_id);
  }

  bool traceRay(const TMatrix &instance_tm, const Point3 &from, const Point3 &dir, float &in_out_t, Point3 *out_normal = nullptr) const
  {
    int outMatId;
    return traceRay(instance_tm, from, dir, in_out_t, out_normal, outMatId);
  }

  bool traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float &in_out_t,
    Point3 *out_normal, int &out_mat_id, int &out_node_id) const;

  bool traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float &in_out_t,
    Point3 *out_normal = nullptr) const
  {
    int outMatId, outNodeId;
    return traceRay(instance_tm, geom_node_tree, from, dir, in_out_t, out_normal, outMatId, outNodeId);
  }

  bool traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float &in_out_t,
    Point3 *out_normal, int &out_mat_id, const CollisionNodeFilter &filter, int ray_mat_id = -1,
    uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool traceRay(const mat44f &tm, const Point3 &from, const Point3 &dir, float &in_out_t, Point3 *out_normal, int &out_mat_id,
    int ray_mat_id = -1, uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter = CollisionNode::TRACEABLE,
    const CollisionNodeMask *collision_node_mask = nullptr, bool force_no_cull = false) const
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f tm;
    v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
    return traceRay(tm, geom_node_tree, from, dir, in_t, intersected_nodes_list, sort_intersections, behavior_filter,
      collision_node_mask, force_no_cull);
  }

  bool traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter = CollisionNode::TRACEABLE,
    const CollisionNodeMask *collision_node_mask = nullptr, bool force_no_cull = false) const;

  bool traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeFilter &filter) const;

  bool traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeMask &collision_node_mask,
    TraceCollisionResourceStats *out_stats) const;

  bool traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, vec3f from, vec3f dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeMask &collision_node_mask,
    TraceCollisionResourceStats *out_stats) const;

  bool traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir,
    float &in_out_t, float radius, Point3 &out_normal, Point3 &out_pos, int &out_mat_id) const;

  bool traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    float radius, IntersectedNode &intersected_node, float bsphere_scale, const CollisionNodeFilter &filter,
    const uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool traceCapsule(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    float radius, IntersectedNode &intersected_node, float bsphere_scale = 1.f,
    const uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool capsuleHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    float radius, CollResHitNodesType &nodes_hit) const;

  bool multiRayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, dag::Span<CollisionTrace> traces) const;

  bool traceMultiRay(const mat44f &tm, dag::Span<CollisionTrace> traces, int ray_mat_id = -1,
    uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool traceMultiRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, dag::Span<CollisionTrace> traces,
    MultirayCollResIntersectionsType &intersected_nodes_list, bool sort_intersections, float bsphere_scale = 1.f,
    uint8_t behavior_filter = CollisionNode::TRACEABLE, const CollisionNodeMask *collision_node_mask = nullptr,
    TraceCollisionResourceStats *out_stats = nullptr) const;

  // Don't use it! It's should not be external. `node` must be the live allNodesList entry:
  // SOLID/TRACEABLE behavior flags are read straight from it, not re-fetched from the resource.
  bool traceRayMeshNodeLocal(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t,
    vec4f *out_norm) const;

  bool traceRayMeshNodeLocalAllHits(const CollisionNode &node, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, bool force_no_cull = false) const;

  bool rayHit(const mat44f &tm, const Point3 &from, const Point3 &dir, float in_t, int ray_mat_id, int &out_mat_id,
    uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool rayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    float bsphere_scale = 1.f, const CollisionNodeMask *collision_node_mask = nullptr, int *out_mat_id = nullptr) const;

  VECTORCALL bool traceQuad(vec3f a00, vec3f a01, vec3f a10, vec3f a11, Point3 & out_point, int &out_node_index) const;

  struct DebugDrawData
  {
    bool localNodeTree;
    bool shouldDrawText;
    E3DCOLOR color;
    uint16_t drawBits;
    dag::Index16 bsphereCNode;
    vec4f bsphereOffset;
    const Bitarray *drawMask;

    DebugDrawData() :
      localNodeTree(false),
      shouldDrawText(false),
      color(255, 32, 32),
      drawBits(CRDD_ALL),
      bsphereOffset(V_C_UNIT_0001),
      bsphereCNode(-1),
      drawMask(nullptr)
    {}
  };
  void drawDebug(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const DebugDrawData &data = DebugDrawData()) const;

  static void registerFactory();

  static bool testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionNodeFilter &filter1,
    const CollisionResource *res2, const TMatrix &tm2, const CollisionNodeFilter &filter2, Point3 &collisionPoint1,
    Point3 &collisionPoint2, uint16_t *nodeIndex1 = NULL, uint16_t *nodeIndex2 = NULL, Tab<uint16_t> *node_indices1 = NULL);
  // This function is more readable and contains new primitives for checking.
  // It does full dispatching for collision primitives.
  // This eliminetes the ordering problem:
  // the function will find an intersection regardless of res1 and res2 order.
  // Only MESH/BOX/SPHERE nodes participate; CAPSULE and CONVEX nodes are not tested.
  static bool testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionResource *res2, const TMatrix &tm2,
    Point3 &collisionPoint1, Point3 &collisionPoint2, bool checkOnlyPhysNodes = false, bool useTraceFaces = false);

  bool checkInclusion(const Point3 &pos, CollResIntersectionsType &intersected_nodes_list) const;

private:
  // No callers anywhere in the tree; scheduled for deletion.
  // Can check mesh-box, box-box and sph-box only, asserts on any other types
  bool calcOffsetForIntersection(const TMatrix &tm1, const CollisionNode &node_to_move, const CollisionNode &node_to_check,
    Point3 &offset) const;

  bool calcOffsetForSeparation(const TMatrix &tm1, const CollisionNode &node_to_move, const CollisionNode &node_to_check,
    const Point3 &axis, Point3 &offset) const;

public:
  // The test (probe) node lives in *this; the restraining (convex) node lives in
  // restraining_resource, which may be the same or a different CollisionResource -- e.g.
  // attachableEditor tests slot volumes against attachment geometry on a separate model.
  bool testInclusion(int test_node_index, const TMatrix &tm_test, const CollisionResource *restraining_resource,
    int restraining_node_index, const TMatrix &tm_restrain, const GeomNodeTree *test_node_tree = NULL,
    const GeomNodeTree *restrain_node_tree = NULL) const;

  bool testInclusion(int test_node_index, const TMatrix &tm_test, dag::ConstSpan<plane3f> convex, const TMatrix &tm_restrain,
    const GeomNodeTree *test_node_tree = NULL, Point3 *res_pos = nullptr) const;

  bool testSphereIntersection(const CollisionNodeFilter &filter, const BSphere3 &sphere, const Point3 &dir_norm, Point3 &out_norm,
    float &out_depth, int &out_node_id) const;
  bool testCapsuleNodeIntersection(const Point3 &p0, const Point3 &p1, float radius) const;

  void initializeWithGeomNodeTree(const GeomNodeTree &geom_node_tree);

  void getCollisionNodeTm(const CollisionNode *node, const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, TMatrix &out_tm)
    const;
  void getCollisionNodeTm(const CollisionNode *node, mat44f_cref instance_tm, const GeomNodeTree *geom_node_tree, mat44f &out_tm)
    const;

  void clipCapsule(const TMatrix &instance_tm, const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized);
  void clipCapsule(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized);

  bool test_sphere_node_intersection(const BSphere3 &sphere, const CollisionNode *node, const Point3 &dir_norm, Point3 &out_norm,
    float &out_depth) const;
  bool test_capsule_node_intersection(const Point3 &p0, const Point3 &p1, float radius, const CollisionNode *node) const;

  template <typename Func, CollisionResourceNodeType node_type = COLLISION_NODE_TYPE_MESH, bool binded_to_gntree = true>
  void visitCollisionNodes(const Func &func) const
  {
    if (node_type != NUM_COLLISION_NODE_TYPES)
    {
      for (uint16_t i = nodeLists[node_type]; i != CollisionNode::INVALID_IDX; i = allNodesList[i].nextNode)
      {
        const CollisionNode &node = allNodesList[i];
        if (!binded_to_gntree || node.geomNodeId)
          func(node);
      }
    }
    else
      for (const CollisionNode &node : allNodesList)
        if (!binded_to_gntree || node.geomNodeId)
          func(node);
  }

  uint32_t getCollisionFlags() const { return collisionFlags; }

  // Mesh node iteration helpers (abstracts away linked list + raw vertex/index access)
  int getMeshNodeCount() const { return (int)numMeshNodes; }

  int getNodeVertCount(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return (n && n->indicesCount) ? (int)n->verticesCount + 1 : 0;
  }
  int getNodeFaceCount(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? (int)(n->indicesCount / 3u) : 0;
  }

  int getNodeIndexCount(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? (int)n->indicesCount : 0;
  }

  // Span over a node's ownIndices slice. EMPTY for BLAS-resident MESH nodes (ownIndices dropped;
  // face data lives in the BLAS leaves) -- consume those via iterateNodeFaces. Only MESH goes
  // BLAS-resident; CONVEX keeps ownIndices.
  dag::ConstSpan<uint16_t> getNodeIndices(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || n->indicesCount == 0)
      return {};
    if (n->flags & CollisionNode::BLAS_RESIDENT)
      return {}; // ownIndices slice dropped; use iterateNodeFaces
    return dag::ConstSpan<uint16_t>(meshIndicesBase + n->indicesOfs, n->indicesCount);
  }

  // Span over a non-resident node's ownVertices slice. EMPTY for BLAS_RESIDENT nodes (verts live in
  // the grid's vert21 array, not a contiguous Point3_vec4 region). Consume those via iterateNodeVerts
  // or the renderBLAS feeder (CollisionGeometryFeeder::buildSwrtBLASFromCollisionResource).
  dag::ConstSpan<Point3_vec4> getNodeVertices(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || n->indicesCount == 0)
      return {};
    if (n->flags & CollisionNode::BLAS_RESIDENT)
      return {}; // verts live in vert21; meshVertsBase + verticesOfs is invalid here
    return dag::ConstSpan<Point3_vec4>(meshVertsBase + n->verticesOfs, (uint32_t)n->verticesCount + 1u);
  }

  // Three vertices of a node's source face by per-node face index. Works for BLAS_RESIDENT nodes
  // (decoded from vert21) and non-resident nodes (read from ownVertices). False if face_idx is out
  // of range or the node has no triangles.
  bool getNodeFaceVerts(int node_id, int face_idx, Point3 &v0, Point3 &v1, Point3 &v2) const;

  // Decode a tri_ref_t back to its source triangle's three verts. BLAS refs (type=1) walk the quad
  // leaf at blasOffset and rebuild the sub_tri-selected triangle from vert21 (no side table);
  // non-BLAS refs (type=0) dispatch to getNodeFaceVerts on the encoded srcFace.
  // A tri_ref_t bakes in nodeIndex, which optimize()/rebuildNodesLL re-permute on every (re)build, so a
  // ref is valid only until the next build -- fine for ephemeral damage-model use, not for persistence.
  bool getNodeFaceVertsByRef(tri_ref_t ref, Point3 & v0, Point3 & v1, Point3 & v2) const;

  template <class CB> // void(int face_idx, uint16_t i0, uint16_t i1, uint16_t i2)
  void iterateNodeFaces(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    if (n->flags & CollisionNode::BLAS_RESIDENT)
    {
      walkBlasResidentNodeLeavesForFaces(*n, cb);
      return;
    }
    const uint16_t *idx = meshIndicesBase + n->indicesOfs;
    for (uint32_t i = 0, fi = 0, e = n->indicesCount; i < e; i += 3, ++fi)
      cb((int)fi, idx[i], idx[i + 1], idx[i + 2]);
  }

  template <class CB> // void(int vert_idx, vec4f v)
  void iterateNodeVerts(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || n->indicesCount == 0)
      return;
    const uint32_t vertCount = (uint32_t)n->verticesCount + 1u;
    if (n->flags & CollisionNode::BLAS_RESIDENT)
    {
      const Grid &g = getBlasGridForResidentNode(*n);
      const uint8_t *vbase = g.blasData.data() + g.blasVertsOfs() + (size_t)n->verticesOfs * BVH_BLAS_VERT21_STRIDE;
      const vec3f invScale = g.blasInvScale;
      const vec3f bmin = g.blasBBox.bmin;
      for (uint32_t i = 0; i < vertCount; ++i)
      {
        vec3f q = RayData::unpackVert21(vbase + i * BVH_BLAS_VERT21_STRIDE);
        cb((int)i, v_madd(q, invScale, bmin));
      }
    }
    else
    {
      const Point3_vec4 *verts = meshVertsBase + n->verticesOfs;
      for (uint32_t i = 0; i < vertCount; ++i)
        cb((int)i, v_ld(&verts[i].x));
    }
  }

  template <class CB> // void(int face_idx, vec4f v0, vec4f v1, vec4f v2)
  void iterateNodeFacesVerts(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    if (n->flags & CollisionNode::BLAS_RESIDENT)
    {
      // Resident nodes are always MESH; ownIndices was dropped. Walk leaves and decode each
      // sub-triangle's verts from the leaf encoding.
      const Grid &g = getBlasGridForResidentNode(*n);
      const uint8_t *vbase = g.blasData.data() + g.blasVertsOfs() + (size_t)n->verticesOfs * BVH_BLAS_VERT21_STRIDE;
      const vec3f invScale = g.blasInvScale;
      const vec3f bmin = g.blasBBox.bmin;
      auto unq = [vbase, invScale, bmin](uint32_t li) -> vec3f {
        return v_madd(RayData::unpackVert21(vbase + li * BVH_BLAS_VERT21_STRIDE), invScale, bmin);
      };
      walkBlasResidentNodeLeavesForFaces(*n,
        [&](int fi, uint16_t i0, uint16_t i1, uint16_t i2) { cb(fi, unq(i0), unq(i1), unq(i2)); });
      return;
    }
    const uint16_t *idx = meshIndicesBase + n->indicesOfs;
    const Point3_vec4 *verts = meshVertsBase + n->verticesOfs;
    for (uint32_t i = 0, fi = 0, e = n->indicesCount; i < e; i += 3, ++fi)
      cb((int)fi, v_ld(&verts[idx[i]].x), v_ld(&verts[idx[i + 1]].x), v_ld(&verts[idx[i + 2]].x));
  }

  // Decoded BLAS leaf layout: where the leaf's verts start (vertBytesOfs into blasData) plus the
  // three sub-tri offsets o1/o2/o3 relative to v0. Emits a leaf's sub-triangles (one for single-tri
  // leaves, two for quads). Encoding in swBLASLeafDefs.hlsli.
  struct BlasLeafLayout
  {
    uint32_t vertBytesOfs; // absolute byte offset into blasData where v0 lives
    uint32_t o1, o2, o3;   // 1-based vert offsets relative to v0 (in 8-byte vert21 strides)
    bool isSingle;         // single-triangle leaf (only sub-tri 0 valid)
    bool isFan;            // sub-tri 1 layout for quads: fan (true) vs strip (false); ignored if isSingle
  };
  static inline BlasLeafLayout decodeBlasLeafLayout(const uint8_t *blas_data, uint32_t data_ofs)
  {
    // data_ofs is the leaf body offset = leafBoxStart + BVH_BLAS_NODE_SIZE; the skip word sits at
    // [-1] (4th uint32 of the 16-byte node header).
    const uint32_t skip = ((const uint32_t *)(blas_data + data_ofs))[-1];
    const int ofs1Rel = ((const int *)(blas_data + data_ofs))[0];
    BlasLeafLayout L;
    L.vertBytesOfs = (uint32_t)((int)data_ofs + ofs1Rel);
    L.o1 = (skip & QUAD_O1_MASK) + 1u;
    L.o2 = ((skip >> QUAD_O2_SHIFT) & QUAD_O2_MASK) + 1u;
    L.o3 = ((skip >> QUAD_O3_SHIFT) & QUAD_O3_MASK) + 1u;
    L.isSingle = (L.o3 == L.o2);
    L.isFan = !L.isSingle && (skip & QUAD_FAN_FLAG);
    return L;
  }

  // Walks a BLAS-resident MESH node's leaves, calling cb(face_idx, local_i0, local_i1, local_i2)
  // per sub-triangle in DFS order. Local indices are node-local 0..verticesCount (same convention
  // ownIndices used). Used by iterateNodeFaces / iterateNodeFacesVerts on BLAS-resident nodes
  // (always MESH). Each leaf's owning node is identified by its first vert21 index vs the node's
  // NodeRange. Low-frequency / one-shot materialisation (Jolt mesh build, AssetViewer, long-form
  // intersection).
  template <class CB> // void(int face_idx, uint16_t i0, uint16_t i1, uint16_t i2)
  void walkBlasResidentNodeLeavesForFaces(const CollisionNode &node, CB cb) const
  {
    const Grid &g = getBlasGridForResidentNode(node);
    if (g.blasData.empty())
      return;
    const Grid::NodeRange *nr = nullptr;
    for (const auto &r : g.blasNodeRanges)
      if (r.nodeIndex == node.nodeIndex)
      {
        nr = &r;
        break;
      }
    if (!nr)
      return;
    const uint8_t *bData = g.blasData.data();
    // Traversal bound is the BVH tree region only ([0, blasTreeBytes)); vert21 follows the tree, so
    // a larger bound would decode verts as node headers (matches the .cpp BLAS walkers).
    const int bSize = (int)g.blasTreeBytes;
    const uint32_t vertsOfs = g.blasVertsOfs();
    const uint32_t nodeVOfs = nr->verticesOfs;
    const uint32_t nodeVEnd = nr->verticesEnd;
    int fi = 0;
    // iterateFiltered fires the leaf callback once per sub-triangle (twice per quad, same dataOfs),
    // but our callback emits the leaf's full sub-tri set. The last-leaf-offset guard drops the quad's
    // second call so its faces aren't emitted twice (overrunning getNodeFaceCount()-sized buffers).
    int lastLeafOfs = -1;
    // Spatial prune to this node's bbox (box-space) is an optimisation -- skips sibling-node subtrees
    // of the combined BLAS. The node is IDENT, so modelBBox is in the BLAS quant frame and contains
    // every leaf of this node (a few-unit pad absorbs quantization rounding), so the prune never drops
    // an owned leaf. The v0Idx NodeRange check below is the authoritative ownership filter.
    const vec3f nodeBoxMin = v_ldu(&node.modelBBox.lim[0].x);
    const vec3f nodeBoxMax = v_ldu_p3(&node.modelBBox.lim[1].x);
    const vec3f boxPad = v_splats(4.f);
    const vec3f nodeBoxMinQ = v_sub(v_madd(nodeBoxMin, g.blasScale, g.blasOfs), boxPad);
    const vec3f nodeBoxMaxQ = v_add(v_madd(nodeBoxMax, g.blasScale, g.blasOfs), boxPad);
    // The leaf callback only consumes `dataOfs` (unquantised vert args unused), so any VertexLoader
    // works. Use the built-in RDVertexLoader to keep the resource-local unquantiser out of this header.
    BLASTraverse<false>::iterateFiltered(
      bData, 0, bSize,
      [nodeBoxMinQ, nodeBoxMaxQ](vec3f bmn, vec3f bmx) {
        return v_check_xyz_all_true(v_and(v_cmp_ge(nodeBoxMaxQ, bmn), v_cmp_ge(bmx, nodeBoxMinQ)));
      },
      [&](vec3f, vec3f, vec3f, int dataOfs) -> bool {
        if (dataOfs == lastLeafOfs)
          return false; // second per-sub-tri callback for the same quad leaf -- already emitted
        lastLeafOfs = dataOfs;
        const BlasLeafLayout L = decodeBlasLeafLayout(bData, (uint32_t)dataOfs);
        const uint32_t v0Idx = (L.vertBytesOfs - vertsOfs) / BVH_BLAS_VERT21_STRIDE;
        if (v0Idx < nodeVOfs || v0Idx >= nodeVEnd)
          return false; // sibling-node leaf
        const uint32_t baseLocal = v0Idx - nodeVOfs;
        // Sub-triangle 0: (v0, v1, v2) at local (base, base+o1, base+o2).
        cb(fi++, (uint16_t)baseLocal, (uint16_t)(baseLocal + L.o1), (uint16_t)(baseLocal + L.o2));
        if (!L.isSingle)
        {
          // Sub-triangle 1: fan -> (v0, v2, v3); strip -> (v2, v1, v3).
          if (L.isFan)
            cb(fi++, (uint16_t)baseLocal, (uint16_t)(baseLocal + L.o2), (uint16_t)(baseLocal + L.o3));
          else
            cb(fi++, (uint16_t)(baseLocal + L.o2), (uint16_t)(baseLocal + L.o1), (uint16_t)(baseLocal + L.o3));
        }
        return false; // continue walking
      },
      BLASTraverse<false>::RDVertexLoader{});
  }

  int getNodeConvexPlaneCount(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? (int)n->planesCount : 0;
  }

  dag::ConstSpan<plane3f> getNodeConvexPlanes(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || n->planesCount == 0)
      return {};
    return dag::ConstSpan<plane3f>(convexPlanes.data() + n->planesOfs, n->planesCount);
  }

  template <class CB> // void(int plane_idx, plane3f plane)
  void iterateNodeConvexPlanes(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    const plane3f *p = convexPlanes.data() + n->planesOfs;
    for (int i = 0, e = (int)n->planesCount; i < e; ++i)
      cb(i, p[i]);
  }

  BBox3 getNodeBBox(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? n->modelBBox : BBox3();
  }
  BSphere3 getNodeBSphere(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || n->boundingSphere.r < 0)
      return BSphere3(); // empty: r = r2 = -1
    return BSphere3(n->boundingSphere.c, n->boundingSphere.r);
  }
  bool getNodeCapsule(int node_id, Capsule &out) const
  {
    const CollisionNode *n = getNode(node_id);
    if (n && n->type == COLLISION_NODE_TYPE_CAPSULE)
    {
      out = capsules[n->capsuleIndex];
      return true;
    }
    return false;
  }
  // Bakes the node's current TM fully into the geometry: transforms vertices, bbox, bsphere; flips face winding when TM is mirrored
  // (det<0); then resets the node TM to identity. Single entry point keeps those updates consistent.
  void bakeNodeTransform(int node_id);

  float getNodeMaxTmScale(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? n->cachedMaxTmScale : 1.f;
  }

  // Unsafe accessors: no bounds check in release, use G_ASSERT for validation
  const TMatrix &getNodeTmUnsafe(int node_id) const
  {
    G_ASSERT((uint32_t)node_id < allNodesList.size());
    return allNodesList[node_id].tm;
  }
  BBox3 getNodeBBoxUnsafe(int node_id) const
  {
    G_ASSERT((uint32_t)node_id < allNodesList.size());
    return allNodesList[node_id].modelBBox;
  }
  BSphere3 getNodeBSphereUnsafe(int node_id) const
  {
    G_ASSERT((uint32_t)node_id < allNodesList.size());
    const auto &b = allNodesList[node_id].boundingSphere;
    if (b.r < 0)
      return BSphere3(); // empty: r = r2 = -1
    return BSphere3(b.c, b.r);
  }
  Point3 getNodeBSphereCenter(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? n->boundingSphere.c : Point3(0, 0, 0);
  }
  float getNodeBSphereRadius(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? n->boundingSphere.r : -1.f;
  }
  const Capsule &getNodeCapsuleUnsafe(int node_id) const
  {
    G_ASSERT((uint32_t)node_id < allNodesList.size());
    G_ASSERT(allNodesList[node_id].type == COLLISION_NODE_TYPE_CAPSULE);
    return capsules[allNodesList[node_id].capsuleIndex];
  }
  float getNodeMaxTmScaleUnsafe(int node_id) const
  {
    G_ASSERT((uint32_t)node_id < allNodesList.size());
    return allNodesList[node_id].cachedMaxTmScale;
  }

  template <class CB> // void(const CollisionNode &node) or bool(const CollisionNode &node) - return true to stop
  void forEachMeshNode(CB cb) const
  {
    forEachNodeImpl(meshNodesHead, cb);
  }
  template <class CB> // void(const CollisionNode &node) or bool(const CollisionNode &node) - return true to stop
  void forEachBoxNode(CB cb) const
  {
    forEachNodeImpl(boxNodesHead, cb);
  }
  template <class CB> // void(const CollisionNode &node) or bool(const CollisionNode &node) - return true to stop
  void forEachSphereNode(CB cb) const
  {
    forEachNodeImpl(sphereNodesHead, cb);
  }
  template <class CB> // void(const CollisionNode &node) or bool(const CollisionNode &node) - return true to stop
  void forEachCapsuleNode(CB cb) const
  {
    forEachNodeImpl(capsuleNodesHead, cb);
  }

  bool getRelGeomNodeTms(int node_no, TMatrix &out_tm) const
  {
    if (node_no < 0 || node_no >= relGeomNodeTms.size())
      return false;
    out_tm = relGeomNodeTms[node_no];
    return true;
  }

  CollisionNode &createNode() { return allNodesList.push_back(); }

  bool checkGridAvailable(uint8_t behavior_filter) const { return hasBlas(behavior_filter); }
  int getMemoryUsed() const;
  bool getGridSize(uint8_t behavior_filter, IPoint3 & width, Point3 & leaf_size) const;
  int getTrianglesCount(uint8_t behavior_filter) const;
  void setBsphereCenterNode(int ni) { bsphereCenterNode = dag::Index16(ni); }
  vec4f getWorldBoundingSphere(const mat44f &tm, const GeomNodeTree *geom_node_tree) const;
  Point3 getWorldBoundingSphere(const TMatrix &tm, const GeomNodeTree *geom_node_tree) const;
  bool validateVerticesForJolt(const char *res_name, auto &&on_degenerate);
  bool validateVerticesForJolt(const char *res_name);
  dag::Vector<DegenerativeNodeData> getDegenerativeNodes(const char *res_name);

  Point3 getBoundingSphereCenter() const { return *(const Point3 *)(const void *)&vBoundingSphere; }
  float getBoundingSphereRad() const { return boundingSphereRad; }
  float getBoundingSphereRadSq() const { return v_extract_w(vBoundingSphere); }
  vec4f getBoundingSphereXYZR() const { return v_perm_xyzd(vBoundingSphere, v_splats(boundingSphereRad)); }
  BSphere3 getBoundingSphereS() const { return BSphere3(getBoundingSphereCenter(), boundingSphereRad); }

  void sortNodesList();  //< sort nodes by size and setup .insideOfNode members
  void rebuildNodesLL(); //< rebuild *NodesHead linked lists for actual nodes

private:
  // Per-pair tri-tri test for testIntersection's mesh-vs-mesh inner loop. A static member (not a
  // free function) so it inherits CollisionNode's friend grant -- it dereferences node1's protected
  // tm / modelBBox / nodeIndex. Defined in collisionGameRes.cpp next to testIntersection.
  static bool testMeshNodePair(const CollisionNode *node1, dag::ConstSpan<Point3_vec4> node1Faces, const CollisionResource *res2,
    const CollisionNode *node2, const TMatrix &tm1ToWorld, const TMatrix &tm2, const TMatrix &tm2to1, Point3 &cp1, Point3 &cp2,
    uint16_t *node_index1, uint16_t *node_index2);

  template <class CB>
  void forEachNodeImpl(uint16_t headIdx, CB & cb) const
  {
    for (uint16_t i = headIdx; i != CollisionNode::INVALID_IDX; i = allNodesList[i].nextNode)
    {
      const CollisionNode &n = allNodesList[i];
      if constexpr (eastl::is_same_v<decltype(cb(n)), bool>)
      {
        if (cb(n))
          return;
      }
      else
        cb(n);
    }
  }

  enum IterationMode
  {
    ALL_INTERSECTIONS,       // all intersections will be passed to callback
    ALL_NODES_INTERSECTIONS, // all nodes intersections, but only one best for each
    FIND_BEST_INTERSECTION, // intersections will be passed to callback only when next intersection better than previous (the best will
                            // be last)
    ANY_ONE_INTERSECTION    // only one first intersection will be passed to callback
  };

  enum class CollisionTraceType
  {
    TRACE_RAY,
    TRACE_CAPSULE,
    RAY_HIT,
    CAPSULE_HIT
  };

  template <IterationMode trace_mode, CollisionTraceType trace_type, typename filter_t, typename callback_t>
  __forceinline bool forEachIntersectedNode(mat44f tm, const GeomNodeTree *geom_node_tree, vec3f from, vec3f dir, float len,
    bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
    TraceCollisionResourceStats *out_stats, bool force_no_cull) const;

  template <IterationMode trace_mode, CollisionTraceType trace_type, bool is_single_ray = false, typename filter_t,
    typename callback_t>
  __forceinline bool forEachIntersectedNode(mat44f tm, const GeomNodeTree *geom_node_tree, dag::Span<CollisionTrace> traces,
    bool calc_normal, float bsphere_scale, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
    TraceCollisionResourceStats *out_stats, bool force_no_cull) const;

  template <bool orthonormalized_instance_tm, IterationMode trace_mode, CollisionTraceType trace_type, bool is_single_ray = false,
    typename filter_t, typename callback_t>
  __forceinline bool forEachIntersectedNode(mat44f tm, float max_tm_scale_sq, vec3f woffset, const GeomNodeTree *geom_node_tree,
    dag::Span<CollisionTrace> traces, bool calc_normal, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
    TraceCollisionResourceStats *out_stats, bool force_no_cull) const;

  template <bool check_bounding>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalCullCCW(const Point3_vec4 *verticesBase, const uint16_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm);
  template <bool check_bounding, uint32_t mainBatchSize = 8>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalCullCCW_AVX256(const Point3_vec4 *verticesBase, const uint16_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm);
  // verts_base + node.verticesOfs must point at `node`'s vert block. For BLAS-resident nodes the
  // caller decodes vert21 into a temp buffer and passes (buffer.data(), node-copy-with-ofs0); for
  // non-resident nodes it passes (meshVertsBase, *meshNode) unchanged.
  DAGOR_NOINLINE bool traceCapsuleMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint16_t *idx_base,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, float &radius, vec4f &v_out_norm,
    vec4f &v_out_pos) const;

  template <bool check_bounding>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalAllHits(const Point3_vec4 *verticesBase, const uint16_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull,
    all_collres_nodes_t &ret_array, all_collres_tri_indices_t &tri_indices);
  template <bool check_bounding, uint32_t mainBatchSize = 8>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalAllHits_AVX256(const Point3_vec4 *verticesBase, const uint16_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull,
    all_collres_nodes_t &ret_array, all_collres_tri_indices_t &tri_indices);

  template <bool check_bounding>
  DAGOR_NOINLINE static bool rayHitMeshNodeLocalCullCCW(const Point3_vec4 *verticesBase, const uint16_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t);
  template <bool check_bounding, uint32_t mainBatchSize = 8>
  DAGOR_NOINLINE static bool rayHitMeshNodeLocalCullCCW_AVX256(const Point3_vec4 *verticesBase, const uint16_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t);

  // Same verts_base / idx_base convention as traceCapsuleMeshNodeLocalCullCCW above.
  DAGOR_NOINLINE bool capsuleHitMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint16_t *idx_base,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, float radius) const;

  // For BLAS-resident nodes: decode verts (and, for MESH, indices) from the active grid into the
  // caller's framemem scratches, build a CollisionNode copy with verticesOfs/indicesOfs rebased to 0,
  // and point out_* at the scratches + copy. For non-resident nodes: point out_* at (meshVertsBase,
  // meshIndicesBase, &node) unchanged (no allocation). Returned pointers are valid until the scratches
  // go out of scope or are resized. The rebased node copy lets the existing pointer+offset idioms
  // ((base + node.verticesOfs) / (base + node.indicesOfs)) work unchanged.
  void resolveNodeVertsForCall(const CollisionNode &node, dag::Vector<Point3_vec4, framemem_allocator> &vertsScratch,
    dag::Vector<uint16_t, framemem_allocator> &idxScratch, CollisionNode &node_copy, const Point3_vec4 *&out_verts_base,
    const uint16_t *&out_idx_base, const CollisionNode *&out_node) const;

  __forceinline mat44f getMeshNodeTmInline(const CollisionNode *node, mat44f_cref instance_tm, vec3f instance_woffset,
    const GeomNodeTree *geom_node_tree) const;

public:
  // Per-behavior container holding the combined BLAS over all behavior_flag-matching IDENT mesh
  // nodes. Public for read access only; mutators (reset, buildBLAS) are for CollisionResource and
  // the friends below. External consumers walk blasData via getBlasGrid(behavior_filter) (e.g.
  // mesh-vs-mesh candidate enumeration).
  struct Grid
  {
    Grid();
    void reset();
    // Build the combined-per-behavior BLAS over all behavior_flag-matching IDENT mesh nodes; not
    // persisted (rebuilt at load). two_sided -> blasTwoSided: whether this grid stands in for a
    // two-sided (CULL_BOTH) FRT or the backface-culling per-node path. See blasTwoSided.
    void buildBLAS(CollisionResource *parent, uint8_t behavior_flag, bool two_sided);

    // BLAS storage. Empty when buildBLAS gates eliminate the build (small resource, non-IDENT mesh,
    // SOLID-behavior). Layout [daBVH tree bytes][pad to 8][vert21 array]; vert base = blasVertsOfs().
    dag::Vector<uint8_t> blasData;      // [BVH tree bytes][pad][vert21 array]
    bbox3f blasBBox = {};               // resource-local bbox over all triangles in the BLAS
    vec3f blasScale = {}, blasOfs = {}; // quantization frame (scale = 65535/extent, ofs = -bmin*scale)
    vec3f blasInvScale = {};            // cached 1/blasScale (per-axis), set in buildBLAS; used by all vert21 decode paths
    uint32_t blasTreeBytes = 0;         // byte size of the BVH tree portion (tree-walk bound)
    // Byte offset of the vert21 stream in blasData. Padded to 8, but no longer required for
    // correctness: MOC RenderBLAS now derives vertex indices relative to this stream base (passed as
    // the vertOffset argument), so the index divide is exact at any tree size. Kept only as a potential
    // perf aid -- it keeps every vert21 (8 B) load 8-aligned.
    uint32_t blasVertsOfs() const { return (blasTreeBytes + 7u) & ~7u; }

    // Per-node vert21 range table, one entry per BLAS-resident MESH node in this grid (CONVEX never
    // goes into the BLAS), sorted by verticesOfs (= per-node flatten insertion order). vert21 indices
    // are contiguous within a node (the fetch-remap pass is dropped). Used two ways: (1) trace dispatch
    // derives a hit's source CollisionNode by upper_bound-ing a leaf's first vert21 index; (2)
    // iterateNodeFaces / getNodeFaceVerts walk the BLAS and filter leaves whose first vert21 index lies
    // in [verticesOfs, verticesEnd). ~50 entries x 16 B on jaguar_ebrc_dm.
    struct NodeRange
    {
      uint32_t verticesOfs; // start of node's vert block in vert21 (inclusive)
      uint32_t verticesEnd; // end of node's vert block in vert21 (exclusive) -- = next node's verticesOfs
      uint16_t nodeIndex;   // source CollisionNode index
    };
    dag::Vector<NodeRange> blasNodeRanges;

    // Cull mode for this grid's BLAS ray test. true = two-sided (matching the CULL_BOTH FRT it
    // replaces); false = backface-cull CCW (matching the per-node traceRayMeshNodeLocalCullCCW path).
    // Set in buildBLAS from need_frt or the persisted HAS_*_FRT disk flag, so trace dispatch
    // reproduces the pre-BLAS cull behavior. Only the per-leaf cull test honors it -- the BLAS is
    // built unconditionally.
    bool blasTwoSided = false;
  };

  // Selector: with COLLISION_RES_FLAG_REUSE_TRACE_FRT set, both behaviors share gridForTraceable;
  // otherwise PHYS_COLLIDABLE uses gridForCollidable and TRACEABLE uses gridForTraceable.
  const Grid &getBlasGrid(uint8_t behavior_filter) const
  {
    if (behavior_filter == CollisionNode::PHYS_COLLIDABLE && !(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT))
      return gridForCollidable;
    return gridForTraceable;
  }
  bool hasBlas(uint8_t behavior_filter) const { return !getBlasGrid(behavior_filter).blasData.empty(); }

  // True when getBlasGrid(behavior_filter) returns gridForCollidable. Trace dispatch stamps the
  // tri_ref::make_blas grid bit from this so a post-trace getNodeFaceVertsByRef picks the right grid.
  bool isCollidableGridForTrace(uint8_t behavior_filter) const
  {
    return behavior_filter == CollisionNode::PHYS_COLLIDABLE && !(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT);
  }

  // Pick the grid a BLAS_RESIDENT node's verticesOfs indexes into. Must mirror the exporter's
  // snap-box choice (exp_collision.cpp vert21 weld): PHYS_COLLIDABLE feeds Jolt, so its grid is
  // authoritative for a node carrying that bit -- the exporter welds such a node's verts onto the
  // COLLIDABLE grid's vert21 cell centers, and only a decode through that grid's quantization frame
  // reproduces them exactly. When the trace and collidable node sets differ the two boxes differ, and
  // decoding a TRACEABLE|PHYS_COLLIDABLE node through gridForTraceable would hand Jolt verts rounded
  // off their snapped cell centers (re-creating the degenerate slivers the snap removes).
  //  - PHYS_COLLIDABLE bit set: gridForCollidable, unless REUSE_TRACE_FRT (single shared grid).
  //  - trace-only node: gridForTraceable.
  // compactOwnVertices stamps verticesOfs via this same selector, keeping stamp and resolve in sync.
  // Caller must ensure the node is BLAS_RESIDENT first (else the returned grid may have empty blasData).
  const Grid &getBlasGridForResidentNode(const CollisionNode &node) const
  {
    if ((node.behaviorFlags & CollisionNode::PHYS_COLLIDABLE) && !(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT))
      return gridForCollidable;
    return gridForTraceable;
  }

  // After both buildBLAS calls, drop ownVertices/ownIndices for nodes that landed in either grid's
  // BLAS, stamping BLAS_RESIDENT + reinterpreted verticesOfs (= vert21 index) + verticesCount
  // (= post-dup count-1) on each (gridForCollidable wins for nodes in both grids -- it feeds Jolt and
  // the exporter snapped to its box; see getBlasGridForResidentNode). indicesOfs is
  // zeroed but indicesCount kept; face data is recovered by walking the grid's BLAS leaves filtered
  // to the node's vert21 range. Non-resident nodes get their slices copied to the head of fresh
  // buffers with offsets rewritten, and keep ownIndices.
  //
  // Idempotent. Intentionally NOT invoked from collapseAndOptimize -- the exporter/editor read back
  // ownVertices to write the asset binary and must not lose it. Runtime-load callers
  // (CollisionResource::load, rendInst's optimize_collres_on_load wrapper) invoke it explicitly.
  void compactOwnVertices();

protected:
  friend class CollisionGameResFactory;
  friend CollisionExporter;
  friend dabuildExp_collision::CollisionExporter;
  friend struct CollisionResourceUnittest;

  static DAGOR_NOINLINE void addTracesProfileTag(dag::Span<CollisionTrace> traces);
  static DAGOR_NOINLINE void addMeshNodesProfileTag(const struct CollResProfileStats &profile_stats);

  Tab<CollisionNode> allNodesList;
  Tab<TMatrix> relGeomNodeTms; // parallel to allNodesList
  // Tightly packed null-terminated names; offset 0 always holds '\0' so nameOfs==0 means empty
  dag::Vector<char> names;
  // Dense storage for capsule-type nodes; CollisionNode::capsuleIndex addresses entries here
  dag::Vector<Capsule> capsules;
  // Dense storage for CONVEX-type node planes; CollisionNode::planesOfs/planesCount address entries here.
  dag::Vector<plane3f> convexPlanes;
  // Owning storage for mesh / convex node geometry. May be empty if mesh data is referenced
  // externally (e.g. via the FRT vertex/face arrays after collapseAndOptimize, or via
  // createSingleMeshNonOwning). meshVertsBase / meshIndicesBase point at the active source.
  dag::Vector<Point3_vec4> ownVertices;
  dag::Vector<uint16_t> ownIndices;
  const Point3_vec4 *meshVertsBase = nullptr;
  const uint16_t *meshIndicesBase = nullptr;
  uint32_t addName(const char *name);

  Grid gridForTraceable;
  Grid gridForCollidable;

  struct TraceMeshNodeLocalFunctions
  {
    decltype(&CollisionResource::traceRayMeshNodeLocalCullCCW<true>) pfnTraceRayMeshNodeLocalCullCCW;
    decltype(&CollisionResource::rayHitMeshNodeLocalCullCCW<true>) pfnRayHitMeshNodeLocalCullCCW;
    decltype(&CollisionResource::traceRayMeshNodeLocalAllHits<true>) pfnTraceRayMeshNodeLocalAllHits;
  };
  struct TraceMeshNodeLocalApi
  {
    int threshold;
    TraceMeshNodeLocalFunctions light;
    TraceMeshNodeLocalFunctions heavy;
  };
  static TraceMeshNodeLocalApi traceMeshNodeLocalApi;
  static const TraceMeshNodeLocalApi traceMeshNodeLocalApi_AVX256;
  static const bool haveTraceMeshNodeLocalApi_AVX256;
  static void check_avx_mesh_api_support();
  void loadLegacyRawFormat(IGenLoad & cb, int res_id, int (*resolve_phmat)(const char *) = nullptr);

end_dclass_decl();
