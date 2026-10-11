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
#include <math/dag_check_nan.h>
#include <scene/dag_physMatIdDecl.h> // PHYSMAT_INVALID (node material-set accessors)
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <generic/dag_carray.h>
#include <generic/dag_relocatableFixedVector.h>
#include <generic/dag_functionRef.h>
#include <memory/dag_framemem.h>
#include <math/dag_e3dColor.h>
#include <EASTL/unique_ptr.h> // completes CollisionResourceInstancePtr for createInstance callers
#include <dag/dag_vector.h>
#include <daBVH/dag_swBLAS_soa4.h> // SoA4 BLAS walkers (iterateLeafRefs) + vert21 unpack for the chunk blocks

class GeomNodeTree;
class Bitarray;

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

struct CollResChunkHit
{
  vec4f normT; // xyz: unnormalized normal, w: node-local t
  tri_ref_t triRef;
};
typedef dag::RelocatableFixedVector<CollResChunkHit, 8, true, framemem_allocator> all_collres_hits_t;
// heap overflow: grown while framemem scratch lives
typedef dag::RelocatableFixedVector<int, 32, /*bEnableOverflow*/ true> CollResHitNodesType;
typedef dag::RelocatableFixedVector<uint16_t, 63, /*bEnableOverflow*/ true, MidmemAlloc, uint16_t> CollResNodeIndicesType; // 128 bytes
// Candidate node ids out of an all-nodes TLAS walk (segment collectors, tlasBoxCandidates); unordered.
typedef dag::RelocatableFixedVector<uint16_t, 32, /*bEnableOverflow*/ true, framemem_allocator> CollResTlasCandidates;

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
  // Bit 4: the TRACEABLE node set serves the PHYS_COLLIDABLE filter too. Originally named for
  // FRT-grid reuse; same bit value, same on-disk semantics.
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
  // Bit 8: every traced mesh-list node sits at identity pose with a box that nearly fills the root
  // box, so the single-ray walk leaves the root box and node sphere rejects to each node's own box
  // test (same answer). The builder derives it for the export; a stream that carries it clear
  // derives it at load.
  COLLISION_RES_FLAG_NODES_COVER_ROOT_BOX = 1 << 8,
};

struct CollisionNode
{
  // Sentinel for the node index words (no node).
  static constexpr uint16_t INVALID_IDX = 0xffff;

  // Persisted authored transform class; runtime pose metadata lives in CollisionResourceInstance.
  enum NodeFlag : uint8_t
  {
    NONE = 0,

    // TransformType
    IDENT = 1,           // Identity tm with zero offset
    TRANSLATE = 2,       // Identity tm with offset
    ORTHONORMALIZED = 4, // Unscaled
    ORTHOUNIFORM = 8,    // With uniform scale


    // The persisted CULL_BOTH markers say this node traces two-sided in the TRACE-CORE ray walks
    // (all-hits, closest, any-hit) at every size; the per-node helpers (traceRayMeshNodeLocal*)
    // and the capsule arms keep CullCCW as before. Stamped by the builder's landing (the v0/v1
    // readers mask disk flags down to the transform class first); the stream persists it.
    TRACE_TWO_SIDED = 16,
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

  // AUTHORED flags: build output, immutable. The live per-entity value is
  // CollisionResourceInstance::PoseMeta::behaviorFlags (see getNodeBehaviorFlags).
  uint16_t behaviorFlags = TRACEABLE | PHYS_COLLIDABLE | FLAG_ALLOW_HOLE | FLAG_DAMAGE_REQUIRED;
  eastl::underlying_type_t<NodeFlag> flags = NodeFlag::NONE;
  CollisionResourceNodeType type = COLLISION_NODE_TYPE_MESH;
  dag::Index16 geomNodeId;
  // SIGN-ENCODED material set: >= 0 one inline id, -1 none, <= -2 a physMatPool slice.
  // Never read or write raw -- go through CollisionResource's material-set accessors.
  int16_t physMatId = -1;

protected:
  // Node-local bounds; public primitive accessors preserve their historical composed frames.
  BBox3 modelBBox;
  float radiusAroundBoxCenter = 0.f; // < 0 is the exporter's zero-vert marker for a primitive
  friend class CollisionResource;
  friend struct CollisionResourceInstance;
  friend class CollisionResourceBVH;
  friend class CollisionGeometryFeeder;
  friend class TestMeshNodeMeshNodesIntersectionAlgo;
  friend class TestMeshNodeBoxNodesIntersectionAlgo;
  friend class TestMeshNodeSphereNodesIntersectionAlgo;
  friend class TestBoxNodeBoxNodesIntersectionAlgo;
  friend struct CollisionResourceUnittest;
  friend struct CollisionResourceBuilder;
  friend struct CollResJoltChunk; // reads the chunk binding (nodeBlasOfs, counts) for the physJolt view

public:
  uint16_t nodeIndex = 0; // In allNodesList.
  // Index into allNodesList of the smallest node whose box holds this one (the builder's sort);
  // 0xffff = not contained by any other node.
  uint16_t insideOfNode = 0xffff;

protected:
  // capsuleIndex valid only for COLLISION_NODE_TYPE_CAPSULE; planesOfs valid only for
  // COLLISION_NODE_TYPE_CONVEX. The two types are mutually exclusive, so they share storage.
  union
  {
    uint16_t capsuleIndex = 0; // index into CollisionResource::Data::capsules
    uint16_t planesOfs;        // start in CollisionResource::Data::convexPlanes
  };
  uint16_t planesCount = 0; // number of planes for this node (0 unless CONVEX)
  // Where this node's box lives in the all-nodes TLAS: (owning tree-node index << 2) | child lane.
  // SoA4 keeps a child's box inside its PARENT, so a leaf has no node of its own. ~0u = no leaf
  // (POINTS, a geometry-less mesh, or no TLAS built). Stamped by buildAllNodesTLAS so a single-node
  // pose write can refit its ancestor chain without re-deriving topology. Runtime-only.
  uint32_t tlasLeafLoc = ~0u;

  // Plain vertex count, uint32 so a heavily-dup'd chunk can exceed 65536 post-dup verts (the BLAS
  // path is index-width agnostic). Only meaningful when indicesCount > 0; for non-mesh / dropped
  // meshes it is 0 -- gate on indicesCount, not on this field.
  uint32_t verticesCount = 0;

  // Byte offset of this node's per-node BLAS chunk inside CollisionResource::Data::nodeBlasData
  // (NodeBlasChunkHeader + quad tree + the node's vert21 stream); ~0u = no chunk (a
  // degenerate-dropped node). Runtime-only: the loader lands the chunks and stamps it. A chunked
  // node's vert21 stream lives at the chunk tail; getPackedNodeVerts21 resolves the base through it.
  uint32_t nodeBlasOfs = ~0u;

  uint32_t indicesCount = 0;

  uint32_t nameOfs = 0; // offset into CollisionResource::Data::names; 0 means empty

protected:
  // The node sphere's center, uncomposed. CollisionResource::getNodeBSphere owns the public sphere,
  // and composes it; handing this out beside a node reference would hand out a different answer.
  Point3 bsphereCenter() const { return modelBBox.center(); }
  vec3f vBsphereCenter() const // the four-lane loads end inside the node
  {
    return v_mul(v_add(v_ldu(&modelBBox.lim[0].x), v_ldu(&modelBBox.lim[1].x)), V_C_HALF);
  }

public:
  int getNodeIdAsInt() const { return (int)geomNodeId; }

  // A degenerate/rejected node is dropped to indicesCount == 0 at load: no per-node BLAS chunk, no
  // verts, nothing to trace or hand a shape builder. Consumers must skip it. Named so a new node
  // iterator cannot silently deref a dropped node (nodeBlasOfs == ~0u, wild in release).
  bool hasGeometry() const { return indicesCount != 0; }

  // If you got crash here with (this == nullptr), it's compiler error. Try to make node iterator simpler.
  bool checkBehaviorFlags(uint16_t f) const { return (behaviorFlags & f) == f; }
  bool isBehaviorFlagsInFilter(uint16_t f) const { return (behaviorFlags | f) == f; }
};
DAG_DECLARE_RELOCATABLE(CollisionNode);
static_assert(sizeof(CollisionNode) == 64, "one node per cache line");

class GeomNodeTree;
class CollisionResource;
class CollisionResourceTraceAdapter;
class String;

// Caller-owned pose of a CollisionResource. Two owning forms plus a wrapper-internal view:
// tree-backed (bound to a GeomNodeTree; matrices are read live from the tree, derived state
// refreshes lazily at trace time) and owned-matrix (tree == null; the embedded default
// instance and manual poses driven via updateNodeTm). Pose mutation must be serialized
// against traces; the lazy refresh itself is internally locked. Concurrent traces must
// agree on entity_tm: derived state is entity-relative, and a changed entity placement
// is a pose change under the same serialization rule.
// Driven node wtms of the tree-backed form may carry scale and shear: the bind meta holds while a wtm stays in the rigid band,
// and the refresh classifies a wtm outside it from the live pose. A wtm with an axis scale of 1e-9 or less hides its node.
// Handedness is not gated: a driven placement of either handedness traces the stored winding, as
// the legacy tree overloads do. Staleness assumes the tree pose generation cannot revisit a
// stamped value un-refreshed (a full 2^32 wrap landing exactly on the stamp is out of contract).
// TLAS uint16 quant frame: the span maps interior values onto [1, 65534] and the encoders keep
// one bin of wall slack on each side (min floors one bin below the mapped value, max ceils one
// above), so a frame-edge box quantizes OUTWARD into the physical [0, 65535] instead of clamping
// inside its true bounds. Namespace scope so CollisionResourceInstance::tlasQuant and
// encodeTlasLeafBox (declared before CollisionResource) share this exact frame.
inline constexpr float COLLRES_TLAS_QUANT_LO = 1.f;
inline constexpr float COLLRES_TLAS_QUANT_SPAN = 65533.f;
// One bin above the quant range: the encoders' float-space clamp ceiling, and the floor of what
// the segment clip must cover -- deriving both from here keeps the conservatism invariant in
// one place (clip frame >= clamp ceiling + one two-bin pad).
inline constexpr float COLLRES_TLAS_QUANT_CLAMP_HI = COLLRES_TLAS_QUANT_LO + COLLRES_TLAS_QUANT_SPAN + 1.f;
// Type arm of the TLAS leafless-recovery predicate: which leafless node kinds a live pose can
// make TRACEABLE again. Meshes/convexes need geometry; prims (capsules included) recover -- tree
// seeding can re-mark even a BAKED (authored-singular) prim traceable, though its geometry stays
// in the exporter-baked bind frame -- while empty markers (bsph r < 0) and POINTS are never
// trace targets. Shared by the blueprint watchlist build and the pose-write transition check.
inline bool collres_tlas_recovery_capable(const CollisionNode &n, float bsph_r)
{
  if (n.type == COLLISION_NODE_TYPE_MESH || n.type == COLLISION_NODE_TYPE_CONVEX)
    return n.hasGeometry();
  if (n.type == COLLISION_NODE_TYPE_BOX || n.type == COLLISION_NODE_TYPE_SPHERE || n.type == COLLISION_NODE_TYPE_CAPSULE)
    return bsph_r >= 0.f;
  return false;
}

// The mesh list's membership rule: rebuildNodeLists groups by it and the trace dispatch's
// mesh pass filters candidates by it, so one edit adds a type to both.
inline bool collres_is_mesh_list_node(CollisionResourceNodeType type)
{
  return type == COLLISION_NODE_TYPE_MESH || type == COLLISION_NODE_TYPE_CONVEX;
}

// Conditioning amplifier sigma_max^3 / |det| of a geometry matrix, scale-invariant down to
// the float det underflow (spectral norm ~2e-13, cbrt(FLT_MIN)): below it det_abs flushes to
// zero and the sentinel declines the TLAS - the linear walk stays correct for such poses.
// Stepwise division; an absolute floor would erase the ratio far above that. Clamped [1, 1e12].
inline float collres_tlas_conditioning(float spectral_norm, float det_abs)
{
  // The floor only keeps the fully degenerate case (norm 0, det 0) NaN-free: r becomes 0 and
  // the sentinel declines below, the same verdict 0/0 would reach; no reachable result changes.
  const float s = spectral_norm > 1e-30f ? spectral_norm : 1e-30f;
  // An OVERFLOWED determinant proves nothing about the ratio: cap out so the envelope declines.
  // check_finite, not an ordering compare: -ffinite-math-only folds NaN comparisons away.
  if (!check_finite(det_abs))
    return 1e12f;
  const float r = ((det_abs / s) / s) / s;
  return r > 1e-12f ? (r < 1.f ? 1.f / r : 1.f) : 1e12f;
}

// Query tranche size in dominant frame DIMENSIONS; the budget derivation lives in
// refitAllTlasLeaves, the per-call gate in tlasQueryMagLimit.
inline constexpr float COLLRES_TLAS_QUERY_ABSORB = 1024.f;

// The shared error-budget accumulation: the load-time build/decline decision and the
// instance-side refit MUST agree, so both call these instead of keeping private copies.
// Per-node compose operand: two transform applications of the raw magnitude plus the composed
// box magnitude, conditioning-amplified.
inline float collres_tlas_node_op(float max_tm_scale, float raw_mag, float box_mag, float ampl)
{
  return (2.f * max_tm_scale * raw_mag + box_mag) * ampl;
}
// Full fp budget: the 25% operand-growth reserve plus the caller-endpoint query tranche, under
// the 16-eps compose chain (see the refit derivation in the change description).
inline float collres_tlas_fp_budget(float max_node_op, float frame_dim, float ampl_max)
{
  return 16.f * 1.19209290e-07f * (1.25f * max_node_op + COLLRES_TLAS_QUERY_ABSORB * frame_dim * ampl_max);
}
// One leaf box to quantized uint16 lane bounds, padded by the compose-error budget plus one bin
// of wall slack, clamped in FLOAT space (overflow converts to INT_MIN on SSE); build and refit MUST agree.
inline void collres_tlas_quant_lane(bbox3f_cref box, vec4f scale, vec4f origin, vec4f fp_err, uint16_t out_mn[3], uint16_t out_mx[3])
{
  const vec4f qLo = v_splats(COLLRES_TLAS_QUANT_LO);
  const vec4f qMin = v_sub(v_madd(v_sub(v_sub(box.bmin, fp_err), origin), scale, qLo), V_C_ONE);
  const vec4f qMax = v_add(v_madd(v_sub(v_add(box.bmax, fp_err), origin), scale, qLo), V_C_ONE);
  const vec4i mi = v_cvt_floori(v_max(v_min(qMin, v_splats(COLLRES_TLAS_QUANT_CLAMP_HI)), v_zero()));
  const vec4i ma = v_cvt_ceili(v_max(v_min(qMax, v_splats(COLLRES_TLAS_QUANT_CLAMP_HI)), v_zero()));
  alignas(16) int mib[4], mab[4];
  v_sti(mib, mi);
  v_sti(mab, ma);
  for (int k = 0; k < 3; ++k)
  {
    out_mn[k] = (uint16_t)mib[k];
    out_mx[k] = (uint16_t)mab[k];
  }
}

// Largest share of the frame's DOMINANT dimension the fp budget may eat before the build and
// the refit decline: the budget pads every leaf on both sides, so at 0.1 a leaf grows by at
// most 20% of the dominant span and the bins keep most of their rejection power (0.25 let the
// pad reach half the range, halving the prefilter's cull on the axis it targets).
inline constexpr float COLLRES_TLAS_BUDGET_MAX_RATIO = 0.1f;

// Hard endpoint-magnitude ceiling of tlasQuantSeg, above every tlasQueryMagLimit envelope: past
// it the doubled magnitudes of the clip arithmetic themselves approach float infinity.
inline constexpr float COLLRES_TLAS_QUERY_MAG_CAP = 1e37f;

// One instance TLAS: quantized SoA4 tree bytes and their uint16 quant frame. The resource's
// blueprint keeps its tree bytes and the topology metadata behind them in the Data block (see
// the tlas*() spans) and no frame: a clone holds tree bytes alone and derives its own frame.
struct CollresTlasFrame
{
  dag::Vector<uint8_t> data;
  vec4f scale = {}, origin = {}; // uint16 quant frame (scale + frame origin) over the leaf-box union
  vec4f fpErr = {};              // per-axis compose-error budget from PRE-compose operand magnitudes
};

struct CollisionResourceInstance
{
  // Dispatch metadata for getNodeGeometryTm and conservative posed bounds.
  struct PoseMeta
  {
    enum StatusBits : uint8_t
    {
      // The builder's authored gate and the tree-backed refresh accept mirrored placements
      // (|det|), the owned live gate (recomputePoseMeta) rejects them (signed det).
      TRACEABLE = 1,      // cleared for singular poses and owned live-mirrored ones
      GEOMETRY_BAKED = 2, // singular authored primitive kept its exporter bake
      DISABLED = 4,       // structural hide, applied before filters

