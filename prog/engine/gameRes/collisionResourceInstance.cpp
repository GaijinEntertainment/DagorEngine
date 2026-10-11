// Copyright (C) Gaijin Games KFT.  All rights reserved.

// CollisionResourceInstance method definitions; the shared file-local helpers live in
// collisionGameResInternal.h.

#include <gameRes/dag_collisionResource.h>
#include <math/dag_geomTree.h>
#include <math/dag_mathUtils.h>
#include <math/dag_math3d.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>
#include <util/dag_string.h>
#include <daBVH/dag_swBLAS_soa4.h>
#include "collisionGameResInternal.h"

const CollisionResourceInstance::PoseMeta CollisionResourceInstance::PoseMeta::absent_slot = {
  1.f, uint8_t(CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED), uint8_t(0), uint16_t(0)};

static String res_name(const CollisionResource *res) { return res ? res->getGameResName() : String("<none>"); }

bool CollisionResourceInstance::isDefault() const { return res && this == &res->defaultInstance; }

BBox3 CollisionResourceInstance::getNodeResourceBBox(int node_index) const
{
  if (!res)
    return BBox3();
  const CollisionNode *n = res->getNode(node_index);
  if (!n)
    return BBox3(); // out-of-range or absent: empty, the checked-accessor convention
  // Untraceable (NaN/singular) poses report empty by contract: the pose is unrealizable.
  if (!isNodeTraceable(node_index))
    return BBox3();
  // Nodes with no traceable geometry report empty: the exporter's zero-vert r < 0 marker (any node
  // type; report empty, as getNodeBSphere does), a degenerate-dropped mesh/convex whose stale
  // modelBBox has no geometry behind it (the recomputeRootBBox predicate), and POINTS -- never a
  // trace target, and its stored bbox is already resource-space, so composing would double-apply.
  if (n->radiusAroundBoxCenter < 0 || n->type == COLLISION_NODE_TYPE_POINTS ||
      ((n->type == COLLISION_NODE_TYPE_MESH || n->type == COLLISION_NODE_TYPE_CONVEX) && !n->hasGeometry()))
    return BBox3();
  if (n->type == COLLISION_NODE_TYPE_SPHERE)
  {
    TMatrix tm;
    v_mat_43cu_from_mat44(tm.array, getNodeGeometryTm(node_index));
    // Analytic AABB matches the dispatch's reach; corner-mapping a sphere inflates under rotation.
    return composed_sphere_box(tm, n->bsphereCenter(), n->radiusAroundBoxCenter);
  }
  bbox3f box;
  v_bbox3_init(box, getNodeGeometryTm(node_index), res->getNodeGeometryBBox(*n));
  BBox3 out;
  v_stu_bbox3(out, box);
  return out;
}

DAGOR_NOINLINE const TMatrix &CollisionResourceInstance::missingOwnedPose(int node_index) const
{
  G_ASSERTF(false, "collres instance %p of res %p <%s>: owned-pose accessor on node %d %s", this, res, res_name(res).str(), node_index,
    tree ? "of a tree-backed instance" : "is out of range");
  LOGERR_ONCE("collres instance %p of res %p <%s>: owned-pose accessor on node %d %s; bind identity is used", this, res,
    res_name(res).str(), node_index, tree ? "of a tree-backed instance" : "is out of range");
  return TMatrix::IDENT;
}

mat44f CollisionResourceInstance::getNodeGeometryTm(int node_index) const
{
  mat44f posed;
  if (DAGOR_UNLIKELY((uint32_t)node_index >= nodeTm.size()))
  {
    v_mat44_make_from_43cu_unsafe(posed, missingOwnedPose(node_index).array);
    return posed;
  }
  v_mat44_make_from_43cu_unsafe(posed, nodeTm[node_index].array);
  return res->geometryTmFromPosed(node_index, posed, poseMeta[node_index]);
}

// An epsilon-IDENT mesh may be traced in either frame (the chunk walk keeps stored geometry
// while the scalar path composes T), so every derived bound must enclose the raw box too.
static inline bool eps_ident_needs_raw_box(const CollisionNode &node, const CollisionResourceInstance::PoseMeta &pm)
{
  return node.type == COLLISION_NODE_TYPE_MESH && (pm.flags & CollisionNode::IDENT);
}
static inline void add_eps_ident_raw_box(bbox3f &box, const CollisionNode &node, const CollisionResourceInstance::PoseMeta &pm,
  bbox3f_cref raw_box)
{
  if (eps_ident_needs_raw_box(node, pm))
    v_bbox3_add_box(box, raw_box);
}

// An exact-identity frame keeps the stored box verbatim: v_bbox3_init reconstructs through
// absolute center/half-extent and can collapse a one-ULP-wide far-origin box.
static bbox3f composed_or_raw_box(mat44f_cref gtm, bbox3f_cref raw_box)
{
  if (v_mat44_is_ident43(gtm))
    return raw_box;
  bbox3f out;
  v_bbox3_init(out, gtm, raw_box);
  return out;
}

static inline void tlas_row_store_lane(float *row, uint32_t lane, bbox3f_cref box)
{
  alignas(16) float lo[4], hi[4];
  v_st(lo, box.bmin);
  v_st(hi, box.bmax);
  for (int a = 0; a < 3; ++a)
  {
    row[a * 4 + lane] = lo[a];
    row[(3 + a) * 4 + lane] = hi[a];
  }
}

void CollisionResourceInstance::mirrorLiveFlagsIfStale() const
{
  if (isDefault()) // the default IS the source; also the hot path of the default-form queries
    return;
  const uint32_t cur = interlocked_acquire_load(res->liveFlagsGen);
  if (DAGOR_LIKELY(interlocked_acquire_load(liveFlagsGenSeen) == cur))
    return;
  while (DAGOR_UNLIKELY(interlocked_compare_exchange(refreshLock, 1, 0) != 0))
    cpu_yield();
  if (liveFlagsGenSeen != cur)
  {
    const dag::Vector<PoseMeta> &def = res->defaultInstance.poseMeta;
    for (uint32_t i = 0, e = min<uint32_t>(def.size(), poseMeta.size()); i < e; ++i)
      poseMeta[i].behaviorFlags = def[i].behaviorFlags;
    interlocked_release_store(liveFlagsGenSeen, cur);
  }
  interlocked_release_store(refreshLock, 0);
}

void CollisionResourceInstance::refreshIfStale(mat44f_cref entity_tm, bool with_tlas) const
{
  // The tree generation is never 0, so 0 stays "never refreshed". The default-pose stamp
  // catches unbound-node writes, which move matrices without touching the tree generation;
  // its acquire pairs with the refresh's release so a default-pose-only re-derive (the tree
  // stamp re-publishes an unchanged value) still orders the derived-state reads.
  // Derived state anchors to the refreshing trace's entity basis: callers keep that basis
  // stable within a pose generation (re-orienting an entity re-poses its tree).
  if (DAGOR_UNLIKELY(interlocked_acquire_load(poseGeneration) != tree->getPoseGeneration() ||
                     interlocked_acquire_load(defaultPoseGenAtRefresh) != res->defaultPoseGen))
    refreshFromTree(entity_tm, with_tlas);
  mirrorLiveFlagsIfStale();
}

