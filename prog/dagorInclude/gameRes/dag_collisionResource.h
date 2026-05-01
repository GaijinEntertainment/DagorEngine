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
#include <util/dag_globDef.h> // DestroyDeleter
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
#include <EASTL/unique_ptr.h>
#include <EASTL/bitvector.h>
#include <sceneRay/dag_sceneRayDecl.h>
#include <dag/dag_vector.h>

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
  Point3 normal;
  union
  {
    float intersectionT;
    int sortKey; // Note: signed to correctly handle -0.0 (which is mapped to INT_MIN)
  };
  Point3 intersectionPos;
  unsigned int collisionNodeId;
  int triangleIndex; // index of the triangle within the collision node (indices[triangleIndex*3])
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
  COLLISION_RES_FLAG_REUSE_TRACE_FRT = 1 << 4,
  COLLISION_RES_FLAG_HAS_TRACE_FRT = 1 << 5,
  COLLISION_RES_FLAG_HAS_COLL_FRT = 1 << 6,
};

struct CollisionNode
{
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

    FLAG_VERTICES_ARE_REFS = 64,
    FLAG_INDICES_ARE_REFS = 128,
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

    FLAG_CHECK_SURROUNDING_PART_FOR_EXCLUSION = 1 << 14,
    FLAG_ALLOW_SPLASH_HOLE = 1 << 15
  };

  uint16_t behaviorFlags = TRACEABLE | PHYS_COLLIDABLE | FLAG_ALLOW_HOLE | FLAG_DAMAGE_REQUIRED;
  eastl::underlying_type_t<NodeFlag> flags = NodeFlag::NONE;
  CollisionResourceNodeType type = COLLISION_NODE_TYPE_MESH;
  dag::Index16 geomNodeId;
  int16_t physMatId = -1;
  CollisionNode *nextNode = nullptr;

protected:
  alignas(16) TMatrix tm = TMatrix::IDENT;
  alignas(16) BBox3 modelBBox;
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

protected:
  SmallTab<plane3f, MidmemAlloc> convexPlanes;

public:
  dag::Span<Point3_vec4> vertices;
  dag::Span<uint16_t> indices;
  uint16_t nodeIndex = 0; // In allNodesList.
  uint16_t insideOfNode = 0xffff;

protected:
  uint32_t nameOfs = 0;      // offset into CollisionResource::names; 0 means empty
  uint16_t capsuleIndex = 0; // index into CollisionResource::capsules; valid only for COLLISION_NODE_TYPE_CAPSULE