      // Baked frame kept for a VALID general placement (a sheared/non-uniform sphere whose
      // Ritter radius cannot be un-baked): still poseable via posed * inverse(authored),
      // unlike the singular bake above. Always set together with GEOMETRY_BAKED.
      RETAINED_BAKE = 8,
      // Finite pose with a finite inverse: accessors may compose it. Superset of TRACEABLE (a
      // finite owned live-mirrored pose is composable but hidden from traces).
      COMPOSABLE = 16,
      // The STORED matrix is bit-exactly identity, so composing it is a no-op. This is not the
      // IDENT transform class: that one is an epsilon classification the loader may keep for a
      // near-identity placement, and the dispatch still has to compose such a placement. Set only
      // by the two writers of nodeTm, and absent by default, so a path that forgets it loses the
      // shortcut rather than taking it wrongly.
      POSE_IS_IDENTITY = 32,
      // Tree-backed seed gate result (rel-tm det floor): the lazy refresh re-derives TRACEABLE
      // as BIND_TRACEABLE && a finite driven pose above the det floor, so a degenerate bone
      // hides and can return.
      BIND_TRACEABLE = 64,
      // Tree-backed: the pose class and scale come from a driven wtm outside the rigid band;
      // the refresh restores the bind meta when the wtm is rigid again.
      LIVE_CLASSIFIED = 128,
    };
    float maxTmScale = 1.f;
    uint8_t flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED; // NodeFlag transform-class bits
    uint8_t status = TRACEABLE | COMPOSABLE;
    // Live CollisionNode::BehaviorFlag bits, seeded from the authored node flags; the per-entity
    // override (flag rules) lives here, outside the shared Data block.
    uint16_t behaviorFlags = 0;
    bool isTraceable() const { return (status & TRACEABLE) != 0; }
    bool isGeometryBaked() const { return (status & GEOMETRY_BAKED) != 0; }
    bool isRetainedBake() const { return (status & RETAINED_BAKE) != 0; }
    bool isDisabled() const { return (status & DISABLED) != 0; }
    bool isComposable() const { return (status & COMPOSABLE) != 0; }
    bool isPoseIdentity() const { return (status & POSE_IS_IDENTITY) != 0; }
    bool isBindTraceable() const { return (status & BIND_TRACEABLE) != 0; }
    bool isLiveClassified() const { return (status & LIVE_CLASSIFIED) != 0; }
    bool checkBehaviorFlags(uint16_t f) const { return (behaviorFlags & f) == f; }
    void setTraceable(bool on) { status = uint8_t(on ? status | TRACEABLE : status & ~TRACEABLE); }
    void setGeometryBaked(bool on) { status = uint8_t(on ? status | GEOMETRY_BAKED : status & ~GEOMETRY_BAKED); }
    void setRetainedBake(bool on) { status = uint8_t(on ? status | RETAINED_BAKE : status & ~RETAINED_BAKE); }
    void setDisabled(bool on) { status = uint8_t(on ? status | DISABLED : status & ~DISABLED); }
    void setComposable(bool on) { status = uint8_t(on ? status | COMPOSABLE : status & ~COMPOSABLE); }
    void setPoseIdentity(bool on) { status = uint8_t(on ? status | POSE_IS_IDENTITY : status & ~POSE_IS_IDENTITY); }
    void setBindTraceable(bool on) { status = uint8_t(on ? status | BIND_TRACEABLE : status & ~BIND_TRACEABLE); }
    void setLiveClassified(bool on) { status = uint8_t(on ? status | LIVE_CLASSIFIED : status & ~LIVE_CLASSIFIED); }
    // Out-of-range read fallback: bind-default compose state (IDENT class, scale 1, enabled)
    // but NOT traceable, matching isNodeTraceable's safe skip of absent slots.
    static const PoseMeta absent_slot;
  };

  CollisionResourceInstance() = default;
  CollisionResourceInstance(const CollisionResourceInstance &) = delete;
  CollisionResourceInstance &operator=(const CollisionResourceInstance &) = delete;

  const CollisionResource *getResource() const { return res; }
  // Null for the owned-matrix form (default instance, manual poses).
  const GeomNodeTree *getTree() const { return tree; }
  // Public instance mutators reject the embedded default instance.
  bool isDefault() const;
  int nodeCount() const { return (int)metaCount(); }

  // Read-side queries may outlive a node mutation, so an absent slot reads as enabled here;
  // every other accessor asserts on out-of-range instead (callers own index validity).
  bool isNodeEnabled(int node_index) const
  {
    return (uint32_t)node_index >= metaCount() || !metaData()[(uint32_t)node_index].isDisabled();
  }
  // Singular poses and owned live-mirrored ones are not traceable; an absent slot is not traceable
  // either (dev asserts, release degrades to the safe answer -- the node is skipped).
  bool isNodeTraceable(int node_index) const
  {
    G_ASSERT((uint32_t)node_index < metaCount());
    return (uint32_t)node_index < metaCount() && metaData()[(uint32_t)node_index].isTraceable();
  }
  bool isNodeComposable(int node_index) const
  {
    G_ASSERT((uint32_t)node_index < metaCount());
    return (uint32_t)node_index < metaCount() && metaData()[(uint32_t)node_index].isComposable();
  }
  // Live behavior flags (an absent slot reads as none). A caller-owned instance mirrors the
  // resource's default at seed and at its first trace after a default write.
  uint16_t getNodeBehaviorFlags(int node_index) const { return getPoseMeta(node_index).behaviorFlags; }
  bool checkNodeBehaviorFlags(int node_index, uint16_t f) const { return getPoseMeta(node_index).checkBehaviorFlags(f); }
  // Resource-local posed matrices exist only on the owned-matrix form; tree-backed poses are
  // read from the tree (see CollisionResource::getCollisionNodeTm for a form-aware fetch).
  // Misuse (tree-backed form or out-of-range index) asserts in dev and degrades to the bind
  // identity with a one-time logerr in release.
  const TMatrix &getNodeTmRef(int node_index) const
  {
    if (DAGOR_LIKELY((uint32_t)node_index < nodeTm.size()))
      return nodeTm[node_index];
    return missingOwnedPose(node_index);
  }
  const PoseMeta &getPoseMeta(int node_index) const
  {
    G_ASSERT((uint32_t)node_index < metaCount());
    if (DAGOR_LIKELY((uint32_t)node_index < metaCount()))
      return metaData()[node_index];
    return PoseMeta::absent_slot;
  }
  // Returns mat44f for the SIMD hot path, unlike the same-named TMatrix accessor on
  // CollisionResource -- a migrating call site changes value type; getNodeTmRef keeps TMatrix.
  mat44f getNodeTm(int node_index) const
  {
    mat44f m;
    v_mat44_make_from_43cu_unsafe(m, getNodeTmRef(node_index).array);
    return m;
  }
  // Maps stored geometry to posed resource space without reapplying exporter-baked prim tms.
  // Owned-matrix form only; misuse degrades like getNodeTmRef.
  mat44f getNodeGeometryTm(int node_index) const;
  // The node's bounds in RESOURCE space, composed through getNodeGeometryTm; the instance tm is
  // NOT applied - compose it yourself. A baked node keeps its baked frame at bind; live-posing makes
  // it untraceable (its baked box stays available via getNodeBBox). A tree-backed instance owns no
  // matrices, so this owned-pose accessor degrades to the BIND-identity box with a one-time
  // logerr, like getNodeTmRef misuse - resolve driven placements through the tree instead. Empty for a zero-vert node
  // (r < 0), a POINTS node (never a trace target), a degenerate-dropped mesh/convex, an untraceable
  // pose, an unbound instance, or an out-of-range id (the checked-accessor convention; asserting
  // stays with the *Unsafe tier). The enable gate is deliberately NOT consulted: a disabled node
  // still reports its box (enable state is dynamic; the root bounds skip it). Axes thinner
  // than the trace slab minimum are inflated to it (see getNodeGeometryBBox); a sphere composes
  // analytically (exact). The result is an enclosing AABB: its corners are not the rotated
  // stored-box corners.
  BBox3 getNodeResourceBBox(int node_index) const;
  // Union of enabled posed node boxes, resource-local; conservative (node boxes carry the
  // trace slab's minimum axis thickness) and grow-only across pose writes until the next
  // full recompute (setNodeEnabled). Tree-backed form: a plain read that
  // may race the lazy trace-side refresh; call under the same serialization as traces.
  bbox3f getRootBBox() const { return rootBBox; }
  // Instance TLAS: a clone of the blueprint topology with refit boxes, a candidate prefilter
  // that may SKIP nodes but never add hits. Absent until a materializing refit (pose write,
  // tree refresh, posed seed, rebuild, deep copy), which may decline. TRUE may still hold an
  // older blueprint's bytes until that refit - currency is tlasCloneCurrent's question.
  // Serialized like traces.
  bool hasTlas() const { return !tlas.data.empty(); }
  // The clone is keyed to the blueprint's generation, stamped once at the load: a clone minted
  // before the blueprint existed is not current. Call under the same serialization as traces.
  bool tlasCloneCurrent(uint32_t res_gen) const { return !tlas.data.empty() && tlasCloneGen == res_gen; }

protected:
  // The dispatch internals below are friend-only: CollisionResource and the trace adapter.
  // Tree-backed leaves bake the refresh trace's entity basis; the max deviation of
  // trace^T * baked from identity is the frame mismatch, spent from the tm displacement budget.
  float tlasEntityBasisColDev(vec3f ti, vec3f ident_col) const
  {
    const vec4f d = v_perm_xycd(v_perm_xayb(v_dot3_x(ti, tlasEntityBasisSeen.col0), v_dot3_x(ti, tlasEntityBasisSeen.col1)),
      v_dot3(ti, tlasEntityBasisSeen.col2));
    return v_extract_x(v_hmax3(v_abs(v_sub(d, ident_col))));
  }
  float tlasEntityBasisDev(mat44f_cref m) const
  {
    if (!tree)
      return 0.f;
    return max(tlasEntityBasisColDev(m.col0, V_C_UNIT_1000),
      max(tlasEntityBasisColDev(m.col1, V_C_UNIT_0100), tlasEntityBasisColDev(m.col2, V_C_UNIT_0010)));
  }
  // A baked-translation mismatch displaces every leaf by the full delta; converted into the
  // same dimensionless budget as the basis deviation (displacement = 3 * dev * frame magnitude).
  float tlasEntityPosDev(vec3f entity_ofs) const
  {
    if (!tree)
      return 0.f;
    const float d = v_extract_x(v_length3_x(v_sub(entity_ofs, tlasEntityBasisSeen.col3)));
    return d / (3.f * (tlasFrameMag > 1e-9f ? tlasFrameMag : 1e-9f));
  }
  const uint8_t *tlasTree() const { return tlas.data.data(); }
  // Resource-local point into this instance's uint16 quant frame.
  // Subtract the frame origin BEFORE scaling: the affine p * scale + ofs form cancels two huge
  // products for narrow bounds far from the origin and can clip a leaf INSIDE its geometry.
  vec4f tlasQuant(vec3f p) const { return v_madd(v_sub(p, tlas.origin), tlas.scale, v_splats(COLLRES_TLAS_QUANT_LO)); }
  // The padded frame box tlasQuantSeg clips against: frame state is call-constant, so a batch
  // derives it once and passes it to every per-trace clip.
  bbox3f tlasClipFrame() const
  {
    bbox3f fb;
    const vec3f binPad = v_div(v_splats(2.f), tlas.scale);
    fb.bmin = v_sub(tlas.origin, binPad);
    fb.bmax = v_add(tlas.origin, v_div(v_splats(COLLRES_TLAS_QUANT_CLAMP_HI + 2.f), tlas.scale));
    return fb;
  }
  // Clip the local segment to the padded frame BEFORE quantizing (far endpoints quantize with
  // multi-bin ULPs; clip-t rounding is longitudinal and pre-compensated, so truncation eats pad,
  // never leaf coverage). Returns 0 when the segment provably misses the frame, 1 when clipped,
  // -1 when the arithmetic cannot stay conservative: the caller falls back to the linear walk.
  int tlasQuantSeg(vec3f from, vec3f to, vec4f &q_from, vec4f &q_to) const
  {
    return tlasQuantSeg(tlasClipFrame(), from, to, q_from, q_to);
  }
  int tlasQuantSeg(bbox3f_cref fb, vec3f from, vec3f to, vec4f &q_from, vec4f &q_to) const
  {
    const vec3f dir = v_sub(to, from);
    const vec4f magX = v_max(v_hmax3(v_abs(from)), v_hmax3(v_abs(to)));
    const vec4f dirMagX = v_hmax3(v_abs(dir));
    if (!v_test_xyz_finite(from) || !v_test_xyz_finite(to) || !(v_extract_x(magX) < COLLRES_TLAS_QUERY_MAG_CAP))
      return -1;
    const vec3f isDiv0 = v_is_unsafe_divisor(dir);
    const vec3f t0 = v_div(v_sub(fb.bmin, from), dir);
    const vec3f t1 = v_div(v_sub(fb.bmax, from), dir);
    const vec3f parOut = v_or(v_cmp_lt(from, fb.bmin), v_cmp_gt(from, fb.bmax));
    const vec3f tmin3 = v_sel(v_min(t0, t1), v_sel(v_neg(V_C_MAX_VAL), V_C_MAX_VAL, parOut), isDiv0);
    const vec3f tmax3 = v_sel(v_max(t0, t1), v_sel(V_C_MAX_VAL, v_neg(V_C_MAX_VAL), parOut), isDiv0);
    // 16 eps: the max-norm under-reads the Euclidean length by at most sqrt(3)
    const vec4f tPad = v_div_x(v_mul(magX, v_splats(16.f * 1.19209290e-07f)), v_max(dirMagX, v_splats(1e-30f)));
    const vec4f tEnter = v_max(v_sub(v_hmax3(tmin3), tPad), v_zero());
    const vec4f tExit = v_min(v_add(v_hmin3(tmax3), tPad), V_C_ONE);
    if (v_extract_x(tEnter) > v_extract_x(tExit))
      return 0;
    q_from = tlasQuant(v_madd(dir, v_splat_x(tEnter), from));
    q_to = tlasQuant(v_madd(dir, v_splat_x(tExit), from));
    return 1;
  }

public:
  // Sticky latch disabling shortcuts whose bounds are valid only at bind.
  bool isPosedSinceBind() const { return posedSinceBind; }