DAGOR_NOINLINE void CollisionResourceInstance::refreshFromTree(mat44f_cref entity_tm, bool with_tlas) const
{
  // Once-per-generation rebuild; concurrent traces of the same instance wait here.
  while (DAGOR_UNLIKELY(interlocked_compare_exchange(refreshLock, 1, 0) != 0))
    cpu_yield();
  const uint32_t cur = tree->getPoseGeneration();
  const uint32_t curDef = res->defaultPoseGen;
  // Lock-skipping readers trust fresh pose state, so a TLAS-only pass must not rewrite it.
  const bool poseStale = interlocked_acquire_load(poseGeneration) != cur || defaultPoseGenAtRefresh != curDef;
  const bool tlasStale =
    with_tlas && (tlasPoseGeneration != cur || tlasBlueprintGenSeen != res->data->tlasGeneration || tlasDefaultPoseGenSeen != curDef);
  const auto publishTlasStamps = [&] {
    interlocked_release_store(tlasDefaultPoseGenSeen, curDef);
    interlocked_release_store(tlasBlueprintGenSeen, res->data->tlasGeneration);
    interlocked_release_store(tlasPoseGeneration, cur);
  };
  if (poseStale || tlasStale)
  {
    if (DAGOR_UNLIKELY((uint32_t)tree->nodeCount() != treeNodeCountAtBind))
    {
      // Pre-drift leaf boxes would make the prefilter skip nodes the new layout reaches; the
      // release below publishes the clear to lock-skipping readers.
      tlas.data.clear();
      if (tlasStale)
        publishTlasStamps();
      else
        interlocked_release_store(tlasPoseGeneration, 0u);
      if (poseStale)
      {
        // Layout drifted under a live binding: disable the posed reject rather than trace stale bounds.
        v_bbox3_init(rootBBox, v_make_vec4f(-FLT_MAX / 4, -FLT_MAX / 4, -FLT_MAX / 4, 0));
        v_bbox3_add_pt(rootBBox, v_make_vec4f(FLT_MAX / 4, FLT_MAX / 4, FLT_MAX / 4, 0));
        hasBsphereCenterLocal = false;
        interlocked_release_store(defaultPoseGenAtRefresh, curDef);
        interlocked_release_store(poseGeneration, cur);
      }
      const bool firstDrift = !layoutDriftAsserted; // latched: one assert per binding, not per generation
      layoutDriftAsserted = true;
      interlocked_release_store(refreshLock, 0);
      // Diagnostics AFTER the release: an assert stop or log emit under the spinlock would
      // stall every concurrent trace of this instance.
      if (DAGOR_UNLIKELY(firstDrift))
      {
        G_ASSERTF(false, "collres instance %p of res %p <%s>: bound tree layout changed (%d nodes vs %d); rebind the instance", this,
          res, res_name(res).str(), (int)tree->nodeCount(), (int)treeNodeCountAtBind);
        LOGERR_ONCE("collres instance %p of res %p <%s>: bound tree layout changed (%d nodes vs %d)", this, res, res_name(res).str(),
          (int)tree->nodeCount(), (int)treeNodeCountAtBind);
      }
      return;
    }
    // Compose translation entity-relative to preserve large-world precision.
    const mat44f entityRot = {entity_tm.col0, entity_tm.col1, entity_tm.col2, v_zero()};
    mat44f invEntity;
    v_mat44_inverse43(invEntity, entityRot);
    const vec3f relOfs = v_sub(tree->getWtmOfs(), entity_tm.col3);
    // An entity basis rigid to 1e-5 maps a rigid posed basis back to a wtm under the COLLRES_RIGID_SIGMA_MAX pad,
    // so a driven wtm takes its own test only where the posed basis is not rigid or a rel tm sits between them.
    vec3f entityDiag, entityOff;
    v_mat33_gram(entityDiag, entityOff, entity_tm.col0, entity_tm.col1, entity_tm.col2);
    const bool wtmFollowsPosed =
      collres_gram_in_band(entityDiag, entityOff, 1e-5f) && !(res->collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID);
    if (poseStale)
    {
      hasBsphereCenterLocal = res->bsphereCenterNode && res->bsphereCenterNode.index() < tree->nodeCount();
      if (hasBsphereCenterLocal)
      {
        mat44f centerTm = tree->getNodeWtmRel(res->bsphereCenterNode);
        centerTm.col3 = v_add(centerTm.col3, relOfs);
        bsphereCenterLocal = v_mat44_mul_vec3p(invEntity, centerTm.col3);
      }
    }
    // Diagnostics collected under the lock, emitted after the release (an assert stop or a
    // log emit under the spinlock stalls every concurrent trace of this instance).
    int hiddenNode = -1;
    bbox3f box;
    v_bbox3_init_empty(box);
    dag::ConstSpan<CollisionNode> nodes = res->getAllNodes();
    // An empty lane contributes no leaf. The operand maxima are taken per factor, a bound over the per-leaf maximum.
    // Refresh-wide scale floor and conditioning max for the budget: only this walk sees the composed frames, not guaranteed R+T.
    bbox3f leafUnion;
    v_bbox3_init_empty(leafUnion);
    float treeScale = 0.f, treeAmpl = 1.f, maxOwnOp = 0.f, maxRawMag = 0.f, maxBoxMag = 0.f;
    // One node in scalar form: the pose gate when the pose is stale, the root box, and with a row the leaf box of that
    // lane of its TLAS node. Returns whether the node bounds.
    auto visitNode = [&](uint32_t i, float *row, int lane) DAGOR_FORCE_INLINE_LAMBDA -> bool {
      const CollisionNode &node = nodes[i];
      PoseMeta &pm = poseMeta[i];
      const mat44f posed = treePosedNodeTm(i, invEntity, relOfs);
      vec3f gramDiag, gramOff;
      v_mat33_gram(gramDiag, gramOff, posed.col0, posed.col1, posed.col2);
      const bool posedRigid = collres_gram_in_band(gramDiag, gramOff, COLLRES_RIGID_GRAM_EPS);
      if (poseStale && node.geomNodeId.index() < tree->nodeCount())
      {
        const mat44f &wtm = tree->getNodeWtmRel(node.geomNodeId);
        // A degenerate driven wtm hides the node like the owned form; it returns when the wtm recovers.
        // Handedness is not gated, as in the legacy tree overloads: a mirrored bone traces its stored winding as they do.
        // All four columns: a NaN translation passes a basis-only det gate.
        // Normalized det floor, not a raw det != 0: a finite near-singular wtm would pass the
        // raw compare and the trace would invert an ill-conditioned matrix unguarded (the
        // owned form hides the same class through the identical floor).
        float ndet;
        const bool finitePose = v_test_mat43_finite(posed);
        bool wtmRigid = posedRigid && wtmFollowsPosed, collapsed = false;
        if (!wtmRigid)
        {
          vec3f wtmDiag, wtmOff;
          v_mat33_gram(wtmDiag, wtmOff, wtm.col0, wtm.col1, wtm.col2);
          wtmRigid = collres_gram_in_band(wtmDiag, wtmOff, COLLRES_RIGID_GRAM_EPS);
          // An axis scale of 1e-9 or less hides the node without a warning: content hides parts this way.
          collapsed = finitePose && v_check_xyz_any_true(v_cmp_le(wtmDiag, v_splats(1e-18f)));
        }
        const bool ok = // a NaN basis is excluded by finitePose before the floor; a rigid basis passes it
          pm.isBindTraceable() && finitePose && !collapsed && (posedRigid || CollisionResource::relativeDetAboveFloor(posed, ndet));
        if (DAGOR_UNLIKELY(!ok && pm.isBindTraceable() && !collapsed) && hiddenNode < 0)
          hiddenNode = (int)i; // warned after the refresh releases the spinlock
        pm.setTraceable(ok);
        if (ok && !wtmRigid)
        {
          mat44f treeWorldTm;
          v_mat44_mul43(treeWorldTm, wtm, drivenRelTm(i));
          setDrivenPoseClass(i, treeWorldTm, true);
        }
        else if (wtmRigid && pm.isLiveClassified())
          setDrivenPoseClass(i, drivenRelTm(i), false);
      }
      else if (poseStale)
      {
        // Unbound nodes read the default pose live, so its reclassification must land here
        // too; a structural hide stays this instance's own.
        PoseMeta dm = res->defaultInstance.poseMeta[i];
        dm.setDisabled(pm.isDisabled());
        pm = dm;
      }
      // hidden or unrealizable poses never trace, so they must not bound
      if (pm.isDisabled() || !pm.isTraceable())
        return false;
      if (node.type == COLLISION_NODE_TYPE_POINTS)
        return false; // never a trace target
      if (node.radiusAroundBoxCenter < 0.f)
        return false; // zero-vert marker: corner-mapping the inverted box would union a phantom
      if ((node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX) && !node.hasGeometry())
        return false;
      const mat44f gtm = res->geometryTmFromPosed((int)i, posed, pm);
      bbox3f nodeBox = res->getNodeGeometryBBox(node);
      bbox3f composedBox = composed_or_raw_box(gtm, nodeBox);
      if (poseStale)
      {
        // NaN never joins and Inf saturates, like every other rootBBox writer: a poisoned box would false-reject every trace.
        join_saturated_finite(box, composedBox);
        // same predicate as the leaf encoders; the join keeps this arm's poison guard
        if (eps_ident_needs_raw_box(node, pm))
          join_saturated_finite(box, nodeBox);
      }
      if (!row)
        return true;
      bbox3f leafBox = composedBox;
      add_eps_ident_raw_box(leafBox, node, pm, nodeBox);
      if (v_bbox3_is_empty(leafBox))
        return false;
      v_bbox3_add_box(leafUnion, leafBox);
      bool gtmRigid = posedRigid; // else gtm is posed or IDENT
      if (CollisionResource::usesAuthoredFrame(node, pm))
      {
        vec3f gtmDiag, gtmOff;
        v_mat33_gram(gtmDiag, gtmOff, gtm.col0, gtm.col1, gtm.col2);
        gtmRigid = collres_gram_in_band(gtmDiag, gtmOff, COLLRES_RIGID_GRAM_EPS);
      }
      if (gtmRigid)
      {
        treeScale = max(treeScale, COLLRES_RIGID_SIGMA_MAX);
        treeAmpl = max(treeAmpl, COLLRES_RIGID_CONDITIONING);
      }
      else
      {
        const float sigma = collres_sigma_max_bound(gtm);
        treeScale = max(treeScale, sigma);
        treeAmpl = max(treeAmpl, collres_tlas_tm_conditioning(gtm, sigma));
      }
      const float rawMag = collres_box_max_abs(nodeBox), boxMag = collres_box_max_abs(leafBox);
      maxRawMag = max(maxRawMag, rawMag);
      maxBoxMag = max(maxBoxMag, boxMag);
      maxOwnOp = max(maxOwnOp, collres_tlas_node_op(pm.maxTmScale, rawMag, boxMag, 1.f));
      tlas_row_store_lane(row, lane, leafBox);
      return true;
    };
    // out of line, so the rare fallback lane leaves the four-wide loop its registers
    auto visitLane = [&](uint32_t i, float *row, int lane) DAGOR_NOINLINE_LAMBDA -> bool { return visitNode(i, row, lane); };

    // TLAS leaves 4-wide, in the TLAS node order: a node's leaf children are one group, and its box rows are what the
    // encode quantizes. A lane takes the scalar path unless it is a bound, rigid, finite, frame-free driven node.
    const uint32_t rowNodes = tlasStale ? res->data->tlasInnerCount : 0u;
    dag::Vector<vec4f, framemem_allocator> rowStore(rowNodes * 6u);
    dag::Vector<uint8_t, framemem_allocator> rowMasks(rowNodes);
    float *rows = (float *)rowStore.data();
    vec4f accMin[3] = {V_C_MAX_VAL, V_C_MAX_VAL, V_C_MAX_VAL}, accMax[3] = {V_C_MIN_VAL, V_C_MIN_VAL, V_C_MIN_VAL};
    vec4f accRaw = v_zero(), accBox = v_zero(), accOwn = v_zero();
    bool anyFastLane = false;
    if (tlasStale)
    {
      const uint8_t *blueprint = res->data->tlasData().data();
      const dag::ConstSpan<uint32_t> nodeOfs = res->tlasNodeOfs(), parentPacked = res->tlasNodeParentPacked();
      // rel tms compose one more matrix per lane: those resources keep the scalar path
      const bool lanesMayRun = !(res->collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID);
      vec4f E[4][3]; // component r of invEntity column c, splat
      for (int c = 0; c < 4; ++c)
      {
        const vec4f col = (&invEntity.col0)[c];
        E[c][0] = v_splat_x(col);
        E[c][1] = v_splat_y(col);
        E[c][2] = v_splat_z(col);
      }
      const vec4f ofs[3] = {v_splat_x(relOfs), v_splat_y(relOfs), v_splat_z(relOfs)};
      const vec4f rigidEps = v_splats(COLLRES_RIGID_GRAM_EPS);
      const vec4f bandLo = v_sub(V_C_ONE, rigidEps), bandHi = v_add(V_C_ONE, rigidEps);
      mat44f identTm;
      v_mat44_ident(identTm);
      for (uint32_t k = 0; k < rowNodes; ++k)
      {
        const soa4::NodeRef nd(blueprint, soa4::makeNodeRef(nodeOfs[k], CollisionResource::tlasChildCountOf(parentPacked[k]), 0u));
        float *row = rows + k * 24u;
        int ci[4] = {-1, -1, -1, -1};
        const uint32_t leafBits = nd.leafMask();
        uint32_t fastBits = 0;
        for (int l = 0; l < nd.N; ++l)
        {
          if (!(leafBits & (1u << l)))
            continue;
          ci[l] = (int)(nd.w()[l] & ~BLAS_LEAF_FLAG);
          if ((uint32_t)ci[l] >= (uint32_t)poseMeta.size())
          {
            ci[l] = -1; // no pose slot: the lane encodes empty
            continue;
          }
          const CollisionNode &node = nodes[ci[l]];
          const PoseMeta &pm = poseMeta[ci[l]];
          if (lanesMayRun && node.geomNodeId.index() < tree->nodeCount() && !CollisionResource::usesAuthoredFrame(node, pm) &&
              !(pm.isGeometryBaked() && !pm.isRetainedBake()) && !(pm.flags & CollisionNode::IDENT) && !pm.isLiveClassified() &&
              node.radiusAroundBoxCenter >= 0.f)
            fastBits |= 1u << l;
        }
        if (!leafBits)
          continue;
        uint32_t validBits = 0;
        if (fastBits)
        {
          const mat44f *w[4];
          bbox3f raw[4];
          for (int l = 0; l < 4; ++l)
          {
            const bool fast = fastBits & (1u << l);
            w[l] = fast ? &tree->getNodeWtmRel(nodes[ci[l]].geomNodeId) : &identTm;
            if (fast)
              raw[l] = res->getNodeGeometryBBox(nodes[ci[l]]);
            else
              raw[l].bmin = raw[l].bmax = v_zero();
          }
          // W[r][j]: component r of wtm column j, one lane per child
          vec4f W[3][4];
          for (int j = 0; j < 4; ++j)
          {
            vec4f a0 = (&w[0]->col0)[j], a1 = (&w[1]->col0)[j], a2 = (&w[2]->col0)[j], a3 = (&w[3]->col0)[j];
            v_mat44_transpose(a0, a1, a2, a3);
            W[0][j] = a0;
            W[1][j] = a1;
            W[2][j] = a2;
          }
          // treePosedNodeTm lane by lane: the same products in the same order, so the result is bitwise the scalar one
          vec4f P[3][4];
          for (int r = 0; r < 3; ++r)
          {
            for (int j = 0; j < 3; ++j)
              P[r][j] = v_madd(E[2][r], W[2][j], v_madd(E[1][r], W[1][j], v_mul(E[0][r], W[0][j])));
            const vec4f t0 = v_add(W[0][3], ofs[0]), t1 = v_add(W[1][3], ofs[1]), t2 = v_add(W[2][3], ofs[2]);
            P[r][3] = v_add(v_madd(E[2][r], t2, v_mul(E[0][r], t0)), v_madd(E[1][r], t1, E[3][r]));
          }
          // v_mat33_gram and collres_gram_in_band lane by lane: the rigid verdict is bitwise the scalar one too
          auto rigidLanes = [&](const vec4f(&M)[3][4]) DAGOR_FORCE_INLINE_LAMBDA {
            vec4f r = v_cmp_lt(v_abs(v_madd(M[1][0], M[1][1], v_madd(M[2][0], M[2][1], v_mul(M[0][0], M[0][1])))), rigidEps);
            r = v_and(r, v_cmp_lt(v_abs(v_madd(M[1][1], M[1][2], v_madd(M[2][1], M[2][2], v_mul(M[0][1], M[0][2])))), rigidEps));
            r = v_and(r, v_cmp_lt(v_abs(v_madd(M[1][2], M[1][0], v_madd(M[2][2], M[2][0], v_mul(M[0][2], M[0][0])))), rigidEps));
            for (int j = 0; j < 3; ++j)
            {
              const vec4f d = v_madd(M[1][j], M[1][j], v_madd(M[2][j], M[2][j], v_mul(M[0][j], M[0][j])));
              r = v_and(r, v_and(v_cmp_gt(d, bandLo), v_cmp_lt(d, bandHi)));
            }
            return r;
          };
          vec4f rigid = rigidLanes(P);
          if (!wtmFollowsPosed) // a scaled bone takes the scalar path, which classifies it live
            rigid = v_and(rigid, rigidLanes(W));
          // one sum per lane: an Inf or NaN anywhere keeps it non-finite, an overflow only sends the lane to the scalar path
          vec4f sum = v_add(v_add(P[0][3], P[1][3]), P[2][3]);
          for (int r = 0; r < 3; ++r)
            sum = v_add(sum, v_add(v_add(P[r][0], P[r][1]), P[r][2]));
          // v_bbox3_rotate_init lane by lane, as composed_or_raw_box runs it, raw where the frame is exact IDENT
          vec4f bmn[3] = {raw[0].bmin, raw[1].bmin, raw[2].bmin}, bmx[3] = {raw[0].bmax, raw[1].bmax, raw[2].bmax};
          vec4f mn3 = raw[3].bmin, mx3 = raw[3].bmax;
          v_mat44_transpose(bmn[0], bmn[1], bmn[2], mn3);
          v_mat44_transpose(bmx[0], bmx[1], bmx[2], mx3);
          vec4f ident = v_and(v_cmp_eq(P[0][0], V_C_ONE), v_and(v_cmp_eq(P[1][1], V_C_ONE), v_cmp_eq(P[2][2], V_C_ONE)));
          vec4f offZero = v_or(v_or(v_abs(P[1][0]), v_abs(P[2][0])), v_or(v_abs(P[0][1]), v_abs(P[2][1])));
          offZero =
            v_or(offZero, v_or(v_or(v_abs(P[0][2]), v_abs(P[1][2])), v_or(v_abs(P[0][3]), v_or(v_abs(P[1][3]), v_abs(P[2][3])))));
          ident = v_and(ident, v_cmp_eq(offZero, v_zero()));
          vec4f lo[3], hi[3], boxSum = v_zero(), rawMag = v_zero(), boxMag = v_zero();
          const vec4f center[3] = {
            v_mul(v_add(bmn[0], bmx[0]), V_C_HALF), v_mul(v_add(bmn[1], bmx[1]), V_C_HALF), v_mul(v_add(bmn[2], bmx[2]), V_C_HALF)};
          const vec4f extent[3] = {
            v_mul(v_sub(bmx[0], bmn[0]), V_C_HALF), v_mul(v_sub(bmx[1], bmn[1]), V_C_HALF), v_mul(v_sub(bmx[2], bmn[2]), V_C_HALF)};
          for (int r = 0; r < 3; ++r)
          {
            const vec4f c = v_madd(P[r][2], center[2], v_madd(P[r][1], center[1], v_mul(P[r][0], center[0])));
            const vec4f e = v_madd(v_abs(P[r][2]), extent[2], v_madd(v_abs(P[r][1]), extent[1], v_mul(v_abs(P[r][0]), extent[0])));
            lo[r] = v_sel(v_add(v_sub(c, e), P[r][3]), bmn[r], ident);
            hi[r] = v_sel(v_add(v_add(c, e), P[r][3]), bmx[r], ident);
            boxSum = v_add(boxSum, v_add(lo[r], hi[r]));
            rawMag = v_max(rawMag, v_max(v_abs(bmn[r]), v_abs(bmx[r])));
            boxMag = v_max(boxMag, v_max(v_abs(lo[r]), v_abs(hi[r])));
          }
          const int rigidBits = v_truemask(rigid), finiteBits = v_truemask(v_and(v_is_finite(sum), v_is_finite(boxSum)));
          for (int l = 0; l < 4; ++l)
          {
            const uint32_t bit = 1u << l;
            if (!(fastBits & bit))
              continue;
            if (!(rigidBits & finiteBits & bit))
            {
              fastBits &= ~bit; // the scalar path hides it or runs the exact floor and budget
              continue;
            }
            PoseMeta &pm = poseMeta[ci[l]];
            if (poseStale)
              pm.setTraceable(pm.isBindTraceable()); // a rigid finite pose passes the scalar gate
            if (pm.isTraceable() && !pm.isDisabled())
              validBits |= bit;
          }
          if (validBits)
          {
            anyFastLane = true;
            const vec4f valid = v_cast_vec4f(
              v_make_vec4i((validBits & 1) ? -1 : 0, (validBits & 2) ? -1 : 0, (validBits & 4) ? -1 : 0, (validBits & 8) ? -1 : 0));
            const vec4f scale =
              v_make_vec4f(ci[0] >= 0 ? poseMeta[ci[0]].maxTmScale : 0.f, ci[1] >= 0 ? poseMeta[ci[1]].maxTmScale : 0.f,
                ci[2] >= 0 ? poseMeta[ci[2]].maxTmScale : 0.f, ci[3] >= 0 ? poseMeta[ci[3]].maxTmScale : 0.f);
            for (int r = 0; r < 3; ++r)
            {
              accMin[r] = v_min(accMin[r], v_sel(V_C_MAX_VAL, lo[r], valid));
              accMax[r] = v_max(accMax[r], v_sel(V_C_MIN_VAL, hi[r], valid));
            }
            accRaw = v_max(accRaw, v_and(rawMag, valid));
            accBox = v_max(accBox, v_and(boxMag, valid));
            accOwn = v_max(accOwn, v_and(v_madd(v_add(scale, scale), rawMag, boxMag), valid));
          }
          for (int r = 0; r < 3; ++r)
          {
            v_st(row + r * 4, lo[r]);
            v_st(row + (3 + r) * 4, hi[r]);
          }
        }
        for (int l = 0; l < 4; ++l)
          if (ci[l] >= 0 && !(fastBits & (1u << l)) && visitLane((uint32_t)ci[l], row, l))
            validBits |= 1u << l;
        rowMasks[k] = (uint8_t)(leafBits | (validBits << 4));
      }
    }
    // the rest in scalar form: every node for a pose-only pass, the nodes without a TLAS leaf otherwise
    const dag::ConstSpan<uint16_t> leafOrd = res->tlasNodeLeafOrd();
    if (poseStale)
      for (uint32_t i = 0, e = min<uint32_t>(nodes.size(), (uint32_t)poseMeta.size()); i < e; ++i)
        if (!tlasStale || leafOrd[i] == 0xffffu)
          visitNode(i, nullptr, 0);
    if (anyFastLane)
    {
      // Fast lanes are finite, so their union joins the root box unsaturated, as each lane would.
      bbox3f fast;
      fast.bmin = v_make_vec4f(v_extract_x(v_hmin(accMin[0])), v_extract_x(v_hmin(accMin[1])), v_extract_x(v_hmin(accMin[2])), 0.f);
      fast.bmax = v_make_vec4f(v_extract_x(v_hmax(accMax[0])), v_extract_x(v_hmax(accMax[1])), v_extract_x(v_hmax(accMax[2])), 0.f);
      v_bbox3_add_box(leafUnion, fast);
      if (poseStale)
        join_saturated_finite(box, fast);
      treeScale = max(treeScale, COLLRES_RIGID_SIGMA_MAX);
      treeAmpl = max(treeAmpl, COLLRES_RIGID_CONDITIONING);
      maxRawMag = max(maxRawMag, v_extract_x(v_hmax(accRaw)));
      maxBoxMag = max(maxBoxMag, v_extract_x(v_hmax(accBox)));
      maxOwnOp = max(maxOwnOp, v_extract_x(v_hmax(accOwn)));
    }
    if (poseStale)
    {
      rootBBox = box;
      // The default stamp is the release publication a default-pose-only re-derive pairs with
      // (poseGeneration may re-publish an unchanged value and give the reader no edge).
      interlocked_release_store(defaultPoseGenAtRefresh, curDef);
      interlocked_release_store(poseGeneration, cur);
    }
    if (tlasStale)
    {
      if (!declineForLeaflessRecovery())
        refitAllTlasLeaves(rows, rowMasks.data(), leafUnion,
          max(treeAmpl * maxOwnOp, collres_tlas_node_op(treeScale, maxRawMag, maxBoxMag, treeAmpl)), treeAmpl);
      tlasEntityBasisSeen = {entity_tm.col0, entity_tm.col1, entity_tm.col2, entity_tm.col3};
      publishTlasStamps();
    }
    interlocked_release_store(refreshLock, 0);
    if (DAGOR_UNLIKELY(hiddenNode >= 0))
      LOGWARN_ONCE("collres instance %p of res %p <%s>: degenerate driven wtm hides node %d <%s> of bone <%s>", this, res,
        res_name(res).str(), hiddenNode, res->getNodeName(hiddenNode), tree->getNodeName(nodes[hiddenNode].geomNodeId));
    return;
  }
  interlocked_release_store(refreshLock, 0);
}

