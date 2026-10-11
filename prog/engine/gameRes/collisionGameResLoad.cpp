// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collisionResource.h>
#include "collisionGameResInternal.h"
#include <gameRes/collResStream.h>
#include <gameRes/collisionResourceBuilder.h>
#include <gameRes/dag_gameResSystem.h>
#include <math/dag_plane3.h>
#include <scene/dag_physMat.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_btagCompr.h>
#include <generic/dag_sort.h>
#include <generic/dag_staticTab.h>
#include <EASTL/algorithm.h>
#include <debug/dag_debug.h>
#include <daBVH/dag_bvhBuild.h>
#include <daBVH/dag_bvhIO.h>
#include <daBVH/dag_swBLAS_soa4.h>
#include <daBVH/swCommon.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <daBVH/dag_swBLAS_soa4Convert.h>
#include <daBVH/dag_swBLAS_soa4Emit.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <util/dag_hashedKeyMap.h>

CollisionResource *CollisionResource::loadResource(IGenLoad &crd, int res_id) { return new CollisionResource(crd, res_id); }

// The name guard is a LINK constraint: get_game_resource_name pulls the gameResSystem object into
// links that lack its ddsx deps (assetsExp). Tools name their assets through res_name instead;
// pack loads run where the kernel is static.
static String game_res_name(int res_id)
{
  String name;
#if _TARGET_STATIC_LIB
  get_game_resource_name(res_id, name);
#else
  G_UNUSED(res_id);
  name = "unknown";
#endif
  return name;
}

String CollisionResource::getGameResName() const
{
#if DAGOR_DBGLEVEL > 0
  return game_res_name(gameResId);
#else
  return String();
#endif
}

CollisionResource::CollisionResource(IGenLoad &_cb, int res_id, const char *res_name, int (*resolve_phmat)(const char *)) :
  CollisionResource()
{
  // A refused stream leaves the constructed-empty state: an arm lands into this object and a
  // refusal past its landing (the body-length check) undoes it here.
  auto resetForLoad = [&]() {
    data = Data::build(nullptr);
    collisionFlags = 0;
    vFullBBox.bmin = vFullBBox.bmax = v_zero(); // the constructed value, spelled for every compiler
    vBoundingSphere = vBindTraceSphere = v_zero();
    boundingBox = BBox3();
    setBoundingSphereRad(0);
    defaultInstance.nodeTm.clear();
    defaultInstance.poseMeta.clear();
    defaultInstance.recomputeRootBBox();
    rebuildNodeLists();
  };

#if DAGOR_DBGLEVEL > 0
  gameResId = res_id;
#endif
  unsigned label = _cb.readInt();
  G_ASSERTF_RETURN((label & COLLRES_STREAM_MAGIC_MASK) == COLLRES_STREAM_MAGIC, , "Invalid collision resource: 0x%8X", label);

  int version = (label & COLLRES_STREAM_VERSION_MASK);
  // 2 is spent: it named a stream layout (a 144-byte header) that never shipped, so a later layout
  // takes a new number rather than that one.
  if (version != 0 && version != 1 && version != COLLRES_STREAM_VERSION)
  {
    logerr("Invalid collision resource version %d (label 0x%08X); the resource stays empty", version, label);
    return;
  }
  String resName;
  if (!res_name)
  {
    resName = game_res_name(res_id);
    res_name = resName.c_str();
  }

  if (version == 0) // the legacy raw dump opens its own blocks
  {
    CollisionResourceBuilder b;
    if (b.loadLegacy(_cb, label, res_name, resolve_phmat))
      b.land(*this, res_name); // a refusal lands nothing: the entry reset's empty resource stays
    return;
  }

  unsigned btag = 0;
  const unsigned compr_data_sz = _cb.beginBlock(&btag);
  alignas(ZstdLoadCB) alignas(OodleLoadCB) uint8_t zcrdStorage[max(sizeof(ZstdLoadCB), sizeof(OodleLoadCB))];

  IGenLoad *zcrd = nullptr;
  if (btag == btag_compr::ZSTD)
    zcrd = new (zcrdStorage, _NEW_INPLACE) ZstdLoadCB(_cb, compr_data_sz);
  else if (btag == btag_compr::OODLE)
    zcrd = new (zcrdStorage, _NEW_INPLACE) OodleLoadCB(_cb, compr_data_sz - 4, _cb.readInt());
  else
    zcrd = &_cb;

  bool ok = false;
  if (version == COLLRES_STREAM_VERSION)
  {
    const int bodyStart = zcrd == &_cb ? _cb.tell() : 0;
    // The deserializer throws on a corrupt wire tree (a fatal where exceptions are off, like the
    // v1 count fatals); every check that needs no parse runs before it.
    DAGOR_TRY { ok = loadStream(*zcrd, res_name, resolve_phmat, zcrd == &_cb ? (int)compr_data_sz : -1); }
    DAGOR_CATCH(IGenLoad::LoadException) { logerr("collision res <%s>: corrupt chunk stream", res_name); }
    // The body must end where the parse ends: a short uncompressed one reads the next resource's
    // bytes (a short compressed one threw above), a long one hides bytes the decoder's close expects consumed.
    uint8_t probe;
    if (ok && (zcrd == &_cb ? _cb.tell() - bodyStart != (int)compr_data_sz : zcrd->tryRead(&probe, 1) == 1))
    {
      logerr("collision res <%s>: stream refused (body length)", res_name);
      ok = false;
    }
  }
  else
  {
    CollisionResourceBuilder b;
    ok = b.loadLegacy(*zcrd, label, res_name, resolve_phmat) && b.land(*this, res_name);
  }
  if (!ok)
  {
    resetForLoad(); // the EMPTY resource of a refused stream
    // a decoder's close expects a consumed input, so the wrappers drain their block; an uncompressed
    // stream stays where the parse stopped (the pack loader seeks each resource before it loads)
    zcrd->ceaseReading();
  }
  if (zcrd != &_cb)
    zcrd->~IGenLoad();
}

