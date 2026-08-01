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
#include <EASTL/unique_ptr.h> // completes CollisionResourceInstancePtr for createInstance callers
#include <EASTL/bitvector.h>
#include <dag/dag_vector.h>
#include <daBVH/dag_swBLAS_soa4.h> // SoA4 BLAS walkers (iterateLeafRefs) + vert21 unpack for BLAS-resident nodes

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
  dag::Vector<uint32_t> indices;
};

typedef dag::RelocatableFixedVector<vec4f, 8, true, framemem_allocator> all_collres_nodes_t;
typedef dag::RelocatableFixedVector<int, 8, true, framemem_allocator> all_collres_tri_indices_t;
typedef dag::RelocatableFixedVector<tri_ref_t, 8, true, framemem_allocator> all_collres_tri_refs_t;
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

    // Grid MEMBERSHIP markers (not transform types): this node's triangles are flattened into that
    // grid's BLAS (stamped by stampBlasResidentNodes from the grid's own ranges), so the grid walk
    // covers it and the per-node trace loop skips it. When the node's AUTHORITATIVE grid's flag is
    // set (isGridResident) the node is grid-RESIDENT: it has no per-node vert block, verticesOfs is
    // reinterpreted as a vert21-array index (8 B/vert) into that grid and verticesCount = the
    // post-dup vert count. Membership in the other grid alone (e.g. GRID_TRACEABLE on a
    // PHYS_COLLIDABLE node whose collidable grid was gate-vetoed) does NOT change storage: such a
    // node keeps its per-node chunk, and a node appended after load (collres__desc_add) is a member
    // of neither grid. Only MESH ever enters a grid (CONVEX never does). Read verts/faces via
    // iterateNodeVerts / iterateNodeFaces(Verts) / getNodeFaceVerts, which dispatch on residency.
    // Runtime-only, never persisted (the exporter never stamps).
    GRID_TRACEABLE = 16, // in gridForTraceable's BLAS
    GRID_PHYS = 32,      // in gridForCollidable's BLAS
    // Membership in at least one grid. NOT the storage-residency test -- that is
    // CollisionResource::isGridResident (the AUTHORITATIVE grid's bit): a gate-vetoed
    // TRACEABLE|PHYS_COLLIDABLE node is a traceable-grid member whose verts live in its chunk,
    // and its verticesOfs is NOT a vert21 index.
    ANY_GRID_RESIDENT = GRID_TRACEABLE | GRID_PHYS,
    // The node's AUTHORITATIVE grid is gridForCollidable. Stamped by stampBlasResidentNodes from the
    // behavior flags current at grid build: behaviorFlags are mutable afterwards (ECS
    // collres__nodeFlagRules) while verticesOfs stays an index into the grid that stamped it, so
    // storage routing must read this frozen bit, never live behaviorFlags.
    AUTH_GRID_PHYS = 64,
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
  friend struct CollisionResourceInstance;
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

  // Plain vertex count, uint32 so a heavily-dup'd chunk can exceed 65536 post-dup verts (the BLAS
  // path is index-width agnostic). Only meaningful when indicesCount > 0; for non-mesh / dropped
  // meshes it is 0 -- gate on indicesCount, not on this field.
  uint32_t verticesCount = 0;

  // Byte offset of this node's per-node BLAS chunk inside CollisionResource::nodeBlasData
  // (NodeBlasChunkHeader + quad tree + the node's vert21 block); ~0u = no chunk (grid resident,
  // external-raw, or chunks not built). ON the node so it survives list sorts.
  // Runtime-only (rebuilt at load, never persisted). A chunked node's vert21 block lives at the chunk
  // tail; getPackedNodeVerts21 resolves the base through this field.
  uint32_t nodeBlasOfs = ~0u;

  // verticesOfs addressing depends on mode/flag: grid-resident -> a grid vert21 index (8 B/vert);
  // external-raw (exporter) -> an element index into the raw_verts workspace. Owning non-resident
  // nodes reach their vert21 block through nodeBlasOfs (chunk tail), not verticesOfs. indicesOfs
  // indexes the transient build-time index staging only (threaded as a span, never stored on the
  // resource); it is not read at runtime (faces come from the BLAS).
  uint32_t verticesOfs = 0;
  uint32_t indicesOfs = 0;
  uint32_t indicesCount = 0;

  uint32_t nameOfs = 0; // offset into CollisionResource::names; 0 means empty

public:
  int getNodeIdAsInt() const { return (int)geomNodeId; }

  // Vert span [ofs, end) of this node in its resident BLAS grid's vert21 array. Valid only for
  // grid-resident nodes: stampBlasResidentNodes() copies it from the grid NodeRange, so the BLAS
  // walkers use it directly instead of rescanning Grid::blasNodeRanges on every query.
  uint32_t getResidentVertsOfs() const { return verticesOfs; }
  uint32_t getResidentVertsEnd() const { return verticesOfs + verticesCount; }

  // A degenerate/rejected node is dropped to indicesCount == 0 at load: no per-node BLAS chunk, no
  // verts, nothing to trace or hand a shape builder. Consumers must skip it. Named so a new node
  // iterator cannot silently deref a dropped node (nodeBlasOfs == ~0u, wild in release).
  bool hasGeometry() const { return indicesCount != 0; }

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
class CollisionResource;

// Caller-owned resource-local pose; updates must be serialized against traces.
// Legacy APIs use the resource's embedded default instance.
struct CollisionResourceInstance
{
  // Dispatch metadata for getNodeGeometryTm and conservative posed bounds.
  struct PoseMeta
  {
    enum StatusBits : uint8_t
    {
      // Authored gate accepts mirrored placements (|det|), the live gate rejects them
      // (signed det): see setAuthoredNodeTm vs recomputePoseMeta.
      TRACEABLE = 1,      // cleared for singular and live-mirrored poses
      GEOMETRY_BAKED = 2, // singular authored primitive kept its exporter bake
      DISABLED = 4,       // structural hide, applied before filters
    };
    float maxTmScale = 1.f;
    uint8_t flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED; // NodeFlag transform-class bits
    uint8_t status = TRACEABLE;
    bool isTraceable() const { return (status & TRACEABLE) != 0; }
    bool isGeometryBaked() const { return (status & GEOMETRY_BAKED) != 0; }
    bool isDisabled() const { return (status & DISABLED) != 0; }
    void setTraceable(bool on) { status = uint8_t(on ? status | TRACEABLE : status & ~TRACEABLE); }
    void setGeometryBaked(bool on) { status = uint8_t(on ? status | GEOMETRY_BAKED : status & ~GEOMETRY_BAKED); }
    void setDisabled(bool on) { status = uint8_t(on ? status | DISABLED : status & ~DISABLED); }
    // Out-of-range read fallback: bind-default compose state (IDENT class, scale 1, enabled)
    // but NOT traceable, matching isNodeTraceable's safe skip of absent slots.
    static const PoseMeta absent_slot;
  };

