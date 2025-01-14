//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_DObject.h>
#include <util/dag_simpleString.h>
#include <util/dag_index16.h>
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
#include <gameRes/dag_collisionResourceClassId.h>
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
  bool isectBounding;  // internal
  bool isHit;          // out
};

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
  bool operator<(const IntersectedNode &other) const { return sortKey < other.sortKey; }
  bool verifyIsLess(const IntersectedNode &other) const { return intersectionT < other.intersectionT; }
};

struct MultirayIntersectedNode : IntersectedNode
{
  int rayId;
  bool operator<(const MultirayIntersectedNode &other) const
  {
    return rayId < other.rayId || (rayId == other.rayId && sortKey < other.sortKey);
  }
  bool verifyIsLess(const MultirayIntersectedNode &other) const
  {
    return rayId < other.rayId || (rayId == other.rayId && intersectionT < other.intersectionT);
  }
};

typedef dag::RelocatableFixedVector<IntersectedNode, 64, /*bEnableOverflow*/ true, framemem_allocator> CollResIntersectionsType;
typedef dag::RelocatableFixedVector<MultirayIntersectedNode, 256, /*bEnableOverflow*/ true, framemem_allocator>
  MultirayCollResIntersectionsType;
typedef dag::RelocatableFixedVector<vec4f, 8, true, framemem_allocator> all_nodes_ret_t;
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

struct CollisionUserData;

typedef eastl::fixed_function<64, bool(int)> CollisionNodeFilter;
using CollisionNodeMask = eastl::bitvector<framemem_allocator>;

struct CollisionTrace;
struct ProfileStats;

struct CollisionNode
{
  struct UserData
  {
    virtual ~UserData() {}
    virtual CollisionUserData *isCollisionUserData() { return NULL; };
  };

  // Be careful using that flags since entity tm might be scaled too and in most cases will be faster to assume that instance_tm*nodeTm
  // is always scaled See also cachedMaxTmScale member if you anyway want use it
  enum TransformType : uint8_t
  {
    IDENT = 1,           // Identity tm with zero offset
    TRANSLATE = 2,       // Identity tm with offset
    ORTHONORMALIZED = 4, // Unscaled
    ORTHOUNIFORM = 8,    // With uniform scale
  };
  enum NodeFlag : uint8_t
  {
    FLAG_VERTICES_ARE_REFS = 0x40,
    FLAG_INDICES_ARE_REFS = 0x80,
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
  uint8_t flags = 0;
  uint8_t type = 0;
  dag::Index16 geomNodeId;
  int16_t physMatId = -1;
  CollisionNode *nextNode = nullptr;
  alignas(16) TMatrix tm = TMatrix::IDENT;

  alignas(16) BBox3 modelBBox;
  BSphere3 boundingSphere;
  float cachedMaxTmScale = 1.f;
  SmallTab<plane3f, MidmemAlloc> convexPlanes;

  dag::Span<Point3_vec4> vertices;
  dag::Span<uint16_t> indices;
  Capsule capsule;
  uint16_t nodeIndex = 0; // In allNodesList.
  uint16_t insideOfNode = 0xffff;

  SimpleString name;

private:
  UserData *userData = nullptr;

public:
  CollisionNode() = default;
  explicit CollisionNode(const CollisionNode &n) : CollisionNode() { operator=(n); }
  CollisionNode(CollisionNode &&n) : CollisionNode() { operator=((CollisionNode &&) n); }
  ~CollisionNode()
  {
    setUserData(nullptr);
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

  const UserData *getUserData() const { return userData; }
  UserData *getUserData() { return userData; }
  int getNodeIdAsInt() const { return (int)geomNodeId; }

  // If you got crash here with (this == nullptr), it's compiler error. Try to make node iterator simpler.
  bool checkBehaviorFlags(uint16_t f) const { return (behaviorFlags & f) == f; }
  bool isBehaviorFlagsInFilter(uint16_t f) const { return (behaviorFlags | f) == f; }

  void setUserData(UserData *data)
  {
    if (userData == data)
      return;
    if (userData)
      del_it(userData);
    userData = data;
  }
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

  dag::Span<CollisionNode> getAllNodes() { return make_span(allNodesList); }
  dag::ConstSpan<CollisionNode> getAllNodes() const { return allNodesList; }

  static CollisionResource *loadResource(IGenLoad & crd, int res_id);
  virtual void load(IGenLoad & cb, int res_id);

  void collapseAndOptimize(bool need_frt = false, bool frt_build_fast = true);

  CollisionNode *getNode(uint32_t index);
  const CollisionNode *getNode(uint32_t index) const;
  int getNodeIndexByName(const char *name) const;
  CollisionNode *getNodeByName(const char *name);
  const CollisionNode *getNodeByName(const char *name) const;

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
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, float bsphere_scale, const CollisionNodeFilter &filter,
    TraceCollisionResourceStats *out_stats = nullptr, const CollisionNodeMask *collision_node_mask = nullptr) const;

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

  bool multiRayHit(const TMatrix &instance_tm, dag::Span<CollisionTrace> traces) const;

