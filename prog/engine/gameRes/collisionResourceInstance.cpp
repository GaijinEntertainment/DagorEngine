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
#include "collisionGameResInternal.h"

const CollisionResourceInstance::PoseMeta CollisionResourceInstance::PoseMeta::absent_slot = {
  1.f, uint8_t(CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED), uint8_t(0)};

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
  if (n->boundingSphere.r < 0 || n->type == COLLISION_NODE_TYPE_POINTS ||
      ((n->type == COLLISION_NODE_TYPE_MESH || n->type == COLLISION_NODE_TYPE_CONVEX) && !n->hasGeometry()))
    return BBox3();
  if (n->type == COLLISION_NODE_TYPE_SPHERE)
  {
    TMatrix tm;
    v_mat_43cu_from_mat44(tm.array, getNodeGeometryTm(node_index));
    // Analytic AABB matches the dispatch's reach; corner-mapping a sphere inflates under rotation.
    return composed_sphere_box(tm, n->boundingSphere.c, n->boundingSphere.r);
  }
  bbox3f box;
  v_bbox3_init(box, getNodeGeometryTm(node_index), res->getNodeGeometryBBox(*n));
  BBox3 out;
  v_stu_bbox3(out, box);
  return out;
}

DAGOR_NOINLINE const TMatrix &CollisionResourceInstance::missingOwnedPose(int node_index) const
{
  G_ASSERTF(false, "collres instance %p of res %p: owned-pose accessor on node %d %s", this, res, node_index,
    tree ? "of a tree-backed instance" : "is out of range");
  LOGERR_ONCE("collres instance %p of res %p: owned-pose accessor on node %d %s; bind identity is used", this, res, node_index,
    tree ? "of a tree-backed instance" : "is out of range");
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

void CollisionResourceInstance::refreshIfStale(mat44f_cref entity_tm) const
{
  // The tree generation is never 0, so 0 stays "never refreshed". The default-pose stamp
  // catches unbound-node writes, which move matrices without touching the tree generation;
  // its acquire pairs with the refresh's release so a default-pose-only re-derive (the tree
  // stamp re-publishes an unchanged value) still orders the derived-state reads.
  // Derived state anchors to the refreshing trace's entity basis: callers keep that basis
  // stable within a pose generation (re-orienting an entity re-poses its tree).
  if (DAGOR_LIKELY(interlocked_acquire_load(poseGeneration) == tree->getPoseGeneration() &&
                   interlocked_acquire_load(defaultPoseGenAtRefresh) == res->defaultPoseGen))
    return;
  refreshFromTree(entity_tm);
}

DAGOR_NOINLINE void CollisionResourceInstance::refreshFromTree(mat44f_cref entity_tm) const
{
  // Once-per-generation rebuild; concurrent traces of the same instance wait here.
  while (DAGOR_UNLIKELY(interlocked_compare_exchange(refreshLock, 1, 0) != 0))
    cpu_yield();
  const uint32_t cur = tree->getPoseGeneration();
  const uint32_t curDef = res->defaultPoseGen;
  if (interlocked_acquire_load(poseGeneration) != cur || defaultPoseGenAtRefresh != curDef)
  {
    if (DAGOR_UNLIKELY((uint32_t)tree->nodeCount() != treeNodeCountAtBind))
    {
      // Layout drifted under a live binding: disable the posed reject rather than trace stale bounds.
      v_bbox3_init(rootBBox, v_make_vec4f(-FLT_MAX / 4, -FLT_MAX / 4, -FLT_MAX / 4, 0));
      v_bbox3_add_pt(rootBBox, v_make_vec4f(FLT_MAX / 4, FLT_MAX / 4, FLT_MAX / 4, 0));
      hasBsphereCenterLocal = false;
      interlocked_release_store(defaultPoseGenAtRefresh, curDef);
      interlocked_release_store(poseGeneration, cur);
      const bool firstDrift = !layoutDriftAsserted; // latched: one assert per binding, not per generation
      layoutDriftAsserted = true;
      interlocked_release_store(refreshLock, 0);
      // Diagnostics AFTER the release: an assert stop or log emit under the spinlock would
      // stall every concurrent trace of this instance.
      if (DAGOR_UNLIKELY(firstDrift))
      {
        G_ASSERTF(false, "collres instance %p of res %p: bound tree layout changed (%d nodes vs %d); rebind the instance", this, res,
          (int)tree->nodeCount(), (int)treeNodeCountAtBind);
        LOGERR_ONCE("collres instance %p of res %p: bound tree layout changed (%d nodes vs %d)", this, res, (int)tree->nodeCount(),
          (int)treeNodeCountAtBind);
      }
      return;
    }
    // Compose translation entity-relative to preserve large-world precision.
    const mat44f entityRot = {entity_tm.col0, entity_tm.col1, entity_tm.col2, v_zero()};
    mat44f invEntity;
    v_mat44_inverse43(invEntity, entityRot);
    const vec3f relOfs = v_sub(tree->getWtmOfs(), entity_tm.col3);
    hasBsphereCenterLocal = res->bsphereCenterNode && res->bsphereCenterNode.index() < tree->nodeCount();
    if (hasBsphereCenterLocal)
    {
      mat44f centerTm = tree->getNodeWtmRel(res->bsphereCenterNode);
      centerTm.col3 = v_add(centerTm.col3, relOfs);
      bsphereCenterLocal = v_mat44_mul_vec3p(invEntity, centerTm.col3);
    }
    // Diagnostics collected under the lock, emitted after the release (an assert stop or a
    // log emit under the spinlock stalls every concurrent trace of this instance).
    int canaryNode = -1, hiddenNode = -1;
    float canaryErr = 0.f;
    G_UNUSED(canaryNode);
    G_UNUSED(canaryErr);
    // Resource-local posed matrix of one node; unbound nodes fall back to the default pose.
    auto posedNodeTm = [&](uint32_t i) {
      const CollisionNode &node = res->getAllNodes()[i];
      auto gnId = node.geomNodeId.index() < tree->nodeCount() ? node.geomNodeId : dag::Index16();
      if (!gnId)
        return res->defaultInstance.getNodeTm((int)i);
      mat44f m = tree->getNodeWtmRel(gnId);
#if DAGOR_DBGLEVEL > 0
      // No-scale contract canary: a scaled or sheared driven wtm invalidates bind metadata;
      // checked on the raw wtm basis before the entity composition touches the columns.
      const float e00 = fabsf(v_extract_x(v_dot3_x(m.col0, m.col0)) - 1.f);
      const float e11 = fabsf(v_extract_x(v_dot3_x(m.col1, m.col1)) - 1.f);
      const float e22 = fabsf(v_extract_x(v_dot3_x(m.col2, m.col2)) - 1.f);
      const float e01 = fabsf(v_extract_x(v_dot3_x(m.col0, m.col1)));
      const float e02 = fabsf(v_extract_x(v_dot3_x(m.col0, m.col2)));
      const float e12 = fabsf(v_extract_x(v_dot3_x(m.col1, m.col2)));
      // 2e-3 admits chained-bone rounding (~1e-5) with 100x headroom. An in-band scale
      // under-bounds the bind-derived sphere culls by at most the band; past it the
      // under-cover grows with the wtm scale factor, and release enforces nothing.
      const float maxErr = max(max(max(e00, e11), max(e22, e01)), max(e02, e12));
      if (DAGOR_UNLIKELY(maxErr > 2e-3f) && canaryNode < 0)
      {
        canaryNode = (int)i; // emitted after the refresh releases the spinlock
        canaryErr = maxErr;
      }
#endif
      m.col3 = v_add(m.col3, relOfs);
      v_mat44_mul43(m, invEntity, m);
      // Missing rel-tm slots on appended nodes mean identity.
      if ((res->collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID) && i < (uint32_t)res->relGeomNodeTms.size())
      {
        mat44f relGeomNodeTm;
        v_mat44_make_from_43cu_unsafe(relGeomNodeTm, res->relGeomNodeTms[i].array);
        v_mat44_mul43(m, m, relGeomNodeTm);
      }
      return m;
    };
    // Bind meta stays valid under the no-scale contract (no spectral norms here); the refresh
    // re-derives only what live state can change: the driven det gate and the unbound meta.
    // The entity's own handedness must cancel out of the driven gate: comparing SIGNS (not a
    // product, whose det(invEntity)^2 factor underflows at extreme entity scales) leaves only
    // the wtm * relTm sign deciding; det != 0 keeps a singular driven wtm hidden.
    const float invEntityDet = v_extract_x(v_dot3_x(invEntity.col0, v_cross3(invEntity.col1, invEntity.col2)));
    bbox3f box;
    v_bbox3_init_empty(box);
    dag::ConstSpan<CollisionNode> nodes = res->getAllNodes();
    for (uint32_t i = 0, e = min<uint32_t>(nodes.size(), (uint32_t)poseMeta.size()); i < e; ++i)
    {
      const CollisionNode &node = nodes[i];
      PoseMeta &pm = poseMeta[i];
      const mat44f posed = posedNodeTm(i);
      if (node.geomNodeId.index() < tree->nodeCount())
      {
        // A mirrored or degenerate driven wtm breaks the no-scale contract: hide the node
        // like the owned form hides a mirrored pose; it returns when the wtm recovers.
        // All four columns, tested SEPARATELY: a NaN translation passes a basis-only det
        // gate, and a max-abs chain eats a basis NaN (v_max lets the other operand win).
        // Normalized det floor, not a raw det != 0: a finite near-singular wtm would pass the
        // raw compare and the trace would invert an ill-conditioned matrix unguarded (the
        // owned form hides the same class through the identical floor).
        float ndet;
        const bool finitePose = v_test_xyz_finite(posed.col0) && v_test_xyz_finite(posed.col1) && v_test_xyz_finite(posed.col2) &&
                                v_test_xyz_finite(posed.col3);
        const bool ok = // a NaN basis is excluded by finitePose before the floor and sign compare
          pm.isBindTraceable() && finitePose && CollisionResource::relativeDetAboveFloor(posed, ndet) &&
          (ndet < 0.f) == (invEntityDet < 0.f);
        if (DAGOR_UNLIKELY(!ok && pm.isBindTraceable()) && hiddenNode < 0)
          hiddenNode = (int)i; // warned after the refresh releases the spinlock
        pm.setTraceable(ok);
      }
      else
      {
        // Unbound nodes read the default pose live, so its reclassification must land here
        // too; a structural hide stays this instance's own.
        PoseMeta dm = res->defaultInstance.poseMeta[i];
        dm.setDisabled(pm.isDisabled());
        pm = dm;
      }
      // hidden or unrealizable poses never trace, so they must not bound
      if (pm.isDisabled() || !pm.isTraceable())
        continue;
      if (node.type == COLLISION_NODE_TYPE_POINTS)
        continue; // never a trace target
      if (node.boundingSphere.r < 0.f)
        continue; // zero-vert marker: corner-mapping the inverted box would union a phantom
      if ((node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX) && !node.hasGeometry())
        continue;
      const mat44f gtm = res->geometryTmFromPosed((int)i, posed, pm);
      bbox3f nodeBox = res->getNodeGeometryBBox(node);
      bbox3f composedBox;
      v_bbox3_init(composedBox, gtm, nodeBox);
      // NaN never joins and Inf saturates, like every other rootBBox writer: a poisoned box
      // would false-reject every trace of the generation.
      join_saturated_finite(box, composedBox);
      // Epsilon-IDENT grid geometry needs both raw and composed bounds.
      if (node.type == COLLISION_NODE_TYPE_MESH && (pm.flags & CollisionNode::IDENT))
        join_saturated_finite(box, nodeBox);
    }
    rootBBox = box;
    // The default stamp is the release publication a default-pose-only re-derive pairs with
    // (poseGeneration may re-publish an unchanged value and give the reader no edge).
    interlocked_release_store(defaultPoseGenAtRefresh, curDef);
    interlocked_release_store(poseGeneration, cur);
    interlocked_release_store(refreshLock, 0);
#if DAGOR_DBGLEVEL > 0 // the canary collects only in dev; release must not carry an always-false compare
    if (DAGOR_UNLIKELY(canaryNode >= 0))
    {
      G_ASSERTF(false,
        "collres instance %p of res %p: scaled/sheared driven wtm for node %d (err %g) breaks the no-scale "
        "pose contract",
        this, res, canaryNode, canaryErr);
      LOGERR_ONCE("collres instance %p of res %p: scaled/sheared driven wtm for node %d (err %g)", this, res, canaryNode, canaryErr);
    }
#endif
    if (DAGOR_UNLIKELY(hiddenNode >= 0))
      LOGWARN_ONCE("collres instance %p of res %p: mirrored/degenerate driven wtm hides node %d", this, res, hiddenNode);
    return;
  }
  interlocked_release_store(refreshLock, 0);
}

void CollisionResourceInstance::recomputePoseMeta(int node_index)
{
  {
    // Before any early return: the bit describes nodeTm, which the caller has just written.
    mat44f stored;
    v_mat44_make_from_43cu_unsafe(stored, nodeTm[node_index].array);
    poseMeta[node_index].setPoseIdentity(collres_is_exact_identity_43(stored));
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
    LOGWARN_ONCE("collres instance %p of res %p: live pose on geometry-baked node %d is not realizable; node stays hidden", this, res,
      node_index);
    return;
  }
  // A non-finite component anywhere in the affine pose (translation included) must not
  // classify as traceable nor reach posed bounds. Max-abs, not a column sum: finite
  // components can overflow the intermediate sum near the scale bound. The exponent-bit
  // test survives -ffinite-math-only, which folds float-domain tricks like x - x away.
  mat44f posedFull;
  v_mat44_make_from_43cu_unsafe(posedFull, nodeTm[node_index].array);
  const bool poseFinite = v_test_xyzw_finite(
    v_max(v_max(v_abs(posedFull.col0), v_abs(posedFull.col1)), v_max(v_abs(posedFull.col2), v_abs(posedFull.col3))));
  const mat44f geometryTm = getNodeGeometryTm(node_index);
  uint8_t flags = CollisionResource::classifyNodeTmFlags(geometryTm, poseMeta[node_index].maxTmScale);
  const bool exactIdentity = collres_is_exact_identity_43(geometryTm);
  // Epsilon classes can still under-bound shear; bit-exact rigid bases keep their exact stamp.
  if (!exactIdentity && !is_exact_rigid_basis_v(geometryTm))
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
    composable = v_test_xyzw_finite(v_max(v_max(v_abs(inv.col0), v_abs(inv.col1)), v_max(v_abs(inv.col2), v_abs(inv.col3))));
  }
  poseMeta[node_index].setComposable(composable);
  const bool traceOk = composable && ndet > 0.f;
  poseMeta[node_index].setTraceable(traceOk);
  if (DAGOR_UNLIKELY(detOk && ndet < 0.f))
    LOGWARN_ONCE("collres instance %p of res %p: mirrored pose (ndet=%g) for node %d; node is not traceable", this, res, ndet,
      node_index);
  else if (DAGOR_UNLIKELY(!traceOk))
    LOGWARN_ONCE("collres instance %p of res %p: singular, non-finite or inverse-overflowing pose (ndet=%g) for node %d; node is not "
                 "traceable",
      this, res, ndet, node_index);
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
  // Max-abs, not a sum: two finite same-sign terms can overflow ONLY in the addition, and a
  // false negative here would keep rootBBox at the old pose and MISS rays at the new one.
  const bool poseFinite = v_test_xyz_finite(v_max(v_max(v_abs(tm.col0), v_abs(tm.col1)), v_max(v_abs(tm.col2), v_abs(tm.col3))));
  const CollisionNode &node = res->getAllNodes()[node_index];
  // Zero-vert markers (r < 0) and permanently hidden geometry bakes never join: they cannot
  // trace at any pose, and the grow-only union keeps their phantom for the resource's lifetime.
  if (poseFinite && isNodeEnabled(node_index) && node.type != COLLISION_NODE_TYPE_POINTS && node.boundingSphere.r >= 0.f &&
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
      if (DAGOR_LIKELY((uint32_t)node_index < res->authoredNodeItm.size()))
        v_mat44_make_from_43cu_unsafe(invAuthored, res->authoredNodeItm[node_index].array);
      else
      {
        mat44f authored;
        v_mat44_make_from_43cu_unsafe(authored, res->authoredNodeTm[node_index].array);
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
      growTraceable =
        v_test_xyzw_finite(v_max(v_max(v_abs(growInv.col0), v_abs(growInv.col1)), v_max(v_abs(growInv.col2), v_abs(growInv.col3))));
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
  v_mat_43cu_from_mat44(nodeTm[node_index].array, tm);
  recomputePoseMeta(node_index);
  // membership, not residency: either grid's walk may cover this node's triangles
  if (res->isAnyGridMember(node))
  {
    // Surface the cost the void signature cannot: one grid-member pose de-licenses every grid
    // path for this pose holder's lifetime.
    if (DAGOR_UNLIKELY(gridResidentPoseAtBind))
      LOGWARN_ONCE("collision: pose write on grid-member node %d of res %p drops the combined-grid fast path", node_index, res);
    gridResidentPoseAtBind = false; // one-way latch: the combined-grid walk no longer covers this pose
  }
  // Nodes that never trace (hidden, unrealizable, POINTS, zero-vert markers, geometry-less
  // mesh/convex) must not widen (or poison) the posed bounds either: same predicate set as
  // recomputeRootBBox, so the grow-only arm never over-widens relative to the next full recompute.
  const bool boundsRelevant =
    isNodeEnabled(node_index) && poseMeta[node_index].isTraceable() && node.type != COLLISION_NODE_TYPE_POINTS &&
    node.boundingSphere.r >= 0.f &&
    !((node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX) && !node.hasGeometry());
  if (boundsRelevant)
  {
    bbox3f nodeBox = res->getNodeGeometryBBox(node);
    v_bbox3_init(nodeBox, getNodeGeometryTm(node_index), nodeBox);
    // Same saturate-never-drop guard as the pre-store grow.
    join_saturated_finite(rootBBox, nodeBox); // grow-only: exact bounds return on the next full update
  }
  if (this == &res->defaultInstance)
    res->defaultPoseGen++; // tree-backed instances re-derive their unbound-node state
  return true;
}

bool CollisionResourceInstance::updateNodeTm(int node_index, mat44f_cref tm)
{
  if (!validateForUpdate())
    return false;
  // A tree-backed pose is driven by its tree; manual node poses need the owned-matrix form.
  G_ASSERTF_RETURN(!tree, false, "collres instance %p of res %p: updateNodeTm needs the owned-matrix form", this, res);
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
  if (!enabled && res->isAnyGridMember(res->getAllNodes()[node_index]))
    gridResidentPoseAtBind = false; // one-way latch: the grid walk would still hit the disabled node
  if (tree)
  {
    // Pose writes must be serialized against traces: catch a refresh racing this hide.
    G_ASSERTF(interlocked_acquire_load(refreshLock) == 0,
      "collres instance %p of res %p: setNodeEnabled during a concurrent trace refresh; serialize pose writes against traces", this,
      res);
    interlocked_release_store(poseGeneration, 0u); // never-refreshed: the next trace re-derives from the live tree
  }
  else
    recomputeRootBBox();
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
    if (node.boundingSphere.r < 0.f)
      continue; // zero-vert marker: corner-mapping the inverted box would union a phantom
    if ((node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX) && !node.hasGeometry())
      continue;
    bbox3f nodeBox = res->getNodeGeometryBBox(node);
    bbox3f composedBox;
    v_bbox3_init(composedBox, getNodeGeometryTm((int)i), nodeBox);
    // Same saturate-never-drop guard as the incremental grows: NaN stays out, an overflowed
    // extent clamps to the float range so the traceable node still bounds.
    join_saturated_finite(box, composedBox);
    // Epsilon-IDENT grid geometry needs both raw and composed bounds; a live pose diverging
    // within the IDENT eps widens this union conservatively.
    if (node.type == COLLISION_NODE_TYPE_MESH && (poseMeta[i].flags & CollisionNode::IDENT))
      v_bbox3_add_box(box, nodeBox);
  }
  rootBBox = box;
}

void CollisionResourceInstance::seedPose()
{
  const CollisionResourceInstance &def = res->defaultInstance;
  G_ASSERT(&def != this);
  poseMeta = def.poseMeta;
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
    // A driven pose deviates from bind and never licenses bind shortcuts or the grid walk.
    posedSinceBind = true;
    gridResidentPoseAtBind = false;
    // Bind meta stays valid for driven nodes under the no-scale contract, except IDENT: a
    // driven node can leave its authored placement, so it demotes to the translate class.
    // maxTmScale of a driven node bounds only the rel-tm (the wtm contributes scale 1).
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
      mat44f relTm;
      if ((res->collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID) && i < (uint32_t)res->relGeomNodeTms.size())
        v_mat44_make_from_43cu_unsafe(relTm, res->relGeomNodeTms[i].array);
      else
        v_mat44_ident(relTm);
      pm.maxTmScale = 1.f;
      uint8_t flags = CollisionResource::classifyNodeTmFlags(relTm, pm.maxTmScale);
      if (!(flags & CollisionNode::ORTHONORMALIZED))
        pm.maxTmScale = max(pm.maxTmScale, mat33_spectral_norm(relTm) * 1.0002f);
      else if (!is_exact_rigid_basis_v(relTm))
        pm.maxTmScale = max(pm.maxTmScale, 1.0015f); // in-class shear: the Gershgorin pad of stampConservativePoseScale
      if (flags & CollisionNode::IDENT)
        flags = (flags & ~CollisionNode::IDENT) | CollisionNode::TRANSLATE;
      pm.flags = flags;
      // Normalized det floor: a raw det/scale^3 compare overflows to Inf > Inf for large
      // finite rel tms and falsely hides well-conditioned nodes (the owned classify shares it).
      float ndet;
      const bool floorOk = CollisionResource::relativeDetAboveFloor(relTm, ndet);
      // Handedness belongs to the refresh's live sign compare on the COMPOSED placement (a
      // mirrored rel under a mirrored wtm is right-handed); the authored gate accepts mirrored
      // placements the same way, so the seed gates on the det floor alone.
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
  gridResidentPoseAtBind = def.gridResidentPoseAtBind;
  // Poses seed verbatim, so the default's rootBBox stays valid unless a hide was cleared.
  if (DAGOR_UNLIKELY(clearedHide))
    recomputeRootBBox();
  else
    rootBBox = def.rootBBox;
}