  CollisionResourceInstance() = default;
  CollisionResourceInstance(const CollisionResourceInstance &) = delete;
  CollisionResourceInstance &operator=(const CollisionResourceInstance &) = delete;

  const CollisionResource *getResource() const { return res; }
  // Public instance mutators reject the embedded default instance.
  bool isDefault() const;
  int nodeCount() const { return (int)nodeTm.size(); }

  // Read-side queries may outlive a node mutation, so an absent slot reads as enabled here;
  // every other accessor asserts on out-of-range instead (callers own index validity).
  bool isNodeEnabled(int node_index) const
  {
    return (uint32_t)node_index >= poseMeta.size() || !poseMeta[(uint32_t)node_index].isDisabled();
  }
  // Singular and live-mirrored poses are not traceable; an absent slot is not traceable
  // either (dev asserts, release degrades to the safe answer -- the node is skipped).
  bool isNodeTraceable(int node_index) const
  {
    G_ASSERT((uint32_t)node_index < poseMeta.size());
    return (uint32_t)node_index < poseMeta.size() && poseMeta[(uint32_t)node_index].isTraceable();
  }
  const TMatrix &getNodeTmRef(int node_index) const
  {
    G_ASSERT((uint32_t)node_index < nodeTm.size());
    if (DAGOR_LIKELY((uint32_t)node_index < nodeTm.size()))
      return nodeTm[node_index];
    return TMatrix::IDENT;
  }
  const PoseMeta &getPoseMeta(int node_index) const
  {
    G_ASSERT((uint32_t)node_index < poseMeta.size());
    if (DAGOR_LIKELY((uint32_t)node_index < poseMeta.size()))
      return poseMeta[node_index];
    return PoseMeta::absent_slot;
  }
  mat44f getNodeTm(int node_index) const
  {
    mat44f m;
    v_mat44_make_from_43cu_unsafe(m, getNodeTmRef(node_index).array);
    return m;
  }
  // Maps stored geometry to posed resource space without reapplying exporter-baked prim tms.
  mat44f getNodeGeometryTm(int node_index) const;
  // Union of enabled posed node boxes, resource-local; conservative (node boxes carry the
  // trace slab's minimum axis thickness) and grow-only across pose writes until the next
  // full recompute (setNodeEnabled or layout finalize).
  bbox3f getRootBBox() const { return rootBBox; }
  // One-way latch licensing the bind-pose combined-grid BLAS.
  bool isGridResidentPoseAtBind() const { return gridResidentPoseAtBind; }
  // Sticky latch disabling shortcuts whose bounds are valid only at bind.
  bool isPosedSinceBind() const { return posedSinceBind; }

  // Mutators return false when the write is dropped (default, foreign, stale or resource-less
  // instance, or an out-of-range node index) so callers can detect and re-create the binding.
  // Rebuild the resource-local pose from the resource's shared geomNodeId binding.
  bool updateFromGeomNodeTree(const GeomNodeTree &tree, mat44f_cref entity_tm);
  // rootBBox grows conservatively until the next full update.
  bool updateNodeTm(int node_index, mat44f_cref tm);
  // Recomputes rootBBox (O(nodes)) because structural hides can shrink it; content-event cadence, not per-frame.
  bool setNodeEnabled(int node_index, bool enabled);

protected:
  friend class CollisionResource;
  friend struct CollisionResourceUnittest;
  bool validateForUpdate() const;
  void seedPose();
  // Shared write path for caller-owned and default instances.
  bool updateNodeTmImpl(int node_index, mat44f_cref tm);
  void resetNodeToBindPose(int node_index);
  void recomputeRootBBox();
  // Reclassifies the STORED pose nodeTm[node_index] (write it first).
  void recomputePoseMeta(int node_index);