  bool traceMultiRay(const mat44f &tm, dag::Span<CollisionTrace> traces, int ray_mat_id = -1,
    uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool traceMultiRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, dag::Span<CollisionTrace> traces,
    MultirayCollResIntersectionsType &intersected_nodes_list, bool sort_intersections, float bsphere_scale = 1.f,
    uint8_t behavior_filter = CollisionNode::TRACEABLE, const CollisionNodeMask *collision_node_mask = nullptr,
    TraceCollisionResourceStats *out_stats = nullptr) const;

  // Don't use it! It's should not be external.
  VECTORCALL bool traceRayMeshNodeLocal(const CollisionNode &node, vec4f v_local_from, vec4f v_local_dir, float &in_out_t,
    vec4f *out_norm) const;

  VECTORCALL bool traceRayMeshNodeLocalAllHits(const CollisionNode &node, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, bool force_no_cull = false) const;

  VECTORCALL bool rayHit(const mat44f &tm, const Point3 &from, const Point3 &dir, float in_t, int ray_mat_id, int &out_mat_id,
    uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  VECTORCALL bool rayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir,
    float in_t, float bsphere_scale = 1.f, const CollisionNodeMask *collision_node_mask = nullptr, int *out_mat_id = nullptr) const;

  VECTORCALL bool traceQuad(const CollisionNodeFilter &filter, vec3f a00, vec3f a01, vec3f a10, vec3f a11, Point3 &out_point,
    int &out_node_index) const;

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
    float &out_depth) const;
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

  static int get_node_tris_count(const CollisionNode &cnode);
  static void get_node_tri(const CollisionNode &cnode, int idx, const Point3 *&p0, const Point3 *&p1, const Point3 *&p2); // aligned!

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
  bool getGridSize(uint8_t behavior_filter, IPoint3 & width, Point3 & leaf_size) const;
  int getTrianglesCount(uint8_t behavior_filter) const;
  void setBsphereCenterNode(int ni) { bsphereCenterNode = dag::Index16(ni); }
  vec4f getWorldBoundingSphere(const mat44f &tm, const GeomNodeTree *geom_node_tree) const;
  Point3 getWorldBoundingSphere(const TMatrix &tm, const GeomNodeTree *geom_node_tree) const;

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
  VECTORCALL DAGOR_NOINLINE static bool traceRayMeshNodeLocalCullCCW(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm);
  template <bool check_bounding>
  VECTORCALL DAGOR_NOINLINE static bool traceRayMeshNodeLocalCullCCW_AVX256(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm);
  VECTORCALL DAGOR_NOINLINE bool traceCapsuleMeshNodeLocalCullCCW(const CollisionNode &node, vec4f v_local_from, vec4f v_local_dir,
    float &in_out_t, float &radius, vec4f &v_out_norm, vec4f &v_out_pos) const;

  template <bool check_bounding>
  VECTORCALL DAGOR_NOINLINE static bool traceRayMeshNodeLocalAllHits(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull, all_nodes_ret_t &ret_array);
  template <bool check_bounding>
  VECTORCALL DAGOR_NOINLINE static bool traceRayMeshNodeLocalAllHits_AVX256(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull, all_nodes_ret_t &ret_array);

  template <bool check_bounding>
  VECTORCALL DAGOR_NOINLINE static bool rayHitMeshNodeLocalCullCCW(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float in_t);
  template <bool check_bounding>
  VECTORCALL DAGOR_NOINLINE static bool rayHitMeshNodeLocalCullCCW_AVX256(const CollisionNode &node, const vec4f &v_local_from,
    const vec4f &v_local_dir, float in_t);

  VECTORCALL DAGOR_NOINLINE bool capsuleHitMeshNodeLocalCullCCW(const CollisionNode &node, vec4f v_local_from, vec4f v_local_dir,
    float in_t, float radius) const;

  __forceinline mat44f getMeshNodeTmInline(const CollisionNode *node, mat44f_cref instance_tm, vec3f instance_woffset,
    const GeomNodeTree *geom_node_tree) const;

protected:
  friend class CollisionGameResFactory;
  friend CollisionExporter;
  friend dabuildExp_collision::CollisionExporter;

  struct Grid
  {
    Grid();
    ~Grid(); // in order to allow forward declaration
    void reset();
    void buildFRT(CollisionResource *parent, uint8_t behavior_flag, bool frt_build_fast);

    eastl::unique_ptr<StaticSceneRayTracerT<uint16_t>> tracer;
  };

  int getNodeIndexByFaceId(int face_id, uint8_t behavior_filter) const;
  static DAGOR_NOINLINE void addTracesProfileTag(dag::Span<CollisionTrace> traces);
  static DAGOR_NOINLINE void addMeshNodesProfileTag(const ProfileStats &profile_stats);

  Tab<CollisionNode> allNodesList;
  Tab<TMatrix> relGeomNodeTms; // parallel to allNodesList

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


inline int CollisionResource::get_node_tris_count(const CollisionNode &cnode) { return cnode.indices.size() / 3; }

inline void CollisionResource::get_node_tri(const CollisionNode &cnode, int idx, const Point3 *&p0, const Point3 *&p1,
  const Point3 *&p2)
{
  int vIdx = idx * 3;
  p0 = &cnode.vertices[cnode.indices[vIdx]];
  p1 = &cnode.vertices[cnode.indices[vIdx + 1]];
  p2 = &cnode.vertices[cnode.indices[vIdx + 2]];
}