  // Mutators return false when the write is dropped (default, foreign, stale, resource-less
  // or tree-backed-where-owned-form-required instance, or an out-of-range node index) so callers
  // can detect and re-create the binding.
  // rootBBox grows conservatively until the next full update. A singular, mirrored or non-finite
  // pose stores and returns true but hides the node (isNodeTraceable turns false) until a
  // realizable pose is written. Owned-matrix form only.
  bool updateNodeTm(int node_index, mat44f_cref tm);
  // Structural hides can shrink rootBBox: the owned-matrix arm recomputes it here (O(nodes)),
  // the tree-backed arm defers to the next trace's refresh. Content-event cadence, not per-frame.
  bool setNodeEnabled(int node_index, bool enabled);

protected:
  friend class CollisionResource;
  friend struct CollisionResourceBuilder;
  friend class CollisionResourceTraceAdapter;
  friend struct CollisionResourceUnittest;
  // Instances never alias: the temp-view dialect lives in the trace adapter's PoseView, whose
  // own isMetaAliased answers for the shared core. Constant false keeps this type's arm of the
  // templated gates folding out.
  bool isMetaAliased() const { return false; }
  // Primitive source for the trace core: an owning instance is its own.
  const CollisionResourceInstance &primPoseSource() const { return *this; }
  const PoseMeta *metaData() const { return poseMeta.data(); }
  uint32_t metaCount() const { return (uint32_t)poseMeta.size(); }
  bool validateForUpdate() const;
  // Cold misuse arm shared by the owned-pose accessors; returns the bind identity.
  DAGOR_NOINLINE const TMatrix &missingOwnedPose(int node_index) const;
  void seedPose();
  // Shared write path for caller-owned and default instances.
  bool updateNodeTmImpl(int node_index, mat44f_cref tm);
  void recomputeRootBBox();
  // Reclassifies the STORED pose nodeTm[node_index] (write it first).
  void recomputePoseMeta(int node_index);
  // Lazy derived-state rebuild for the tree-backed form; entity_tm is the trace's instance tm.
  // with_tlas: the trace may take the TLAS, so the same node walk refits it.
  void refreshIfStale(mat44f_cref entity_tm, bool with_tlas) const;
  // Flags-only mirror of the default, keyed on liveFlagsGen; both owning forms take it per trace.
  void mirrorLiveFlagsIfStale() const;
  // One walk for the stale parts: the pose, and the TLAS when with_tlas is set, which only non-capsule traces of an
  // all-nodes-TLAS resource ask for, so other queries never pay it. O(nodes).
  DAGOR_NOINLINE void refreshFromTree(mat44f_cref entity_tm, bool with_tlas) const;
  void refreshTlasIfStale(mat44f_cref entity_tm) const;
  // Resource-local posed matrix of one tree-driven node; unbound nodes read the default pose.
  mat44f treePosedNodeTm(uint32_t node_index, mat44f_cref inv_entity, vec3f rel_ofs) const;
  // The rel tm of a driven node, identity when the resource has none for it (an appended node has no slot).
  mat44f drivenRelTm(uint32_t node_index) const;
  // Pose class and scale bound of a driven node: live from wtm * rel, or the bind meta from the rel tm, padded for a rigid-band wtm.
  void setDrivenPoseClass(uint32_t node_index, mat44f_cref node_tm, bool live) const;

  // Full refit: re-derive the quant frame, re-encode every leaf, re-union inner boxes; the
  // topology is never touched. Owned entry composes from the stored poses; frame_headroom
  // inflates the frame for escape-triggered refits.
  void refitAllTlasLeaves(float frame_headroom = 0.f) const;
  // Commit of a composed pose on an all-nodes-TLAS resource, shared by the owned entry and the tree refresh: the leaf boxes,
  // their union and the error budget's operand and conditioning maxima, all from one walk.
  // leaf_rows: per TLAS node 6 rows of 4 lanes (box min xyz, max xyz); row_masks: leaf lanes | valid lanes << 4.
  void refitAllTlasLeaves(const float *leaf_rows, const uint8_t *row_masks, bbox3f_cref leaf_union, float max_node_op, float ampl_max,
    float frame_headroom = 0.f) const;
  // True after declining: a leafless node the pose makes traceable has no leaf to visit.
  bool declineForLeaflessRecovery() const;
  // Single leaf in the existing frame plus a parent-chain grow; a leaf escaping the frame falls
  // back to a full refit. geom_tm/posed_box reuse the caller's compose.
  void refitTlasLeaf(int node_index, const mat44f *geom_tm = nullptr, const bbox3f *posed_box = nullptr) const;
  // box == nullptr recomputes the node's posed box from the owned pose. leaf_loc is the packed
  // (tree node << 2) | lane CollisionNode::tlasLeafLoc carries.
  void encodeTlasLeafBox(int node_index, uint32_t leaf_loc, const bbox3f *box) const;
  // Posed node box, or false when the node contributes no leaf (hidden, untraceable,
  // geometry-less). Owned form only; out_raw_box returns the raw box to spare a re-fetch.
  bool ownedPosedNodeBox(int node_index, bbox3f &out_box, bbox3f *out_raw_box = nullptr) const;
  // Decline latch: this blueprint generation keeps the linear walk. The clear does NOT free:
  // a reader that raced the serialization contract degrades on stale bytes instead of a
  // use-after-free, and the kept capacity lets a later materialize avoid moving the buffer.
  void declineTlas() const;
  // Padded quant bounds of one leaf box: the frame error budget (world space) plus one bin
  // (q-space margin for exact-integer faces against segment-endpoint rounding).
  void tlasQuantPadded(bbox3f_cref box, vec4f &q_min, vec4f &q_max) const
  {
    const vec4f qLo = v_splats(COLLRES_TLAS_QUANT_LO);
    q_min = v_sub(v_madd(v_sub(v_sub(box.bmin, tlas.fpErr), tlas.origin), tlas.scale, qLo), V_C_ONE);
    q_max = v_add(v_madd(v_sub(v_add(box.bmax, tlas.fpErr), tlas.origin), tlas.scale, qLo), V_C_ONE);
  }

  const CollisionResource *res = nullptr;
  const GeomNodeTree *tree = nullptr; // tree-backed form only (legacy views live in PoseView)
  dag::Vector<TMatrix> nodeTm;        // owned-matrix form only: T[i] as 3x4 floats, by nodeIndex
  // Owning forms; parallel to the resource node list. Mutable: the tree-backed lazy refresh
  // re-derives driven traceability and re-syncs unbound meta under refreshLock.
  mutable dag::Vector<PoseMeta> poseMeta;
  mutable bbox3f rootBBox = {};          // union of enabled posed node boxes, resource-local
  mutable vec4f bsphereCenterLocal = {}; // selected tree node in resource space after refresh
  // 0 = never refreshed; compared against the bound tree's pose generation (never 0).
  mutable uint32_t poseGeneration = 0;
  // Resource defaultPoseGen at the last refresh: unbound nodes read the default pose live,
  // so a default write stales the derived state without touching the tree generation.
  mutable uint32_t defaultPoseGenAtRefresh = 0;
  mutable uint32_t liveFlagsGenSeen = 0; // resource liveFlagsGen at the last flags mirror
  // Generations the clone was last refit for; release/acquire contract at refreshTlasIfStale.
  mutable uint32_t tlasPoseGeneration = 0;
  mutable uint32_t tlasBlueprintGenSeen = 0;
  mutable uint32_t tlasDefaultPoseGenSeen = 0; // default pose moves without a tree bump
  // Entity basis and translation the tree-backed leaves baked. Written before the stamps above,
  // so their release publishes it: read it only after acquiring them (refreshTlasIfStale).
  mutable mat44f tlasEntityBasisSeen = {};
  mutable int refreshLock = 0;      // spins only on the once-per-generation refresh
  uint32_t treeNodeCountAtBind = 0; // layout lock for the tree-backed form
  // One drift assert per binding: the mismatch persists until a rebind, and re-asserting on
  // every later pose generation would spam while the conservative degrade already stands.
  mutable bool layoutDriftAsserted = false;
  bool posedSinceBind = false; // see isPosedSinceBind
  mutable bool hasBsphereCenterLocal = false;
  // Instance TLAS clone (see hasTlas): tree bytes and their quant frame, re-derived on every
  // full refit - the same CollresTlasFrame the resource blueprint uses, tree bytes alone here.
  mutable CollresTlasFrame tlas;
  mutable float tlasFrameNodeOp = 0.f;  // GEOMETRY-ONLY operand magnitude the frame budget was derived from
  mutable float tlasFrameAmpl = 1.f;    // conditioning maximum the budget (query tranche included) was scaled by
  mutable float tlasFrameMag = 0.f;     // max abs coordinate of the padded, headroom-grown frame (worst in-frame magnitude)
  mutable float tlasFrameDim = 0.f;     // dominant pre-pad dimension of the leaf union (query tranche base)
  mutable uint32_t tlasDeclinedGen = 0; // blueprint generation the decline latched against
  // The leaf pad splits into DISJOINT spends: compose rounding keeps its share, the remainder
  // halves between the two admission gates (a shared budget could admit ~1.5x pad jointly).
  float tlasGatePad() const
  {
    const float composeSpend = 16.f * 1.19209290e-07f * tlasFrameNodeOp;
    const float pad = v_extract_x(tlas.fpErr) - composeSpend;
    return pad > 0.f ? pad : 0.f;
  }
  // Largest basis deviation the segment can absorb: displacement bounds at 3 * dev * frame
  // magnitude (one output coordinate sums three entries of (A^T A - I) p), within half the pad.
  float tlasSegTmTolerance() const { return tlasGatePad() * (0.5f / 3.f) / (tlasFrameMag > 1e-9f ? tlasFrameMag : 1e-9f); }
  // Largest query endpoint magnitude candidate mode absorbs at a given basis deviation:
  // (3 * dev + 16 eps) * |p| must fit its half of the budgeted query tranche. An endpoint past
  // it declines the call to the linear walk - the operative decline for far-world callers.
  float tlasQueryMagLimit(float dev) const
  {
    return 0.5f * 16.f * 1.19209290e-07f * COLLRES_TLAS_QUERY_ABSORB * tlasFrameDim * tlasFrameAmpl /
           (3.f * dev + 16.f * 1.19209290e-07f);
  }
  mutable bool tlasDeclined = false; // envelope decline latch; cleared at full-refit boundaries
  mutable uint32_t tlasCloneGen = 0; // blueprint generation this clone was taken from

  // Compile-time capability: temp views set this false, folding the candidate gate out of
  // their dispatch instantiations.
  static constexpr bool mayUseTlas = true;
  // Dev observability: whether the last dispatch ran candidate mode. Advisory, interlocked.
  mutable int lastDispatchByCand = 0;
  void noteDispatchByCand(bool v) const { interlocked_release_store(lastDispatchByCand, v ? 1 : 0); }
};

enum CollisionResourceDrawDebugBits
{
  CRDD_NODES = 1,
  // Include unbound mesh nodes in the GeomNodeTree form.
  CRDD_NON_GEOM_TREE_NODES = 2,
  CRDD_BSPHERE = 4,
  CRDD_ALL = (unsigned short)~(unsigned short)0u
};

using TraceCollisionResourceStats = dag::Vector<int, framemem_allocator>;

// Only the embedded default instance is mutable after setup. Create independent instances after
// all node-list mutations so their pose arrays remain parallel.
decl_dclass_and_id(CollisionResource, DObject, CollisionGameResClassId)
public:
  bbox3f vFullBBox = {};      // all nodes, including box
  vec4f vBoundingSphere = {}; // center|r^2 in w
  // The resource sphere widened over the bind pose; w is a radius^2 (0 for an empty resource).
  vec4f vBindTraceSphere = {};
  alignas(16) BBox3 boundingBox = {};

private:
  float boundingSphereRad = 0; // setBoundingSphereRad / getBoundingSphereRad
  // min(0.008, 0.025 / boundingSphereRad): the orthonormal-class tolerance of a trace's instance
  // tm (0.025 * sqrt(3) of absolute error max), derived with the radius instead of divided per call.
  float traceTmEps = 0.008f;

public:
  uint32_t collisionFlags = 0;

  // The node indices grouped by list (mesh with convex, points, box, sphere, capsule), each group in
  // node order: the per-type walks and the trace dispatch's fallback under no TLAS. The group bounds
  // lead the same array, so the Data header carries none of them (see NODE_ORDER).
  dag::ConstSpan<uint16_t> nodeList(int list) const
  {
    const dag::ConstSpan<uint16_t> order = data->nodeOrder();
    // Out of domain answers empty, like getNode does for an index: NUM_COLLISION_NODE_TYPES is this
    // class's own all-nodes sentinel, and reading its bound would return a garbage-length span.
    if (order.empty() || (unsigned)list >= NUM_COLLISION_NODE_TYPES)
      return dag::ConstSpan<uint16_t>();
    return dag::ConstSpan<uint16_t>(order.data() + Data::LIST_BOUNDS + order[list], order[list + 1] - order[list]);
  }
  dag::ConstSpan<uint16_t> meshNodes() const { return nodeList(COLLISION_NODE_TYPE_MESH); }
  dag::ConstSpan<uint16_t> boxNodes() const { return nodeList(COLLISION_NODE_TYPE_BOX); }
  dag::ConstSpan<uint16_t> sphereNodes() const { return nodeList(COLLISION_NODE_TYPE_SPHERE); }
  dag::ConstSpan<uint16_t> capsuleNodes() const { return nodeList(COLLISION_NODE_TYPE_CAPSULE); }
  dag::Index16 bsphereCenterNode;

protected:
  struct Data;
  // deepCopy installs its clone directly, skipping the default ctor's throwaway block.
  explicit CollisionResource(Data * use_data)
  {
    data = use_data;
    defaultInstance.res = this;
  }

  // The block's only holder: Data's refcount counts every holder of one block, the CollisionResource
  // objects sharing it (deepCopy) and the physJolt shares of long-lived chunk consumers
  // (CollResJoltChunk::Share under a live shape); not the game-resource holders (the DObject refcount).
  bool ownsData() const;

public:
  // The empty resource: no nodes, the state a refused stream leaves.
  CollisionResource() : CollisionResource(Data::build(nullptr)) {}
  // A refused or corrupt stream leaves the resource empty (one logerr; a corrupt chunk stream is
  // a fatal where exceptions are off). res_name is the diagnostics name (null: resolved from
  // res_id); resolve_phmat maps a persisted material name to an id (null: the PhysMat table).
  CollisionResource(IGenLoad & crd, int res_id, const char *res_name = nullptr, int (*resolve_phmat)(const char *) = nullptr);
  ~CollisionResource() { delete[] liveBoundWords; }
  // Live flags of the default instance: what every trace and query honours;
  // CollisionNode::behaviorFlags keeps the authored value. Out of range reads as
  // no flags set (dev asserts) - the skip answer.
  uint16_t getNodeBehaviorFlags(int node_index) const { return defaultInstance.getNodeBehaviorFlags(node_index); }
  bool checkNodeBehaviorFlags(int node_index, uint16_t f) const { return defaultInstance.checkNodeBehaviorFlags(node_index, f); }
  // Writes the LIVE flags (mutable half, legal on a shared Data block); false on a bad
  // index. Ordering and the racing-trace contract: see liveFlagsGen.
  bool setNodeBehaviorFlags(int node_index, uint16_t flags);
  // The bind claim of one node: false (logerr) on a shared Data block, false on a bad index.
  bool setNodeGeomNodeId(int node_index, dag::Index16 id);
  // Shares the immutable Data with the source (a per-entity copy is pose-sized) and takes its
  // current pose.
  CollisionResource *deepCopy(void *inplace_mem_ptr = nullptr) const;
  // The pack resource name, for dev diagnostics: "<null>" when not loaded from a pack, "unknown" in a dynamic-kernel build;
  // a release build keeps no id and answers an empty name.
  String getGameResName() const;

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
  // The resource (and the tree, when bound) must outlive each instance. A bound tree makes
  // the instance tree-backed: no matrices are stored, poses are read live from the tree and
  // derived state refreshes lazily at trace time. Bind after all node-list mutations and
  // rebind whenever the tree is recreated.
  CollisionResourceInstancePtr createInstance(const GeomNodeTree *tree = nullptr) const;
  // Rebind externally owned storage and seed it from the current pose.
  void initInstance(CollisionResourceInstance & inst, const GeomNodeTree *tree = nullptr) const;