  const CollisionResource *res = nullptr;
  dag::Vector<TMatrix> nodeTm;        // T[i] as 3x4 floats, indexed by CollisionNode::nodeIndex
  dag::Vector<PoseMeta> poseMeta;     // parallel to nodeTm
  bbox3f rootBBox = {};               // union of enabled posed node boxes, resource-local
  vec4f bsphereCenterLocal = {};      // selected tree node in resource space after full update
  bool gridResidentPoseAtBind = true; // see isGridResidentPoseAtBind
  bool posedSinceBind = false;        // see isPosedSinceBind
  bool hasBsphereCenterLocal = false;
};


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
  // Tight bind-pose trace sphere; valid only while bindTraceSphereStamped (w is a radius^2,
  // so 0 is a legal stamped value for an empty resource).
  vec4f vBindTraceSphere = {};
  bool bindTraceSphereStamped = false;
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
  // Runtime-derived in rebuildNodesLL: every mesh-list node is a grid-eligible IDENT mesh node (no
  // convex / non-IDENT) carrying both TRACEABLE and PHYS_COLLIDABLE. When a BLAS exists, the per-node
  // mesh loop in forEachIntersectedNode would skip all of them, so it is bypassed. Not serialized.
  bool allMeshNodesBlasEligible = false;
  dag::Index16 bsphereCenterNode;

  CollisionResource()
  {
    memset(nodeLists, 0xff, sizeof(nodeLists)); // CollisionNode::INVALID_IDX in every slot
    defaultInstance.res = this;
  }
  CollisionResource *deepCopy(void *inplace_mem_ptr = nullptr) const;

  // Current pose used by tree-less APIs.
  const CollisionResourceInstance &getDefaultInstance() const { return defaultInstance; }
  // Null or unbound means the current pose; a foreign instance asserts, logs once in
  // release and falls back to it. Caller-migration surface: game call sites adopt it in
  // follow-up changes.
  const CollisionResourceInstance &instanceOrDefault(const CollisionResourceInstance *instance) const
  {
    if (DAGOR_LIKELY(instance && instance->getResource() == this))
      return *instance;
    return instanceOrDefaultFallback(instance);
  }
  // The resource must outlive each instance.
  CollisionResourceInstancePtr createInstance() const;
  // Rebind externally owned storage and seed it from the current pose.
  void initInstance(CollisionResourceInstance & inst) const;

  dag::Span<CollisionNode> getAllNodes() { return make_span(allNodesList); }
  dag::ConstSpan<CollisionNode> getAllNodes() const { return allNodesList; }

  static CollisionResource *loadResource(IGenLoad & crd, int res_id);
  void load(IGenLoad & cb, int res_id);

  // Add typed collision nodes. Returns nodeIndex. Sets type, modelBBox, boundingSphere, capsule.
  int addSphereNode(const char *name, int16_t phys_mat_id, const BSphere3 &bsphere);
  int addBoxNode(const char *name, int16_t phys_mat_id, const BBox3 &bbox);
  int addCapsuleNode(const char *name, int16_t phys_mat_id, const Point3 &p0, const Point3 &p1, float radius);
  // Add a MESH or CONVEX node fully populated in one call. Mirrors the addSphereNode /
  // addBoxNode / addCapsuleNode "create at once" pattern. Verts/faces are packed straight into a
  // per-node BLAS chunk (the node's owning storage); multiple calls accumulate into a single
  // resource. (Provided for API parity with the BVH-backed CollisionResource so the same
  // unittest source compiles against both implementations.)
  // Default behavior_flags mirrors CollisionNode's member default so omitting the arg preserves
  // standard hole/damage participation; pass an explicit value to opt out.
  // uint32 indices: no vert-count cap. Source nodes >65536 verts are fine (the per-node chunk encoding is
  // uint32-clean; an oversized node goes grid-resident like any other). The uint16 overload below widens
  // and forwards here, kept for legacy 16-bit-index callers (loaded assets, addConvexNode).
  // build_chunk == false defers the per-node BLAS chunk build: the node gets verticesOfs == 0 and the
  // caller must immediately collapseAndOptimize() feeding the same verts/indices as in_raw_*. Valid only
  // as the sole geometry node (createSingleMesh); every other caller keeps the default true.
  int addMeshNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox, const BSphere3 &bsphere,
    dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint32_t> indices,
    uint16_t behavior_flags =
      CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE | CollisionNode::FLAG_ALLOW_HOLE | CollisionNode::FLAG_DAMAGE_REQUIRED,
    uint8_t flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED, bool build_chunk = true);
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

  // Create a single owning MESH-node resource from verts/indices. Wraps addMeshNode + the standard
  // optimize path, so the caller does not touch the per-node BLAS / grid storage internals; node_flags
  // is OR'd into the node's flags (IDENT is implied). node_name is the node's debug name (logerrs only).
  // Returns a heap resource the caller owns.
  static CollisionResource *createSingleMesh(dag::ConstSpan<Point3_vec4> vertices, dag::ConstSpan<uint16_t> indices, const BBox3 &bbox,
    const BSphere3 &bsphere, uint32_t node_flags = 0, const char *node_name = "mesh");
  // uint32-index variant: lets the caller build a resource with more than 65535 vertices.
  static CollisionResource *createSingleMesh(dag::ConstSpan<Point3_vec4> vertices, dag::ConstSpan<uint32_t> indices, const BBox3 &bbox,
    const BSphere3 &bsphere, uint32_t node_flags = 0, const char *node_name = "mesh");

  // raw_verts_out / raw_indices_out (exporter only): when non-null, the final post-collapse raw verts
  // and source-face indices are moved into them and kept full-precision there (the resource never holds
  // raw geometry) so the export pipeline keeps reading/writing full-precision spans (weld, Jolt
  // validation, serialization stay bit-identical). Pass both or neither. When null (runtime), the result
  // is vert21-packed into per-node BLAS chunks (or a grid for BLAS-resident nodes) and the index staging
  // is dropped.
  // in_raw_verts / in_raw_indices (runtime single-mesh build only): when non-null, the staging is read
  // straight from these full-precision spans instead of decoding the per-node BLAS chunks, so a caller
  // that has the raw geometry (createSingleMesh) can skip building a throwaway chunk in addMeshNode and
  // let the collapse tail own the one real build. Node verticesOfs/indicesOfs must index into them.
  void collapseAndOptimize(const char *res_name, bool need_frt = false, bool frt_build_fast = true,
    dag::Vector<Point3_vec4> *raw_verts_out = nullptr, dag::Vector<uint32_t> *raw_indices_out = nullptr,
    dag::ConstSpan<Point3_vec4> in_raw_verts = {}, dag::ConstSpan<uint32_t> in_raw_indices = {});

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
  // By value: node accessors may compute their result rather than return stored members.
  // Do not keep references or pointers to the returned value across calls.
  TMatrix getNodeTm(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? n->tm : TMatrix::IDENT;
  }
  void setNodeTm(int node_id, const TMatrix &new_tm)
  {
    CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    n->tm = new_tm;
    // Keep the default pose mirror in sync; node fields stay authoritative for dispatch.
    // Posing a grid-member node also drops the mirror's one-way bind-grid latch (unread here).
    mat44f t;
    v_mat44_make_from_43cu_unsafe(t, new_tm.array);
    G_VERIFYF(defaultInstance.updateNodeTmImpl(node_id, t), "collision: pose mirror out of sync with the node list");
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
  // Caller-migration surface: adopted when the legacy tree trace API is dropped.
  bool isGeomNodeTreeBound() const { return geomNodeTreeBound; }

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
    return (n && n->hasGeometry()) ? (int)n->verticesCount : 0;
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

  // Three vertices of a node's source face by per-node face index. Works for grid-resident nodes
  // (grid leaf walk) and owning non-resident nodes (per-node chunk vert21 block decode).
  // False if face_idx is out of range or the node has no triangles.
  bool getNodeFaceVerts(int node_id, int face_idx, Point3 &v0, Point3 &v1, Point3 &v2) const;

  // Decode a tri_ref_t back to its source triangle's three verts. BLAS refs (type=1) walk the quad
  // leaf at blasToken and rebuild the sub_tri-selected triangle from vert21 (no side table);
  // non-BLAS refs (type=0) dispatch to getNodeFaceVerts on the encoded srcFace.
  // A tri_ref_t bakes in nodeIndex, which optimize()/rebuildNodesLL re-permute on every (re)build, so a
  // ref is valid only until the next build -- fine for ephemeral damage-model use, not for persistence.
  bool getNodeFaceVertsByRef(tri_ref_t ref, Point3 & v0, Point3 & v1, Point3 & v2) const;

  // ownVerts21 block layout: an 8-byte-aligned self-describing block
  // [float bmin[3]][float invScale[3]][vert21 x verticesCount], living in the node's BLAS chunk
  // tail (resolved via getPackedNodeVerts21). The 24 B header carries the node's quantization frame;
  // decode is
  // v_madd(unpackVert21(p), invScale, bmin) -- the same form as the grid vert21 decode. The 24 B
  // header is a multiple of 8 and the chunk tree is padded to 8 (alignVert21StreamOfs) before the
  // block, so the vert21 stream is 8-aligned -- same invariant as the grid's blasVertsOfs().
  static constexpr uint32_t OWN_VERTS21_HEADER_BYTES = 24;
  // vert21 streams are 8-byte aligned everywhere (grid BLAS via blasVertsOfs, per-node chunk via
  // ownVertsBlockPtr / buildOneNodeBlasChunk): the tree region is padded up to 8 before the vert
  // block so a byte-offset/8 index recovery is always exact. Same rule, one place.
  static constexpr uint32_t alignVert21StreamOfs(uint32_t tree_bytes) { return (tree_bytes + 7u) & ~7u; }
  // Read-only decode view over an owning non-resident node's per-node vert21 block. The pointers are
  // resource-stable (live as long as nodeBlasData is untouched), which is what lets async consumers
  // (SW occluder tasks) reference the stream directly. Caller must ensure the resource is in owning
  // mode (not external-raw) and the node is not grid-resident (mirrors the getBlasGridForResidentNode
  // contract).
  struct PackedVerts21
  {
    const uint8_t *verts21; // node's vert21 stream (8 B/vert, verticesCount entries)
    vec3f invScale;         // per-axis decode scale; w lane is undefined
    vec3f bmin;             // quantization frame origin (node-slice bbox min, stored space); w lane undefined
  };
  // Per-node BLAS chunk prologue (chunk = [header][SoA4 tree][ownVerts21-format block]). `scale` is
  // the EXACT pack scale the block was quantized with (not rcp(block invScale)), so the trace ray
  // transform into the chunk's q-space frame reproduces the build frame bit-for-bit. bmin is not stored
  // here -- the q-space transform reads it from the block (PackedVerts21::bmin / ownVerts21 header),
  // the single copy. The 24 B size is a multiple of 8 so the vert21 stream (past the 8-padded tree and
  // the 24 B block header) stays 8-aligned.
  struct NodeBlasChunkHeader // -V730 (filled field-by-field by the chunk builder)
  {
    float scale[3];
    uint32_t treeBytes;    // SoA4 tree size; the vert block follows, padded up to 8 (alignVert21StreamOfs)
    soa4::RootRef rootRef; // SoA4 tree root, relative to the tree base right after this header
    uint32_t _resv;        // keeps sizeof a multiple of 8 (vert21 stream alignment invariant)
  };

  // Owning-mode vert21 block base: every owning non-resident node carries its block inside its
  // per-node BLAS chunk tail (resident nodes decode from the grid instead).
  const uint8_t *ownVertsBlockPtr(const CollisionNode &node) const
  {
    // Every owning mesh/convex node's vert21 block lives inside its per-node BLAS chunk (combined-grid
    // residents decode from the grid, not here; degenerate nodes are dropped to indicesCount == 0).
    G_ASSERT(node.nodeBlasOfs != ~0u);
    const uint8_t *chunk = nodeBlasData.data() + node.nodeBlasOfs;
    return chunk + sizeof(NodeBlasChunkHeader) + alignVert21StreamOfs(((const NodeBlasChunkHeader *)chunk)->treeBytes);
  }

  PackedVerts21 getPackedNodeVerts21(const CollisionNode &node) const
  {
    const uint8_t *block = ownVertsBlockPtr(node);
    PackedVerts21 r;
    r.bmin = v_ldu_p3((const float *)block);
    r.invScale = v_ldu_p3((const float *)block + 3);
    r.verts21 = block + OWN_VERTS21_HEADER_BYTES;
    return r;
  }

  bool hasNodeBlas(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n && n->nodeBlasOfs != ~0u;
  }

  // SW occluder feed: the RenderBlasSOA4 chunk params (tree base, root ref, vert21 stream byte
  // offset) plus the node's decode frame (bmin/invScale) in a single chunk resolution.
  // vertOffset is 8-aligned (tree padded, then the 24 B block header), so the walker's
  // (apexByteOfs - vertOffset)/STRIDE recovers node-local vert indices. Node must have a chunk
  // (nodeBlasOfs != ~0u). Resolving the frame separately via getPackedNodeVerts21 would re-cast the
  // header and re-run alignVert21StreamOfs a second time per node, every occluder build.
  struct NodeOccluderBlas
  {
    const uint8_t *blasData;
    soa4::RootRef rootRef;
    uint32_t vertOffset;
    vec3f invScale;
    vec3f bmin; // w lanes undefined
  };
  NodeOccluderBlas getNodeOccluderBlas(const CollisionNode &node) const
  {
    const uint8_t *chunk = nodeBlasData.data() + node.nodeBlasOfs;
    const NodeBlasChunkHeader *hdr = (const NodeBlasChunkHeader *)chunk;
    const uint32_t alignedTree = alignVert21StreamOfs(hdr->treeBytes);
    const uint8_t *block = chunk + sizeof(NodeBlasChunkHeader) + alignedTree;
    NodeOccluderBlas r;
    r.blasData = chunk + sizeof(NodeBlasChunkHeader);
    r.rootRef = hdr->rootRef;
    r.vertOffset = alignedTree + OWN_VERTS21_HEADER_BYTES;
    r.bmin = v_ldu_p3((const float *)block);
    r.invScale = v_ldu_p3((const float *)block + 3);
    return r;
  }

  template <class CB> // void(int face_idx, uint32_t i0, uint32_t i1, uint32_t i2)
  void iterateNodeFaces(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    if (isGridResident(*n))
    {
      walkBlasResidentNodeLeavesForFaces(*n, cb);
      return;
    }
    walkNodeChunkLeavesForFaces(*n, cb); // owning mode: faces live in the per-node chunk tree
  }

  template <class CB> // void(int vert_idx, vec4f v)
  void iterateNodeVerts(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || !n->hasGeometry())
      return;
    const uint32_t vertCount = (uint32_t)n->verticesCount;
    if (isGridResident(*n))
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
      // owning mode: verts live in the node's per-node BLAS chunk (vert21 block)
      const PackedVerts21 p = getPackedNodeVerts21(*n);
      for (uint32_t i = 0; i < vertCount; ++i)
        cb((int)i, v_madd(RayData::unpackVert21(p.verts21 + i * 8u), p.invScale, p.bmin));
    }
  }

  template <class CB> // void(int face_idx, vec4f v0, vec4f v1, vec4f v2)
  void iterateNodeFacesVerts(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || !n->hasGeometry())
      return; // degenerate-dropped node: no per-node block to decode (mirrors iterateNodeVerts)
    if (isGridResident(*n))
    {
      // Resident nodes are always MESH and keep no index list. Walk leaves and decode each
      // sub-triangle's verts from the leaf encoding.
      const Grid &g = getBlasGridForResidentNode(*n);
      const uint8_t *vbase = g.blasData.data() + g.blasVertsOfs() + (size_t)n->verticesOfs * BVH_BLAS_VERT21_STRIDE;
      const vec3f invScale = g.blasInvScale;
      const vec3f bmin = g.blasBBox.bmin;
      auto unq = [vbase, invScale, bmin](uint32_t li) -> vec3f {
        return v_madd(RayData::unpackVert21(vbase + li * BVH_BLAS_VERT21_STRIDE), invScale, bmin);
      };
      walkBlasResidentNodeLeavesForFaces(*n,
        [&](int fi, uint32_t i0, uint32_t i1, uint32_t i2) { cb(fi, unq(i0), unq(i1), unq(i2)); });
      return;
    }
    // owning mode: faces + verts both live in the per-node chunk (tree gives the face list, the
    // ownVerts21 block the positions)
    const PackedVerts21 p = getPackedNodeVerts21(*n);
    auto unq = [&p](uint32_t vi) -> vec3f { return v_madd(RayData::unpackVert21(p.verts21 + vi * 8u), p.invScale, p.bmin); };
    walkNodeChunkLeavesForFaces(*n, [&](int fi, uint32_t i0, uint32_t i1, uint32_t i2) { cb(fi, unq(i0), unq(i1), unq(i2)); });
  }

  // Walks a BLAS-resident MESH node's leaves, calling cb(face_idx, local_i0, local_i1, local_i2)
  // per sub-triangle in DFS order. Local indices are node-local [0, verticesCount) (the source-face
  // index convention). Used by iterateNodeFaces / iterateNodeFacesVerts on BLAS-resident nodes
  // (always MESH). Each leaf's owning node is identified by its first vert21 index vs the node's
  // NodeRange. Low-frequency / one-shot materialisation (Jolt mesh build, AssetViewer, long-form
  // intersection).
  template <class CB> // void(int face_idx, uint32_t i0, uint32_t i1, uint32_t i2)
  void walkBlasResidentNodeLeavesForFaces(const CollisionNode &node, CB cb) const
  {
    const Grid &g = getBlasGridForResidentNode(node);
    if (g.blasData.empty())
      return;
    const uint8_t *bData = g.blasData.data();
    const uint32_t vertsOfs = g.blasVertsOfs();
    // stampBlasResidentNodes() copied this node's NodeRange (from this same grid) into
    // verticesOfs/Count, so use them directly instead of rescanning blasNodeRanges per call.
    const uint32_t nodeVOfs = node.getResidentVertsOfs();
    const uint32_t nodeVEnd = node.getResidentVertsEnd();
    int fi = 0;
    // Spatial prune to this node's bbox (box-space) is an optimisation -- skips sibling-node subtrees
    // of the combined BLAS. The node is IDENT, so modelBBox is in the BLAS quant frame and contains
    // every leaf of this node (a few-unit pad absorbs quantization rounding), so the prune never drops
    // an owned leaf. The v0Idx NodeRange check below is the authoritative ownership filter.
    const vec3f nodeBoxMin = v_ldu(&node.modelBBox.lim[0].x);
    const vec3f nodeBoxMax = v_ldu_p3(&node.modelBBox.lim[1].x);
    const vec3f boxPad = v_splats(4.f);
    const vec3f nodeBoxMinQ = v_sub(v_madd(nodeBoxMin, g.blasScale, g.blasOfs), boxPad);
    const vec3f nodeBoxMaxQ = v_add(v_madd(nodeBoxMax, g.blasScale, g.blasOfs), boxPad);
    // iterateLeafRefs visits each overlapping leaf once and decodes no vertices -- this path only
    // needs the leaf's decoded fields, then emits the leaf's sub-triangle index triples via the
    // shared expandQuadLeafTris (the one authority for the W1/W2/W3 sub-tri winding).
    soa4::iterateLeafRefs(
      bData, g.blasRootRef,
      [nodeBoxMinQ, nodeBoxMaxQ](vec3f bmn, vec3f bmx) {
        return (bool)v_check_xyz_all_true(v_and(v_cmp_ge(nodeBoxMaxQ, bmn), v_cmp_ge(bmx, nodeBoxMinQ)));
      },
      // -V657: always returns false by design -- emits every owned leaf's sub-tris with no
      // early-out; false means "continue iterating" per the iterateLeafRefs contract.
      [&](vec3f, vec3f, soa4::LeafRef, const soa4::LeafLoc &l) -> bool { //-V657
        const QuadLeafFields f = soa4::leafFields(bData, l);
        const uint32_t v0Idx = ((uint32_t)l.bodyOfs + f.relBaseBytes - vertsOfs) / BVH_BLAS_VERT21_STRIDE;
        if (v0Idx < nodeVOfs || v0Idx >= nodeVEnd)
          return false; // sibling-node leaf
        // Quad A and quad B share a source node (pairing is node-constrained), so quad A's v0 owning
        // this node implies quad B's verts are in the same node range. Indices stay uint32: a resident
        // node may span over 65536 node-local verts (verticesCount is uint32).
        const uint32_t baseLocal = v0Idx - nodeVOfs;
        expandQuadLeafTris(f, baseLocal, [&](uint32_t i0, uint32_t i1, uint32_t i2) { cb(fi++, i0, i1, i2); });
        return false; // continue walking
      });
  }

  // Owning (non-resident) twin of walkBlasResidentNodeLeavesForFaces: walks the node's OWN per-node
  // BLAS chunk tree (single node, so no NodeRange filter and no sibling prune -- every leaf is this
  // node's). Emits cb(face_idx, i0, i1, i2) per sub-triangle in DFS order; local indices are
  // [0, verticesCount) over the chunk's ownVerts21 block (getPackedNodeVerts21). The vert21 stream sits
  // past the tree and the 24 B block header, so the leaf's vertBytesOfs rebases against
  // alignVert21StreamOfs(treeBytes) + OWN_VERTS21_HEADER_BYTES.
  template <class CB> // void(int face_idx, uint32_t i0, uint32_t i1, uint32_t i2)
  void walkNodeChunkLeavesForFaces(const CollisionNode &node, CB cb) const
  {
    if (node.nodeBlasOfs == ~0u)
      return; // degenerate-dropped node: no chunk
    const uint8_t *chunk = nodeBlasData.data() + node.nodeBlasOfs;
    const NodeBlasChunkHeader *hdr = (const NodeBlasChunkHeader *)chunk;
    const uint8_t *tree = chunk + sizeof(NodeBlasChunkHeader);
    const uint32_t vertsRel = alignVert21StreamOfs(hdr->treeBytes) + OWN_VERTS21_HEADER_BYTES;
    int fi = 0;
    // Single node's tree: every leaf is this node's, so no NodeRange filter and an always-true node
    // test. iterateLeafRefs visits each leaf once; decode + emit its sub-tris via the shared
    // double-quad authority (leafFields / expandQuadLeafTris).
    soa4::iterateLeafRefs(
      tree, hdr->rootRef, [](vec3f, vec3f) { return true; },
      [&](vec3f, vec3f, soa4::LeafRef, const soa4::LeafLoc &l) -> bool {
        const QuadLeafFields f = soa4::leafFields(tree, l);
        const uint32_t baseLocal = ((uint32_t)l.bodyOfs + f.relBaseBytes - vertsRel) / BVH_BLAS_VERT21_STRIDE;
        expandQuadLeafTris(f, baseLocal, [&](uint32_t i0, uint32_t i1, uint32_t i2) { cb(fi++, i0, i1, i2); });
        return false;
      });
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
  // Stored-space bounds for every node type, with degenerate axes conservatively inflated.
  bbox3f getNodeGeometryBBox(const CollisionNode &node) const;
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
  // Appends a rebuilt per-node BLAS chunk and orphans the node's old one; the caller must run
  // compactNodeBlasData() once after a batch of bakes to reclaim the orphans. Prefer bakeMirroredNodes(),
  // which owns that lifecycle.
  void bakeNodeTransform(int node_id);

  // Bake every mirrored (det<0) mesh/convex node and reclaim the orphaned chunks in one pass.
  void bakeMirroredNodes();

  // Rebuild nodeBlasData keeping only the live per-node chunks, restamping each node's nodeBlasOfs.
  // Run once after a batch of bakeNodeTransform calls to reclaim the orphans (O(total), vs O(nodes^2)
  // per-node compaction).
  void compactNodeBlasData();

  float getNodeMaxTmScale(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? n->cachedMaxTmScale : 1.f;
  }

  // Unsafe accessors: no bounds check in release, use G_ASSERT for validation.
  // Same by-value contract as the checked forms.
  TMatrix getNodeTmUnsafe(int node_id) const
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
  Capsule getNodeCapsuleUnsafe(int node_id) const
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

  CollisionNode &createNode()
  {
    // the default pose arrays stay parallel to allNodesList: every append creates a bind slot
    defaultInstance.nodeTm.push_back(TMatrix::IDENT);
    authoredNodeTm.push_back(TMatrix::IDENT);
    defaultInstance.poseMeta.push_back();
    return allNodesList.push_back();
  }

  bool checkGridAvailable(uint8_t behavior_filter) const { return hasBlas(behavior_filter); }
  // Caller-owned instances are not counted.
  int getMemoryUsed() const;
  bool getGridSize(uint8_t behavior_filter, IPoint3 & width, Point3 & leaf_size) const;
  int getTrianglesCount(uint8_t behavior_filter) const;
  void setBsphereCenterNode(int ni) { bsphereCenterNode = dag::Index16(ni); }
  vec4f getWorldBoundingSphere(const mat44f &tm, const GeomNodeTree *geom_node_tree) const;
  Point3 getWorldBoundingSphere(const TMatrix &tm, const GeomNodeTree *geom_node_tree) const;
  // raw_verts / raw_indices (exporter only): the full-precision geometry the export pipeline owns. The
  // gate then validates the exact floats + face list that serialize. Empty (runtime) -> verts decode
  // from vert21 and faces from the BLAS. Pass both or neither.
  bool validateVerticesForJolt(const char *res_name, auto &&on_degenerate, dag::ConstSpan<Point3_vec4> raw_verts = {},
    dag::ConstSpan<uint32_t> raw_indices = {});
  bool validateVerticesForJolt(const char *res_name, dag::ConstSpan<Point3_vec4> raw_verts = {},
    dag::ConstSpan<uint32_t> raw_indices = {});
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
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalCullCCW(const Point3_vec4 *verticesBase, const uint32_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm);
  template <bool check_bounding, uint32_t mainBatchSize = 8>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalCullCCW_AVX256(const Point3_vec4 *verticesBase, const uint32_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, vec4f *v_out_norm);
  // verts_base + node.verticesOfs must point at `node`'s vert block. The caller
  // (resolveNodeVertsForCall) decodes the node's verts -- from the grid (BLAS-resident) or the
  // per-node chunk block (owning non-resident) -- into a temp buffer and passes (buffer.data(),
  // node-copy-with-ofs0).
  DAGOR_NOINLINE bool traceCapsuleMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint32_t *idx_base,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, float &radius, vec4f &v_out_norm,
    vec4f &v_out_pos) const;

  template <bool check_bounding>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalAllHits(const Point3_vec4 *verticesBase, const uint32_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull,
    all_collres_nodes_t &ret_array, all_collres_tri_indices_t &tri_indices);
  template <bool check_bounding, uint32_t mainBatchSize = 8>
  DAGOR_NOINLINE static bool traceRayMeshNodeLocalAllHits_AVX256(const Point3_vec4 *verticesBase, const uint32_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, bool calc_normal, bool force_no_cull,
    all_collres_nodes_t &ret_array, all_collres_tri_indices_t &tri_indices);

  template <bool check_bounding>
  DAGOR_NOINLINE static bool rayHitMeshNodeLocalCullCCW(const Point3_vec4 *verticesBase, const uint32_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t);
  template <bool check_bounding, uint32_t mainBatchSize = 8>
  DAGOR_NOINLINE static bool rayHitMeshNodeLocalCullCCW_AVX256(const Point3_vec4 *verticesBase, const uint32_t *indicesBase,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t);

  // Chunk twins of the scalar capsule helpers: same per-triangle math, triangles supplied by the
  // node's per-node quad-BLAS filtered with the swept capsule's q-space box.
  DAGOR_NOINLINE bool traceCapsuleNodeChunkCullCCW(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
    float &in_out_t, float &radius, vec4f &v_out_norm, vec4f &v_out_pos) const;
  DAGOR_NOINLINE bool capsuleHitNodeChunkCullCCW(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
    float in_t, float radius) const;
  // Closest-hit twin of the chunk ray descent: descends the node's quad-BLAS instead of materialising
  // and scanning every face. Same q-space ray transform, CCW cull and unnormalized normal as the scalar
  // traceRayMeshNodeLocalCullCCW, so the public per-node ray helper keeps its contract.
  DAGOR_NOINLINE bool traceRayNodeChunkCullCCW(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
    float &in_out_t, vec4f *v_out_norm) const;
  // All-hits twin of the chunk ray descent: collects every triangle the node-local ray crosses within
  // [0, in_t] from the node's quad-BLAS (one walk, no t-pruning), with the same per-tri test, cull
  // rule and unnormalized normal as the scalar traceRayMeshNodeLocalAllHits so the hit set matches.
  // Each hit carries a BLAS tri_ref (opaque soa4::LeafRef leaf token + sub-tri), the identity the
  // closest-hit chunk arm emits; getNodeFaceVertsByRef decodes it.
  DAGOR_NOINLINE bool traceAllHitsNodeChunk(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t,
    bool calc_normal, bool force_no_cull, all_collres_nodes_t &ret_array, all_collres_tri_refs_t &ret_refs) const;

  // Same verts_base / idx_base convention as traceCapsuleMeshNodeLocalCullCCW above.
  DAGOR_NOINLINE bool capsuleHitMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint32_t *idx_base,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, float radius) const;

  // For BLAS-resident nodes: decode verts (and, for MESH, indices) from the active grid into the
  // caller's framemem scratches, build a CollisionNode copy with verticesOfs/indicesOfs rebased to 0,
  // and point out_* at the scratches + copy. For owning non-resident nodes: decode verts from the
  // node's per-node chunk block into vertsScratch and materialise the face list from the chunk leaf
  // walk into idxScratch (no kept index list), node copy with verticesOfs/indicesOfs rebased. Returned
  // pointers are valid until the scratches go out of
  // scope or are resized. The rebased node copy lets the existing pointer+offset idioms
  // ((base + node.verticesOfs) / (base + node.indicesOfs)) work unchanged.
  void resolveNodeVertsForCall(const CollisionNode &node, dag::Vector<Point3_vec4, framemem_allocator> &vertsScratch,
    dag::Vector<uint32_t, framemem_allocator> &idxScratch, CollisionNode &node_copy, const Point3_vec4 *&out_verts_base,
    const uint32_t *&out_idx_base, const CollisionNode *&out_node) const;

  __forceinline mat44f getMeshNodeTmInline(const CollisionNode *node, mat44f_cref instance_tm, vec3f instance_woffset,
    const GeomNodeTree *geom_node_tree) const;

  // Cold arm of instanceOrDefault: null/unbound normalizes silently, foreign asserts + logs.
  const CollisionResourceInstance &instanceOrDefaultFallback(const CollisionResourceInstance *instance) const;
  // Foreign or stale instances fall back to the current pose.
  const CollisionResourceInstance *resolveInstanceForTrace(const CollisionResourceInstance &instance) const;
  // A null tree selects the current pose.
  const CollisionResourceInstance *dispatchPose(const GeomNodeTree *geom_node_tree) const
  {
    return geom_node_tree ? nullptr : &defaultInstance;
  }
  // Class-aware inverse of the node's GEOMETRY frame (inv of getNodeGeometryTm), not of the
  // raw posed matrix; IDENT preserves exporter-baked primitive geometry.
  TMatrix invNodeTm(int node_index) const;
  static TMatrix invInstNodeTm(const CollisionResourceInstance &instance, int node_index);
  // Erase a node and every parallel per-node slot in lockstep.
  void eraseNodeAt(int node_index);

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
    void buildBLAS(CollisionResource *parent, dag::ConstSpan<Point3_vec4> raw_verts, dag::ConstSpan<uint32_t> raw_indices,
      uint8_t behavior_flag, bool two_sided);

    // BLAS storage. Empty when buildBLAS gates eliminate the build (small resource, non-IDENT mesh,
    // SOLID-behavior, or a failed SoA4 conversion). The CPU tree is the SoA4 layout
    // (daBVH/dag_swBLAS_soa4.h); the stackless GPU layout is re-emitted on demand at the daSWRT
    // feeder seam. Layout [SoA4 tree bytes][pad to 8][vert21 array]; vert base = blasVertsOfs().
    // The vert21 region is byte-identical to the stackless build's, so all vertex-index attribution
    // (blasNodeRanges, MOC index recovery) is layout-neutral.
    dag::Vector<uint8_t> blasData;      // [SoA4 tree bytes][pad][vert21 array]
    bbox3f blasBBox = {};               // resource-local bbox over all triangles in the BLAS
    vec3f blasScale = {}, blasOfs = {}; // quantization frame (scale = 65535/extent, ofs = -bmin*scale)
    vec3f blasInvScale = {};            // cached 1/blasScale (per-axis), set in buildBLAS; used by all vert21 decode paths
    uint32_t blasTreeBytes = 0;         // byte size of the SoA4 tree portion (tree-walk bound)
    soa4::RootRef blasRootRef;          // SoA4 tree root (invalid when blasData is empty)
    // Byte offset of the vert21 stream in blasData. Padded to 8, but no longer required for
    // correctness: the MOC walkers derive vertex indices relative to this stream base (passed as
    // the vertOffset argument), so the index divide is exact at any tree size. Kept only as a potential
    // perf aid -- it keeps every vert21 (8 B) load 8-aligned.
    uint32_t blasVertsOfs() const { return alignVert21StreamOfs(blasTreeBytes); }

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
      uint32_t facesCount;  // emitted (post degenerate-drop) face count -- stamps node.indicesCount = facesCount*3
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

  // Pick the grid a grid-resident node's verticesOfs indexes into. Must mirror the exporter's
  // snap-box choice (exp_collision.cpp vert21 weld): PHYS_COLLIDABLE feeds Jolt, so its grid is
  // authoritative for a node carrying that bit -- the exporter welds such a node's verts onto the
  // COLLIDABLE grid's vert21 cell centers, and only a decode through that grid's quantization frame
  // reproduces them exactly. When the trace and collidable node sets differ the two boxes differ, and
  // decoding a TRACEABLE|PHYS_COLLIDABLE node through gridForTraceable would hand Jolt verts rounded
  // off their snapped cell centers (re-creating the degenerate slivers the snap removes).
  //  - PHYS_COLLIDABLE bit set at grid build: gridForCollidable, unless REUSE_TRACE_FRT (shared grid).
  //  - trace-only node: gridForTraceable.
  // Reads the stamped AUTH_GRID_PHYS bit, not live behaviorFlags: those are mutable after load (ECS
  // collres__nodeFlagRules) and a post-stamp change must not re-route the decode of a verticesOfs
  // stamped from the other grid. stampBlasResidentNodes stamps bit and verticesOfs together.
  // Caller must ensure isGridResident(node) first (else the returned grid may have empty blasData).
  const Grid &getBlasGridForResidentNode(const CollisionNode &node) const
  {
    return (node.flags & CollisionNode::AUTH_GRID_PHYS) ? gridForCollidable : gridForTraceable;
  }

  // The membership flag of the node's AUTHORITATIVE grid -- same stamped bit as
  // getBlasGridForResidentNode, keeping flag and grid in sync.
  uint8_t residentGridFlag(const CollisionNode &node) const
  {
    return (node.flags & CollisionNode::AUTH_GRID_PHYS) ? CollisionNode::GRID_PHYS : CollisionNode::GRID_TRACEABLE;
  }
  // Storage residency: the node's AUTHORITATIVE grid absorbed it, so it has no per-node vert block and
  // verticesOfs is a vert21 index into getBlasGridForResidentNode(node). ANY_GRID_RESIDENT is NOT this
  // test: a node can be a member of the other grid only and still store its verts in its chunk.
  bool isGridResident(const CollisionNode &node) const { return (node.flags & residentGridFlag(node)) != 0; }
  // Membership in ANY built grid. This, not residency, is the pose-latch test: a gate-vetoed
  // dual-behavior node keeps chunk storage yet its triangles still sit in the other, walked grid.
  bool isAnyGridMember(const CollisionNode &node) const { return (node.flags & CollisionNode::ANY_GRID_RESIDENT) != 0; }