void CollisionResource::finishLoad(const char *res_name)
{
  rebuildNodeLists();
  defaultInstance.nodeTm.assign(data->authoredNodeTm().begin(), data->authoredNodeTm().end());
  defaultInstance.recomputeRootBBox();
  // The builder derives the bit for the export; a stream that carries it clear (an older cook)
  // derives it here, until every asset is re-cooked.
  if (!(collisionFlags & COLLISION_RES_FLAG_NODES_COVER_ROOT_BOX) &&
      meshNodesCoverRootBox(defaultInstance.getRootBBox(), data->allNodesList(), make_span_const(defaultInstance.poseMeta)))
    collisionFlags |= COLLISION_RES_FLAG_NODES_COVER_ROOT_BOX;
  buildAllNodesTLAS(res_name); // into the reservation; a decline stays a supported state
}

bool CollisionResource::meshNodesCoverRootBox(bbox3f_cref root, dag::ConstSpan<CollisionNode> nodes,
  dag::ConstSpan<CollisionResourceInstance::PoseMeta> pose)
{
  // Such a node rejects almost nothing the root box did not; a posed node's box would need
  // composing, so it stays out of the shortcut.
  const vec3f minSize = v_mul(v_bbox3_size(root), v_splats(0.9f));
  bool cover = false;
  for (const CollisionNode &n : nodes)
  {
    const CollisionResourceInstance::PoseMeta &pm = pose[n.nodeIndex];
    if (!collres_is_mesh_list_node(n.type) || !n.hasGeometry() || pm.isDisabled() || !pm.isTraceable() ||
        !pm.checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    cover = (pm.flags & CollisionNode::IDENT) != 0 && v_check_xyz_all_true(v_cmp_ge(v_bbox3_size(v_ldu_bbox3(n.modelBBox)), minSize));
    if (!cover)
      return false;
  }
  return cover;
}

// ===== all-nodes blueprint TLAS =====

namespace
{
// A TLAS node is the SoA4 node layout minus the inline leaf bodies: [uint16 min/max per axis, N
// wide][uint32 word, N wide]. A child word is either a leaf (bit 31 set, low bits = data->allNodesList()
// index) or a ref to another node, encoded exactly as soa4 does so soa4::NodeSoA reads it
// verbatim; the emitter owns the bit-31 flag, callbacks hand it the payload alone.

// One predicate for the leaf set and the recovery watchlist (a leafless node outside the
// watchlist could never decline); an empty prim (r < 0) would inject inverted boxes.
bool node_has_tlas_leaf(const CollisionNode &n, float bsph_r) { return collres_tlas_recovery_capable(n, bsph_r); }
} // namespace

void CollisionResource::buildAllNodesTLAS(const char *res_name)
{
  G_ASSERT(!hasAllNodesTLAS()); // built once per resource, by finishLoad
  if (data->allNodesList().size() > MAX_COLLISION_NODES)
  {
    logerr("CollisionResource '%s': node count %u exceeds the %u-node TLAS limit", res_name ? res_name : "<unnamed>",
      (unsigned)data->allNodesList().size(), (unsigned)MAX_COLLISION_NODES);
    return;
  }

  int instCount = 0;
  for (const CollisionNode &n : data->allNodesList())
    instCount += node_has_tlas_leaf(n, n.radiusAroundBoxCenter) && defaultInstance.poseMeta[n.nodeIndex].isTraceable() ? 1 : 0;
  if (instCount < (int)MIN_TLAS_LEAVES)
    return; // linear node walk wins at this size

  // Bind-pose leaf boxes in the frame the refit composes with; the refit's exact-identity
  // refinements may differ harmlessly - clones re-encode on materialization.
  dag::Vector<bbox3f, framemem_allocator> leafBoxes(instCount);
  dag::Vector<uint16_t, framemem_allocator> instToNodeIdx(instCount);
  bbox3f leafUnion;
  v_bbox3_init_empty(leafUnion);
  // Budget and frame mirror the refit derivation (see refitAllTlasLeaves) so the build/decline
  // decision agrees with it; the blueprint is never traced directly (only cloned and refit).
  int idx = 0;
  float maxNodeOp = 0.f, amplMax = 1.f;
  for (const CollisionNode &n : data->allNodesList())
  {
    // A bind-untraceable (singular-authored) node never traces and its transform can be
    // non-finite: it must not contaminate the frame or the SAH input.
    if (!node_has_tlas_leaf(n, n.radiusAroundBoxCenter) || !defaultInstance.poseMeta[n.nodeIndex].isTraceable())
      continue;
    const int nodeIdx = n.nodeIndex;
    const mat44f gtm = geometryTmFromPosed(nodeIdx, defaultInstance.getNodeTm(nodeIdx), defaultInstance.poseMeta[nodeIdx]);
    const bbox3f rawBox = getNodeGeometryBBox(n);
    v_bbox3_init(leafBoxes[idx], gtm, rawBox);
    // Per-leaf, not only on the final union: v_min/v_max let a later finite box shadow a NaN
    // lane, and an encoded NaN clamps to the corner bin - a leaf the prefilter would drop.
    if (DAGOR_UNLIKELY(!v_test_xyz_finite(v_add(leafBoxes[idx].bmin, leafBoxes[idx].bmax))))
    {
      logerr("CollisionResource '%s': non-finite leaf bound on node %d; falling back to the linear walk",
        res_name ? res_name : "<unnamed>", nodeIdx);
      return;
    }
    v_bbox3_add_box(leafUnion, leafBoxes[idx]);
    instToNodeIdx[idx] = (uint16_t)nodeIdx;
    const float ampl = collres_tlas_tm_conditioning(gtm); // one derivation with the refit's budget
    amplMax = max(amplMax, ampl);
    maxNodeOp = max(maxNodeOp, collres_tlas_node_op(defaultInstance.poseMeta[nodeIdx].maxTmScale, collres_box_max_abs(rawBox),
                                 collres_box_max_abs(leafBoxes[idx]), ampl));
    ++idx;
  }
  G_ASSERT(idx == instCount);
  // The shared helper keeps the build/decline decision in lockstep with the refit's budget.
  const float fpErrS = collres_tlas_fp_budget(maxNodeOp, v_extract_x(v_hmax3(v_sub(leafUnion.bmax, leafUnion.bmin))), amplMax);
  const vec4f fpErrV = v_splats(fpErrS); // axis-independent scalar: rotation mixes source axes (see refit)
  // Outside the provable envelope (non-finite or budget rivaling the bins) the prefilter cannot
  // help: no TLAS, the linear walk stays.
  {
    const vec3f envSize = v_sub(v_add(leafUnion.bmax, fpErrV), v_sub(leafUnion.bmin, fpErrV));
    // Dominant-dimension ratio: thin cross-axes span mostly pad harmlessly (see the refit gate).
    const float budgetRatio = fpErrS / max(v_extract_x(v_hmax3(envSize)), 1e-30f);
    if (!(fpErrS < 1e30f) || !v_test_xyz_finite(v_add(leafUnion.bmin, leafUnion.bmax)) || budgetRatio > COLLRES_TLAS_BUDGET_MAX_RATIO)
    {
      // logwarn, not logerr: the decline is the budget gate working as designed on legitimately
      // extreme content, and the linear walk keeps full correctness (only the prefilter is lost).
      logwarn("CollisionResource '%s': TLAS conditioning declined (fp budget %.3f of the dominant dimension, e.g. a strongly "
              "non-uniform authored placement); falling back to the linear walk",
        res_name ? res_name : "<unnamed>", budgetRatio);
      return;
    }
  }
  // The quant frame is the build's own: the emitted boxes carry it and a clone derives its own
  // from the posed leaf union, so the block keeps none.
  const vec3f pMin = v_sub(leafUnion.bmin, fpErrV), pMax = v_add(leafUnion.bmax, fpErrV);
  const vec3f pSize = v_sub(pMax, pMin);
  const vec3f safeSize = v_max(pSize, v_splats(0.0001f));
  const vec3f effectiveMin = v_sub(pMin, v_mul(v_sub(safeSize, pSize), V_C_HALF));
  const vec4f tlasScale = v_div(v_splats(COLLRES_TLAS_QUANT_SPAN), safeSize);
  const vec4f tlasOrigin = effectiveMin;

  // stamp the SAH leaf keys into leafBoxes in place: this is its last reader, and the builder
  // reorders the input anyway
  for (int i = 0; i < instCount; ++i)
  {
    leafBoxes[i].bmin = v_perm_xyzd(leafBoxes[i].bmin, v_cast_vec4f(v_splatsi(i)));
    leafBoxes[i].bmax = v_perm_xyzd(leafBoxes[i].bmax, v_zero()); // zeroed for deterministic bytes; leaves key on w(bmin)
  }
  // tmpmem, not framemem: leafBoxes and instToNodeIdx above are framemem and outlive this, so a
  // growing sahNodes would abandon its block over theirs (LIFO discipline).
  Tab<bbox3f> sahNodes(tmpmem);
  int maxDepth = 0;
  build_bvh::create_bvh_node_sah(sahNodes, leafBoxes.data(), instCount, 4, maxDepth);
  // No depth pre-check: maxDepth counts SAH levels with the root at 1 and leaves included, while
  // the emitter counts inner recursion from 0 -- the emit refusal below is the exact authority.

  // Every SAH entry except the root is one 16-byte child entry in its parent's block; the root's
  // slot is the 16-byte tail that covers NodeRef's word load past the last block and doubles as
  // the emitter's per-node guard slack for a trailing node under 4 children.
  const uint32_t tlasBytes = 16u * (uint32_t)sahNodes.size();
  // one metadata slot per emitted BLOCK: the internal nodes, exactly the non-leaf SAH entries
  const uint32_t innerCount = (uint32_t)sahNodes.size() - (uint32_t)instCount;
  // every count is known before the emit, so tree bytes and metadata land in ONE allocation:
  // the watchlist is the leafless recovery-capable set, computable from leaf membership alone
  dag::Vector<bool, framemem_allocator> isLeafNode(data->allNodesList().size(), false);
  for (int i = 0; i < instCount; ++i)
    isLeafNode[instToNodeIdx[i]] = true;
  uint32_t watchCount = 0;
  for (const CollisionNode &n : data->allNodesList())
    watchCount += (!isLeafNode[n.nodeIndex] && collres_tlas_recovery_capable(n, n.radiusAroundBoxCenter)) ? 1u : 0u;
  const uint32_t totalBytes =
    tlasBytesFor((uint32_t)sahNodes.size(), innerCount, (uint32_t)instCount + watchCount + (uint32_t)data->allNodesList().size());
  // The tree lands in the reservation (tlasReserveBytes: the same arithmetic at its bound). Reachable
  // only if the SAH builder emits past 2L-1 entries: it asserts in dev and declines in release (the
  // block never moves past the load).
  G_ASSERTF(totalBytes <= data->tlasData().size(), "TLAS of %u B outgrows the %u B reservation of %u nodes", totalBytes,
    (unsigned)data->tlasData().size(), (unsigned)data->allNodesList().size());
  if (DAGOR_UNLIKELY(totalBytes > data->tlasData().size()))
  {
    logerr("CollisionResource '%s': TLAS of %u B outgrows the %u B reservation of %u nodes; falling back to the linear walk",
      res_name ? res_name : "<unnamed>", totalBytes, (unsigned)data->tlasData().size(), (unsigned)data->allNodesList().size());
    return;
  }
  // Zeroed explicitly: the emit leaves pruned child slots untouched.
  memset(data->tlasData().data(), 0, totalBytes);
  uint32_t *nodeOfsW = (uint32_t *)(data->tlasData().data() + tlasBytes);
  uint32_t *parentW = nodeOfsW + innerCount;
  dag::Vector<uint32_t, framemem_allocator> leafLoc(instCount, ~0u);
  uint32_t blockK = 0;
  soa4::SahEmitState st{data->tlasData().data(), tlasBytes};
  const uint32_t rootRef = soa4::emitFromSah(
    st, sahNodes.data(), (int)sahNodes.size(),
    [&](int se, uint32_t, int, uint16_t *mn, uint16_t *mx) {
      collres_tlas_quant_lane(sahNodes[se], tlasScale, tlasOrigin, fpErrV, mn, mx);
    },
    [&](int face, uint32_t node_idx, int lane) {
      leafLoc[face] = (node_idx << 2) | (uint32_t)lane;
      return (uint32_t)instToNodeIdx[face];
    },
    [&](uint32_t, uint32_t ofs, uint32_t parent, int n) {
      nodeOfsW[blockK] = ofs;
      // fused parent word: (n - 1) << 30 over the packed parent bits; the root's ~0 masks to
      // TLAS_PARENT_ROOT
      parentW[blockK] = (uint32_t(n - 1) << 30) | (parent & TLAS_PARENT_ROOT);
      ++blockK;
    });
  if (!st.ok)
  {
    static const char *emitRefusalName[] = {"none", "fanout", "depth past the traversal stacks", "node offset", "node block size"};
    logerr("CollisionResource '%s': TLAS emit refused (%s); falling back to the linear walk", res_name ? res_name : "<unnamed>",
      emitRefusalName[(int)st.refusal]);
    return; // the metadata keeps its no-tree values: nothing reads the partial emit
  }
  // The exact bound leaves no worst-case tail to trim: a full emit lands on tlasBytes (a pruned
  // entry leaves a few zeroed child slots), and the metadata already sits behind it.
  data->tlasRootRef = rootRef;
  data->tlasTreeBytes = tlasBytes;
  data->tlasInnerCount = innerCount;
  data->tlasLeafCount = (uint16_t)instCount;
  data->tlasWatchCount = (uint16_t)watchCount;

  for (int i = 0; i < instCount; ++i)
    data->allNodesList()[instToNodeIdx[i]].tlasLeafLoc = leafLoc[i];

  // Leaf list and recovery watchlist: refits iterate these, not data->allNodesList(), so a POINTS-heavy
  // resource pays per leaf. The watchlist is every leafless node a live pose could recover.
  uint16_t *leafW = (uint16_t *)(parentW + innerCount);
  for (int i = 0; i < instCount; ++i)
    leafW[i] = instToNodeIdx[i];
  uint16_t *watchW = leafW + instCount;
  for (const CollisionNode &n : data->allNodesList())
    if (n.tlasLeafLoc == ~0u && collres_tlas_recovery_capable(n, n.radiusAroundBoxCenter))
      *watchW++ = n.nodeIndex;
  G_ASSERT(watchW == leafW + instCount + watchCount);
  uint16_t *leafOrdW = leafW + instCount + watchCount;
  for (uint32_t o = 0, oe = (uint32_t)data->allNodesList().size(); o < oe; ++o)
    leafOrdW[o] = 0xffffu;
  for (int i = 0; i < instCount; ++i)
    leafOrdW[instToNodeIdx[i]] = (uint16_t)i;
}

// ===== the stream (collResStream.h) =====
// The reader and the writer are one loop over the Data arrays each; the chunk records are the
// only special case. Physmat ids are runtime values: on the wire they are MAT_NAMES indices.
static constexpr uint8_t COLLRES_NODE_FLAGS_PERSISTED = CollisionNode::IDENT | CollisionNode::TRANSLATE |
                                                        CollisionNode::ORTHONORMALIZED | CollisionNode::ORTHOUNIFORM |
                                                        CollisionNode::TRACE_TWO_SIDED;
static constexpr uint8_t COLLRES_POSE_CLASS_BITS =
  CollisionNode::IDENT | CollisionNode::TRANSLATE | CollisionNode::ORTHONORMALIZED | CollisionNode::ORTHOUNIFORM;
static constexpr uint8_t COLLRES_BIND_STATUS_PERSISTED =
  CollisionResourceInstance::PoseMeta::TRACEABLE | CollisionResourceInstance::PoseMeta::GEOMETRY_BAKED |
  CollisionResourceInstance::PoseMeta::RETAINED_BAKE | CollisionResourceInstance::PoseMeta::COMPOSABLE;
// Only a mesh or convex node carries geometry on the wire (a POINTS node may hold source verts in a
// legacy load, but no chunk and no trace target): the one home of the rule for the writer and the loader.
static inline bool collres_stream_geometry_type(uint8_t type)
{
  return type == COLLISION_NODE_TYPE_MESH || type == COLLISION_NODE_TYPE_CONVEX;
}
// A node with a chunk record.
static inline bool collres_stream_geometry(const CollisionNode &n) { return collres_stream_geometry_type(n.type) && n.hasGeometry(); }

bool CollisionResource::loadStream(IGenLoad &crd, const char *res_name, int (*resolve_phmat)(const char *), int body_bytes)
{
  using PoseMeta = CollisionResourceInstance::PoseMeta;
  G_STATIC_ASSERT(COLLRES_STREAM_ARRAYS == Data::NODE_ORDER); // the wire carries the arrays before NODE_ORDER
  // The wire's array order IS this enum's order, so a reorder here is a format change, not a
  // runtime one: pinned so it cannot happen by accident (collres_info.py reads the same order).
  G_STATIC_ASSERT(Data::NODES == 0 && Data::AUTHORED_TM == 1 && Data::AUTHORED_ITM == 2 && Data::REL_GEOM_TMS == 3 &&
                  Data::NODE_BLAS == 4 && Data::TLAS == 5 && Data::PHYS_MAT_POOL == 6 && Data::CAPSULES == 7 &&
                  Data::CONVEX_PLANES == 8 && Data::NAMES == 9 && Data::MAT_NAMES == 10);
  auto refuse = [&](const char *what) {
    logerr("collision res <%s>: stream refused (%s)", res_name, what);
    return false;
  };
  const int bodyStart = body_bytes >= 0 ? crd.tell() : 0;
  CollResStreamHeader h;
  crd.read(&h, sizeof(h));
  const uint32_t N = h.counts[Data::NODES];
  const vec4f vBsph = v_ldu(h.boundingSphere), vBind = v_ldu(h.bindTraceSphere);
  if (!v_test_xyzw_finite(vBsph) || !v_test_xyzw_finite(vBind) || !v_test_xyz_finite(v_ldu(&h.boundingBox[0].x)) ||
      !v_test_xyz_finite(v_ldu(&h.boundingBox[1].x)) || !check_finite(h.boundingSphereRad))
    return refuse("bounds");
  // the bind trace sphere is the resource sphere, widened: a moved or smaller stamp would shrink the trace prefilter
  if (!v_check_xyz_all_true(v_cmp_eq(vBind, vBsph)) || v_extract_w(vBind) < v_extract_w(vBsph))
    return refuse("bind sphere");
  if (h.collisionFlags & (COLLISION_RES_FLAG_HAS_TRACE_FRT | COLLISION_RES_FLAG_HAS_COLL_FRT))
    return refuse("flags");
  if (N > MAX_COLLISION_NODES || h.counts[Data::AUTHORED_TM] != N ||
      (h.counts[Data::AUTHORED_ITM] != 0 && h.counts[Data::AUTHORED_ITM] != N) ||
      h.counts[Data::REL_GEOM_TMS] != ((h.collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID) ? N : 0u) ||
      h.counts[Data::TLAS] != 0)           // the blueprint TLAS is not on the wire
    return refuse("counts");               // no pool caps: the block total bounds their bytes
  uint32_t counts[Data::ARRAY_COUNT] = {}; // the wire carries the arrays before NODE_ORDER
  for (int id = 0; id < COLLRES_STREAM_ARRAYS; ++id)
    counts[id] = h.counts[id];
  if (Data::blockBytesFor(counts) > MAX_COLLRES_BLOCK_BYTES) // the cap is on the wire arrays
    return refuse("block size");
  counts[Data::TLAS] = tlasReserveBytes(N);                 // the blueprint's home is a load-time reservation, no wire fact
  counts[Data::NODE_ORDER] = N ? Data::LIST_BOUNDS + N : 0; // the bounds lead the array; no nodes, no array
  data = Data::build(counts);                               // the one allocation
  // 1, not the member's 0: a ref whose generation bits are zero must never match a live chunk
  data->nodeBlasBuildId = 1;

  reserve_and_resize(defaultInstance.poseMeta, N);
  dag::Vector<CollResBindRec, framemem_allocator> binds(N);
  if (N)
    crd.read(binds.data(), (int)(N * sizeof(CollResBindRec)));
  for (uint32_t i = 0; i < N; ++i)
  {
    PoseMeta &pm = defaultInstance.poseMeta[i];
    pm.maxTmScale = binds[i].maxTmScale;
    pm.flags = binds[i].flags & COLLRES_POSE_CLASS_BITS;
    pm.status = binds[i].status & COLLRES_BIND_STATUS_PERSISTED;
    pm.behaviorFlags = binds[i].behaviorFlags;
  }

  dag::Span<CollisionNode> nodes = data->allNodesList();
  auto expandNode = [&](CollisionNode &n, const CollResNodeRec &r, uint32_t index) {
    const bool geometry = collres_stream_geometry_type(r.type);
    if (r.type >= NUM_COLLISION_NODE_TYPES || (r.nameOfs != 0 && r.nameOfs >= h.counts[Data::NAMES]) ||
        (r.insideOfNode != CollisionNode::INVALID_IDX && r.insideOfNode >= N) || r.indicesCount % 3u != 0 ||
        r.indicesCount > 0x4000000u || (r.indicesCount && !geometry))
      return false;
    if (r.type == COLLISION_NODE_TYPE_CAPSULE  ? (r.capsuleOrPlanesOfs >= h.counts[Data::CAPSULES] || r.planesCount != 0)
        : r.type == COLLISION_NODE_TYPE_CONVEX ? (uint32_t)r.capsuleOrPlanesOfs + r.planesCount > h.counts[Data::CONVEX_PLANES]
                                               : r.planesCount != 0)
      return false;
    // The node's own bound, the last wire-derived one left unchecked: a NaN radius would answer
    // every sphere query with NaN instead of refusing, and an inverted box would persist as bounds
    // (the v0 arm drops such a node). The zero-vert marker is the one negative radius allowed.
    if (!v_test_xyz_finite(v_ldu(&r.modelBBox[0].x)) || !v_test_xyz_finite(v_ldu(&r.modelBBox[1].x)) ||
        !check_finite(r.radiusAroundBoxCenter) || r.modelBBox[0].x > r.modelBBox[1].x || r.modelBBox[0].y > r.modelBBox[1].y ||
        r.modelBBox[0].z > r.modelBBox[1].z)
      return false;
    n.nameOfs = r.nameOfs;
    n.verticesCount = 0; // the chunk walk fills it from the deserializer
    n.indicesCount = r.indicesCount;
    n.behaviorFlags = r.behaviorFlags;
    n.physMatId = r.physMatId;
    n.insideOfNode = r.insideOfNode;
    n.capsuleIndex = r.capsuleOrPlanesOfs;
    n.planesCount = r.planesCount;
    n.flags = r.flags & COLLRES_NODE_FLAGS_PERSISTED;
    n.type = (CollisionResourceNodeType)r.type;
    n.modelBBox = r.modelBBox;
    n.radiusAroundBoxCenter = r.radiusAroundBoxCenter;
    n.nodeIndex = (uint16_t)index;
    return true;
  };
  for (int id = 0; id < COLLRES_STREAM_ARRAYS; ++id)
  {
    if (id == Data::NODES)
    {
      dag::Vector<CollResNodeRec, framemem_allocator> recs(N);
      if (N)
        crd.read(recs.data(), (int)(N * sizeof(CollResNodeRec)));
      for (uint32_t i = 0; i < N; ++i)
        if (!expandNode(nodes[i], recs[i], i))
          return refuse("node");
    }
    else if (id == Data::NODE_BLAS)
    {
      if (const char *why = readChunks(crd))
        return refuse(why);
    }
    else if (id != Data::TLAS && h.counts[id])
      crd.read((char *)data.get() + data->arrays[id].byteOfs, (int)(h.counts[id] * Data::ELEM_SIZE[id]));
  }

  const dag::Span<char> names = data->names(); // empty for a node-less resource
  if (!names.empty() && names.back() != 0)
    return refuse("names");
  const dag::ConstSpan<char> matNames = data->arr<char>(Data::MAT_NAMES);
  if (!matNames.empty() && matNames.back() != 0)
    return refuse("material names");
  dag::Vector<int16_t, framemem_allocator> ids; // ids[k] = the k-th name, resolved once
  for (const char *s = matNames.data(), *e = s + matNames.size(); s < e; s += strlen(s) + 1)
    ids.push_back((int16_t)(resolve_phmat ? resolve_phmat(s) : PhysMat::getMaterialId(s)));
  auto toId = [&](uint16_t &ref) {
    if (ref != 0xffffu && ref >= ids.size()) // 0xffff: no material
      return false;
    ref = ref == 0xffffu ? ref : (uint16_t)ids[ref];
    return true;
  };
  // the pool walked by its count cells (the slices lie back to back): every entry mapped, an overrun
  // refused; a slice offset off a walked start reads a wrong material, bounded by the accessors
  dag::Span<uint16_t> pool = data->physMatPool();
  for (uint32_t ofs = 0; ofs < pool.size();)
  {
    const uint32_t count = pool[ofs];
    if (ofs + 1u + count > pool.size())
      return refuse("material pool");
    for (uint32_t i = ofs + 1u; i <= ofs + count; ++i)
      if (!toId(pool[i]))
        return refuse("material pool");
    ofs += 1u + count;
  }
  for (CollisionNode &n : nodes)
  {
    uint16_t ref = (uint16_t)n.physMatId;
    if (n.physMatId >= 0 && !toId(ref))
      return refuse("material");
    n.physMatId = (int16_t)ref;
  }

  // The pose seed: the authored placements with the shortcut bit re-derived and the builder's bind
  // gates re-applied; an out-of-band bind scale is recomputed, never refused (an all-zero tm carries 0).
  for (uint32_t i = 0; i < N; ++i)
  {
    mat44f tm;
    v_mat44_make_from_43cu(tm, data->authoredNodeTm()[i].array);
    PoseMeta &pm = defaultInstance.poseMeta[i];
    pm.setPoseIdentity(v_mat44_is_ident43(tm));
    if (!(pm.maxTmScale >= FLT_MIN && pm.maxTmScale <= 1.f / FLT_MIN))
      pm.maxTmScale = mat33SpectralNorm(tm);
    if (!v_test_mat43_finite(tm))
      pm.status &= ~(PoseMeta::TRACEABLE | PoseMeta::COMPOSABLE);
  }
  // The tail: records this loader does not know, skipped by their size. An uncompressed body is
  // bounded by its block; a decoded one ends when the decoder does.
  for (;;)
  {
    if (body_bytes >= 0 && crd.tell() - bodyStart + (int)sizeof(CollResTailRec) > body_bytes)
      break;
    CollResTailRec t;
    const int got = crd.tryRead(&t, sizeof(t));
    if (got == 0)
      break;
    if (got != (int)sizeof(t) || t.tag == 0 || t.bytes > MAX_COLLRES_BLOCK_BYTES ||
        (body_bytes >= 0 && (int64_t)(crd.tell() - bodyStart) + t.bytes > body_bytes))
      return refuse("tail");
    for (uint32_t left = t.bytes; left;)
    {
      char skip[1024];
      const int n = (int)min<uint32_t>(left, sizeof(skip));
      if (crd.tryRead(skip, n) != n)
        return refuse("tail");
      left -= (uint32_t)n;
    }
  }
  // the vector form of the same box, with the lanes past it zero in memory
  vFullBBox.bmin = v_perm_xyzd(v_ldu(&h.boundingBox[0].x), v_zero());
  vFullBBox.bmax = v_perm_xyzd(v_ldu(&h.boundingBox[1].x), v_zero());
  vBoundingSphere = vBsph;
  vBindTraceSphere = vBind;
  boundingBox = h.boundingBox;
  setBoundingSphereRad(h.boundingSphereRad);
  collisionFlags = h.collisionFlags;
  finishLoad(res_name);
  return true;
}

// The chunk records land at a running cursor: the frame into the chunk header, the tree and the verts
// deserialized into the array's tail from there; the header's tree fields come from the result.
const char *CollisionResource::readChunks(IGenLoad &crd)
{
  dag::Span<uint8_t> blas = data->nodeBlasData();
  uint32_t at = 0;
  for (CollisionNode &n : data->allNodesList())
  {
    n.nodeBlasOfs = ~0u;
    if (!n.indicesCount)
      continue;
    if ((uint64_t)at + sizeof(NodeBlasChunkHeader) > blas.size())
      return "chunk past the array";
    NodeBlasChunkHeader *hdr = (NodeBlasChunkHeader *)(blas.data() + at);
    crd.read(hdr, sizeof(CollResChunkFrame));
    // a bad frame reaches Jolt undecoded, whose degenerate check is a shutdown fatal
    const vec3f scale = v_ldu_p3(hdr->scale), invScale = v_ldu_p3(hdr->invScale);
    if (!v_test_xyz_finite(scale) || !v_test_xyz_finite(invScale) || !v_test_xyz_finite(v_ldu_p3(hdr->bmin)) ||
        !v_check_xyz_all_true(v_cmp_gt(scale, v_zero())) || // the pack scale is positive: a mirrored pair keeps the product
        !v_check_xyz_all_true(v_cmp_le(v_abs(v_sub(v_mul(scale, invScale), V_C_ONE)), v_splats(1e-3f))))
      return "chunk frame";
    const build_bvh::Soa4DeserializeResult r = build_bvh::deserializeQuadBLASToSoA4(crd,
      blas.subspan(at + (uint32_t)sizeof(NodeBlasChunkHeader)), build_bvh::Soa4FlagSlots::FromWire);
    if (!r.root.valid())
      return "chunk root";
    // consumers size buffers by the record's face count, so it must be the chunk's
    uint32_t faces = 0;
    const uint8_t *tree = blas.data() + at + sizeof(NodeBlasChunkHeader);
    soa4::iterateLeafRefs(
      tree, r.root, [](vec3f, vec3f) { return true; },
      [&](vec3f, vec3f, soa4::LeafRef, const soa4::LeafLoc &l) {
        faces += quadLeafTriCount(soa4::leafFields(tree, l));
        return false;
      });
    if (faces * 3u != n.indicesCount)
      return "chunk face count";
    n.verticesCount = (uint32_t)r.vertCount;
    hdr->treeBytes = (uint32_t)r.treeBytes;
    hdr->rootRef = r.root;
    hdr->flags = r.edgeFlags ? NodeBlasChunkHeader::HAS_EDGE_FLAGS : 0u;
    n.nodeBlasOfs = at;
    at += (uint32_t)nodeChunkBytes(hdr, n.verticesCount); // a multiple of 8: the MOC "index = ofs / 8" contract holds
  }
  return at == blas.size() ? nullptr : "chunk cursor";
}

// The writer's refusals name their cause: the exporter reports one line per asset.
static bool collres_write_refuse(const char *what)
{
  logerr("collision stream: not written (%s)", what);
  return false;
}

bool CollisionResource::write(IGenSave &cwr, const char *(*mat_name)(int id)) const
{
  const dag::ConstSpan<CollisionNode> nodes = data->allNodesList();
  const uint32_t N = (uint32_t)nodes.size();
  if (defaultInstance.poseMeta.size() != N)
    return collres_write_refuse("the bind meta is not node-parallel");
  if (N > MAX_COLLISION_NODES)
    return collres_write_refuse("more nodes than the loader accepts");
  // the material names in first-use order; index k = the k-th string
  dag::Vector<char> matNames;
  dag::Vector<int16_t> matIds;
  bool matNameMissing = false;
  auto toIndex = [&](uint16_t &ref) {
    if (ref == 0xffffu) // no material
      return;
    for (uint32_t k = 0; k < matIds.size(); ++k)
      if (matIds[k] == (int16_t)ref)
      {
        ref = (uint16_t)k;
        return;
      }
    const char *name = mat_name((int16_t)ref);
    if (!name)
    {
      matNameMissing = true;
      return;
    }
    matNames.insert(matNames.end(), name, name + strlen(name) + 1);
    matIds.push_back((int16_t)ref);
    ref = (uint16_t)(matIds.size() - 1);
  };
  dag::Vector<CollResNodeRec> recs(N);
  uint64_t chunkBytes = 0;
  for (uint32_t i = 0; i < N; ++i)
  {
    const CollisionNode &n = nodes[i];
    CollResNodeRec &r = recs[i];
    const bool geometry = collres_stream_geometry(n);
    if (geometry) // a finalized source has a chunk for every geometry node
    {
      const NodeBlasChunkHeader *hdr = (const NodeBlasChunkHeader *)nodeChunkPtr(n);
      if (!hdr)
        return collres_write_refuse("a geometry node without its chunk");
      chunkBytes += nodeChunkBytes(hdr, n.verticesCount);
    }
    r.nameOfs = n.nameOfs;
    r.indicesCount = geometry ? n.indicesCount : 0u;
    r.behaviorFlags = n.behaviorFlags;
    uint16_t ref = (uint16_t)n.physMatId;
    if (n.physMatId >= 0)
      toIndex(ref);
    r.physMatId = (int16_t)ref;
    r.insideOfNode = n.insideOfNode;
    r.capsuleOrPlanesOfs = n.capsuleIndex;
    r.planesCount = n.planesCount;
    r.flags = n.flags & COLLRES_NODE_FLAGS_PERSISTED;
    r.type = n.type;
    r.modelBBox = n.modelBBox;
    r.radiusAroundBoxCenter = n.radiusAroundBoxCenter;
  }
  dag::Vector<uint16_t> pool(data->physMatPool().begin(), data->physMatPool().end());
  for (uint32_t ofs = 0; ofs < pool.size();)
  {
    const uint32_t count = pool[ofs];
    if (ofs + 1u + count > pool.size())
      return collres_write_refuse("the material pool overruns");
    for (uint32_t i = ofs + 1u; i <= ofs + count; ++i)
      toIndex(pool[i]);
    ofs += 1u + count;
  }
  if (matNameMissing)
    return collres_write_refuse("a material the name map does not know");

  CollResStreamHeader h = {};
  uint32_t counts[Data::ARRAY_COUNT] = {}; // the wire arrays; TLAS and NODE_ORDER are the loader's own
  for (int id = 0; id < COLLRES_STREAM_ARRAYS; ++id)
    counts[id] = data->arrays[id].elemCount;
  counts[Data::NODE_BLAS] = (uint32_t)chunkBytes; // the live chunks in node order, as the loader lands them
  counts[Data::TLAS] = 0;
  counts[Data::MAT_NAMES] = (uint32_t)matNames.size();
  if (Data::blockBytesFor(counts) > MAX_COLLRES_BLOCK_BYTES) // the loader's cap: what it would refuse is not written
    return collres_write_refuse("the block exceeds the loader's cap");
  for (int id = 0; id < COLLRES_STREAM_ARRAYS; ++id)
    h.counts[id] = counts[id];
  v_stu(h.boundingSphere, vBoundingSphere);
  v_stu(h.bindTraceSphere, vBindTraceSphere);
  h.boundingBox = boundingBox;
  h.boundingSphereRad = boundingSphereRad;
  h.collisionFlags = collisionFlags & ~(COLLISION_RES_FLAG_HAS_TRACE_FRT | COLLISION_RES_FLAG_HAS_COLL_FRT);
  cwr.write(&h, sizeof(h));
  dag::Vector<CollResBindRec> binds(N);
  for (uint32_t i = 0; i < N; ++i)
  {
    const CollisionResourceInstance::PoseMeta &pm = defaultInstance.poseMeta[i];
    binds[i] = {pm.maxTmScale, (uint8_t)(pm.flags & COLLRES_POSE_CLASS_BITS), (uint8_t)(pm.status & COLLRES_BIND_STATUS_PERSISTED),
      pm.behaviorFlags};
  }
  cwr.write(binds.data(), data_size(binds));
  for (int id = 0; id < COLLRES_STREAM_ARRAYS; ++id)
    switch (id)
    {
      case Data::NODES: cwr.write(recs.data(), data_size(recs)); break;
      case Data::NODE_BLAS:
        if (!writeChunks(cwr))
          return false;
        break;
      case Data::TLAS: break; // rebuilt at load
      case Data::PHYS_MAT_POOL: cwr.write(pool.data(), data_size(pool)); break;
      case Data::MAT_NAMES: cwr.write(matNames.data(), data_size(matNames)); break;
      default: cwr.write((const char *)data.get() + data->arrays[id].byteOfs, (int)(h.counts[id] * Data::ELEM_SIZE[id])); break;
    }
  return true;
}

void CollisionResourceBuilder::writeRaw(const CollisionResource &res, IGenSave &cwr)
{
  const CollResRawHead h = {res.vBoundingSphere, res.vBindTraceSphere, res.boundingBox, res.boundingSphereRad, res.collisionFlags,
    res.data->blockBytes(), res.bsphereCenterNode, res.defaultInstance.posedSinceBind};
  cwr.write(&h, sizeof(h));
  cwr.write(res.data.get(), h.blockBytes);
  cwr.write(res.defaultInstance.poseMeta.data(), data_size(res.defaultInstance.poseMeta));
  cwr.write(res.defaultInstance.nodeTm.data(), data_size(res.defaultInstance.nodeTm));
}

bool CollisionResource::writeChunks(IGenSave &cwr) const
{
  dag::Vector<uint8_t> stk; // the serializer takes the stackless form
  dag::Vector<uint16_t> words;
  for (const CollisionNode &n : data->allNodesList())
  {
    if (!collres_stream_geometry(n))
      continue;
    const NodeBlasChunkHeader *hdr = (const NodeBlasChunkHeader *)nodeChunkPtr(n);
    if (!hdr)
      return collres_write_refuse("a geometry node without its chunk");
    const uint8_t *tree = (const uint8_t *)(hdr + 1);
    const int vertBytes = (int)n.verticesCount * 8;
    cwr.write(hdr, sizeof(CollResChunkFrame));
    const soa4::StacklessResult sr =
      soa4::buildStackless(tree, hdr->rootRef, (int)alignVert21StreamOfs(hdr->treeBytes), vertBytes, stk);
    if (!sr.valid())
      return collres_write_refuse("a chunk the stackless converter refuses");
    words.clear();
    if (hdr->flags & NodeBlasChunkHeader::HAS_EDGE_FLAGS)
      build_bvh::collectLeafEdgeFlags(tree, hdr->rootRef, words);
    bbox3f box; // write-only on the wire: the loader takes the frame from the record
    box.bmin = v_ldu_p3(hdr->bmin);
    box.bmax = v_madd(v_ldu_p3(hdr->invScale), v_splats(65535.f), box.bmin);
    if (!build_bvh::serializeQuadBLAS(cwr, stk.data(), sr.treeBytes, sr.vertsOfs, (int)n.verticesCount, box, BVH_BLAS_LEAF_SIZE, 8,
          make_span_const(words)))
      return collres_write_refuse("a chunk the bvhIO serializer refuses");
  }
  return true;
}