  // Block-interior views: this span and every pointer the read accessors hand out (getNode,
  // getNodeByName, getNodeNameStr, nodeChunkPtr, nodeVerts21Ptr, getNodeOccluderBlas) live as long
  // as the resource holds its block; only a tree bind with a foreign layout on a shared block moves
  // the resource to a private clone (initializeWithGeomNodeTree, and stampGeomNodeIds directly,
  // which a prefixed bind reaches).
  dag::Span<CollisionNode> getAllNodes() { return data->allNodesList(); }
  dag::ConstSpan<CollisionNode> getAllNodes() const { return data->allNodesList(); }

  static CollisionResource *loadResource(IGenLoad & crd, int res_id);
  // The body of the current stream (engine/sharedInclude/gameRes/collResStream.h); the caller wraps it in the label
  // and the block. mat_name maps a material id to its persisted name (null: refused). False, the cause logged,
  // when the source cannot be persisted (a geometry node without its chunk, a block past the loader's cap, a
  // chunk the serializer refuses, a malformed material pool or bind meta, a material without a name).
  bool write(IGenSave & cwr, const char *(*mat_name)(int id)) const;

  // Every triangle of the chunk-backed mesh/convex nodes (the mesh list) whose bbox
  // intersects `box`, in resource space with the source winding, plus the face's material
  // (per-leaf for a multi-material node; PHYSMAT_INVALID when the node has no material or the
  // palette entry is the no-material one; an unstamped leaf answers set index 0).
  // Verts are the chunk's vert21 decode. Return true from the visitor to stop; the call answers
  // whether it was stopped. behavior_filter gates nodes on their LIVE behavior flags, like every
  // other query walk (0 = every node).
  // Bind-frame query: only IDENT-class nodes contribute (others skip with a one-time logerr).
  // A non-finite box answers false up front.
  // The node enumeration prunes through the default instance's TLAS clone when one is
  // materialized and current (a pose write materializes it); a never-posed resource, no TLAS, or
  // a stale clone walks the mesh list linear.
  // Both arms visit in ascending node index (candidates are sorted to the linear list's order),
  // so an early-stopping visitor diverges only through the arms' node sets: a node the TLAS
  // holds no leaf for is enumerated (and its non-IDENT skip logerr fired) by the linear arm alone.
  // The leaf prune carries a one-quant-unit pad, so a box-touching triangle is never missed;
  // each visited triangle's own decoded bbox intersects the box.
  // node_index is the node's allNodesList position: getAllNodes()[node_index] is the node.
  using BoxTriVisitor = dag::FunctionRef<bool(vec3f v0, vec3f v1, vec3f v2, int phys_mat_id, int node_index) const>;
  bool visitTrianglesInBox(bbox3f_cref box, uint16_t behavior_filter, BoxTriVisitor visitor) const;

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
  const char *getNodeNameStr(const CollisionNode &n) const { return data->names().empty() ? "" : data->names().data() + n.nameOfs; }
  // Current default node placement, returned by value and shared by tree-less APIs.
  TMatrix getNodeTm(int node_id) const;
  // Replaces the SHARED default placement (every no-instance trace sees it); per-entity poses
  // belong on an instance (updateNodeTm). Singular/mirrored live poses become untraceable, and
  // posing any node latches isPosedSinceBind, dropping bind-only shortcuts.
  void setNodeTm(int node_id, const TMatrix &new_tm);

  using TraceTmCache = CollResTraceTmCache; // a caller-owned entry for a run of traces on one basis

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

  // force_cull culls CCW even on a TRACE_TWO_SIDED node, for a caller whose query has a side.
  bool traceRay(const mat44f &tm, const Point3 &from, const Point3 &dir, float &in_out_t, Point3 *out_normal, int &out_mat_id,
    int ray_mat_id = -1, uint8_t behavior_filter = CollisionNode::TRACEABLE, TraceTmCache *tm_cache = nullptr, bool force_cull = false)
    const;

  bool traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter = CollisionNode::TRACEABLE,
    const CollisionNodeMask *collision_node_mask = nullptr, bool force_no_cull = false, TraceTmCache *tm_cache = nullptr) const
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f tm;
    v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
    return traceRay(tm, geom_node_tree, from, dir, in_t, intersected_nodes_list, sort_intersections, behavior_filter,
      collision_node_mask, force_no_cull, tm_cache);
  }

  bool traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter = CollisionNode::TRACEABLE,
    const CollisionNodeMask *collision_node_mask = nullptr, bool force_no_cull = false, TraceTmCache *tm_cache = nullptr) const;

  bool traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeFilter &filter) const;

  bool traceRay(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeMask &collision_node_mask,
    TraceCollisionResourceStats *out_stats, TraceTmCache *tm_cache = nullptr) const;

  bool traceRay(const mat44f &tm, const GeomNodeTree *geom_node_tree, vec3f from, vec3f dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeMask &collision_node_mask,
    TraceCollisionResourceStats *out_stats, TraceTmCache *tm_cache = nullptr) const;

  // Explicit-instance twins of the transitional GeomNodeTree overloads.
  // Foreign or stale instances assert and fall back to the current pose.
  bool traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float &in_out_t, Point3 *out_normal, int &out_mat_id, int &out_node_id) const;

  bool traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float &in_out_t, Point3 *out_normal = nullptr) const
  {
    int outMatId, outNodeId;
    return traceRay(instance_tm, instance, from, dir, in_out_t, out_normal, outMatId, outNodeId);
  }

  bool traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float &in_out_t, Point3 *out_normal, int &out_mat_id, const CollisionNodeFilter &filter, int ray_mat_id = -1,
    uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
    uint8_t behavior_filter = CollisionNode::TRACEABLE, const CollisionNodeMask *collision_node_mask = nullptr,
    bool force_no_cull = false) const
  {
    alignas(EA_CACHE_LINE_SIZE) mat44f tm;
    v_mat44_make_from_43cu_unsafe(tm, instance_tm.array);
    return traceRay(tm, instance, from, dir, in_t, intersected_nodes_list, sort_intersections, behavior_filter, collision_node_mask,
      force_no_cull);
  }

  bool traceRay(const mat44f &tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, uint8_t behavior_filter = CollisionNode::TRACEABLE,
    const CollisionNodeMask *collision_node_mask = nullptr, bool force_no_cull = false) const;

  bool traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeFilter &filter) const;

  bool traceRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float in_t, CollResIntersectionsType &intersected_nodes_list, bool sort_intersections,
    const CollisionNodeMask &collision_node_mask, TraceCollisionResourceStats *out_stats) const;

  bool traceRay(const mat44f &tm, const CollisionResourceInstance &instance, vec3f from, vec3f dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, const CollisionNodeMask &collision_node_mask,
    TraceCollisionResourceStats *out_stats) const;

  bool traceCapsule(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float &in_out_t, float radius, Point3 &out_normal, Point3 &out_pos, int &out_mat_id) const;

  bool traceCapsule(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale, const CollisionNodeFilter &filter,
    const uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool traceCapsule(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float in_t, float radius, IntersectedNode &intersected_node, float bsphere_scale = 1.f,
    const uint8_t behavior_filter = CollisionNode::TRACEABLE) const;

  bool capsuleHit(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir,
    float in_t, float radius, CollResHitNodesType &nodes_hit) const;

  bool multiRayHit(const TMatrix &instance_tm, const CollisionResourceInstance &instance, dag::Span<CollisionTrace> traces) const;

  bool traceMultiRay(const TMatrix &instance_tm, const CollisionResourceInstance &instance, dag::Span<CollisionTrace> traces,
    MultirayCollResIntersectionsType &intersected_nodes_list, bool sort_intersections, float bsphere_scale = 1.f,
    uint8_t behavior_filter = CollisionNode::TRACEABLE, const CollisionNodeMask *collision_node_mask = nullptr,
    TraceCollisionResourceStats *out_stats = nullptr) const;

  // rayHit material-id conventions differ by overload family and are kept for source
  // compatibility: int& forms always write it, int* forms only through a non-null pointer.
  bool rayHit(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const Point3 &from, const Point3 &dir, float in_t,
    float bsphere_scale = 1.f, const CollisionNodeMask *collision_node_mask = nullptr, int *out_mat_id = nullptr) const;

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

  // Don't use it! It's should not be external. `node` must be the live allNodesList entry (its
  // geometry is read straight from it); TRACEABLE comes from the resource's live flags (this
  // closest form never reads SOLID; the all-hits form takes force_no_cull from its caller).
  bool traceRayMeshNodeLocal(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t,
    vec4f *out_norm) const;

  bool traceRayMeshNodeLocalAllHits(const CollisionNode &node, const Point3 &from, const Point3 &dir, float in_t,
    CollResIntersectionsType &intersected_nodes_list, bool sort_intersections, bool force_no_cull = false) const;

  bool rayHit(const mat44f &tm, const Point3 &from, const Point3 &dir, float in_t, int ray_mat_id, int &out_mat_id,
    uint8_t behavior_filter = CollisionNode::TRACEABLE, TraceTmCache *tm_cache = nullptr) const;

  bool rayHit(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir, float in_t,
    float bsphere_scale = 1.f, const CollisionNodeMask *collision_node_mask = nullptr, int *out_mat_id = nullptr,
    TraceTmCache *tm_cache = nullptr) const;

  bool rayHitHullBothWays(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir,
    float in_t) const
  {
    return rayHit(instance_tm, geom_node_tree, from, dir, in_t) || rayHit(instance_tm, geom_node_tree, from + dir * in_t, -dir, in_t);
  }

  bool rayHitHullBothWays(const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, const Point3 &from, const Point3 &dir) const
  {
    const float maxScale = max(length(instance_tm.getcol(0)), max(length(instance_tm.getcol(1)), length(instance_tm.getcol(2))));
    const float posedScale = geom_node_tree ? sqrtf(3.f) : 1.f;
    const float t =
      length(getWorldBoundingSphere(instance_tm, geom_node_tree) - from) + getBoundingSphereRad() * maxScale * posedScale;
    return rayHitHullBothWays(instance_tm, geom_node_tree, from, dir, t);
  }

  VECTORCALL bool traceQuad(vec3f a00, vec3f a01, vec3f a10, vec3f a11, Point3 & out_point, int &out_node_index) const;

  struct DebugDrawData
  {
    // Used only by the GeomNodeTree form.
    bool localNodeTree;
    bool shouldDrawText;
    E3DCOLOR color;
    uint16_t drawBits;
    // GeomNodeTree-only bounding-sphere center override.
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
  // Foreign/stale instance: assert in dev; log and draw the bind pose in release.
  void drawDebug(const TMatrix &instance_tm, const CollisionResourceInstance &instance, const DebugDrawData &data = DebugDrawData())
    const;

  static void registerFactory();

  // Tree-less forms use both resources' current poses.
  static bool testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionNodeFilter &filter1,
    const CollisionResource *res2, const TMatrix &tm2, const CollisionNodeFilter &filter2, Point3 &collisionPoint1,
    Point3 &collisionPoint2, uint16_t *nodeIndex1 = NULL, uint16_t *nodeIndex2 = NULL, CollResNodeIndicesType *node_indices1 = NULL);
  static bool testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionNodeFilter &filter1,
    const CollisionResourceInstance &instance1, const CollisionResource *res2, const TMatrix &tm2, const CollisionNodeFilter &filter2,
    const CollisionResourceInstance &instance2, Point3 &collisionPoint1, Point3 &collisionPoint2, uint16_t *nodeIndex1 = NULL,
    uint16_t *nodeIndex2 = NULL, CollResNodeIndicesType *node_indices1 = NULL);
  // This function is more readable and contains new primitives for checking.
  // It does full dispatching for collision primitives.
  // This eliminetes the ordering problem:
  // the function will find an intersection regardless of res1 and res2 order.
  // Only MESH/BOX/SPHERE nodes participate; CAPSULE and CONVEX nodes are not tested.
  // Posed instances derive the whole-pair reject from rootBBox. Foreign or stale instances
  // assert and fall back to the current pose; disabled and untraceable nodes are skipped
  // in every node loop, matching trace dispatch.
  static bool testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionResource *res2, const TMatrix &tm2,
    Point3 &collisionPoint1, Point3 &collisionPoint2, bool checkOnlyPhysNodes = false);
  static bool testIntersection(const CollisionResource *res1, const TMatrix &tm1, const CollisionResourceInstance &instance1,
    const CollisionResource *res2, const TMatrix &tm2, const CollisionResourceInstance &instance2, Point3 &collisionPoint1,
    Point3 &collisionPoint2, bool checkOnlyPhysNodes = false);

  // The result list is an UNORDERED set of the intersected nodes (the enumeration arm decides
  // the order); callers must not read meaning into it.
  bool checkInclusion(const Point3 &pos, CollResIntersectionsType &intersected_nodes_list) const;
  // Instance form: nodes test at their instance pose (disabled/untraceable nodes never report).
  bool checkInclusion(const Point3 &pos, CollResIntersectionsType &intersected_nodes_list, const CollisionResourceInstance &instance)
    const;

private:
  // Shared geometry-frame tail of the testInclusion overloads.
  bool testNodeInclusionInConvex(const CollisionNode &node, const TMatrix &test_tm, dag::ConstSpan<plane3f> convex,
    const TMatrix &tm_restrain, Point3 *res_pos) const;

  // No callers anywhere in the tree; scheduled for deletion.
  // Can check mesh-box, box-box and sph-box only, asserts on any other types
  bool calcOffsetForIntersection(const TMatrix &tm1, const CollisionNode &node_to_move, const CollisionNode &node_to_check,
    Point3 &offset) const;

  bool calcOffsetForSeparation(const TMatrix &tm1, const CollisionNode &node_to_move, const CollisionNode &node_to_check,
    const Point3 &axis, Point3 &offset) const;