protected:
  friend class CollisionGameResFactory;
  friend CollisionExporter;
  friend dabuildExp_collision::CollisionExporter;
  friend struct CollisionResourceUnittest;
  friend struct CollisionResourceInstance;

  // Shared transform classifier for authored and live poses.
  static uint8_t classifyNodeTmFlags(mat44f_cref tm, float &out_max_scale);
  // Authored mirrored placements remain traceable; singular placements do not.
  // Also invalidates the cached bind trace sphere until the next layout finalize.
  void setAuthoredNodeTm(int node_index, mat44f_cref tm, uint8_t class_flags, float max_scale);
  // Runtime-only conservative scale for transforms under-bounded by column lengths.
  void stampConservativePoseScale(int node_index);

  // Immutable base for relative motion of exporter-baked primitives.
  dag::Vector<TMatrix> authoredNodeTm;
  bool geomNodeTreeBound = false; // initializeWithGeomNodeTree ran at least once

  CollisionResourceInstance defaultInstance;

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
  // The resource holds NO source-face index list (at build or runtime): every node enumerates faces
  // from its BLAS (per-node chunk tree, or the grid for residents). The build pipeline (load /
  // collapseAndOptimize / buildBLAS / buildNodeBlasChunks) threads the transient index staging as an
  // explicit span; the exporter owns it externally and passes it along.
  // Per-node BLAS chunks, back to back: [NodeBlasChunkHeader][quad tree][the node's vert21 block].
  // Built at load/collapse (runtime owning mode only) for every NON-grid-resident mesh/convex node
  // so leaf hits can descend a BVH instead of brute-forcing the node's triangle list; the node's
  // vert21 block lives HERE (single storage, no separate ownVerts21 container; zero requantization:
  // the tree is built in the block's own q-space). node.nodeBlasOfs = chunk byte offset.
  // Runtime-only, never persisted; rebuilt like the grids.
  dag::Vector<uint8_t> nodeBlasData;
  // Bumped whenever the per-node chunks are re-chunked or re-packed (compactNodeBlasData / bake),
  // which moves leaf bytes around. Stamped (low tri_ref::NODE_BLAS_GEN_BITS) into every per-node BLAS
  // ref so getNodeFaceVertsByRef can reject a ref minted before such a rebuild instead of decoding
  // stale-but-in-bounds geometry. Wraps at 256; refs are ephemeral (valid only until the next build).
  uint8_t nodeBlasBuildId = 0;
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
  // raw_verts_out / raw_indices_out (exporter only): when non-null, the loaded raw verts and
  // source-face indices are placed there and kept full-precision (no packing; the resource never holds
  // raw geometry) so the export pipeline reads full-precision spans. Pass both or neither. When null
  // (runtime), verts are vert21-packed into per-node BLAS chunks (or a grid for resident nodes) at the
  // end of the load and the index staging is dropped.
  void loadLegacyRawFormat(IGenLoad & cb, int res_id, int (*resolve_phmat)(const char *) = nullptr,
    dag::Vector<Point3_vec4> *raw_verts_out = nullptr, dag::Vector<uint32_t> *raw_indices_out = nullptr);

  // Pack one node's raw vert slice as an ownVerts21 block (header + vert21) written at `block`
  // (OWN_VERTS21_HEADER_BYTES + count * 8 bytes) and return the EXACT pack scale (so the caller writes
  // it into the chunk header for the bit-exact q-space trace transform). The quantization frame is the
  // slice's own bbox (floored at build_bvh::blas_size_eps per axis), so the decode error is bounded by
  // node extent / 2^21. Per node -- NOT a resource-level box: with a resource frame, a small Jolt-fed
  // node inside a large resource would quantize on a grid orders of magnitude coarser than the
  // per-node-referenced-bounds grid validateVerticesForJolt cleared at export, merging verts that
  // Jolt's own quantization then rejects as degenerate.
  static vec3f packOwnVerts21Block(uint8_t * block, const Point3_vec4 *verts, uint32_t count);
  // Shared tail of load / loadLegacyRawFormat / collapseAndOptimize for owning resources: stamp the
  // GRID_TRACEABLE/GRID_PHYS membership flags and the resident reinterpretation from the built grids
  // (empty grids stamp nothing). Verts are not packed here -- buildNodeBlasChunks already packed every
  // non-resident node's vert21 block into nodeBlasData, and residents live in the grid.
  void stampBlasResidentNodes();
  // Authoritative-grid choice from CURRENT behavior flags. Valid only while those still match the
  // built grids -- i.e. at grid build/stamp time (buildNodeBlasChunks runs before the AUTH_GRID_PHYS
  // stamp). Runtime readers use the stamped bit (getBlasGridForResidentNode) instead.
  bool nodeAuthGridIsPhys(const CollisionNode &n) const
  {
    return (n.behaviorFlags & CollisionNode::PHYS_COLLIDABLE) && !(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT);
  }
  // Build per-node BLAS chunks for big NON-grid-resident mesh/convex nodes from the raw staging verts
  // + indices. Must run after BOTH buildBLAS calls (residency is predicted from blasNodeRanges) and
  // BEFORE stampBlasResidentNodes (chunked nodes' blocks are packed here).
  void buildNodeBlasChunks(dag::ConstSpan<Point3_vec4> staging, dag::ConstSpan<uint32_t> staging_indices);
  // Build one node's BLAS chunk from raw verts + indices (read-only; the reordered connectivity is
  // baked into the chunk tree, not written back). Shared by buildNodeBlasChunks, addMeshNode and
  // bakeNodeTransform. False only if the node is degenerate (no buildable quad prims); the post-dup
  // vert count is not capped (indices are uint32, so the over-spread dup may push it past 65536).
  bool buildOneNodeBlasChunk(CollisionNode & node, const Point3_vec4 *node_verts, unsigned node_vert_count, const uint32_t *node_idx,
    unsigned node_idx_count, dag::Vector<vec4f> &node_verts_src, dag::Vector<vec4f> &node_verts_opt,
    dag::Vector<Point3_vec4> &pack_src, dag::Vector<vec4f> &q_verts, dag::Vector<uint8_t> &stk_tmp, dag::Vector<uint8_t> &soa_out);

end_dclass_decl();