mat44f CollisionResourceInstance::treePosedNodeTm(uint32_t i, mat44f_cref inv_entity, vec3f rel_ofs) const
{
  const CollisionNode &node = res->getAllNodes()[i];
  auto gnId = node.geomNodeId.index() < tree->nodeCount() ? node.geomNodeId : dag::Index16();
  if (!gnId)
    return res->defaultInstance.getNodeTm((int)i);
  mat44f m = tree->getNodeWtmRel(gnId);
  m.col3 = v_add(m.col3, rel_ofs);
  v_mat44_mul43(m, inv_entity, m);
  if (res->collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    v_mat44_mul43(m, m, drivenRelTm(i));
  return m;
}

mat44f CollisionResourceInstance::drivenRelTm(uint32_t i) const
{
  mat44f relTm;
  if ((res->collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID) && i < (uint32_t)res->data->relGeomNodeTms().size())
    v_mat44_make_from_43cu_unsafe(relTm, res->data->relGeomNodeTms()[i].array);
  else
    v_mat44_ident(relTm);
  return relTm;
}

void CollisionResourceInstance::setDrivenPoseClass(uint32_t i, mat44f_cref node_tm, bool live) const
{
  PoseMeta &pm = poseMeta[i];
  uint8_t flags = CollisionResource::classifyNodeTmFlags(node_tm, pm.maxTmScale);
  if (!(flags & CollisionNode::ORTHONORMALIZED))
    pm.maxTmScale = max(pm.maxTmScale, mat33_spectral_norm(node_tm) * 1.0002f);
  else if (!v_mat44_is_signed_perm33(node_tm))
    pm.maxTmScale = max(pm.maxTmScale, 1.0015f);         // in-class shear: the Gershgorin pad of the builder's scale stamp
  pm.maxTmScale *= live ? 1.f : COLLRES_RIGID_SIGMA_MAX; // the rigid-band wtm the bind meta stands for
  // A driven node can leave its authored placement, so IDENT demotes to the translate class.
  if (flags & CollisionNode::IDENT)
    flags = (flags & ~CollisionNode::IDENT) | CollisionNode::TRANSLATE;
  pm.flags = flags;
  pm.setLiveClassified(live);
}

void CollisionResourceInstance::refreshTlasIfStale(mat44f_cref entity_tm) const
{
  if (!tree || !res || !res->hasAllNodesTLAS())
    return;
  // Acquire pairs with the refit's release: a trace that skips the lock on equal stamps sees
  // the fully refit clone and frame. A blueprint-only rebuild leaves the pose stamp unchanged,
  // so the blueprint stamp carries its own publication edge. The resource stamps read plain:
  // rebuilds and default-pose writes are serialized against traces.
  if (DAGOR_UNLIKELY(interlocked_acquire_load(tlasPoseGeneration) != interlocked_acquire_load(poseGeneration) ||
                     interlocked_acquire_load(tlasBlueprintGenSeen) != res->data->tlasGeneration ||
                     interlocked_acquire_load(tlasDefaultPoseGenSeen) != res->defaultPoseGen))
    refreshFromTree(entity_tm, true);
}

// A leafless node the current pose makes traceable has no place in the cloned topology, so
// candidate mode would never visit it; both the pose write and the full refit check this.
static inline bool tlas_leafless_recovery(const CollisionNode &n, const CollisionResourceInstance::PoseMeta &pm, float bsph_r)
{
  return !pm.isDisabled() && pm.isTraceable() && collres_tlas_recovery_capable(n, bsph_r);
}

void CollisionResourceInstance::declineTlas() const
{
  tlas.data.clear();
  tlasDeclined = true;
  tlasDeclinedGen = res->data->tlasGeneration;
}

bool CollisionResourceInstance::ownedPosedNodeBox(int node_index, bbox3f &out_box, bbox3f *out_raw_box) const
{
  if ((uint32_t)node_index >= poseMeta.size() || (uint32_t)node_index >= nodeTm.size())
    return false;
  const PoseMeta &pm = poseMeta[node_index];
  // hidden or unrealizable poses never trace, so they must not bound
  if (pm.isDisabled() || !pm.isTraceable())
    return false;
  const CollisionNode &node = res->getAllNodes()[node_index];
  if (node.type == COLLISION_NODE_TYPE_POINTS)
    return false;
  if ((node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX) && !node.hasGeometry())
    return false;
  mat44f posed;
  v_mat44_make_from_43cu_unsafe(posed, nodeTm[node_index].array);
  const bbox3f rawBox = res->getNodeGeometryBBox(node);
  if (out_raw_box)
    *out_raw_box = rawBox;
  out_box = composed_or_raw_box(res->geometryTmFromPosed(node_index, posed, pm), rawBox);
  add_eps_ident_raw_box(out_box, node, pm, rawBox);
  return true;
}

// An empty lane (min 65535 / max 0) is neutral under this union but NOT culled by the slab
// test, so a hidden node still reaches the passes: their traceability checks are load-bearing.
static inline void tlas_node_union(const uint16_t *a, int n, uint16_t mn[3], uint16_t mx[3])
{
  for (int k = 0; k < 3; ++k)
  {
    uint16_t lo = 65535, hi = 0;
    for (int l = 0; l < n; ++l)
    {
      const uint16_t v0 = a[k * n + l], v1 = a[(3 + k) * n + l];
      lo = v0 < lo ? v0 : lo;
      hi = v1 > hi ? v1 : hi;
    }
    mn[k] = lo;
    mx[k] = hi;
  }
}

void CollisionResourceInstance::encodeTlasLeafBox(int node_index, uint32_t leaf_loc, const bbox3f *box) const
{
  const uint32_t ni = leaf_loc >> 2;
  const int lane = (int)(leaf_loc & 3u);
  uint16_t *a = (uint16_t *)(tlas.data.data() + res->tlasNodeOfs()[ni]);
  bbox3f nodeBox;
  if (box)
    nodeBox = *box;
  else if (!ownedPosedNodeBox(node_index, nodeBox))
    v_bbox3_init_empty(nodeBox);
  uint16_t mn[3], mx[3];
  if (v_bbox3_is_empty(nodeBox))
  {
    mn[0] = mn[1] = mn[2] = 65535;
    mx[0] = mx[1] = mx[2] = 0;
  }
  else
    collres_tlas_quant_lane(nodeBox, tlas.scale, tlas.origin, tlas.fpErr, mn, mx);
  soa4::storeLaneBoxU16((uint8_t *)a, CollisionResource::tlasChildCountOf(res->tlasNodeParentPacked()[ni]), lane, mn, mx);
}

bool CollisionResourceInstance::declineForLeaflessRecovery() const
{
  // The precomputed watchlist keeps the scan per leaf, never per node.
  for (uint16_t wi : res->tlasWatchNodes())
    if (DAGOR_UNLIKELY(wi < (uint32_t)poseMeta.size() && !poseMeta[wi].isDisabled() && poseMeta[wi].isTraceable()))
    {
      declineTlas();
      return true;
    }
  return false;
}

void CollisionResourceInstance::refitAllTlasLeaves(float frame_headroom) const
{
  if (!res || !res->hasAllNodesTLAS() || nodeTm.empty() || declineForLeaflessRecovery())
    return;
  dag::ConstSpan<CollisionNode> nodes = res->getAllNodes();
  dag::Vector<vec4f, framemem_allocator> rowStore(res->data->tlasInnerCount * 6u);
  dag::Vector<uint8_t, framemem_allocator> rowMasks(res->data->tlasInnerCount);
  bbox3f leafUnion;
  v_bbox3_init_empty(leafUnion);
  // Budget from pre-compose RAW operands (products cancel in composed AABBs), over
  // contributing leaves only (an untraceable pose can carry NaN and its box stays empty).
  float maxNodeOp = 0.f, amplMax = 1.f;
  for (uint16_t i : res->tlasLeafNodes())
  {
    const uint32_t k = nodes[i].tlasLeafLoc >> 2, lane = nodes[i].tlasLeafLoc & 3u;
    rowMasks[k] |= 1u << lane;
    bbox3f box, rawBox;
    v_bbox3_init_empty(rawBox); // MSVC C4701: the empty-box continue guards the unwritten arm, but flow analysis cannot see it
    if (!ownedPosedNodeBox(i, box, &rawBox))
      v_bbox3_init_empty(box);
    if (v_bbox3_is_empty(box))
      continue;
    rowMasks[k] |= 16u << lane;
    tlas_row_store_lane((float *)rowStore.data() + k * 24u, lane, box);
    v_bbox3_add_box(leafUnion, box);
    // The node test inverts the GEOMETRY matrix, so its conditioning amplifies the budget until the envelope declines.
    mat44f pt;
    v_mat44_make_from_43cu_unsafe(pt, nodeTm[i].array);
    const float ampl = collres_tlas_tm_conditioning(res->geometryTmFromPosed(i, pt, poseMeta[i]));
    amplMax = max(amplMax, ampl);
    maxNodeOp =
      max(maxNodeOp, collres_tlas_node_op(poseMeta[i].maxTmScale, collres_box_max_abs(rawBox), collres_box_max_abs(box), ampl));
  }
  refitAllTlasLeaves((const float *)rowStore.data(), rowMasks.data(), leafUnion, maxNodeOp, amplMax, frame_headroom);
}

void CollisionResourceInstance::refitAllTlasLeaves(const float *leaf_rows, const uint8_t *row_masks, bbox3f_cref leaf_union,
  float max_node_op, float ampl_max, float frame_headroom) const
{
  // The clone materializes only AFTER the envelope gate passes:
  // a declined pose must not reallocate and copy the blueprint just to decline and free it again.
  // The frame derives from the FRESH leaf union, not the grow-only rootBBox:
  // a rootBBox frame would stay coarse forever after a distant excursion returns.
  const bool allHidden = v_bbox3_is_empty(leaf_union);
  // All-hidden with the latch, or with no CURRENT clone: stay clone-less - no frame derives
  // from an empty union, and tree bytes beside a stale frame would mis-encode the next leaf.
  if (allHidden && ((tlasDeclined && tlasDeclinedGen == res->data->tlasGeneration) || tlas.data.empty() ||
                     tlasCloneGen != res->data->tlasGeneration))
  {
    tlas.data.clear();
    return;
  }
  if (!allHidden)
  {
    tlasFrameDim = v_extract_x(v_hmax3(v_sub(leaf_union.bmax, leaf_union.bmin)));
    // Shared helper, in lockstep with the load-time decision - except the tree-backed arm
    // budgets with per-factor maxima and the refresh-wide conditioning max, so heterogeneous
    // content can decline an envelope the per-node build model passes; the decline is the
    // conservative direction, and the linear walk stays correct.
    const float opReserve = 1.25f * max_node_op;
    const float fpErrS = collres_tlas_fp_budget(max_node_op, tlasFrameDim, ampl_max);
    tlas.fpErr = v_splats(fpErrS); // axis-independent: rotation mixes source axes into every output
    const vec3f pMin = v_sub(leaf_union.bmin, tlas.fpErr), pMax = v_add(leaf_union.bmax, tlas.fpErr);
    const vec3f pSize = v_sub(pMax, pMin);
    // Outside the provable envelope the prefilter must not guess: LATCH the decline, or every
    // later pose write would recopy, walk all nodes and decline again. Judged on the dominant
    // dimension: thin cross-axes span mostly pad harmlessly.
    const float maxRatio = fpErrS / max(v_extract_x(v_hmax3(pSize)), 1e-30f);
    if (!(fpErrS < 1e30f) || !v_test_xyz_finite(v_add(pMin, pMax)) || maxRatio > COLLRES_TLAS_BUDGET_MAX_RATIO)
    {
      declineTlas();
      return;
    }
    tlasDeclined = false; // the envelope passed: a previously declined pose has returned
    // Geometry-only operand (with reserve) and conditioning maxima: the single-leaf guard
    // compares against these, so a later pose cannot consume the query tranche unnoticed.
    tlasFrameNodeOp = opReserve;
    tlasFrameAmpl = ampl_max;
    // An escape refit inflates the frame GEOMETRICALLY: a monotonic one-by-one drift then
    // amortizes to O(log) full refits. Coarser bins, still conservative.
    const vec3f headGrow = v_mul(pSize, v_splats(frame_headroom));
    const vec3f hMin = v_sub(pMin, headGrow), hMax = v_add(pMax, headGrow);
    const vec3f hSize = v_sub(hMax, hMin);
    const vec3f safeSize = v_max(hSize, v_splats(0.0001f));
    const vec3f effectiveMin = v_sub(hMin, v_mul(v_sub(safeSize, hSize), V_C_HALF));
    // From the ENCODED frame (expanded, then floored): the incremental path lets a leaf drift
    // anywhere inside it, so the deviation gate must assume the worst in-frame magnitude.
    tlasFrameMag = v_extract_x(v_hmax3(v_max(v_abs(effectiveMin), v_abs(v_add(effectiveMin, safeSize)))));
    tlas.scale = v_div(v_splats(COLLRES_TLAS_QUANT_SPAN), safeSize);
    tlas.origin = effectiveMin;
  }
  if (tlas.data.empty() || tlasCloneGen != res->data->tlasGeneration)
  {
    // A clone keyed to another generation is replaced, never refit at the new offsets; the
    // generation is stamped once at the load, so this is the first clone. The clone takes the
    // TREE BYTES alone: the topology metadata behind them stays resource-side.
    tlas.data.resize_noinit(res->data->tlasTreeBytes);
    memcpy(tlas.data.data(), res->data->tlasData().data(), res->data->tlasTreeBytes);
    tlasCloneGen = res->data->tlasGeneration;
  }

  uint8_t *treeData = tlas.data.data();
  const dag::ConstSpan<uint32_t> nodeOfs = res->tlasNodeOfs();
  const dag::ConstSpan<uint32_t> parentPacked = res->tlasNodeParentPacked();
  // collres_tlas_quant_lane four lanes at a time; an all-leaf node takes each row in one store
  const vec4f qLo = v_splats(COLLRES_TLAS_QUANT_LO), qHi = v_splats(COLLRES_TLAS_QUANT_CLAMP_HI);
  const vec4f origin[3] = {v_splat_x(tlas.origin), v_splat_y(tlas.origin), v_splat_z(tlas.origin)};
  const vec4f scale[3] = {v_splat_x(tlas.scale), v_splat_y(tlas.scale), v_splat_z(tlas.scale)};
  for (uint32_t k = 0, ke = res->data->tlasInnerCount; k < ke; ++k)
  {
    const uint32_t leafBits = row_masks[k] & 15u, emptyBits = leafBits & ~(uint32_t)(row_masks[k] >> 4);
    if (!leafBits)
      continue;
    const float *row = leaf_rows + k * 24u;
    alignas(16) uint16_t q[6][8];
    for (int a = 0; a < 3; ++a)
    {
      const vec4f qMin = v_sub(v_madd(v_sub(v_sub(v_ld(row + a * 4), tlas.fpErr), origin[a]), scale[a], qLo), V_C_ONE);
      const vec4f qMax = v_add(v_madd(v_sub(v_add(v_ld(row + (3 + a) * 4), tlas.fpErr), origin[a]), scale[a], qLo), V_C_ONE);
      v_sti(q[a], v_packus(v_cvt_floori(v_max(v_min(qMin, qHi), v_zero()))));
      v_sti(q[3 + a], v_packus(v_cvt_ceili(v_max(v_min(qMax, qHi), v_zero()))));
    }
    for (int l = 0; l < 4; ++l)
      if (emptyBits & (1u << l))
        for (int r = 0; r < 6; ++r)
          q[r][l] = r < 3 ? 65535 : 0; // the empty lane's encode
    const int n = CollisionResource::tlasChildCountOf(parentPacked[k]);
    uint16_t *lanes = (uint16_t *)(treeData + nodeOfs[k]);
    if (n == 4 && !allHidden) // the sweep below rewrites every inner lane, so whole rows may carry junk there
      for (int r = 0; r < 6; ++r)
        v_stui_half(lanes + r * 4, v_ldui_half(q[r]));
    else
      for (int l = 0; l < n; ++l)
        if (leafBits & (1u << l))
          for (int r = 0; r < 6; ++r)
            lanes[r * n + l] = q[r][l];
  }
  if (allHidden)
    return; // inner boxes stay where they were: conservative, never a missed hit

  // The emitter writes a parent before its children, so one reverse sweep publishes every node's
  // union into its parent's lane with each child already final.
  for (uint32_t i = res->data->tlasInnerCount - 1; i > 0; --i)
  {
    uint16_t mn[3], mx[3];
    tlas_node_union((const uint16_t *)(treeData + nodeOfs[i]), CollisionResource::tlasChildCountOf(parentPacked[i]), mn, mx);
    const uint32_t pp = CollisionResource::tlasParentOf(parentPacked[i]);
    soa4::storeLaneBoxU16(treeData + nodeOfs[pp >> 2], CollisionResource::tlasChildCountOf(parentPacked[pp >> 2]), (int)(pp & 3u), mn,
      mx);
  }
}

void CollisionResourceInstance::refitTlasLeaf(int node_index, const mat44f *geom_tm, const bbox3f *posed_box) const
{
  if (!res || !res->hasAllNodesTLAS())
    return;
  // Latched decline: this blueprint generation keeps the linear walk (see declineTlas).
  if (tlasDeclined && tlasDeclinedGen == res->data->tlasGeneration)
    return;
  const uint32_t loc = res->getAllNodes()[node_index].tlasLeafLoc;
  if (loc == ~0u)
    return; // no leaf (POINTS or geometry-less): nothing to refit, and nothing worth cloning for
  if (tlas.data.empty() || tlasCloneGen != res->data->tlasGeneration)
  {
    // First write with no usable clone: the full refit materializes and derives the frame.
    tlasDeclined = false;
    refitAllTlasLeaves();
    return;
  }

  const CollisionNode &node = res->getAllNodes()[node_index];
  bbox3f nodeBox, rawBox;
  v_bbox3_init_empty(rawBox); // MSVC C4701: hasBox guards the unwritten arm, invisibly to flow analysis
  bool hasBox = posed_box != nullptr;
  if (hasBox)
  {
    // a leaf must enclose both frames, and the caller's grow box carries no eps-IDENT union
    rawBox = res->getNodeGeometryBBox(node);
    nodeBox = *posed_box;
    add_eps_ident_raw_box(nodeBox, node, poseMeta[node_index], rawBox);
  }
  else
    hasBox = ownedPosedNodeBox(node_index, nodeBox, &rawBox);
  if (hasBox)
  {
    // A pose write can raise the operand magnitude or conditioning past the frame budget, and a
    // box escaping the frame would clamp to the wall and false-miss rays outside it: either way
    // re-derive by full refit, with growth headroom (a monotonic drift would trip again on the
    // very next write). The escape test uses the SAME padded values the encoder writes.
    const float rawMag = collres_box_max_abs(rawBox), boxMag = collres_box_max_abs(nodeBox);
    const float scale = (uint32_t)node_index < (uint32_t)poseMeta.size() ? poseMeta[node_index].maxTmScale : 1.f;
    mat44f gt;
    if (geom_tm)
      gt = *geom_tm;
    else
    {
      mat44f pt;
      v_mat44_make_from_43cu_unsafe(pt, nodeTm[node_index].array);
      gt = res->geometryTmFromPosed(node_index, pt, poseMeta[node_index]);
    }
    const float ampl = collres_tlas_tm_conditioning(gt);
    bool needFull = !(collres_tlas_node_op(scale, rawMag, boxMag, ampl) <= tlasFrameNodeOp) || !(ampl <= tlasFrameAmpl);
    if (!needFull)
    {
      vec4f qmin, qmax;
      tlasQuantPadded(nodeBox, qmin, qmax);
      needFull = !v_check_xyz_all_true(v_and(v_cmp_ge(qmin, v_zero()), v_cmp_ge(v_splats(COLLRES_TLAS_QUANT_CLAMP_HI), qmax)));
    }
    if (needFull)
    {
      refitAllTlasLeaves(/*frame_headroom*/ 0.25f);
      return;
    }
  }
  encodeTlasLeafBox(node_index, loc, hasBox ? &nodeBox : nullptr);

  uint8_t *treeData = tlas.data.data();
  const dag::ConstSpan<uint32_t> nodeOfs = res->tlasNodeOfs();
  const dag::ConstSpan<uint32_t> parentPacked = res->tlasNodeParentPacked();
  uint32_t idx = loc >> 2;
  int guard = 0;
  while (CollisionResource::tlasParentOf(parentPacked[idx]) != CollisionResource::TLAS_PARENT_ROOT &&
         guard++ <= (int)res->data->tlasInnerCount)
  {
    uint16_t mn[3], mx[3];
    tlas_node_union((const uint16_t *)(treeData + nodeOfs[idx]), CollisionResource::tlasChildCountOf(parentPacked[idx]), mn, mx);
    const uint32_t pp = CollisionResource::tlasParentOf(parentPacked[idx]);
    const uint32_t pi = pp >> 2;
    const int pn = CollisionResource::tlasChildCountOf(parentPacked[pi]), lane = (int)(pp & 3u);
    uint16_t *pa = (uint16_t *)(treeData + nodeOfs[pi]);
    bool changed = false;
    for (int k = 0; k < 3; ++k)
    {
      if (pa[k * pn + lane] != mn[k])
      {
        pa[k * pn + lane] = mn[k];
        changed = true;
      }
      if (pa[(3 + k) * pn + lane] != mx[k])
      {
        pa[(3 + k) * pn + lane] = mx[k];
        changed = true;
      }
    }
    if (!changed)
      break; // this ancestor already enclosed the new box, so every ancestor above it does too
    idx = pi;
  }
}

void CollisionResourceInstance::recomputePoseMeta(int node_index)
{
  {
    // Before any early return: the bit describes nodeTm, which the caller has just written.
    mat44f stored;
    v_mat44_make_from_43cu_unsafe(stored, nodeTm[node_index].array);
    poseMeta[node_index].setPoseIdentity(v_mat44_is_ident43(stored));
  }
  // A geometry-baked (singular-authored) primitive has no recoverable node-local frame,
  // so no live pose is realizable: keep the node hidden rather than classifying the
  // identity geometry tm, which would resurrect it at its baked bind pose. A RETAINED bake
  // (valid general sphere) IS poseable: it falls through and classifies its compatibility
  // transform like any other node.
  if (DAGOR_UNLIKELY(poseMeta[node_index].isGeometryBaked() && !poseMeta[node_index].isRetainedBake()))
  {
    poseMeta[node_index].setTraceable(false);
    poseMeta[node_index].setComposable(false); // stored baked values are the right report
    LOGWARN_ONCE("collres instance %p of res %p <%s>: live pose on geometry-baked node %d <%s> is not realizable; node stays hidden",
      this, res, res_name(res).str(), node_index, res->getNodeName(node_index));
    return;
  }
  // A non-finite component anywhere in the affine pose (translation included) must not
  // classify as traceable nor reach posed bounds. Per lane, not a column sum: finite
  // components can overflow the intermediate sum near the scale bound.
  mat44f posedFull;
  v_mat44_make_from_43cu_unsafe(posedFull, nodeTm[node_index].array);
  const bool poseFinite = v_test_mat43_finite(posedFull);
  const mat44f geometryTm = getNodeGeometryTm(node_index);
  uint8_t flags = CollisionResource::classifyNodeTmFlags(geometryTm, poseMeta[node_index].maxTmScale);
  const bool exactIdentity = v_mat44_is_ident43(geometryTm);
  // Epsilon classes can still under-bound shear; bit-exact rigid bases keep their exact stamp.
  if (!exactIdentity && !v_mat44_is_signed_perm33(geometryTm))
    poseMeta[node_index].maxTmScale = max(poseMeta[node_index].maxTmScale, mat33_spectral_norm(geometryTm) * 1.0002f);
  if ((flags & CollisionNode::IDENT) && !exactIdentity)
    flags = (flags & ~CollisionNode::IDENT) | CollisionNode::TRANSLATE;
  poseMeta[node_index].flags = flags;
  // The relative determinant gate accepts tiny valid poses but rejects collapsed or mirrored
  // ones; normalized so a large finite pose cannot overflow det and scale^3 into Inf > Inf.
  float ndet;
  const bool detOk = CollisionResource::relativeDetAboveFloor(geometryTm, ndet);
  // Inverse-based dispatch needs the whole inverse finite: near-bound translations and
  // huge-scale cofactors overflow it while the forward pose stays representable.
  bool composable = poseFinite && detOk;
  if (composable)
  {
    mat44f inv;
    v_mat44_inverse43(inv, geometryTm);
    composable = v_test_mat43_finite(inv);
  }
  poseMeta[node_index].setComposable(composable);
  const bool traceOk = composable && ndet > 0.f;
  poseMeta[node_index].setTraceable(traceOk);
  if (DAGOR_UNLIKELY(detOk && ndet < 0.f))
    LOGWARN_ONCE("collres instance %p of res %p <%s>: mirrored pose (ndet=%g) for node %d <%s>; node is not traceable", this, res,
      res_name(res).str(), ndet, node_index, res->getNodeName(node_index));
  else if (DAGOR_UNLIKELY(!traceOk))
    LOGWARN_ONCE("collres instance %p of res %p <%s>: singular, non-finite or inverse-overflowing pose (ndet=%g) for node %d <%s>; "
                 "node is not traceable",
      this, res, res_name(res).str(), ndet, node_index, res->getNodeName(node_index));
}

bool CollisionResourceInstance::validateForUpdate() const
{
  const bool bound = res && !isDefault();
  G_ASSERTF(bound, "updating a default or resource-less CollisionResourceInstance");
  if (DAGOR_UNLIKELY(!bound))
  {
    LOGERR_ONCE("updating a default or resource-less CollisionResourceInstance is ignored");
    return false;
  }
  return check_instance_owned_and_fresh(res, *this, "update"); // a failed update is ignored
}

bool CollisionResourceInstance::updateNodeTmImpl(int node_index, mat44f_cref tm)
{
  G_ASSERT_RETURN((uint32_t)node_index < nodeTm.size(), false);
  // rootBBox grows FIRST (conservative raw compose; the exact grow re-runs after the store): a
  // racing trace whose reject load lands after the grow sees bounds covering the new pose. No
  // cross-thread ordering is implied -- a reject load may still precede the grow, and poseMeta
  // recomputes after the store -- so this only narrows the unsynchronized window: pose writes
  // vs. traces on one resource stay the caller's synchronization duty (legacy contract).
  // Structural mutations (setNodeEnabled, resets: rootBBox can shrink) keep exclusive access.
  // A non-finite pose must not reach the union: it would poison the grow-only box for good
  // (recomputePoseMeta below only hides the node).
  // Per lane, not a sum: two finite same-sign terms can overflow ONLY in the addition, and a
  // false negative here would keep rootBBox at the old pose and MISS rays at the new one.
  const bool poseFinite = v_test_mat43_finite(tm);
  const CollisionNode &node = res->getAllNodes()[node_index];
  // Zero-vert markers (r < 0) and permanently hidden geometry bakes never join: they cannot
  // trace at any pose, and the grow-only union keeps their phantom for the resource's lifetime.
  if (poseFinite && isNodeEnabled(node_index) && node.type != COLLISION_NODE_TYPE_POINTS && node.radiusAroundBoxCenter >= 0.f &&
      !(poseMeta[node_index].isGeometryBaked() && !poseMeta[node_index].isRetainedBake()) &&
      !((node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX) && !node.hasGeometry()))
  {
    bbox3f newBox;
    // The incoming pose's COMPATIBILITY transform, not tm itself: an eps-IDENT or retained-bake
    // prim stores geometry in its authored frame, and a raw-tm grow would leave the authored
    // offset uncovered for the window before the store.
    mat44f growTm = tm;
    if (DAGOR_UNLIKELY(CollisionResource::usesAuthoredFrame(node, poseMeta[node_index])))
    {
      mat44f invAuthored;
      if (DAGOR_LIKELY((uint32_t)node_index < res->data->authoredNodeItm().size()))
        v_mat44_make_from_43cu_unsafe(invAuthored, res->data->authoredNodeItm()[node_index].array);
      else
      {
        mat44f authored;
        v_mat44_make_from_43cu_unsafe(authored, res->data->authoredNodeTm()[node_index].array);
        v_mat44_inverse43(invAuthored, authored);
      }
      v_mat44_mul43(growTm, tm, invAuthored);
    }
    // A pose that will hide (singular, mirrored, inverse-overflowing) must not widen the
    // grow-only union: it cannot shrink back before a structural recompute.
    float growNdet;
    bool growTraceable = CollisionResource::relativeDetAboveFloor(growTm, growNdet) && growNdet > 0.f;
    if (growTraceable)
    {
      mat44f growInv;
      v_mat44_inverse43(growInv, growTm);
      growTraceable = v_test_mat43_finite(growInv);
    }
    if (growTraceable)
    {
      v_bbox3_init(newBox, growTm, res->getNodeGeometryBBox(node));
      // A finite basis can still overflow the composed extent: saturate, never drop (the node
      // stays traceable, so a short root box would cull rays to its reachable part).
      join_saturated_finite(rootBBox, newBox);
    }
  }
  posedSinceBind = true;
  const bool wasTraceable = poseMeta[node_index].isTraceable();
  // A byte-identical re-push cannot move the leaf box, so it skips the refit (per-frame
  // writers re-push static nodes); any representational difference conservatively re-refits.
  TMatrix asStored;
  v_mat_43cu_from_mat44(asStored.array, tm);
  const bool poseUnchanged = memcmp(&asStored, &nodeTm[node_index], sizeof(TMatrix)) == 0; //-V1014
  nodeTm[node_index] = asStored;
  recomputePoseMeta(node_index);
  // A leafless node this write recovers (see tlas_leafless_recovery) declines the clone; one that
  // un-recovers re-arms THROUGH a full refit (a bare latch clear would strand it clone-less).
  if (DAGOR_UNLIKELY(!wasTraceable && res->getAllNodes()[node_index].tlasLeafLoc == ~0u && res->hasAllNodesTLAS() &&
                     tlas_leafless_recovery(res->getAllNodes()[node_index], poseMeta[node_index],
                       res->getAllNodes()[node_index].radiusAroundBoxCenter)))
    declineTlas();
  else if (DAGOR_UNLIKELY(
             wasTraceable && !poseMeta[node_index].isTraceable() && tlasDeclined && res->getAllNodes()[node_index].tlasLeafLoc == ~0u))
    refitAllTlasLeaves(); // re-evaluates: re-arms on a passing envelope, re-declines otherwise
  // Nodes that never trace (hidden, unrealizable, POINTS, zero-vert markers, geometry-less
  // mesh/convex) must not widen (or poison) the posed bounds either: same predicate set as
  // recomputeRootBBox, so the grow-only arm never over-widens relative to the next full recompute.
  const bool boundsRelevant =
    isNodeEnabled(node_index) && poseMeta[node_index].isTraceable() && node.type != COLLISION_NODE_TYPE_POINTS &&
    node.radiusAroundBoxCenter >= 0.f &&
    !((node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX) && !node.hasGeometry());
  mat44f geomTm;
  bbox3f nodeBox;
  if (boundsRelevant)
  {
    // composed_or_raw_box: a return to exact identity must keep the stored box verbatim.
    geomTm = getNodeGeometryTm(node_index);
    nodeBox = composed_or_raw_box(geomTm, res->getNodeGeometryBBox(node));
    // Same saturate-never-drop guard as the pre-store grow.
    join_saturated_finite(rootBBox, nodeBox); // grow-only: exact bounds return on the next full update
  }
  // Runs AFTER the pose store: pose writes concurrent with traces of the same holder require
  // external serialization on the TLAS path (the public contract). Reuses the grow's compose.
  if (!poseUnchanged || !tlasCloneCurrent(res->data->tlasGeneration))
    refitTlasLeaf(node_index, boundsRelevant ? &geomTm : nullptr, boundsRelevant ? &nodeBox : nullptr);
  if (this == &res->defaultInstance && !poseUnchanged)
    res->defaultPoseGen++; // tree-backed instances re-derive their unbound-node state
  return true;
}

bool CollisionResourceInstance::updateNodeTm(int node_index, mat44f_cref tm)
{
  if (!validateForUpdate())
    return false;
  // A tree-backed pose is driven by its tree; manual node poses need the owned-matrix form.
  G_ASSERTF_RETURN(!tree, false, "collres instance %p of res %p <%s>: updateNodeTm needs the owned-matrix form", this, res,
    res_name(res).str());
  return updateNodeTmImpl(node_index, tm);
}

// Valid on tree-backed instances too: structural hides are per-instance metadata,
// independent of the matrix source (unlike updateNodeTm's owned-form-only contract).
// The caller-side serialization contract covers the plain meta write; the generation
// reset below makes the next trace re-derive everything the hide affects.
bool CollisionResourceInstance::setNodeEnabled(int node_index, bool enabled)
{
  if (!validateForUpdate())
    return false;
  // Out-of-range writes assert and drop. The bound is the meta array, which BOTH forms
  // carry (updateNodeTmImpl bounds on the owned-form nodeTm instead).
  G_ASSERT_RETURN((uint32_t)node_index < poseMeta.size(), false);
  PoseMeta &pm = poseMeta[(uint32_t)node_index];
  if (pm.isDisabled() == !enabled)
    return true; // already in the requested state: accepted, not dropped
  pm.setDisabled(!enabled);
  if (tree)
  {
    // Pose writes must be serialized against traces: catch a refresh racing this hide.
    G_ASSERTF(interlocked_acquire_load(refreshLock) == 0,
      "collres instance %p of res %p <%s>: setNodeEnabled during a concurrent trace refresh; serialize pose writes against traces",
      this, res, res_name(res).str());
    interlocked_release_store(poseGeneration, 0u); // never-refreshed: the next trace re-derives from the live tree
    // pose STATE changed without a tree generation bump: the lazy TLAS refresh must re-run
    interlocked_release_store(tlasPoseGeneration, 0u);
  }
  else
  {
    recomputeRootBBox();
    // A hide moves the frame and every leaf; a latched decline refits too, since hiding the
    // declining node removes the reason and only the refit can re-arm candidate mode.
    if (!tlas.data.empty() || tlasDeclined)
      refitAllTlasLeaves();
  }
  return true;
}

void CollisionResourceInstance::recomputeRootBBox()
{
  bbox3f box;
  v_bbox3_init_empty(box);
  dag::ConstSpan<CollisionNode> nodes = res->getAllNodes();
  for (uint32_t i = 0, e = min<uint32_t>(nodes.size(), nodeTm.size()); i < e; ++i)
  {
    const CollisionNode &node = nodes[i];
    if (!isNodeEnabled((int)i) || !poseMeta[i].isTraceable())
      continue; // hidden or unrealizable poses do not trace and must not bound
    if (node.type == COLLISION_NODE_TYPE_POINTS)
      continue; // never a trace target
    if (node.radiusAroundBoxCenter < 0.f)
      continue; // zero-vert marker: corner-mapping the inverted box would union a phantom
    if ((node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX) && !node.hasGeometry())
      continue;
    bbox3f nodeBox = res->getNodeGeometryBBox(node);
    bbox3f composedBox = composed_or_raw_box(getNodeGeometryTm((int)i), nodeBox);
    // Same saturate-never-drop guard as the incremental grows: NaN stays out, an overflowed
    // extent clamps to the float range so the traceable node still bounds.
    join_saturated_finite(box, composedBox);
    // A live pose diverging within the IDENT eps widens this union conservatively.
    add_eps_ident_raw_box(box, node, poseMeta[i], nodeBox);
  }
  rootBBox = box;
}

void CollisionResourceInstance::seedPose()
{
  const CollisionResourceInstance &def = res->defaultInstance;
  G_ASSERT(&def != this);
  // Generation read BEFORE the copy: a racing write leaves seen behind the current
  // generation, so the first mirror re-copies instead of freezing pre-write flags.
  const uint32_t liveGenAtSeed = interlocked_acquire_load(res->liveFlagsGen);
  poseMeta = def.poseMeta;
  interlocked_release_store(liveFlagsGenSeen, liveGenAtSeed);
  // Pose state seeds, but a fresh instance starts with every node enabled.
  bool clearedHide = false;
  for (PoseMeta &pm : poseMeta)
  {
    clearedHide |= pm.isDisabled();
    pm.setDisabled(false);
  }
  if (tree)
  {
    nodeTm.clear();
    // A driven pose deviates from bind and never licenses bind shortcuts.
    posedSinceBind = true;
    // The bind meta of a driven node comes from its rel tm under a rigid-band wtm;
    // the refresh classifies a wtm outside the band live.
    dag::ConstSpan<CollisionNode> nodes = res->getAllNodes();
    for (uint32_t i = 0, e = min<uint32_t>(nodes.size(), (uint32_t)poseMeta.size()); i < e; ++i)
    {
      if (nodes[i].geomNodeId.index() >= treeNodeCountAtBind)
        continue;                         // unbound: keeps its default-pose meta and matrices
      poseMeta[i].setPoseIdentity(false); // driven: no stored matrix to shortcut
      PoseMeta &pm = poseMeta[i];
      // A geometry-baked (singular-authored) prim has no recoverable node-local frame, so no
      // driven pose is realizable: keep the load-time hide instead of reclassifying it from
      // the rel tm (the owned form and the removed eager path hide it the same way). A
      // RETAINED bake is poseable and classifies below.
      if (DAGOR_UNLIKELY(pm.isGeometryBaked() && !pm.isRetainedBake()))
      {
        pm.setBindTraceable(false); // the refresh re-derives TRACEABLE from this: hidden every generation
        continue;
      }
      const mat44f relTm = drivenRelTm(i);
      setDrivenPoseClass(i, relTm, false);
      // Normalized det floor: a raw det/scale^3 compare overflows to Inf > Inf for large
      // finite rel tms and falsely hides well-conditioned nodes (the owned classify shares it).
      float ndet;
      const bool floorOk = CollisionResource::relativeDetAboveFloor(relTm, ndet);
      // Handedness is not gated (the legacy tree overloads trace any), so the seed gates on the
      // det floor alone.
      pm.setTraceable(floorOk);
      pm.setBindTraceable(floorOk); // the refresh re-derives TRACEABLE from this
      pm.setComposable(floorOk);    // tree seeds skip the inverse probe; the det floor stands in
    }
    // rootBBox stays empty until the first trace refreshes it (poseGeneration == 0).
    v_bbox3_init_empty(rootBBox);
    return;
  }
  nodeTm = def.nodeTm;
  posedSinceBind = def.posedSinceBind; // a posed default seeds a posed copy
  // Poses seed verbatim, so the default's rootBBox stays valid unless a hide was cleared.
  if (DAGOR_UNLIKELY(clearedHide))
    recomputeRootBBox();
  else
    rootBBox = def.rootBBox;
  // A posed seed can stay read-only forever: materialize the clone now or the prefilter
  // stays silently absent.
  if (posedSinceBind && res->hasAllNodesTLAS())
    refitAllTlasLeaves();
}