public:
  // Every instance-taking test/inclusion/clip entry below skips disabled (setNodeEnabled)
  // and untraceable (singular or mirrored-pose) nodes, matching trace dispatch.
  // The probe belongs to this resource; the restraining convex may belong to another.
  bool testInclusion(int test_node_index, const TMatrix &tm_test, const CollisionResource *restraining_resource,
    int restraining_node_index, const TMatrix &tm_restrain, const GeomNodeTree *test_node_tree = NULL,
    const GeomNodeTree *restrain_node_tree = NULL) const;

  bool testInclusion(int test_node_index, const TMatrix &tm_test, dag::ConstSpan<plane3f> convex, const TMatrix &tm_restrain,
    const GeomNodeTree *test_node_tree = NULL, Point3 *res_pos = nullptr) const;

  // Instance forms pose the probe and restraining convex independently.
  bool testInclusion(int test_node_index, const TMatrix &tm_test, const CollisionResource *restraining_resource,
    int restraining_node_index, const TMatrix &tm_restrain, const CollisionResourceInstance &test_instance,
    const CollisionResourceInstance &restrain_instance) const;

  bool testInclusion(int test_node_index, const TMatrix &tm_test, dag::ConstSpan<plane3f> convex, const TMatrix &tm_restrain,
    const CollisionResourceInstance &test_instance, Point3 *res_pos = nullptr) const;

  bool testSphereIntersection(const CollisionNodeFilter &filter, const BSphere3 &sphere, const Point3 &dir_norm, Point3 &out_norm,
    float &out_depth, int &out_node_id) const;
  // Penetration depth is exact under uniform posed scale; a non-uniform pose converts the local
  // depth by the pose's stretch along the contact normal (hits are never missed; the conservative
  // probe inflation can overshoot the reported depth slightly).
  bool testSphereIntersection(const CollisionNodeFilter &filter, const BSphere3 &sphere, const Point3 &dir_norm, Point3 &out_norm,
    float &out_depth, int &out_node_id, const CollisionResourceInstance &instance) const;
  bool testCapsuleNodeIntersection(const Point3 &p0, const Point3 &p1, float radius) const;
  bool testCapsuleNodeIntersection(const Point3 &p0, const Point3 &p1, float radius, const CollisionResourceInstance &instance) const;

  // One GeomNodeTree layout per shared Data block, enforced here: the first binder
  // claims the block, and a binder with a different layout takes a private copy first.
  void initializeWithGeomNodeTree(const GeomNodeTree &geom_node_tree);
  // The stamp alone under the same one-layout policy, without binding this resource. name_prefix
  // prepends to every node name at the tree lookup (attachable models stamp through their
  // attachment prefix).
  void stampGeomNodeIds(const GeomNodeTree &geom_node_tree, const char *name_prefix);
  // Caller-migration surface: adopted when the legacy tree trace API is dropped.
  bool isGeomNodeTreeBound() const { return geomNodeTreeBound; }

  void getCollisionNodeTm(const CollisionNode *node, const TMatrix &instance_tm, const GeomNodeTree *geom_node_tree, TMatrix &out_tm)
    const;
  void getCollisionNodeTm(const CollisionNode *node, mat44f_cref instance_tm, const GeomNodeTree *geom_node_tree, mat44f &out_tm)
    const;
  // Instance forms return instance_tm * node pose for every node type. Foreign or stale
  // instances assert and fall back to the current pose. Exception: a tree-backed instance's
  // driven node returns the tree wtm with instance_tm NOT composed (the tree already carries
  // the entity placement); EntityCollTrace relies on this.
  void getCollisionNodeTm(const CollisionNode *node, const TMatrix &instance_tm, const CollisionResourceInstance &instance,
    TMatrix &out_tm) const;
  void getCollisionNodeTm(const CollisionNode *node, mat44f_cref instance_tm, const CollisionResourceInstance &instance,
    mat44f &out_tm) const;

  void clipCapsule(const TMatrix &instance_tm, const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized);
  // No-instance form clips against the current pose.
  void clipCapsule(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized);
  // Foreign or stale instances assert and fall back to the current pose.
  void clipCapsule(const Capsule &c, Point3 &cp1, Point3 &cp2, real &md, const Point3 &movedirNormalized,
    const CollisionResourceInstance &instance);

  bool test_sphere_node_intersection(const BSphere3 &sphere, const CollisionNode *node, const Point3 &dir_norm, Point3 &out_norm,
    float &out_depth) const;
  bool test_sphere_node_intersection(const BSphere3 &sphere, const CollisionNode *node, const CollisionResourceInstance &instance,
    const Point3 &dir_norm, Point3 &out_norm, float &out_depth) const;
  bool test_capsule_node_intersection(const Point3 &p0, const Point3 &p1, float radius, const CollisionNode *node) const;
  bool test_capsule_node_intersection(const Point3 &p0, const Point3 &p1, float radius, const CollisionNode *node,
    const CollisionResourceInstance &instance) const;

  template <typename Func, CollisionResourceNodeType node_type = COLLISION_NODE_TYPE_MESH, bool binded_to_gntree = true>
  void visitCollisionNodes(const Func &func) const
  {
    if (node_type != NUM_COLLISION_NODE_TYPES)
    {
      for (uint16_t i : nodeList(node_type))
      {
        const CollisionNode &node = data->allNodesList()[i];
        if (!binded_to_gntree || node.geomNodeId)
          func(node);
      }
    }
    else
      for (const CollisionNode &node : data->allNodesList())
        if (!binded_to_gntree || node.geomNodeId)
          func(node);
  }

  uint32_t getCollisionFlags() const { return collisionFlags; }

  // ===== node material sets =====
  // physMatId's SIGN discriminates: >= 0 one inline material, -1 none, <= -2 a slice offset (-physMatId - 2) into physMatPool holding
  // [count][id x count], coverage-ordered (dominant first) and deduped by content. A leaf's 6 user bits index its OWNING node's set.
  static constexpr uint32_t MAX_NODE_PHYS_MATS = QUAD_LEAF_USER_MASK + 1;

  // ===== material authority =====
  // True: the node's own field answers (one material or none). False: per-face questions need the leaf.
  // Consumers read through the accessors below, never the raw field.
  bool isNodeOwnMaterialAuthority(const CollisionNode &node) const { return node.physMatId >= PHYSMAT_INVALID; }

  // One of a node's materials by palette_index (a traced hit's leaf user bits).
  // Bounds-checked against the pool itself; an out-of-range index or unknown node answers PHYSMAT_INVALID.
  int getNodePhysMatId(int node_index, int palette_index) const
  {
    const CollisionNode *n = getNode(node_index);
    if (!n)
      return PHYSMAT_INVALID;
    if (n->physMatId >= 0)
      return palette_index == 0 ? (int)n->physMatId : PHYSMAT_INVALID;
    if (n->physMatId == PHYSMAT_INVALID)
      return PHYSMAT_INVALID;
    return physMatFromNodeSlice(n->physMatId, palette_index);
  }
  // 0 none, 1 inline, else the slice size; pairs with getNodePhysMatId over 0..count-1.
  int getNodePhysMatCount(int node_index) const
  {
    const CollisionNode *n = getNode(node_index);
    if (!n || n->physMatId == PHYSMAT_INVALID)
      return 0;
    if (n->physMatId >= 0)
      return 1;
    const uint32_t ofs = nodePhysMatSliceOfs(n->physMatId);
    if (ofs >= data->physMatPool().size())
      return 0;
    // Clamp to the entries after the offset and to the leaf field's width: the stored count is trustworthy only at a real slice start.
    uint32_t count = data->physMatPool()[ofs];
    const uint32_t tail = (uint32_t)data->physMatPool().size() - ofs - 1u;
    if (count > tail)
      count = tail;
    if (count > MAX_NODE_PHYS_MATS)
      count = MAX_NODE_PHYS_MATS;
    return (int)count;
  }

  // The one dispatch home over a node's materials: own-authority answers its single material
  // (a material-less node asks PHYSMAT_INVALID); a set-holding node iterates its slice. A forged
  // or clamped-empty slice walks nothing: a corrupt physMatId excludes the node from filters.
  template <class Body> // bool(int phys_mat_id): return false to stop the walk
  void visitNodeMaterials(const CollisionNode &node, Body body) const
  {
    if (isNodeOwnMaterialAuthority(node))
    {
      body(getNodePhysMatId(node.nodeIndex, 0));
      return;
    }
    for (int i = 0, e = getNodePhysMatCount(node.nodeIndex); i < e; ++i)
      if (!body(getNodePhysMatId(node.nodeIndex, i)))
        return;
  }

  // Whole-node early-out: could ANY face of this node pass the predicate?
  // A first pass, not a verdict: an accepted node still holds the failing materials' faces, so the trace re-asks per leaf.
  template <class Pred> // bool(int phys_mat_id)
  bool anyNodeMaterialPasses(const CollisionNode &node, Pred pred) const
  {
    bool any = false;
    visitNodeMaterials(node, [&](int mat_id) {
      any = pred(mat_id);
      return !any;
    });
    return any;
  }

  // Both verdicts of a node's material set in one walk: any_pass = some material passes pred
  // (node-level eligibility), all_pass = none fails (a filter carves nothing).
  template <class Pred>
  void classifyNodeMaterials(const CollisionNode &node, Pred pred, bool &any_pass, bool &all_pass) const
  {
    any_pass = false;
    all_pass = true;
    visitNodeMaterials(node, [&](int mat_id) {
      if (pred(mat_id))
        any_pass = true;
      else
        all_pass = false;
      return !any_pass || all_pass; // stop once both verdicts are settled
    });
  }

  // Material of a traced hit.
  // A set-holding node answers its face's own material only from a STAMPED leaf; a non-triangle hit and an unstamped leaf both read
  // index 0 (the dominant, not a guarantee), and a source-face tri ref answers PHYSMAT_INVALID -- see getLeafPhysMat for which leaves
  // carry a stamp. Not for a rebased ref: its token addresses another resource's tree.
  int getHitPhysMat(tri_ref_t ref) const
  {
    const int nodeIndex = (int)tri_ref::nodeIndex(ref);
    const CollisionNode *n = getNode(nodeIndex);
    if (!n)
      return PHYSMAT_INVALID;
    if (isNodeOwnMaterialAuthority(*n) || !tri_ref::hasTri(ref))
      return getNodePhysMatId(nodeIndex, 0);
    return getLeafPhysMat(ref);
  }
  // The hit leaf's own material: user bits resolved through the OWNING node's set (the ref carries the node index).
  // PHYSMAT_INVALID when the ref addresses no leaf. An unstamped leaf reads 0 and answers index 0, whatever the set.
  // Not for a rebased ref: foreign token, undetectably.
  int getLeafPhysMat(tri_ref_t ref) const;

  // Mesh node iteration helpers (abstracts away the per-type index slice + raw vertex/index access)
  int getMeshNodeCount() const { return (int)meshNodes().size(); }

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

  // Three vertices of a node's source face by per-node face index, decoded from the node's
  // chunk vert21 block.
  // False if face_idx is out of range or the node has no triangles.
  bool getNodeFaceVerts(int node_id, int face_idx, Point3 &v0, Point3 &v1, Point3 &v2) const;

  // Decode a tri_ref_t back to its source triangle's three verts. Per-node BLAS refs walk the
  // chunk's quad leaf at the token and rebuild the sub_tri-selected triangle from vert21;
  // non-BLAS refs (type=0) dispatch to getNodeFaceVerts on the encoded srcFace; a retired
  // grid-era ref fails safe.
  // A ref names a leaf of THIS resource's chunks; one minted on another resource (a rebuilt copy)
  // is foreign, undetectably. Not for persistence.
  bool getNodeFaceVertsByRef(tri_ref_t ref, Point3 & v0, Point3 & v1, Point3 & v2) const;

  // vert21 streams are 8-byte aligned (nodeVerts21Ptr / buildOneNodeBlasChunk): the tree region
  // is padded up to 8 before the stream so a byte-offset/8 index recovery is always exact.
  static constexpr uint32_t alignVert21StreamOfs(uint32_t tree_bytes) { return (tree_bytes + 7u) & ~7u; }
  // Read-only decode view over a node's per-node vert21 stream. The pointers are resource-stable
  // (the chunks never move), which is what lets async consumers (SW occluder tasks) reference the
  // stream directly. vert(i) decodes through Vert21MetricLoader.
  struct PackedVerts21
  {
    const uint8_t *verts21; // node's vert21 stream (8 B/vert, verticesCount entries)
    vec3f invScale;         // per-axis decode scale; w lane is undefined
    vec3f bmin;             // quantization frame origin (node-slice bbox min, stored space); w lane undefined
    vec3f vert(uint32_t i) const
    {
      Vert21MetricLoader unq;
      unq.invScale = invScale;
      unq.bmin = bmin;
      return unq(verts21, 0, (int)i);
    }
  };
  // Per-node BLAS chunk prologue (chunk = [header][SoA4 tree][pad to 8][vert21 stream]). `scale` is
  // the EXACT pack scale the stream was quantized with (not rcp(invScale)), so the trace ray transform
  // into q-space reproduces the build frame bit-for-bit; invScale and bmin are the decode frame. A
  // multiple of 8, so the stream past the 8-padded tree stays 8-aligned.
  struct NodeBlasChunkHeader // -V730 (filled field-by-field by the chunk builder)
  {
    float scale[3];
    float invScale[3];
    float bmin[3];
    uint32_t treeBytes;    // SoA4 tree size; the vert21 stream follows at alignVert21StreamOfs(treeBytes)
    soa4::RootRef rootRef; // SoA4 tree root, relative to the tree base right after this header
    // bit 0: the tree carries the edge flags words (stamped from PHYS_COLLIDABLE at chunk build; a
    // post-load behavior mutation cannot change the layout).
    static constexpr uint32_t HAS_EDGE_FLAGS = 1;
    uint32_t flags;

    vec3f scaleV() const { return v_ldu(scale); }
    vec3f invScaleV() const { return v_ldu(invScale); }
    vec3f bminV() const { return v_perm_xyzd(v_ldu(bmin), v_zero()); } // 4th lane would be treeBytes, a denormal
  };
  // The one home of a chunk's extent: [header][tree padded to 8][vert21s].
  static size_t nodeChunkBytes(const NodeBlasChunkHeader *hdr, uint32_t vert_count)
  {
    return sizeof(NodeBlasChunkHeader) + alignVert21StreamOfs(hdr->treeBytes) + (size_t)vert_count * BVH_BLAS_VERT21_STRIDE;
  }

  // Vert21 stream base: every geometry-holding node carries its stream inside its per-node BLAS
  // chunk tail.
  const uint8_t *nodeVerts21Ptr(const CollisionNode &node) const
  {
    // Degenerate nodes are dropped to indicesCount == 0 and never reach here.
    G_ASSERT(node.nodeBlasOfs != ~0u);
    const uint8_t *chunk = data->nodeBlasData().data() + node.nodeBlasOfs;
    return chunk + sizeof(NodeBlasChunkHeader) + alignVert21StreamOfs(((const NodeBlasChunkHeader *)chunk)->treeBytes);
  }

  PackedVerts21 getPackedNodeVerts21(const CollisionNode &node) const
  {
    const NodeBlasChunkHeader *hdr = (const NodeBlasChunkHeader *)(data->nodeBlasData().data() + node.nodeBlasOfs);
    PackedVerts21 r;
    r.bmin = hdr->bminV();
    r.invScale = hdr->invScaleV();
    r.verts21 = nodeVerts21Ptr(node);
    return r;
  }

  bool hasNodeBlas(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n && n->nodeBlasOfs != ~0u;
  }

  // SW occluder feed: the RenderBlasSOA4 chunk params (tree base, root ref, vert21 stream byte
  // offset) plus the node's decode frame (bmin/invScale) in a single chunk resolution.
  // vertOffset is 8-aligned (the tree padded to 8), so the walker's (apexByteOfs - vertOffset)/STRIDE
  // recovers node-local vert indices. Node must have a chunk (nodeBlasOfs != ~0u).
  struct NodeOccluderBlas
  {
    const uint8_t *blasData;
    soa4::RootRef rootRef;
    uint32_t vertOffset;
    vec3f invScale;
    vec3f bmin; // w lanes undefined
  };
  // The one home of the chunk-storage binding: a node's [NodeBlasChunkHeader][tree][vert21] base,
  // null when the node has no chunk (degenerate).
  const uint8_t *nodeChunkPtr(const CollisionNode &n) const
  {
    return n.nodeBlasOfs != ~0u ? data->nodeBlasData().data() + n.nodeBlasOfs : nullptr;
  }
  // blasData == nullptr when the node has no chunk: the accessor owns that check, so callers
  // need no separate hasNodeBlas gate.
  NodeOccluderBlas getNodeOccluderBlas(const CollisionNode &node) const
  {
    const uint8_t *chunk = nodeChunkPtr(node);
    if (!chunk)
      return NodeOccluderBlas{};
    const NodeBlasChunkHeader *hdr = (const NodeBlasChunkHeader *)chunk;
    NodeOccluderBlas r;
    r.blasData = chunk + sizeof(NodeBlasChunkHeader);
    r.rootRef = hdr->rootRef;
    r.vertOffset = alignVert21StreamOfs(hdr->treeBytes);
    r.bmin = hdr->bminV();
    r.invScale = hdr->invScaleV();
    return r;
  }

  template <class CB> // void(int face_idx, uint32_t i0, uint32_t i1, uint32_t i2)
  void iterateNodeFaces(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    walkNodeChunkLeavesForFaces(*n, cb); // owning mode: faces live in the per-node chunk tree
  }

  // Every face with the material of THAT face: an own-authority node answers from the node (its leaves read 0 anyway), a set-holding
  // node from the leaf. Total on purpose: silence must never read as "this node has no faces".
  template <class CB> // void(int face_idx, uint32_t i0, uint32_t i1, uint32_t i2, int phys_mat_id)
  void iterateNodeFacesWithLeafMaterial(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    if (!isNodeOwnMaterialAuthority(*n))
    {
      auto perLeaf = [&](int fi, uint32_t i0, uint32_t i1, uint32_t i2, uint32_t user) {
        cb(fi, i0, i1, i2, getNodePhysMatId(node_id, (int)user));
      };
      walkNodeChunkLeavesForFaces</*WithUser*/ true>(*n, perLeaf);
      return;
    }
    const int nodeMat = getNodePhysMatId(node_id, 0);
    iterateNodeFaces(node_id, [&](int fi, uint32_t i0, uint32_t i1, uint32_t i2) { cb(fi, i0, i1, i2, nodeMat); });
  }

  // ===== material-filtered face enumeration =====
  // Count sizes a consumer's buffer, iterate fills it, so both MUST select the same faces. An
  // own-authority node answers from its face count with one predicate call; a set-holding node walks
  // its leaves per face. Verts are not filtered: an unindexed vert just sits unused in the buffer.

  template <class Pred> // bool(int phys_mat_id)
  uint32_t countNodeFacesByMaterial(int node_id, Pred pred) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || !n->hasGeometry())
      return 0;
    if (isNodeOwnMaterialAuthority(*n))
      return pred(getNodePhysMatId(node_id, 0)) ? n->indicesCount / 3u : 0u;
    uint32_t faces = 0;
    iterateNodeFacesWithLeafMaterial(node_id, [&](int, uint32_t, uint32_t, uint32_t, int mat) { faces += pred(mat) ? 1u : 0u; });
    return faces;
  }

  // face_idx keeps the unfiltered iterateNodeFaces numbering, so a caller can cross-reference the two walks; only the non-passing
  // faces are left out.
  template <class Pred, class CB> // bool(int phys_mat_id); void(int face_idx, uint32_t i0, uint32_t i1, uint32_t i2)
  void iterateNodeFacesByMaterial(int node_id, Pred pred, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || !n->hasGeometry())
      return;
    if (isNodeOwnMaterialAuthority(*n))
    {
      if (pred(getNodePhysMatId(node_id, 0)))
        iterateNodeFaces(node_id, cb);
      return;
    }
    iterateNodeFacesWithLeafMaterial(node_id, [&](int fi, uint32_t i0, uint32_t i1, uint32_t i2, int mat) {
      if (pred(mat))
        cb(fi, i0, i1, i2);
    });
  }

  template <class CB> // void(int vert_idx, vec4f v)
  void iterateNodeVerts(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || !n->hasGeometry())
      return;
    const uint32_t vertCount = (uint32_t)n->verticesCount;
    // owning mode: verts live in the node's per-node BLAS chunk (vert21 block)
    const PackedVerts21 p = getPackedNodeVerts21(*n);
    for (uint32_t i = 0; i < vertCount; ++i)
      cb((int)i, p.vert(i));
  }

  template <class CB> // void(int face_idx, vec4f v0, vec4f v1, vec4f v2)
  void iterateNodeFacesVerts(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n || !n->hasGeometry())
      return; // degenerate-dropped node: no per-node block to decode (mirrors iterateNodeVerts)
    // owning mode: faces + verts both live in the per-node chunk (tree gives the face list, the
    // vert21 stream the positions)
    const PackedVerts21 p = getPackedNodeVerts21(*n);
    walkNodeChunkLeavesForFaces(*n,
      [&](int fi, uint32_t i0, uint32_t i1, uint32_t i2) { cb(fi, p.vert(i0), p.vert(i1), p.vert(i2)); });
  }


  // Walks the node's OWN per-node BLAS chunk tree (single node: every leaf is this
  // node's). Emits cb(face_idx, i0, i1, i2) per sub-triangle in DFS order; local indices are
  // [0, verticesCount) over the chunk's vert21 stream (getPackedNodeVerts21), which sits past the
  // 8-padded tree, so the leaf's vertBytesOfs rebases against alignVert21StreamOfs(treeBytes).
  template <bool WithUser = false, class CB> // void(int face_idx, uint32_t i0, uint32_t i1, uint32_t i2 [, uint32_t leaf_user])
  void walkNodeChunkLeavesForFaces(const CollisionNode &node, CB cb) const
  {
    if (node.nodeBlasOfs == ~0u)
      return; // degenerate-dropped node: no chunk
    const uint8_t *chunk = data->nodeBlasData().data() + node.nodeBlasOfs;
    const NodeBlasChunkHeader *hdr = (const NodeBlasChunkHeader *)chunk;
    const uint8_t *tree = chunk + sizeof(NodeBlasChunkHeader);
    const uint32_t vertsRel = alignVert21StreamOfs(hdr->treeBytes);
    int fi = 0;
    // Single node's tree: every leaf is this node's, so no NodeRange filter and an always-true node
    // test. iterateLeafRefs visits each leaf once; decode + emit its sub-tris via the shared
    // double-quad authority (leafFields / expandQuadLeafTris).
    soa4::iterateLeafRefs(
      tree, hdr->rootRef, [](vec3f, vec3f) { return true; },
      [&](vec3f, vec3f, soa4::LeafRef, const soa4::LeafLoc &l) -> bool {
        const QuadLeafFields f = soa4::leafFields(tree, l);
        const uint32_t baseLocal = ((uint32_t)l.bodyOfs + f.relBaseBytes - vertsRel) / BVH_BLAS_VERT21_STRIDE;
        expandQuadLeafTris(f, baseLocal, [&](uint32_t i0, uint32_t i1, uint32_t i2) {
          if constexpr (WithUser)
            cb(fi++, i0, i1, i2, (uint32_t)f.user);
          else
            cb(fi++, i0, i1, i2);
        });
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
    return dag::ConstSpan<plane3f>(data->convexPlanes().data() + n->planesOfs, n->planesCount);
  }

  template <class CB> // void(int plane_idx, plane3f plane)
  void iterateNodeConvexPlanes(int node_id, CB cb) const
  {
    const CollisionNode *n = getNode(node_id);
    if (!n)
      return;
    const plane3f *p = data->convexPlanes().data() + n->planesOfs;
    for (int i = 0, e = (int)n->planesCount; i < e; ++i)
      cb(i, p[i]);
  }

  // Historical per-type frames: BOX/SPHERE composed with the current pose (conservative
  // corner-mapped AABB for boxes, analytic for spheres); mesh/convex/capsule stay stored-space.
  // A stored-and-hidden unrealizable pose reports the stored value (never NaN bounds).
  // Composing getNodeTm on top of a BOX/SPHERE result re-applies the pose (as it always
  // double-applied the baked placement); compose it only over mesh/convex/capsule results.
  // STORED geometry -> the node-local frame bone/tree wtms bind to: inverse(authored) where
  // the loader kept the authored frame, identity otherwise (a singular bake stays raw).
  TMatrix getStoredToNodeLocalTm(int node_id) const;
  // Prefer the uniform-frame getNodeResourceBBox / getNodeGeometryBBox in new code.
  BBox3 getNodeBBox(int node_id) const;
  // The node's bounds in RESOURCE space per the default instance's CURRENT pose (legacy setNodeTm
  // writes land there); same contract as the instance overload above.
  BBox3 getNodeResourceBBox(int node_id) const { return defaultInstance.getNodeResourceBBox(node_id); }
  // Resource-space center of the node's bounds; a geometry-less or untraceable node
  // degenerates to the node placement (finite while the stored pose is).
  Point3 getNodeResourceCenter(int node_id) const;
  // The node's CURRENT placement classifies as identity: stored geometry is consumable without
  // composing the node tm. node.flags class bits are load-only; the live class follows the pose.
  bool isIdentNode(int node_id) const
  {
    return getNode(node_id) &&
           (defaultInstance.poseMeta[node_id].flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) == CollisionNode::IDENT;
  }
  // Stored-space bounds for every node type, with degenerate axes conservatively inflated.
  bbox3f getNodeGeometryBBox(const CollisionNode &node) const;
  // SPHERE composed with the pose: center mapped, radius widened by the conservative scale
  // stamp (1.0015 tolerance-volume bound for non-bit-exact eps classes, spectral * 1.0002 for
  // live poses, exact for bit-exact rigid bases).
  BSphere3 getNodeBSphere(int node_id) const;
  // The capsule at its CURRENT default placement (stored node-local capsule transformed by T).
  // Fails for the exporter's zero-vert marker (r < 0): there is no capsule to report.
  bool getNodeCapsule(int node_id, Capsule &out) const
  {
    const CollisionNode *n = getNode(node_id);
    if (n && n->type == COLLISION_NODE_TYPE_CAPSULE && n->radiusAroundBoxCenter >= 0.f)
    {
      out = getNodeCapsuleUnsafe(node_id);
      return true;
    }
    return false;
  }
  // Max scale of the node's CURRENT placement (see getNodeTm): the default PoseMeta value.
  float getNodeMaxTmScale(int node_id) const;

  // Source-compatible wrappers; only getNodeCapsuleUnsafe still indexes unchecked.
  TMatrix getNodeTmUnsafe(int node_id) const
  {
    G_ASSERT((uint32_t)node_id < data->allNodesList().size());
    return getNodeTm(node_id);
  }
  BBox3 getNodeBBoxUnsafe(int node_id) const
  {
    G_ASSERT((uint32_t)node_id < data->allNodesList().size());
    return getNodeBBox(node_id);
  }
  BSphere3 getNodeBSphereUnsafe(int node_id) const
  {
    G_ASSERT((uint32_t)node_id < data->allNodesList().size());
    return getNodeBSphere(node_id);
  }
  Point3 getNodeBSphereCenter(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? getNodeBSphere(node_id).c : Point3(0, 0, 0);
  }
  float getNodeBSphereRadius(int node_id) const
  {
    const CollisionNode *n = getNode(node_id);
    return n ? getNodeBSphere(node_id).r : -1.f;
  }
  Capsule getNodeCapsuleUnsafe(int node_id) const // by value: composed with the node's current default T
  {
    G_ASSERT((uint32_t)node_id < data->allNodesList().size());
    G_ASSERT(data->allNodesList()[node_id].type == COLLISION_NODE_TYPE_CAPSULE);
    Capsule c = data->capsules()[data->allNodesList()[node_id].capsuleIndex];
    // A stored-and-hidden unrealizable pose (see updateNodeTm) must not compose: report the
    // stored capsule instead of a NaN/collapsed one.
    if (DAGOR_UNLIKELY(!defaultInstance.isNodeComposable(node_id)))
      return c;
    const float storedR = c.r;
    c.transform(getNodeTm(node_id));
    // Capsule::transform scales r by |col0| alone; the trace tests the exact pose-mapped shape, so
    // a non-uniform pose needs the max scale to keep the reported capsule conservative.
    c.r = storedR * getNodeMaxTmScale(node_id);
    return c;
  }
  float getNodeMaxTmScaleUnsafe(int node_id) const
  {
    G_ASSERT((uint32_t)node_id < data->allNodesList().size());
    return getNodeMaxTmScale(node_id);
  }

  template <class CB> // void(const CollisionNode &node) or bool(const CollisionNode &node) - return true to stop
  void forEachMeshNode(CB cb) const
  {
    forEachNodeImpl(meshNodes(), cb);
  }
  template <class CB> // void(const CollisionNode &node) or bool(const CollisionNode &node) - return true to stop
  void forEachBoxNode(CB cb) const
  {
    forEachNodeImpl(boxNodes(), cb);
  }
  template <class CB> // void(const CollisionNode &node) or bool(const CollisionNode &node) - return true to stop
  void forEachSphereNode(CB cb) const
  {
    forEachNodeImpl(sphereNodes(), cb);
  }
  template <class CB> // void(const CollisionNode &node) or bool(const CollisionNode &node) - return true to stop
  void forEachCapsuleNode(CB cb) const
  {
    forEachNodeImpl(capsuleNodes(), cb);
  }

  bool getRelGeomNodeTms(int node_no, TMatrix &out_tm) const
  {
    if (node_no < 0 || node_no >= data->relGeomNodeTms().size())
      return false;
    out_tm = data->relGeomNodeTms()[node_no];
    return true;
  }

  // Caller-owned instances are not counted. A shared Data block reports a fair share
  // per holder (rounded up), so a sum over the sharers never under-counts the block.
  int getMemoryUsed() const;
  int getTrianglesCount(uint8_t behavior_filter) const;
  void setBsphereCenterNode(int ni) { bsphereCenterNode = dag::Index16(ni); }
  vec4f getWorldBoundingSphere(const mat44f &tm, const GeomNodeTree *geom_node_tree) const;
  Point3 getWorldBoundingSphere(const TMatrix &tm, const GeomNodeTree *geom_node_tree) const;
  // Instance forms preserve a selected tree-node center even without collision geometry.
  // Foreign or stale instances assert and fall back to the current pose.
  vec4f getWorldBoundingSphere(const mat44f &tm, const CollisionResourceInstance &instance) const;
  Point3 getWorldBoundingSphere(const TMatrix &tm, const CollisionResourceInstance &instance) const;
  // Jolt's per-node MeshShape degenerate check over the chunks' verts and faces: the exact values
  // every trimesh path hands Jolt.
  bool validateVerticesForJolt(const char *res_name, auto &&on_degenerate);
  bool validateVerticesForJolt(const char *res_name);
  dag::Vector<DegenerativeNodeData> getDegenerativeNodes(const char *res_name);

  Point3 getBoundingSphereCenter() const { return *(const Point3 *)(const void *)&vBoundingSphere; }
  float getBoundingSphereRad() const { return boundingSphereRad; }
  void setBoundingSphereRad(float r)
  {
    boundingSphereRad = r;
    traceTmEps = r > 0.f ? min(0.008f, 0.025f / r) : 0.008f; // an empty resource keeps the cap (MSVC C4723 on 0.025 / 0)
  }
  float getBoundingSphereRadSq() const { return v_extract_w(vBoundingSphere); }
  vec4f getBoundingSphereXYZR() const { return v_perm_xyzd(vBoundingSphere, v_splats(boundingSphereRad)); }
  BSphere3 getBoundingSphereS() const { return BSphere3(getBoundingSphereCenter(), boundingSphereRad); }

protected:
  // nodeIndex and the per-type index slices (Data::nodeOrder), from each node's type and position
  // alone (CONVEX joins the mesh list).
  void rebuildNodeLists();

private:
  // Mesh-pair narrow phase; node2_wtm and returned contacts are world-space. Faces brute-force
  // through the caller's posed tms.
  // Pre-materialised face triples plus their enclosing box, folded at construction: the pair
  // test prunes the partner's chunk walk by the box, an under-covering box would be a silent
  // false negative, and one constructor keeps the two from diverging.
  struct MeshNodeFaces
  {
    dag::ConstSpan<Point3_vec4> tris;
    bbox3f box;
    explicit MeshNodeFaces(dag::ConstSpan<Point3_vec4> t) : tris(t)
    {
      v_bbox3_init_empty(box);
      for (const Point3_vec4 &p : t)
        v_bbox3_add_pt(box, v_ld(&p.x));
    }
  };
  static bool testMeshNodePair(const CollisionNode *node1, const MeshNodeFaces &node1_faces, const CollisionResource *res2,
    const CollisionNode *node2, const TMatrix &node2_wtm, const TMatrix &tm1ToWorld, const TMatrix &tm2to1, Point3 &cp1, Point3 &cp2,
    uint16_t *node_index1, uint16_t *node_index2);

  template <class CB>
  void forEachNodeImpl(dag::ConstSpan<uint16_t> list, CB & cb) const
  {
    for (uint16_t i : list)
    {
      const CollisionNode &n = data->allNodesList()[i];
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

  // The single pose source: an owned-matrix instance, a persistent tree-backed instance, or
  // the PoseView aggregate the legacy entry points build over a GeomNodeTree argument.
  // pose_may_refresh: only a persistent tree-backed instance can be a pose generation behind;
  // it defaults to true (a new call site is correct-but-slower rather than stale) and the
  // legacy surface passes false. pose_t is whatever the core reads the pose through
  // (CollisionResourceInstance or CollisionResourceTraceAdapter::PoseView).
  // ray_mat_id is the ray's material filter, for the PER-LEAF material gate only: the node filter carries the whole-node early-out (it
  // is the only one that can be expressed through an opaque node predicate), and a node holding several materials passes that gate
  // while still holding faces the ray must skip. A trace with no material filter leaves it at PHYSMAT_INVALID and no leaf is tested.
  // An unstamped leaf reads index 0, so per-face exactness needs the per-face stamps of the fuse.
  template <IterationMode trace_mode, CollisionTraceType trace_type, bool pose_may_refresh = true, typename pose_t, typename filter_t,
    typename callback_t>
  __forceinline bool forEachIntersectedNode(mat44f tm, const pose_t &instance, vec3f from, vec3f dir, float len, bool calc_normal,
    float bsphere_scale, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
    TraceCollisionResourceStats *out_stats, bool force_no_cull, int ray_mat_id = PHYSMAT_INVALID, TraceTmCache *tm_cache = nullptr,
    bool force_cull = false) const;

  template <IterationMode trace_mode, CollisionTraceType trace_type, bool is_single_ray = false, bool pose_may_refresh = true,
    typename pose_t, typename filter_t, typename callback_t>
  __forceinline bool forEachIntersectedNode(mat44f tm, const pose_t &instance, dag::Span<CollisionTrace> traces, bool calc_normal,
    float bsphere_scale, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
    TraceCollisionResourceStats *out_stats, bool force_no_cull, int ray_mat_id = PHYSMAT_INVALID, TraceTmCache *tm_cache = nullptr,
    bool force_cull = false) const;

  template <bool orthonormalized_instance_tm, IterationMode trace_mode, CollisionTraceType trace_type, bool is_single_ray = false,
    typename pose_t, typename filter_t, typename callback_t>
  __forceinline bool forEachIntersectedNodePrepared(mat44f tm, float max_tm_scale_sq, vec3f woffset, const pose_t &instance,
    dag::Span<CollisionTrace> traces, bool calc_normal, uint8_t behavior_filter, const filter_t &filter, const callback_t &callback,
    TraceCollisionResourceStats *out_stats, bool force_no_cull, int ray_mat_id, const mat44f *cached_itm, bool force_cull = false)
    const;

  // verts_base and idx_base are `node`'s own blocks; the box-prim trace path synthesizes its own
  // arrays and a node copy.
  DAGOR_NOINLINE bool traceCapsuleMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint32_t *idx_base,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float &in_out_t, float &radius, vec4f &v_out_norm,
    vec4f &v_out_pos) const;

  // Per-leaf accept hook of the chunk walks below (collision_blas::LeafAcceptRef shape): a
  // type-erased predicate over a leaf the walk reached, so a caller can reject part of a node's geometry without the walkers knowing
  // why. nullptr accepts every leaf, which is what every caller with nothing to test passes, and is the walk that ran before the hook.
  using leaf_accept_t = bool (*)(void *ctx, soa4::LeafRef ref);

  // Chunk twins of the scalar capsule helpers: same per-triangle math, triangles supplied by the
  // node's per-node quad-BLAS filtered with the swept capsule's q-space box.
  DAGOR_NOINLINE bool traceCapsuleNodeChunkCullCCW(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
    float &in_out_t, float &radius, vec4f &v_out_norm, vec4f &v_out_pos, leaf_accept_t accept_leaf, void *accept_ctx) const;
  DAGOR_NOINLINE bool capsuleHitNodeChunkCullCCW(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
    float in_t, float radius, leaf_accept_t accept_leaf, void *accept_ctx) const;
  // Closest-hit chunk ray descent: descends the node's quad-BLAS instead of materialising and
  // scanning every face. CCW cull; the normal is the unnormalized face cross product.
  DAGOR_NOINLINE bool traceRayNodeChunkCullCCW(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir,
    float &in_out_t, vec4f *v_out_norm, leaf_accept_t accept_leaf, void *accept_ctx) const;
  // All-hits chunk ray descent: collects every triangle the node-local ray crosses within
  // [0, in_t] from the node's quad-BLAS (one walk, no t-pruning); CCW cull unless flagged or
  // forced, force_cull over both, normals unnormalized.
  // Each hit carries a BLAS tri_ref (opaque soa4::LeafRef leaf token + sub-tri), the identity the
  // closest-hit chunk arm emits; getNodeFaceVertsByRef decodes it.
  DAGOR_NOINLINE bool traceAllHitsNodeChunk(const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t,
    bool calc_normal, bool force_no_cull, bool force_cull, all_collres_hits_t &ret_hits, leaf_accept_t accept_leaf, void *accept_ctx)
    const;

  // Same verts_base / idx_base convention as traceCapsuleMeshNodeLocalCullCCW above.
  DAGOR_NOINLINE bool capsuleHitMeshNodeLocalCullCCW(const Point3_vec4 *verts_base, const uint32_t *idx_base,
    const CollisionNode &node, const vec4f &v_local_from, const vec4f &v_local_dir, float in_t, float radius) const;

  // Form-aware posed node->world matrix: tree wtm for driven nodes of a tree-backed pose,
  // instance_tm * stored T otherwise (unbound nodes fall back to the default pose).
  template <typename pose_t>
  __forceinline mat44f getPosedNodeWtmInline(const CollisionNode *node, mat44f_cref instance_tm, vec3f instance_woffset,
    const pose_t &instance) const;

  // Cold arm of instanceOrDefault: null/unbound normalizes silently, foreign asserts + logs.
  const CollisionResourceInstance &instanceOrDefaultFallback(const CollisionResourceInstance *instance) const;
  // Foreign or stale instances fall back to the current pose.
  const CollisionResourceInstance *resolveInstanceForTrace(const CollisionResourceInstance &instance) const;
  // Stored-pose queries additionally reject tree-backed poses (default pose + LOGWARN).
  const CollisionResourceInstance *resolveOwnedPoseForQuery(const CollisionResourceInstance &instance, const char *site) const;
  // Shared geometry-frame core: maps stored geometry to the given resource-local posed matrix
  // without reapplying exporter-baked prim tms.
  mat44f geometryTmFromPosed(int node_index, mat44f_cref posed, const CollisionResourceInstance::PoseMeta &pm) const;
  // Class-aware inverse of the node's GEOMETRY frame (inv of getNodeGeometryTm), not of the
  // raw posed matrix; IDENT preserves exporter-baked primitive geometry.
  TMatrix invNodeTm(int node_index) const;
  static TMatrix invInstNodeTm(const CollisionResourceInstance &instance, int node_index);
  bool testSphereNodeIntersectionLocal(const TMatrix &itm, const BSphere3 &sphere, const CollisionNode *node, const Point3 &dir_norm,
    Point3 &out_norm, float &out_depth) const;


protected:
  // All build output in ONE heap block: this header, then the arrays back to back in hot-to-cold
  // order, addressed by byte offsets from `this` (relocatable: a clone is a byte copy). Filled by the
  // load, immutable after it while shared; an owned block still takes the bind claim, whole
  // (initializeWithGeomNodeTree, stampGeomNodeIds) or per node (setNodeGeomNodeId).
  struct alignas(16) Data // the block base carries the strictest array alignment (plane3f)
  {
    // The enum order is the block order: a 1-2 node resource reads its nodes, poses and chunk
    // next to the header; the by-name, capsule and plane pools sit last.
    enum ArrayId : uint8_t
    {
      NODES,         // CollisionNode
      AUTHORED_TM,   // TMatrix, parallel to NODES
      AUTHORED_ITM,  // TMatrix, parallel to NODES or empty
      REL_GEOM_TMS,  // TMatrix, parallel to NODES under COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID, else empty
      NODE_BLAS,     // uint8_t, the per-node BLAS chunks
      TLAS,          // uint8_t, the blueprint TLAS or its reservation
      PHYS_MAT_POOL, // uint16_t, the material-set slices
      CAPSULES,      // Capsule, addressed by CollisionNode::capsuleIndex
      CONVEX_PLANES, // plane3f, addressed by CollisionNode::planesOfs/planesCount
      NAMES,         // char, terminated names; offset 0 is the empty name
      MAT_NAMES,     // char, the stream's material names (load only: resolved to ids, then cold)
      NODE_ORDER,    // uint16_t, LIST_BOUNDS group bounds then the node indices grouped by list; derived at load, never on the wire
      ARRAY_COUNT
    };
    // Element bytes and array start alignment, in ArrayId order. The two byte pools hold SoA4 node
    // words and the vert21 stream, whose reads are 8-aligned. The enum order is the stream's wire
    // order up to NODE_ORDER.
    static constexpr uint32_t ELEM_SIZE[ARRAY_COUNT] = {sizeof(CollisionNode), sizeof(TMatrix), sizeof(TMatrix), sizeof(TMatrix), 1, 1,
      sizeof(uint16_t), sizeof(Capsule), sizeof(plane3f), 1, 1, sizeof(uint16_t)};
    static constexpr uint32_t BLOCK_ALIGN = 64; // the node array starts on a cache line, one node per line
    // NODE_ORDER leads with the group bounds: list L holds [order[L], order[L + 1]) of the indices
    // behind them. In the array, not in this header, so the header stays two cache lines.
    static constexpr uint32_t LIST_BOUNDS = NUM_COLLISION_NODE_TYPES + 1;
    static constexpr uint32_t ELEM_ALIGN[ARRAY_COUNT] = {BLOCK_ALIGN, alignof(TMatrix), alignof(TMatrix), alignof(TMatrix), 8, 8,
      alignof(uint16_t), alignof(Capsule), alignof(plane3f), 1, 1, alignof(uint16_t)};
    // The block is allocated at BLOCK_ALIGN, so an offset aligned inside it is aligned absolutely.
    static constexpr bool blockAlignServesEveryArray()
    {
      bool ok = BLOCK_ALIGN % alignof(Data) == 0;
      for (uint32_t a : ELEM_ALIGN)
        ok = ok && BLOCK_ALIGN % a == 0;
      return ok;
    }
    struct ArrayRef
    {
      uint32_t byteOfs = 0;
      uint32_t elemCount = 0;
    };
    // the holders sharing this block: CollisionResource objects (deepCopy) and physJolt chunk
    // shares (CollResJoltChunk::Share); build and clone start a copy at 0
    int refCnt = 0;
    ArrayRef arrays[ARRAY_COUNT];
    uint32_t blockBytes() const // the last array ends the block
    {
      const ArrayRef &last = arrays[ARRAY_COUNT - 1];
      return last.byteOfs + last.elemCount * ELEM_SIZE[ARRAY_COUNT - 1];
    }
    // One holder's share of the block, rounded up so a sum over the holders covers it.
    size_t bytesPerHolder() const
    {
      const size_t holders = (size_t)max(interlocked_acquire_load(refCnt), 1);
      return (blockBytes() + holders - 1) / holders;
    }
    // Stamped (low tri_ref::NODE_BLAS_GEN_BITS) into every per-node BLAS ref and checked by
    // getNodeFaceVertsByRef; the chunks never move after the load, so it stays at the load's value.
    uint8_t nodeBlasBuildId = 0;
    // The first tree bind claimed this block's geomNodeId layout (see initializeWithGeomNodeTree).
    bool treeLayoutClaimed = false;
    // Blueprint all-nodes TLAS metadata (see buildAllNodesTLAS and tlasData()).
    uint32_t tlasTreeBytes = 0;  // metadata start: the tree bytes end (a 16-multiple)
    uint32_t tlasInnerCount = 0; // emitted blocks: entries in the node-offset and parent spans
    uint16_t tlasLeafCount = 0;  // leaf-bearing nodes: entries in the leaf span
    uint16_t tlasWatchCount = 0; // leafless trace-capable recovery watchlist entries
    uint32_t tlasRootRef = 0;    // soa4 child ref of the root
    // The blueprint generation the instance clones stamp; starts above a clone stamp's 0 init, so
    // a clone that never refit always reads stale.
    uint32_t tlasGeneration = 1;

    Data &operator=(const Data &) = delete;

  private:
    Data() = default; // only build() makes a block (the arrays live past sizeof(Data))

  public:
    void addRef() { interlocked_increment(refCnt); }
    void delRef()
    {
      if (interlocked_decrement(refCnt) == 0)
        destroy();
    }
    template <typename T>
    dag::Span<T> arr(ArrayId id)
    {
      return dag::Span<T>((T *)((char *)this + arrays[id].byteOfs), arrays[id].elemCount);
    }
    template <typename T>
    dag::ConstSpan<T> arr(ArrayId id) const
    {
      return dag::ConstSpan<T>((const T *)((const char *)this + arrays[id].byteOfs), arrays[id].elemCount);
    }
    dag::Span<CollisionNode> allNodesList() { return arr<CollisionNode>(NODES); }
    dag::ConstSpan<CollisionNode> allNodesList() const { return arr<CollisionNode>(NODES); }
    dag::Span<uint16_t> nodeOrder() { return arr<uint16_t>(NODE_ORDER); }
    dag::ConstSpan<uint16_t> nodeOrder() const { return arr<uint16_t>(NODE_ORDER); }
    // Immutable base for relative motion of exporter-baked primitives.
    dag::Span<TMatrix> authoredNodeTm() { return arr<TMatrix>(AUTHORED_TM); }
    dag::ConstSpan<TMatrix> authoredNodeTm() const { return arr<TMatrix>(AUTHORED_TM); }
    // inverse(authoredNodeTm[i]) for the slots that compose through it (usesAuthoredFrame); empty
    // when no slot does, and a reader computes the inverse then.
    // getStoredToNodeLocalTm stays un-cached on purpose: it returns scalar inverse() bits.
    dag::Span<TMatrix> authoredNodeItm() { return arr<TMatrix>(AUTHORED_ITM); }
    dag::ConstSpan<TMatrix> authoredNodeItm() const { return arr<TMatrix>(AUTHORED_ITM); }
    dag::Span<TMatrix> relGeomNodeTms() { return arr<TMatrix>(REL_GEOM_TMS); }
    dag::ConstSpan<TMatrix> relGeomNodeTms() const { return arr<TMatrix>(REL_GEOM_TMS); }
    dag::Span<char> names() { return arr<char>(NAMES); }
    dag::ConstSpan<char> names() const { return arr<char>(NAMES); }
    dag::Span<Capsule> capsules() { return arr<Capsule>(CAPSULES); }
    dag::ConstSpan<Capsule> capsules() const { return arr<Capsule>(CAPSULES); }
    dag::Span<plane3f> convexPlanes() { return arr<plane3f>(CONVEX_PLANES); }
    dag::ConstSpan<plane3f> convexPlanes() const { return arr<plane3f>(CONVEX_PLANES); }
    // Material-set slices of the multi-material nodes, back to back (see "node material sets"),
    // addressed by the offsets held in CollisionNode::physMatId.
    dag::Span<uint16_t> physMatPool() { return arr<uint16_t>(PHYS_MAT_POOL); }
    dag::ConstSpan<uint16_t> physMatPool() const { return arr<uint16_t>(PHYS_MAT_POOL); }
    // Per-node BLAS chunks back to back, [NodeBlasChunkHeader][quad tree][pad][vert21 stream], landed
    // from the stream; node.nodeBlasOfs = chunk byte offset. The resource holds NO source-face index
    // list: faces are enumerated from the chunk tree.
    dag::Span<uint8_t> nodeBlasData() { return arr<uint8_t>(NODE_BLAS); }
    dag::ConstSpan<uint8_t> nodeBlasData() const { return arr<uint8_t>(NODE_BLAS); }
    // Blueprint all-nodes TLAS (buildAllNodesTLAS; runtime only): SoA4 tree bytes, 16*N B per internal
    // node plus one 16 B pad slot (NodeRef's word load reads 16 B even for N<4), then, 16-aligned behind
    // them, the topology metadata the tlas*() spans address: u32 node offsets, u32 fused parent words,
    // u16 leaf, watch and leaf-ordinal arrays.
    dag::Span<uint8_t> tlasData() { return arr<uint8_t>(TLAS); }
    dag::ConstSpan<uint8_t> tlasData() const { return arr<uint8_t>(TLAS); }

    // The block's byte size for `counts` (the header included) and, when asked, each array's start
    // offset: the one layout rule, shared by build, the writer and the loader's check before it allocates.
    static uint64_t blockBytesFor(const uint32_t counts[ARRAY_COUNT], uint64_t out_ofs[ARRAY_COUNT] = nullptr);
    // A block of `counts` elements per array (null: the empty block), unshared, every slot at its
    // default (a constructed node, an identity matrix, zero bytes).
    static Data *build(const uint32_t counts[ARRAY_COUNT]);
    static Data *clone(const Data &src);
    void destroy();
  };
  Ptr<Data> data; // never null

public:
  // The uint16 node-index and leaf-ordinal stores (tlasLeafNodes/tlasWatchNodes/tlasNodeLeafOrd
  // spans) and a max_children=4 tree staying under 2N nodes keep the whole tree addressable
  // under this cap.
  static constexpr uint32_t MAX_COLLISION_NODES = 32768;
  // Below this leaf count no TLAS is built (hasAllNodesTLAS() == false): a 4-wide traversal
  // would test about as many boxes as the linear walk while still paying setup and refit.
  static constexpr uint32_t MIN_TLAS_LEAVES = 10;
  // The blueprint TLAS emit's byte size (buildAllNodesTLAS): one 16 B SoA4 child entry per SAH entry (the
  // root's slot is the word-load tail pad), 8 B of offset + parent metadata per emitted block, one 2 B slot
  // per leaf, watch and ordinal entry.
  static constexpr uint32_t tlasBytesFor(uint32_t sah_entries, uint32_t inner_blocks, uint32_t meta_slots)
  {
    return sah_entries * 16u + inner_blocks * 8u + meta_slots * 2u;
  }
  // TLAS bytes the block reserves for n nodes (MIN_TLAS_LEAVES or more): the emit's arithmetic at its
  // bound (at most 2L-1 SAH entries over L <= n leaves, L-1 inner blocks, L leaf + n-L watch + n ordinal
  // slots), so the load-time build lands inside the one allocation. Keyed on the node count alone.
  static constexpr uint32_t tlasReserveBytes(uint32_t node_count)
  {
    return node_count >= MIN_TLAS_LEAVES ? tlasBytesFor(2u * node_count - 1u, node_count - 1u, 2u * node_count) : 0u;
  }
  // Built: the tree bytes end past 0 (the array itself may hold the reservation alone).
  bool hasAllNodesTLAS() const { return data->tlasTreeBytes != 0; }

protected:
  // Candidate node ids for a resource-space query box through `inst`'s all-nodes-TLAS clone (the
  // trace dispatch's candidate set, unordered). false = no current clone: the caller walks its
  // node list instead. Read-only - clones materialize at finalize points and pose writes, never
  // on a query - and owned-form instances only (tree clones decline to the linear walk).
  // The box queries (the gather and the sphere/capsule/quad/inclusion/clip family) and the pair
  // arms are the callers; each re-runs its own per-node gates over the candidates.
  bool tlasBoxCandidates(const CollisionResourceInstance &inst, bbox3f_cref box, CollResTlasCandidates &out) const;

public:
protected:
  // Test-only (via the CollisionResourceUnittest friend): how many nodes the TLAS marks reachable
  // for one resource-local segment, so a test can prove the prefilter narrows the walk; the
  // query-magnitude decline arm is not mirrored (see the definition).
  int countTlasCandidates(const CollisionResourceInstance &inst, vec3f local_from, vec3f local_to) const;
  friend class CollisionGameResFactory;
  friend struct ITestIntersectionAlgo;    // pair arms call tlasBoxCandidates
  friend struct CollisionResourceBuilder; // lands its finalized arrays (engine/sharedInclude/gameRes/collisionResourceBuilder.h)
  friend class CollisionResourceTraceAdapter;
  friend struct CollisionResourceUnittest;
  friend struct CollisionResourceInstance;
  friend struct CollResJoltChunk; // physJolt's chunk view + Data-block share (engine/phys/physJolt/collResJoltChunk.h)

  // Shared transform classifier for authored and live poses.
  static uint8_t classifyNodeTmFlags(mat44f_cref tm, float &out_max_scale);
  // A slot whose readers compose through inverse(authoredNodeTm): the eps-IDENT box/sphere
  // compatibility frame and retained bakes. Writer, readers and the unittest all key on this
  // one test, so a new consumer class lands everywhere at once.
  static bool usesAuthoredFrame(const CollisionNode &n, const CollisionResourceInstance::PoseMeta &pm)
  {
    return pm.isRetainedBake() ||
           ((n.type == COLLISION_NODE_TYPE_BOX || n.type == COLLISION_NODE_TYPE_SPHERE) && (n.flags & CollisionNode::IDENT));
  }
  // COLLISION_RES_FLAG_NODES_COVER_ROOT_BOX: every traced mesh-list node (geometry, enabled, live
  // traceable, TRACEABLE behavior) sits at identity pose with a box of at least 0.9 of the root
  // extent on every axis; false with no such node. pose is nodeIndex-parallel; the builder's
  // bounds step and finishLoad share it.
  static bool meshNodesCoverRootBox(bbox3f_cref root, dag::ConstSpan<CollisionNode> nodes,
    dag::ConstSpan<CollisionResourceInstance::PoseMeta> pose);
  // The traces that read the instance inverse (pre-walk segment reject, primitive pass, TLAS
  // frame); a tree-backed mesh-only trace composes per-node inverses instead.
  template <class pose_t>
  bool traceReadsInstanceInverse(const pose_t &instance, bool orthonormal_tm, bool capsule, bool single_ray) const;
  // One tree-driven ray against meshNodes() spheres [first, first + 64), four at a time, with a meta_src slot per node:
  // the returned bits are a superset of what the per-node sphere test passes.
  uint64_t preCullTreeMeshSpheres(const CollisionTrace &trace, const CollisionResourceInstance &meta_src, const GeomNodeTree &tree,
    vec3f woffset, uint32_t first, bool authored_meta, bool live_bound) const;
  // Largest singular value (see mat33_spectral_norm); shared with the loader's un-bake divisor.
  static float mat33SpectralNorm(mat44f_cref tm);
  // Scale-free singularity floor (max-element-normalized, overflow-proof); out_ndet is the
  // normalized signed determinant for mirror detection.
  static bool relativeDetAboveFloor(mat44f_cref tm, float &out_ndet);

  bool geomNodeTreeBound = false; // initializeWithGeomNodeTree ran at least once
  // Mesh nodes a legacy tree trace sizes by the live wtm: scaled or sheared at any tree stamp, or remapped per node.
  // constraint: the first mark publishes the words once, as stamps mark while other threads trace
  mutable uint32_t *volatile liveBoundWords = nullptr;
  bool isLiveBoundNode(uint32_t node_index) const
  {
    const uint32_t *words = interlocked_acquire_load_ptr(liveBoundWords);
    return words && ((interlocked_relaxed_load(words[node_index / 32]) >> (node_index % 32)) & 1);
  }
  bool hasLiveBoundNodes() const { return interlocked_acquire_load_ptr(liveBoundWords) != nullptr; }
  uint32_t liveBoundWordCount() const { return (getAllNodes().size() + 31) / 32; }
  void markLiveBoundNode(uint32_t node_index);
#if DAGOR_DBGLEVEL > 0
  int gameResId = -1;
#endif
  // Bumped by default-pose mutations: tree-backed instances read unbound nodes from the
  // default pose live, so their lazy refresh keys on this beside the tree generation.
  // Mutable because instances hold const resource pointers; writes ride the pose-write
  // serialization contract.
  mutable uint32_t defaultPoseGen = 1;
  // Live-flag publish order: the flags store precedes the release bump, the mirror acquires
  // before it copies, so instances mirror the flags alone (no pose re-derive, no TLAS refit).
  // Direct default-instance readers take no edge: a racing trace reads old-or-new per node,
  // the caller's serialization duty like every default-pose write.
  mutable uint32_t liveFlagsGen = 1;

  CollisionResourceInstance defaultInstance;

  static DAGOR_NOINLINE void addTracesProfileTag(dag::Span<CollisionTrace> traces);
  static DAGOR_NOINLINE void addMeshNodesProfileTag(const struct CollResProfileStats &profile_stats);


  // Metadata spans behind the tree bytes; empty until a build. Refits and candidate-density
  // thresholds work in LEAVES, so POINTS-heavy resources pay per leaf, not per node; every span
  // is valid per blueprint generation, like the leafLoc stamps.
  // tree node -> byte offset; a parent always precedes its children
  dag::ConstSpan<uint32_t> tlasNodeOfs() const
  {
    return dag::ConstSpan<uint32_t>((const uint32_t *)(data->tlasData().data() + data->tlasTreeBytes), data->tlasInnerCount);
  }
  // tree node -> fused parent word: (child count - 1) << 30 | (parent index << 2) | lane in
  // parent; the root's parent bits read TLAS_PARENT_ROOT
  dag::ConstSpan<uint32_t> tlasNodeParentPacked() const
  {
    return dag::ConstSpan<uint32_t>((const uint32_t *)(data->tlasData().data() + data->tlasTreeBytes) + data->tlasInnerCount,
      data->tlasInnerCount);
  }
  static constexpr uint32_t TLAS_PARENT_ROOT = 0x3FFFFFFFu;
  static uint32_t tlasParentOf(uint32_t w) { return w & 0x3FFFFFFFu; }
  static int tlasChildCountOf(uint32_t w) { return int(w >> 30) + 1; }
  // leaf-bearing node indices, the refit iteration order
  dag::ConstSpan<uint16_t> tlasLeafNodes() const
  {
    return dag::ConstSpan<uint16_t>((const uint16_t *)(data->tlasData().data() + data->tlasTreeBytes + data->tlasInnerCount * 8u),
      data->tlasLeafCount);
  }
  dag::ConstSpan<uint16_t> tlasWatchNodes() const
  {
    return dag::ConstSpan<uint16_t>(tlasLeafNodes().data() + data->tlasLeafCount, data->tlasWatchCount);
  }
  // node index -> ordinal in tlasLeafNodes (~0 for leafless): O(1) for the dense-batch marker
  // dedup, built once per blueprint instead of searched per candidate.
  dag::ConstSpan<uint16_t> tlasNodeLeafOrd() const
  {
    return dag::ConstSpan<uint16_t>(tlasWatchNodes().data() + data->tlasWatchCount,
      data->tlasTreeBytes == 0 ? 0 : (int)data->allNodesList().size());
  }

  // The current stream's arm of the constructor past its prologue. False (logged) on a refused
  // stream; a corrupt chunk stream throws IGenLoad::LoadException out of the deserializer.
  // A stream body; body_bytes bounds the tail on an uncompressed stream (-1: the decoder ends it).
  bool loadStream(IGenLoad & crd, const char *res_name, int (*resolve_phmat)(const char *), int body_bytes);
  const char *readChunks(IGenLoad & crd); // the refusal cause, null once every chunk landed
  // The one landing of a filled block, from the block and the default instance's bind meta alone.
  // Both the stream load and the builder end here.
  void finishLoad(const char *res_name);
  // All-nodes blueprint TLAS over the bind pose, into the block's reservation: SoA4 nodes plus a
  // 16 B word-load tail pad. Instances clone the topology and refit boxes; the blueprint itself is
  // never traced, never persisted. A decline (too few leaves, a budget past the envelope, an emit
  // refusal) keeps the linear walk. res_name feeds the diagnostics only.
  void buildAllNodesTLAS(const char *res_name);
  bool writeChunks(IGenSave & cwr) const;

  // Largest slice START a signed 16-bit physMatId can address (-32768 -> 32766).
  // Only the start is encoded: the count cell sits at it and the entries follow at plain pool indices, so the boundary slice is a full
  // palette (and every slice holds >= 2 entries -- one stays inline).
  static constexpr uint32_t MAX_PHYS_MAT_POOL_OFS = 32766;
  // physMatPool offset of a multi-material node's slice; valid only for physMatId <= -2.
  static uint32_t nodePhysMatSliceOfs(int16_t phys_mat_id) { return (uint32_t)(-(int)phys_mat_id - 2); }
  // Nothing here is trusted: leaf bits can out-index a shorter slice and a forged offset reads an ID as its count, so the addressed
  // entry is bounded against the pool, not the stated count alone.
  int physMatFromNodeSlice(int16_t phys_mat_id, int idx) const
  {
    const uint32_t ofs = nodePhysMatSliceOfs(phys_mat_id);
    if (ofs >= data->physMatPool().size() || (uint32_t)idx >= (uint32_t)data->physMatPool()[ofs])
      return PHYSMAT_INVALID;
    const uint32_t at = ofs + 1u + (uint32_t)idx; // idx passed the count test above, so this cannot wrap
    if (at >= data->physMatPool().size())
      return PHYSMAT_INVALID;
    return (int)(int16_t)data->physMatPool()[at];
  }

end_dclass_decl();