public:
  CollisionNode() = default;
  explicit CollisionNode(const CollisionNode &n) : CollisionNode() { operator=(n); }
  CollisionNode(CollisionNode &&n) : CollisionNode() { operator=((CollisionNode &&)n); }
  ~CollisionNode()
  {
    resetVertices();
    resetIndices();
  }
  CollisionNode &operator=(const CollisionNode &n);
  CollisionNode &operator=(CollisionNode &&n);

  void resetVertices(dag::Span<Point3_vec4> new_val = {})
  {
    if (!(flags & FLAG_VERTICES_ARE_REFS))
      midmem->free(vertices.data());
    vertices.set(new_val);
  }
  void resetIndices(dag::Span<uint16_t> new_val = {})
  {
    if (!(flags & FLAG_INDICES_ARE_REFS))
      midmem->free(indices.data());
    indices.set(new_val);
  }

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
  bbox3f vFullBBox;      // all nodes, including box
  vec4f vBoundingSphere; // center|r^2 in w
  alignas(16) BBox3 boundingBox;
  float boundingSphereRad = 0;
  uint32_t collisionFlags = 0;

  union
  {
    struct
    {
      const CollisionNode *meshNodesHead;
      const CollisionNode *pointsNodesHead;
      const CollisionNode *boxNodesHead;
      const CollisionNode *sphereNodesHead;
      const CollisionNode *capsuleNodesHead;
    };
    const CollisionNode *nodeLists[NUM_COLLISION_NODE_TYPES];
  };
  uint32_t numMeshNodes = 0, numBoxNodes = 0;
  dag::Index16 bsphereCenterNode;

  CollisionResource() { memset(nodeLists, 0, sizeof(nodeLists)); }
  CollisionResource *deepCopy(void *inplace_mem_ptr = nullptr) const;

  dag::Span<CollisionNode> getAllNodes() { return make_span(allNodesList); }
  dag::ConstSpan<CollisionNode> getAllNodes() const { return allNodesList; }

  static CollisionResource *loadResource(IGenLoad & crd, int res_id);
  // Create a single-node MESH resource with non-owning vertex span.
  // Vertex data are referenced (FLAG_VERTICES_ARE_REFS), caller must keep data alive.
  // Index data is owned by resource
  static CollisionResource *createSingleMesh(dag::Span<Point3_vec4> vertices, dag::Span<uint16_t> indices, const BBox3 &bbox,
    const BSphere3 &bsphere, uint32_t node_flags);
  static CollisionResource *createSingleMeshNonOwningVerts(dag::Span<Point3_vec4> vertices, dag::Span<uint16_t> indices,
    const BBox3 &bbox, const BSphere3 &bsphere)
  {
    return createSingleMesh(vertices, indices, bbox, bsphere, CollisionNode::FLAG_VERTICES_ARE_REFS);
  }
  void load(IGenLoad & cb, int res_id);

  // Add typed collision nodes. Returns nodeIndex. Sets type, modelBBox, boundingSphere, capsule.
  int addSphereNode(const char *name, int16_t phys_mat_id, const BSphere3 &bsphere);
  int addBoxNode(const char *name, int16_t phys_mat_id, const BBox3 &bbox);
  int addCapsuleNode(const char *name, int16_t phys_mat_id, const Point3 &p0, const Point3 &p1, float radius);

  void collapseAndOptimize(const char *res_name, bool need_frt = false, bool frt_build_fast = true);

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

  // Don't use it! It's should not be external.
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
  static bool testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionResource *res2, const TMatrix &tm2,
    Point3 &collisionPoint1, Point3 &collisionPoint2, bool checkOnlyPhysNodes = false, bool useTraceFaces = false);

  bool checkInclusion(const Point3 &pos, CollResIntersectionsType &intersected_nodes_list) const;

  // Can check mesh-box, box-box and sph-box only, asserts on any other types
  static bool calcOffsetForIntersection(const TMatrix &tm1, const CollisionNode &node_to_move, const CollisionNode &node_to_check,
    Point3 &offset);

  static bool calcOffsetForSeparation(const TMatrix &tm1, const CollisionNode &node_to_move, const CollisionNode &node_to_check,
    const Point3 &axis, Point3 &offset);


  bool testInclusion(const CollisionNode &node_to_test, const TMatrix &tm_test, const CollisionNode &restraining_node,
    const TMatrix &tm_restrain, const GeomNodeTree *test_node_tree = NULL, const GeomNodeTree *restrain_node_tree = NULL) const;

  bool testInclusion(const CollisionNode &node_to_test, const TMatrix &tm_test, dag::ConstSpan<plane3f> convex,
    const TMatrix &tm_restrain, const GeomNodeTree *test_node_tree = NULL, Point3 *res_pos = nullptr) const;

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

  static bool test_sphere_node_intersection(const BSphere3 &sphere, const CollisionNode *node, const Point3 &dir_norm,
    Point3 &out_norm, float &out_depth);
  static bool test_capsule_node_intersection(const Point3 &p0, const Point3 &p1, float radius, const CollisionNode *node);

  template <typename Func, CollisionResourceNodeType node_type = COLLISION_NODE_TYPE_MESH, bool binded_to_gntree = true>
  void visitCollisionNodes(const Func &func) const
  {
    if (node_type != NUM_COLLISION_NODE_TYPES)
    {
      for (const CollisionNode *node = nodeLists[node_type]; node; node = node->nextNode)
        if (!binded_to_gntree || node->geomNodeId)
          func(*node);
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
    return n ? (int)n->vertices.size() : 0;
  }
  int getNodeFaceCount(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? (int)n->indices.size() / 3 : 0;
  }

  int getNodeIndexCount(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? (int)n->indices.size() : 0;
  }

  bool getNodeFaceVerts(int node_id, int face_idx, Point3 &v0, Point3 &v1, Point3 &v2) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || face_idx * 3 + 2 >= (int)n->indices.size())
      return false;
    v0 = n->vertices[n->indices[face_idx * 3 + 0]];
    v1 = n->vertices[n->indices[face_idx * 3 + 1]];
    v2 = n->vertices[n->indices[face_idx * 3 + 2]];
    return true;
  }

  template <class CB> // void(int face_idx, uint16_t i0, uint16_t i1, uint16_t i2)
  void iterateNodeFaces(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    const uint16_t *idx = n->indices.data();
    for (int i = 0, fi = 0, e = (int)n->indices.size(); i < e; i += 3, ++fi)
      cb(fi, idx[i], idx[i + 1], idx[i + 2]);
  }

  template <class CB> // void(int vert_idx, vec4f v)
  void iterateNodeVerts(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    const Point3_vec4 *verts = n->vertices.data();
    for (int i = 0, e = (int)n->vertices.size(); i < e; ++i)
      cb(i, v_ld(&verts[i].x));
  }

  template <class CB> // void(int face_idx, vec4f v0, vec4f v1, vec4f v2)
  void iterateNodeFacesVerts(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    const uint16_t *idx = n->indices.data();
    const Point3_vec4 *verts = n->vertices.data();
    for (int i = 0, fi = 0, e = (int)n->indices.size(); i < e; i += 3, ++fi)
      cb(fi, v_ld(&verts[idx[i]].x), v_ld(&verts[idx[i + 1]].x), v_ld(&verts[idx[i + 2]].x));
  }

  int getNodeConvexPlaneCount(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? (int)n->convexPlanes.size() : 0;
  }

  dag::ConstSpan<plane3f> getNodeConvexPlanes(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? dag::ConstSpan<plane3f>(n->convexPlanes.data(), n->convexPlanes.size()) : dag::ConstSpan<plane3f>();
  }

  template <class CB> // void(int plane_idx, plane3f plane)
  void iterateNodeConvexPlanes(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    for (int i = 0, e = (int)n->convexPlanes.size(); i < e; ++i)
      cb(i, n->convexPlanes[i]);
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
    for (const CollisionNode *n = meshNodesHead; n; n = n->nextNode)
    {
      if constexpr (eastl::is_same_v<decltype(cb(*n)), bool>)
      {
        if (cb(*n))
          return;
      }
      else
        cb(*n);
    }
  }

  bool getRelGeomNodeTms(int node_no, TMatrix &out_tm) const
  {
    if (node_no < 0 || node_no >= relGeomNodeTms.size())
      return false;
    out_tm = relGeomNodeTms[node_no];
    return true;
  }

  CollisionNode &createNode() { return allNodesList.push_back(); }

  auto *getTracer(uint8_t behavior_filter) const
  {
    if (behavior_filter == CollisionNode::PHYS_COLLIDABLE && !(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT))
      return gridForCollidable.tracer.get();
    return gridForTraceable.tracer.get();
  }
  bool checkGridAvailable(uint8_t behavior_filter) const { return getTracer(behavior_filter) != nullptr; }
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
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalCullCCW(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm);
  template <bool check_bounding, uint32_t mainBatchSize = 8>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalCullCCW_AVX256(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm);
  DAGOR_NOINLINE bool traceCapsuleMeshNodeLocalCullCCW(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
    float &in_out_t, float &radius, vec4f &v_out_norm, vec4f &v_out_pos) const;

  template <bool check_bounding>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalAllHits(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull, all_collres_nodes_t &ret_array,
    all_collres_tri_indices_t &tri_indices);
  template <bool check_bounding, uint32_t mainBatchSize = 8>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalAllHits_AVX256(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull, all_collres_nodes_t &ret_array,
    all_collres_tri_indices_t &tri_indices);

  template <bool check_bounding>
  DAGOR_NOINLINE static bool rayHitMeshNodeLocalCullCCW(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
    float in_t);
  template <bool check_bounding, uint32_t mainBatchSize = 8>
  DAGOR_NOINLINE static bool rayHitMeshNodeLocalCullCCW_AVX256(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float in_t);

  DAGOR_NOINLINE bool capsuleHitMeshNodeLocalCullCCW(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
    float in_t, float radius) const;

  __forceinline mat44f getMeshNodeTmInline(const CollisionNode *node, mat44f_cref instance_tm, vec3f instance_woffset,
    const GeomNodeTree *geom_node_tree) const;

protected:
  friend class CollisionGameResFactory;
  friend CollisionExporter;
  friend dabuildExp_collision::CollisionExporter;
  friend struct CollisionResourceUnittest;

  struct Grid
  {
    Grid();
    ~Grid(); // in order to allow forward declaration
    void reset();
    void buildFRT(CollisionResource *parent, uint8_t behavior_flag, bool frt_build_fast);

    using SSRT16 = const StaticSceneRayTracerT<uint16_t>;
    eastl::unique_ptr<SSRT16, DestroyDeleter<SSRT16>> tracer;
  };

  int getNodeIndexByFaceId(int face_id, uint8_t behavior_filter) const;
  static DAGOR_NOINLINE void addTracesProfileTag(dag::Span<CollisionTrace> traces);
  static DAGOR_NOINLINE void addMeshNodesProfileTag(const struct CollResProfileStats &profile_stats);

  Tab<CollisionNode> allNodesList;
  Tab<TMatrix> relGeomNodeTms; // parallel to allNodesList
  // Tightly packed null-terminated names; offset 0 always holds '\0' so nameOfs==0 means empty
  dag::Vector<char> names;
  // Dense storage for capsule-type nodes; CollisionNode::capsuleIndex addresses entries here
  dag::Vector<Capsule> capsules;
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
