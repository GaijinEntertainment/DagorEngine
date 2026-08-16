// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <gameRes/dag_collisionResource.h>
#include <gameRes/dag_gameResSystem.h>
#include <math/dag_plane3.h>
#include <sceneRay/dag_sceneRay.h>
#include <scene/dag_physMat.h>
#include <ioSys/dag_oodleIo.h>
#include <ioSys/dag_zstdIo.h>
#include <ioSys/dag_chainedMemIo.h>
#include <ioSys/dag_btagCompr.h>
#include <generic/dag_sort.h>
#include <debug/dag_debug.h>
#include <daBVH/dag_bvhBuild.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <daBVH/dag_swBLAS_soa4Convert.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <util/dag_hashedKeyMap.h>

#if (_TARGET_PC && !_TARGET_STATIC_LIB)
// Enable FRT for all objects in daEditor for capsule clipping
#define MIN_FACES_TO_CREATE_GRID 0
#define MIN_WIDTH_TO_CREATE_GRID 0
#else
#define MIN_FACES_TO_CREATE_GRID 64
#define MIN_WIDTH_TO_CREATE_GRID 5
#endif
#define USE_TRACE_GRID 1

CollisionResource *CollisionResource::loadResource(IGenLoad &crd, int res_id)
{
  CollisionResource *resource = new CollisionResource;
  resource->load(crd, res_id);
  return resource;
}

int CollisionResource::addSphereNode(const char *name, int16_t phys_mat_id, const BSphere3 &bsphere)
{
  // Builder geometry starts with an identity placement.
  CollisionNode &n = createNode();
  n.nameOfs = addName(name);
  n.physMatId = phys_mat_id;
  n.type = COLLISION_NODE_TYPE_SPHERE;
  n.flags = CollisionNode::IDENT;
  n.boundingSphere.c = bsphere.c;
  n.boundingSphere.r = bsphere.r;
  n.modelBBox = BBox3(bsphere);
  n.nodeIndex = (uint16_t)(allNodesList.size() - 1);
  return n.nodeIndex;
}

int CollisionResource::addBoxNode(const char *name, int16_t phys_mat_id, const BBox3 &bbox)
{
  CollisionNode &n = createNode();
  n.nameOfs = addName(name);
  n.physMatId = phys_mat_id;
  n.type = COLLISION_NODE_TYPE_BOX;
  n.flags = CollisionNode::IDENT;
  n.modelBBox = bbox;
  n.boundingSphere.c = bbox.center();
  n.boundingSphere.r = bbox.width().length() / 2.f;
  n.nodeIndex = (uint16_t)(allNodesList.size() - 1);
  return n.nodeIndex;
}

int CollisionResource::addCapsuleNode(const char *name, int16_t phys_mat_id, const Point3 &p0, const Point3 &p1, float radius)
{
  CollisionNode &n = createNode();
  n.nameOfs = addName(name);
  n.physMatId = phys_mat_id;
  n.type = COLLISION_NODE_TYPE_CAPSULE;
  n.flags = CollisionNode::IDENT;
  n.modelBBox = (BBox3(BSphere3(p0, radius)) += BSphere3(p1, radius));
  n.boundingSphere.c = (p0 + p1) / 2.f;
  n.boundingSphere.r = (p0 - p1).length() / 2.f + radius;
  n.capsuleIndex = (uint16_t)capsules.size();
  capsules.push_back(Capsule(p0, p1, radius));
  n.nodeIndex = (uint16_t)(allNodesList.size() - 1);
  return n.nodeIndex;
}

CollisionResource *CollisionResource::createSingleMesh(dag::ConstSpan<Point3_vec4> vertices, dag::ConstSpan<uint16_t> indices,
  const BBox3 &bbox, const BSphere3 &bsphere, uint32_t node_flags, const char *node_name)
{
  // Legacy 16-bit entry point: widen and forward to the uint32 path.
  dag::Vector<uint32_t, framemem_allocator> idx32(indices.size());
  for (int i = 0, e = (int)indices.size(); i < e; ++i)
    idx32[i] = indices[i];
  return createSingleMesh(vertices, dag::ConstSpan<uint32_t>(idx32.data(), idx32.size()), bbox, bsphere, node_flags, node_name);
}

CollisionResource *CollisionResource::createSingleMesh(dag::ConstSpan<Point3_vec4> vertices, dag::ConstSpan<uint32_t> indices,
  const BBox3 &bbox, const BSphere3 &bsphere, uint32_t node_flags, const char *node_name)
{
  CollisionResource *res = new CollisionResource;
  res->boundingBox = bbox;
  v_bbox3_init(res->vFullBBox, v_ldu(&bbox[0].x));
  v_bbox3_add_pt(res->vFullBBox, v_ldu(&bbox[1].x));
  res->vBoundingSphere = v_perm_xyzd(v_ldu(&bsphere.c.x), v_splats(bsphere.r2));
  res->boundingSphereRad = bsphere.r;
  // Defer the per-node chunk build (build_chunk=false): collapseAndOptimize below reads the raw verts/
  // indices straight from the spans (in_raw_*) and builds the one real chunk from its optimized staging,
  // avoiding a throwaway build + lossy vert21 round-trip in addMeshNode.
  res->addMeshNode(node_name, -1, TMatrix::IDENT, bbox, bsphere, vertices, indices,
    CollisionNode::TRACEABLE | CollisionNode::PHYS_COLLIDABLE | CollisionNode::FLAG_ALLOW_HOLE | CollisionNode::FLAG_DAMAGE_REQUIRED,
    (uint8_t)(node_flags | CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED), /*build_chunk*/ false);
  // addMeshNode appends the node but leaves the per-type lists empty; collapseAndOptimize early-returns
  // on meshNodesHead == INVALID_IDX, so link the node first or the optimize is a no-op.
  res->rebuildNodesLL();
  res->collapseAndOptimize(node_name, /*need_frt*/ false, /*frt_build_fast*/ true, /*raw_verts_out*/ nullptr,
    /*raw_indices_out*/ nullptr, vertices, indices);
  return res;
}

int CollisionResource::addMeshNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox,
  const BSphere3 &bsphere, dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint32_t> indices, uint16_t behavior_flags, uint8_t flags,
  bool build_chunk)
{
  // No vert-count cap: indices are uint32 and buildOneNodeBlasChunk is uint32-clean, so a node may hold
  // >65536 verts (it then goes grid-resident like any other). buildOneNodeBlasChunk reads the indices
  // (it builds its own reorder copy), so the caller's span is passed straight through.
  G_ASSERTF_RETURN(verts.size() > 2 && indices.size() > 2 && (indices.size() % 3) == 0, -1,
    "addMeshNode: malformed mesh geometry: verts.size=%d (need >2), indices.size=%d (need >2 and %%3==0)", (int)verts.size(),
    (int)indices.size());
  CollisionNode &n = createNode();
  const int idx = (int)allNodesList.size() - 1;
  n.nameOfs = addName(name);
  n.physMatId = phys_mat_id;
  n.type = COLLISION_NODE_TYPE_MESH;
  n.flags = flags;
  n.behaviorFlags = behavior_flags;
  {
    mat44f vTm;
    v_mat44_make_from_43cu_unsafe(vTm, tm.array);
    const float len0sq = tm.getcol(0).lengthSq();
    const float len1sq = tm.getcol(1).lengthSq();
    const float len2sq = tm.getcol(2).lengthSq();
    const float maxLenSq = max(len0sq, max(len1sq, len2sq));
    // Squared column lengths overflow for large finite scales and FTZ-underflow to zero below
    // ~1e-19; the spectral norm normalizes internally, so a valid basis always stamps a finite
    // nonzero conservative scale.
    setAuthoredNodeTm(idx, vTm, flags, (maxLenSq >= FLT_MIN && maxLenSq <= FLT_MAX) ? sqrtf(maxLenSq) : mat33SpectralNorm(vTm));
  }
  n.modelBBox = bbox;
  n.boundingSphere.c = bsphere.c;
  n.boundingSphere.r = bsphere.r;
  n.nodeIndex = (uint16_t)idx;
  if (!indices.empty())
  {
    n.indicesOfs = 0;
    n.indicesCount = (uint32_t)indices.size();
  }
  if (!verts.empty())
  {
    n.verticesCount = (uint32_t)verts.size();
    if (!build_chunk)
    {
      // Deferred build: caller owns the raw geometry and collapseAndOptimize() will build the one real
      // chunk from the optimized staging. verticesOfs/indicesOfs index that raw span (sole node -> 0).
      n.verticesOfs = 0;
    }
    else
    {
      // The owning vertex storage is a per-node BLAS chunk (like every other mesh/convex node);
      // rejected/degenerate chunks are dropped to indicesCount == 0 below.
      dag::Vector<vec4f> sScr, oScr;
      dag::Vector<Point3_vec4> pScr;
      dag::Vector<vec4f> qScr;
      dag::Vector<uint8_t> stkScr, soaScr;
      if (indices.empty() || !buildOneNodeBlasChunk(n, verts.data(), (unsigned)verts.size(), indices.data(), (unsigned)n.indicesCount,
                               sScr, oScr, pScr, qScr, stkScr, soaScr))
      {
        // Rejected chunk (reason logerr'd by the builder): drop its geometry -- no collision surface.
        n.indicesCount = 0;
      }
    }
  }
  numMeshNodes++;
  return idx;
}

int CollisionResource::addMeshNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox,
  const BSphere3 &bsphere, dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint16_t> indices, uint16_t behavior_flags, uint8_t flags)
{
  // Legacy 16-bit-index entry point: widen into a transient buffer and forward to the uint32 path.
  dag::Vector<uint32_t, framemem_allocator> idx32(indices.size());
  for (int i = 0, e = (int)indices.size(); i < e; ++i)
    idx32[i] = indices[i];
  return addMeshNode(name, phys_mat_id, tm, bbox, bsphere, verts, dag::ConstSpan<uint32_t>(idx32.data(), idx32.size()), behavior_flags,
    flags);
}

int CollisionResource::addConvexNode(const char *name, int16_t phys_mat_id, const TMatrix &tm, const BBox3 &bbox,
  const BSphere3 &bsphere, dag::ConstSpan<Point3_vec4> verts, dag::ConstSpan<uint16_t> indices, dag::ConstSpan<plane3f> convex_planes,
  uint16_t behavior_flags, uint8_t flags)
{
  // Reuses addMeshNode for the geometry payload, then re-types as CONVEX and appends planes.
  // numMeshNodes counts mesh-typed nodes; bump back down since this node ends up CONVEX.
  // A CONVEX node with planesCount == 0 is malformed (convex code paths iterate planesCount
  // directly), so reject an empty plane span before the type retag.
  G_ASSERTF_RETURN(!convex_planes.empty(), -1, "addConvexNode: convex_planes must be non-empty");
  G_ASSERTF_RETURN(convexPlanes.size() + convex_planes.size() < 0x10000u, -1, "convex plane buffer overflow: %d + %d > 65536",
    (int)convexPlanes.size(), (int)convex_planes.size());
  const int idx = addMeshNode(name, phys_mat_id, tm, bbox, bsphere, verts, indices, behavior_flags, flags);
  if (idx < 0)
    return idx;
  CollisionNode &n = allNodesList[idx];
  n.type = COLLISION_NODE_TYPE_CONVEX;
  if (numMeshNodes > 0)
    numMeshNodes--;
  n.planesOfs = (uint16_t)convexPlanes.size();
  n.planesCount = (uint16_t)convex_planes.size();
  convexPlanes.insert(convexPlanes.end(), convex_planes.begin(), convex_planes.end());
  return idx;
}

template <typename T>
static inline void readTab(IGenLoad &cb, T &tab)
{
  int s = cb.readInt();
  tab.resize(0);
  if (s)
  {
    reserve_and_resize(tab, s);
    cb.read(tab.data(), data_size(tab));
  }
}

static inline auto load_frt16(IGenLoad &cb) { return DeserializedStaticSceneRayTracerT<uint16_t>::load(cb); }

void CollisionResource::setAuthoredNodeTm(int node_index, mat44f_cref tm, uint8_t class_flags, float max_scale)
{
  G_ASSERT_RETURN((uint32_t)node_index < defaultInstance.nodeTm.size(), );
  // Invalidate the cached bind trace sphere until layout finalization.
  vBindTraceSphere = v_zero();
  bindTraceSphereStamped = false;
  v_mat_43cu_from_mat44(defaultInstance.nodeTm[node_index].array, tm);
  v_mat_43cu_from_mat44(authoredNodeTm[node_index].array, tm);
  if ((uint32_t)node_index < authoredNodeItm.size())
  {
    mat44f inv;
    v_mat44_inverse43(inv, tm);
    v_mat_43cu_from_mat44(authoredNodeItm[node_index].array, inv);
  }
  CollisionResourceInstance::PoseMeta &pm = defaultInstance.poseMeta[node_index];
  pm = CollisionResourceInstance::PoseMeta(); // node slots are reused (legacy drop path): no stale status bits
  // Serialized stamps are data: a NaN/Inf/denormal scale passes every MATRIX gate below yet
  // collapses or explodes the sphere culls that square it. Recompute out-of-band stamps.
  if (!(max_scale >= FLT_MIN && max_scale <= 1.f / FLT_MIN))
    max_scale = mat33SpectralNorm(tm);
  pm.maxTmScale = max_scale;
  pm.setPoseIdentity(collres_is_exact_identity_43(tm));
  pm.flags =
    class_flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE | CollisionNode::ORTHONORMALIZED | CollisionNode::ORTHOUNIFORM);
  // Capsule geometry is never exporter-baked, so epsilon-identity motion must still apply.
  if ((pm.flags & CollisionNode::IDENT) && allNodesList[node_index].type == COLLISION_NODE_TYPE_CAPSULE &&
      !collres_is_exact_identity_43(tm))
    pm.flags = (pm.flags & ~CollisionNode::IDENT) | CollisionNode::TRANSLATE;
  // Authored mirrored placements remain valid; only singular placements are hidden.
  // Any non-finite affine component (translation included) is unrealizable. Max-abs, not a
  // column sum: finite components can overflow the intermediate sum near the scale bound. The
  // exponent-bit test survives -ffinite-math-only, which folds float-domain tricks away.
  const bool tmFinite = v_test_xyzw_finite(v_max(v_max(v_abs(tm.col0), v_abs(tm.col1)), v_max(v_abs(tm.col2), v_abs(tm.col3))));
  // Scale-free gate on the matrix itself, not the serialized scale (a retained bake
  // serializes its EFFECTIVE scale); evaluated unconditionally so the log never reads an
  // unwritten ndet.
  float ndet;
  const bool detOk = relativeDetAboveFloor(tm, ndet);
  bool traceOk = tmFinite && detOk;
  if (traceOk)
  {
    // Inverse-based dispatch needs the whole inverse finite: near-bound translations and
    // huge-scale cofactors overflow it while the forward placement stays representable.
    mat44f inv;
    v_mat44_inverse43(inv, tm);
    traceOk = v_test_xyzw_finite(v_max(v_max(v_abs(inv.col0), v_abs(inv.col1)), v_max(v_abs(inv.col2), v_abs(inv.col3))));
  }
  pm.setTraceable(traceOk);
  pm.setComposable(traceOk); // authored mirrors are load-traceable, so composable follows
  // Per occurrence (not once): affected content must be enumerable from the log.
  if (DAGOR_UNLIKELY(!pm.isTraceable()))
    logwarn("collision: singular authored tm (ndet=%g) on node %d <%s> of res %p; hidden from the pose mirror "
            "(legacy dispatch unchanged until the migration reads it)",
      ndet, node_index, getNodeName(node_index), this);
  // Non-uniform primitive placement is supported conservatively but remains a content error.
  else if (DAGOR_UNLIKELY(pm.flags == 0 && (allNodesList[node_index].type == COLLISION_NODE_TYPE_SPHERE ||
                                             allNodesList[node_index].type == COLLISION_NODE_TYPE_CAPSULE)))
    LOGWARN_ONCE("collision: non-uniform authored tm on %s node %d of res %p traces as an ellipsoid",
      allNodesList[node_index].type == COLLISION_NODE_TYPE_SPHERE ? "sphere" : "capsule", node_index, this);
}

// Convert exporter-baked primitive geometry to node-local storage.
void CollisionResource::unbakePrimNode(CollisionNode &n, const TMatrix &tm, CollisionResourceInstance::PoseMeta &pm)
{
  if (pm.flags & CollisionNode::IDENT)
    return; // nothing baked beyond an eps-identity; dispatch treats IDENT T as a no-op
  if (!pm.isTraceable())
  {
    // A singular authored primitive must remain in its baked frame.
    pm.setGeometryBaked(true);
    return;
  }
  // Zero-vert marker: bounds are empty; corner-mapping the inverted box would explode it into
  // a huge phantom that accessors and a later re-export preserve.
  if (n.boundingSphere.r < 0.f)
    return;
  float sphereRadDivisor = 1.f;
  if (n.type == COLLISION_NODE_TYPE_SPHERE)
  {
    // A NON-CONFORMAL placement cannot un-bake the exporter's vert-fit sphere (any scalar
    // division under-recovers): it keeps the baked frame as a RETAINED, poseable bake with an
    // identity effective bind frame. Conformality is checked scale-relatively on a
    // max-element-normalized basis (squared lengths overflow/underflow at extreme scales, and
    // an absolute divisor floor would corrupt tiny valid placements).
    const Point3 c0 = tm.getcol(0), c1 = tm.getcol(1), c2 = tm.getcol(2);
    float m = 0.f;
    for (int a = 0; a < 3; a++)
    {
      const Point3 c = tm.getcol(a);
      m = max(m, max(fabsf(c.x), max(fabsf(c.y), fabsf(c.z))));
    }
    if (!(m >= FLT_MIN && m <= 1.f / FLT_MIN))
    {
      // Outside the invertible normal-float band there is no recoverable local frame.
      pm.setGeometryBaked(true);
      pm.setRetainedBake(true);
      pm.flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED; // effective bind frame is identity
      pm.maxTmScale = 1.f;
      return;
    }
    const Point3 n0 = c0 / m, n1 = c1 / m, n2 = c2 / m;
    const float l0 = lengthSq(n0), l1 = lengthSq(n1), l2 = lengthSq(n2);
    const float d01 = fabsf(n0 * n1), d02 = fabsf(n0 * n2), d12 = fabsf(n1 * n2);
    bool conformal = pm.flags != 0;
    if (conformal && !(pm.flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)))
    {
      const float s2 = max(l0, max(l1, l2));
      const float relTol = 2e-3f * s2;
      conformal = fabsf(l0 - s2) <= relTol && fabsf(l1 - s2) <= relTol && fabsf(l2 - s2) <= relTol && d01 <= relTol && d02 <= relTol &&
                  d12 <= relTol;
    }
    if (!conformal)
    {
      pm.setGeometryBaked(true);
      pm.setRetainedBake(true);
      pm.flags = CollisionNode::IDENT | CollisionNode::ORTHONORMALIZED; // effective bind frame is identity
      pm.maxTmScale = 1.f;
      return;
    }
    // The baked fit can sit along the SMALLEST stretch: divide by the Gershgorin sigma_min
    // lower bound; the conformality band caps the overshoot.
    const float nSigmaMin = sqrtf(max(min(l0 - d01 - d02, min(l1 - d01 - d12, l2 - d02 - d12)), 0.f));
    sphereRadDivisor = max(m * nSigmaMin, FLT_MIN);
  }
  // Always the full inverse: ORTHONORMALIZED is an epsilon class, and the transpose shortcut
  // compounds its scale error into the stored geometry on every load-export cycle.
  TMatrix itm = inverse(tm);
  BBox3 localBox;
  for (int k = 0; k < 8; k++)
    localBox += itm * n.modelBBox.point(k);
  // EXACT identity basis, not the eps TRANSLATE class: a 5e-4 basis slack still expands the
  // corner-mapped box, and skipping the refit would leave the narrow phase outside its culls.
  const bool exactIdentityBasis =
    tm.getcol(0) == Point3(1, 0, 0) && tm.getcol(1) == Point3(0, 1, 0) && tm.getcol(2) == Point3(0, 0, 1);
  if (!exactIdentityBasis)
  {
    // Rotated or skewed AABBs can only be un-baked conservatively.
    BBox3 recomposed;
    for (int k = 0; k < 8; k++)
      recomposed += tm * localBox.point(k);
    const float eps = 1e-3f * n.modelBBox.width().length() + 1e-5f;
    if (n.type == COLLISION_NODE_TYPE_BOX &&
        ((recomposed[0] - n.modelBBox[0]).length() > eps || (recomposed[1] - n.modelBBox[1]).length() > eps))
      LOGWARN_ONCE("collision: rotated/skewed authored placement on a baked box node; local bounds are conservative");
    if (n.type == COLLISION_NODE_TYPE_BOX && n.boundingSphere.r >= 0.f)
    {
      // The conservative box can outgrow the exporter's vert-fit sphere, and every sphere cull
      // must cover what the box narrow phase tests -- refit the sphere over the un-baked box.
      n.boundingSphere.c = localBox.center();
      n.boundingSphere.r = localBox.width().length() * 0.5f;
    }
  }
  n.modelBBox = localBox;
  if (n.type == COLLISION_NODE_TYPE_SPHERE)
  {
    // Only sphere bounding spheres are exporter-baked (verts * wtm); box bounding
    // spheres accumulate raw node-local verts at export, so they need no un-bake.
    n.boundingSphere.c = itm * n.boundingSphere.c;
    if (n.boundingSphere.r >= 0.f)
    {
      // Conformal classes only reach here (general sphere placements stay baked at entry).
      n.boundingSphere.r /= sphereRadDivisor;
      // Rebuild exact sphere bounds after the conservative corner transform.
      const Point3 r3(n.boundingSphere.r, n.boundingSphere.r, n.boundingSphere.r);
      n.modelBBox[0] = n.boundingSphere.c - r3;
      n.modelBBox[1] = n.boundingSphere.c + r3;
    }
  }
}

// Legacy on-disk bits in CollisionNode::flags signaling that the per-node mesh data was written
// as an offset into the FRT vertex/face dump rather than as raw data. The runtime no longer keeps
// these flags; the loader decodes the FRT slice into the vert + index staging and clears the bits.
static constexpr uint8_t LEGACY_FLAG_VERTICES_ARE_REFS = 64;
static constexpr uint8_t LEGACY_FLAG_INDICES_ARE_REFS = 128;

void CollisionResource::load(IGenLoad &_cb, int res_id)
{
  collisionFlags = 0;
  gridForTraceable.reset();
  gridForCollidable.reset();
  allNodesList.clear();
  names.clear();
  capsules.clear();
  convexPlanes.clear();
  // Per-node BLAS chunk storage. Clear here (grids reset above, so no live leaf points into it) so a reload
  // that skips buildNodeBlasChunks (empty/non-mesh) keeps no stale chunk bytes counted by getMemoryUsed().
  nodeBlasData.clear();
  // The default pose restarts from the authored placements read below; a reload must also
  // return every bind-state latch to its constructed value.
  defaultInstance.nodeTm.clear();
  authoredNodeTm.clear();
  authoredNodeItm.clear();
  defaultInstance.poseMeta.clear();
  defaultInstance.gridResidentPoseAtBind = true;
  defaultInstance.posedSinceBind = false;
  defaultInstance.bsphereCenterLocal = v_zero();
  defaultInstance.hasBsphereCenterLocal = false;
  geomNodeTreeBound = false; // replacement nodes have never been bound

  // Raw verts + source-face indices live only in this transient staging while loading: buildBLAS
  // flattens from it (so the BLAS quantizes the disk data directly -- the exporter weld targets exactly
  // that) and buildNodeBlasChunks packs the non-resident remainder into per-node chunks; after this
  // function only vert21 representations remain (grid blasData + per-node chunks in nodeBlasData).
  dag::Vector<Point3_vec4> stagingVerts;
  dag::Vector<uint32_t> stagingIndices;

  unsigned label = _cb.readInt();
  G_ASSERTF_RETURN((label & 0xFFFF0000) == 0xACE50000, , "Invalid collision resource: 0x%8X", label);

  int version = (label & 0xFFFF);
  if (version == 0)
    return loadLegacyRawFormat(_cb, res_id); // rebuildNodesLL inside restamps pose scales

  unsigned btag = 0;
  const unsigned compr_data_sz = _cb.beginBlock(&btag);
  uint8_t zcrdStorage[max(sizeof(ZstdLoadCB), sizeof(OodleLoadCB))];
  String tmp_str;

  IGenLoad *zcrd = nullptr;
  if (btag == btag_compr::ZSTD)
    zcrd = new (zcrdStorage, _NEW_INPLACE) ZstdLoadCB(_cb, compr_data_sz);
  else if (btag == btag_compr::OODLE)
    zcrd = new (zcrdStorage, _NEW_INPLACE) OodleLoadCB(_cb, compr_data_sz - 4, _cb.readInt());
  else
    zcrd = &_cb;

  zcrd->read(&vFullBBox, sizeof(vFullBBox));
  zcrd->read(&vBoundingSphere, sizeof(vBoundingSphere));
  zcrd->read(&boundingBox, sizeof(boundingBox));
  zcrd->readReal(boundingSphereRad);
  collisionFlags = zcrd->readInt();

  // Legacy FRT blocks on disk: drained into local unique_ptrs (released at end of this function), so
  // the runtime never holds them beyond load. Consulted only by LEGACY_FLAG_VERTICES_ARE_REFS /
  // LEGACY_FLAG_INDICES_ARE_REFS node entries to source mesh vertex/index data. The combined-per-
  // behavior BLAS is the only runtime acceleration structure, built below from the vert + index staging.
  using LegacyFRT = const StaticSceneRayTracerT<uint16_t>;
  eastl::unique_ptr<LegacyFRT, DestroyDeleter<LegacyFRT>> legacyTraceFRT, legacyCollFRT;
  if (collisionFlags & COLLISION_RES_FLAG_HAS_TRACE_FRT)
    legacyTraceFRT.reset(load_frt16(*zcrd));
  if ((collisionFlags & COLLISION_RES_FLAG_HAS_COLL_FRT) && !(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT))
    legacyCollFRT.reset(load_frt16(*zcrd));

  reserve_and_resize(allNodesList, zcrd->readInt());
  reserve_and_resize(defaultInstance.nodeTm, allNodesList.size());
  reserve_and_resize(authoredNodeTm, allNodesList.size());
  reserve_and_resize(defaultInstance.poseMeta, allNodesList.size());
  String tmp_node_name;
  // Reused across iterations to stage planes before committing them to the resource-level
  // convexPlanes pool. The commit happens at the bottom of each iteration so that any future
  // node-drop logic added between read and commit cannot orphan plane data.
  dag::RelocatableFixedVector<plane3f, 16, true, framemem_allocator> stagedPlanes;
  for (auto &n : allNodesList)
  {
    zcrd->readString(tmp_node_name);
    n.nameOfs = addName(tmp_node_name.c_str());
    zcrd->readString(tmp_str);
    n.physMatId = PhysMat::getMaterialId(tmp_str.str());

    zcrd->read(&n.modelBBox, sizeof(n.modelBBox));
    {
      BSphere3 tmpSph;
      zcrd->read(&tmpSph, sizeof(BSphere3));
      n.boundingSphere.c = tmpSph.c;
      n.boundingSphere.r = tmpSph.r;
    }
    n.behaviorFlags = zcrd->readIntP<2>();
    n.flags = zcrd->readIntP<1>();
    n.type = (CollisionResourceNodeType)zcrd->readIntP<1>();
    // Preserve serialized pose metadata verbatim.
    TMatrix authoredTm;
    float authoredMaxScale;
    zcrd->readReal(authoredMaxScale);
    zcrd->read(&authoredTm, sizeof(authoredTm));
    n.insideOfNode = zcrd->readIntP<2>();

    {
      uint16_t planesCnt = (uint16_t)zcrd->readIntP<2>();
      stagedPlanes.resize(planesCnt);
      if (planesCnt)
        zcrd->read(stagedPlanes.data(), planesCnt * sizeof(plane3f));
    }

    if (int cnt = zcrd->readInt())
    {
      if ((uint32_t)cnt > 0x10000u)
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        resName = "unknown";
#endif
        DAG_FATAL("Mesh vertex count %d > 65536 in node <%s> of res <%s>", cnt, getNodeNameStr(n), resName.c_str());
      }
      const uint32_t prev = (uint32_t)stagingVerts.size();
      n.verticesOfs = prev;
      n.verticesCount = (uint32_t)cnt;
      stagingVerts.resize(prev + cnt);
      if (n.flags & LEGACY_FLAG_VERTICES_ARE_REFS)
      {
        int ofs = zcrd->readInt();
        const auto &legacyFRT = (ofs & 0x40000000) ? legacyCollFRT : legacyTraceFRT;
        memcpy(stagingVerts.data() + prev, &legacyFRT->verts(ofs & 0xFFFFFF), cnt * sizeof(Point3_vec4)); //-V780
      }
      else
        zcrd->read(stagingVerts.data() + prev, cnt * sizeof(Point3_vec4));
    }

    if (int cnt = zcrd->readInt())
    {
      // cnt comes straight from the pack. A negative value sign-extends to a huge size_t in the
      // idx16 allocation below; a non-multiple-of-3 is not a triangle list. The upper bound is a pure
      // allocation guard against a corrupt count, not a format limit: index count is faces*3 and is
      // unrelated to the 65536 vertex bound, and the exporter writes idxs.size() uncapped, so the
      // bound must clear any node a real cook can produce or the pack fails to round-trip.
      if (cnt < 0 || (cnt % 3) != 0 || (uint32_t)cnt > 0x4000000u)
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        resName = "unknown";
#endif
        DAG_FATAL("Malformed index count %d in node <%s> of res <%s>", cnt, getNodeNameStr(n), resName.c_str());
      }
      const uint32_t prev = (uint32_t)stagingIndices.size();
      n.indicesOfs = prev;
      n.indicesCount = (uint32_t)cnt;
      stagingIndices.resize(prev + cnt);
      // The pack stores 16-bit node-local indices; the staging is uint32 (runtime per-node BLAS chunks
      // may dup past 65536 verts), so read the 16-bit slice into a temp and widen element-wise.
      dag::Vector<uint16_t, framemem_allocator> idx16((size_t)cnt);
      if (n.flags & LEGACY_FLAG_INDICES_ARE_REFS)
      {
        int ofs = zcrd->readInt();
        const auto &legacyFRT = (ofs & 0x40000000) ? legacyCollFRT : legacyTraceFRT;
        memcpy(idx16.data(), (uint16_t *)legacyFRT->faces(0).v + (ofs & 0xFFFFFF), cnt * sizeof(uint16_t));
      }
      else
        zcrd->read(idx16.data(), cnt * sizeof(uint16_t));
      uint32_t *dst = stagingIndices.data() + prev;
      for (int i = 0; i < cnt; ++i)
        dst[i] = idx16[i];
    }
    n.flags &= ~(LEGACY_FLAG_VERTICES_ARE_REFS | LEGACY_FLAG_INDICES_ARE_REFS);
    {
      const int nodeIdx = (int)(&n - allNodesList.data());
      mat44f vAuthoredTm;
      v_mat44_make_from_43cu_unsafe(vAuthoredTm, authoredTm.array);
      setAuthoredNodeTm(nodeIdx, vAuthoredTm, n.flags, authoredMaxScale);
      if (n.type == COLLISION_NODE_TYPE_BOX || n.type == COLLISION_NODE_TYPE_SPHERE)
        unbakePrimNode(n, authoredTm, defaultInstance.poseMeta[nodeIdx]);
    }
    if (n.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      // Capsule geometry remains node-local; T carries its placement.
      Capsule c;
      if (DAGOR_UNLIKELY(n.boundingSphere.r < 0.f))
      {
        // Zero-vert marker: Capsule::set on the inverted empty box would fabricate a huge
        // phantom segment; store a degenerate capsule and keep the node out of tracing.
        c.set(Point3(0, 0, 0), Point3(0, 0, 0), 0.f);
        defaultInstance.poseMeta[(int)(&n - allNodesList.data())].setTraceable(false);
      }
      else
        c.set(n.modelBBox);
      n.capsuleIndex = (uint16_t)capsules.size();
      capsules.push_back(c);
    }

    // Commit staged planes only after the node is fully populated and known to be kept.
    // If a future revision adds an empty-bbox or validation-based node-drop here, simply
    // `continue` without running this block to avoid orphan entries in convexPlanes.
    if (!stagedPlanes.empty())
    {
      size_t prevSize = convexPlanes.size();
      // planesOfs is uint16_t; offsets above 65535 would wrap on cast and read the wrong slice
      // back. Bail out with a fatal error rather than silently corrupting the resource.
      if (prevSize > eastl::numeric_limits<uint16_t>::max())
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        G_UNUSED(res_id);
        resName = "unknown";
#endif
        DAG_FATAL("Convex planes total %u exceeds uint16_t offset limit in node <%s> of res <%s>", (unsigned)prevSize,
          getNodeNameStr(n), resName.c_str());
      }
      n.planesOfs = (uint16_t)prevSize;
      n.planesCount = (uint16_t)stagedPlanes.size();
      convexPlanes.resize(prevSize + stagedPlanes.size());
      memcpy(convexPlanes.data() + prevSize, stagedPlanes.data(), stagedPlanes.size() * sizeof(plane3f));
    }

    n.nodeIndex = &n - allNodesList.data();
  }
  if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
  {
    reserve_and_resize(relGeomNodeTms, allNodesList.size());
    zcrd->read(relGeomNodeTms.data(), data_size(relGeomNodeTms));
  }
  if (zcrd != &_cb)
    zcrd->~IGenLoad();

  names.shrink_to_fit();
  capsules.shrink_to_fit();
  convexPlanes.shrink_to_fit();
  rebuildNodesLL();

  // Build the runtime BLAS from the raw staging, but ONLY for assets already in final
  // collapsed/baked layout (COLLISION_RES_FLAG_OPTIMIZED on disk). A non-optimized asset is later
  // handed to collapseAndOptimize (rendinst optimize_collres_on_load, level streaming), which
  // materializes the packed verts, rebuilds, and re-packs there. (Any FRT bytes on disk were drained
  // into the local legacy*FRT unique_ptrs above; BLAS is the only acceleration structure the runtime
  // keeps.) Same selector as the FRT-era: REUSE_TRACE_FRT set -> gridForTraceable holds the shared
  // BLAS; else gridForCollidable is separate.
  if (collisionFlags & COLLISION_RES_FLAG_OPTIMIZED)
  {
    // Cull parity: BLAS is rebuilt (not serialized), so cull mode is restored from the persisted
    // COLLISION_RES_FLAG_BLAS_TWO_SIDED marker (set by collapseAndOptimize from need_frt). Legacy
    // assets predating it fall back to HAS_*_FRT (carried a CULL_BOTH FRT iff set); an optimized
    // asset with neither marker nor FRT traced via the backface-culling path, so its BLAS culls CCW.
    // Recompute the trace/collidable equality gate from the loaded node sets instead of trusting the
    // persisted bit: a stale REUSE_TRACE_FRT set when the sets differ would route
    // getBlasGrid(PHYS_COLLIDABLE) to gridForTraceable and skip gridForCollidable below, dropping
    // collidable-only IDENT mesh nodes from collision. Mirrors the recompute collapseAndOptimize runs.
    recomputeTraceReuseFlagFromNodeSets();
    const bool twoSidedMarker = (collisionFlags & COLLISION_RES_FLAG_BLAS_TWO_SIDED) != 0;
    gridForTraceable.buildBLAS(this, make_span_const(stagingVerts), make_span_const(stagingIndices), CollisionNode::TRACEABLE,
      /*two_sided*/ twoSidedMarker || (collisionFlags & COLLISION_RES_FLAG_HAS_TRACE_FRT) != 0);
    if (!(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT))
      gridForCollidable.buildBLAS(this, make_span_const(stagingVerts), make_span_const(stagingIndices), CollisionNode::PHYS_COLLIDABLE,
        /*two_sided*/ twoSidedMarker || (collisionFlags & COLLISION_RES_FLAG_HAS_COLL_FRT) != 0);
  }
  // Stamp grid membership + residency for nodes the grids absorbed (no-op when no BLAS was built) and
  // pack the remainder as per-node chunks. MUST run after BOTH buildBLAS calls so the second build
  // still reads unmodified verticesOfs slices for nodes the first build included.
  // Per-node BLAS chunks for big non-resident mesh nodes: needs the raw staging (reads slices,
  // packs the chunked blocks itself) and the final grids (residency prediction), so it sits
  // between the BLAS builds and the stamp.
  buildNodeBlasChunks(stagingVerts, stagingIndices);
  stampBlasResidentNodes();
  /* if (!validateVerticesForJolt())
  {
    String resName;
    get_game_resource_name(res_id, resName);
    logerr("Degenerative triangles detected in res: %s", resName.c_str());
  }*/
}

void CollisionResource::loadLegacyRawFormat(IGenLoad &_cb, int res_id, int (*resolve_phmat)(const char *),
  dag::Vector<Point3_vec4> *raw_verts_out, dag::Vector<uint32_t> *raw_indices_out)
{
  convexPlanes.clear();
  // Raw verts + source-face indices are read into staging: the exporter (raw_verts_out/raw_indices_out)
  // keeps them as its full-precision workspace (external-raw mode -- the weld/validate/serialize
  // pipeline must see exact floats); the runtime (null) packs them into per-node chunks / the grid at
  // the end and drops the staging.
  dag::Vector<Point3_vec4> localStaging;
  dag::Vector<uint32_t> localStagingIdx;
  dag::Vector<Point3_vec4> &stagingVerts = raw_verts_out ? *raw_verts_out : localStaging;
  dag::Vector<uint32_t> &stagingIndices = raw_indices_out ? *raw_indices_out : localStagingIdx;
  stagingVerts.clear();
  stagingIndices.clear();
  int version = _cb.readInt();
  bool hasMaterialData = version >= 0x20150115;
  bool hasCollisionFlags = version >= 0x20180510;
  if (version != 0x20200300 && version != 0x20180510 && version != 0x20160120 && version != 0x20150115 && version != 0x20080925)
    DAG_FATAL("Invalid collision resource version %#08X", version);

  BSphere3 boundingSphere;
  _cb.beginBlock();
  _cb.read(&boundingSphere, sizeof(BSphere3));
  _cb.endBlock();
  vBoundingSphere = v_perm_xyzd(v_ldu(&boundingSphere.c.x), v_splats(boundingSphere.r2));
  boundingSphereRad = boundingSphere.r;

  unsigned int blockFlags = 0;
  const int blockSize = _cb.beginBlock(&blockFlags);
  struct MemoryChainedData *unpacked_data = nullptr;

  if (blockFlags == btag_compr::ZSTD)
  {
    MemorySaveCB cwrUnpack(clamp((blockSize * 4 + 0xFFF) & ~0xFFF, 2 << 10, 64 << 10));
    zstd_decompress_data(cwrUnpack, _cb, blockSize);
    unpacked_data = cwrUnpack.takeMem();
  }
  else if (blockFlags == btag_compr::OODLE)
  {
    int decompSize = _cb.readInt();
    MemorySaveCB cwrUnpack(eastl::min(decompSize, 64 << 10));
    oodle_decompress_data(cwrUnpack, _cb, blockSize - sizeof(int), decompSize);
    unpacked_data = cwrUnpack.takeMem();
  }

  MemoryLoadCB crd(unpacked_data, true);
  IGenLoad &cb = blockFlags != btag_compr::NONE ? static_cast<IGenLoad &>(crd) : _cb;

  if (hasCollisionFlags)
    collisionFlags = cb.readInt();
  unsigned int numNodes = cb.readInt();
  reserve_and_resize(allNodesList, numNodes);
  reserve_and_resize(defaultInstance.nodeTm, numNodes);
  reserve_and_resize(authoredNodeTm, numNodes);
  reserve_and_resize(defaultInstance.poseMeta, numNodes);
  defaultInstance.gridResidentPoseAtBind = true;
  defaultInstance.posedSinceBind = false;
  defaultInstance.bsphereCenterLocal = v_zero();
  defaultInstance.hasBsphereCenterLocal = false;
  geomNodeTreeBound = false; // replacement nodes have never been bound
  if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
    reserve_and_resize(relGeomNodeTms, numNodes);

  dag::RelocatableFixedVector<int, 4096> tempIndices;
  dag::RelocatableFixedVector<Point3, 1024> tempVertices;
  SmallTab<Plane3, TmpmemAlloc> tempConvexPlanes;
  bbox3f totalBBox;
  v_bbox3_init_empty(totalBBox); // recalc bbox from nodes
  for (size_t nodeNo = 0; nodeNo < numNodes; nodeNo++)
  {
    CollisionNode &node = allNodesList[nodeNo];

    node.nodeIndex = nodeNo;

    String tmp_node_name;
    cb.readString(tmp_node_name);
    node.nameOfs = addName(tmp_node_name.c_str());
    if (hasMaterialData)
    {
      String matName;
      cb.readString(matName);
      node.physMatId = resolve_phmat ? resolve_phmat(matName.c_str()) : PhysMat::getMaterialId(matName.c_str());
    }
    else
      node.physMatId = PHYSMAT_INVALID;

    uint32_t typeAndBehFlags = cb.readInt();
    node.type = (CollisionResourceNodeType)(typeAndBehFlags & 0xFFu);
    if ((collisionFlags & COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS) == COLLISION_RES_FLAG_HAS_BEHAVIOUR_FLAGS)
    {
      cb.read(&node.behaviorFlags, 1);
      node.behaviorFlags = (typeAndBehFlags & 0xFF00) | (node.behaviorFlags & 0x00FF);
    }

    TMatrix authoredTm;
    cb.read(&authoredTm, sizeof(TMatrix));
    if (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID)
      cb.read(&relGeomNodeTms[nodeNo], sizeof(TMatrix));
    mat44f vNodeTm;
    v_mat44_make_from_43cu_unsafe(vNodeTm, authoredTm.array);
    float authoredMaxScale;
    node.flags = classifyNodeTmFlags(vNodeTm, authoredMaxScale);
    setAuthoredNodeTm((int)nodeNo, vNodeTm, node.flags, authoredMaxScale);

    {
      BSphere3 tmpSph;
      cb.read(&tmpSph, sizeof(BSphere3));
      node.boundingSphere.c = tmpSph.c;
      node.boundingSphere.r = tmpSph.r;
    }
    cb.read(&node.modelBBox, sizeof(BBox3));
    // Stage convex planes in tempConvexPlanes; they are pushed into the resource-level
    // convexPlanes array only after the bbox-validity check below, so dropped nodes do not
    // orphan plane data.
    if (node.type == COLLISION_NODE_TYPE_CONVEX)
    {
      readTab(cb, tempConvexPlanes);
      for (int i = 0; i < tempConvexPlanes.size(); ++i)
        tempConvexPlanes[i].normalize();
    }
    else
      tempConvexPlanes.clear();
    readTab(cb, tempVertices);
    if (!tempVertices.empty())
    {
      int verticesLimit = 0x10000; // 16-bit indices address 0..65535, so 65536 verts fit (matches the modern loader / addMeshNode)
      if (tempVertices.size() > verticesLimit)
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        G_UNUSED(res_id);
        resName = "unknown";
#endif
        DAG_FATAL("Mesh vertexes count %i > %i in node <%s> of res <%s>", tempVertices.size(), verticesLimit, getNodeNameStr(node),
          resName.c_str());
      }
    }

    readTab(cb, tempIndices);
    bbox3f nodeBBox = v_ldu_bbox3(node.modelBBox);
    if (!tempIndices.empty() && !tempVertices.empty())
    {
      v_bbox3_init_empty(nodeBBox);
      for (int i = 0; i < tempVertices.size(); i++)
        v_bbox3_add_pt(nodeBBox, v_ldu(&tempVertices[i].x));

      v_stu_bbox3(node.modelBBox, nodeBBox);
    }

    if (!v_bbox3_is_empty(nodeBBox))
    {
      if (node.type == COLLISION_NODE_TYPE_MESH || node.type == COLLISION_NODE_TYPE_CONVEX || node.type == COLLISION_NODE_TYPE_CAPSULE)
        v_bbox3_init(nodeBBox, vNodeTm, nodeBBox);
      v_bbox3_add_box(totalBBox, nodeBBox);
    }

    if (node.modelBBox.lim[0].x > node.modelBBox.lim[1].x) // Particle View 01, isempty() throws float exception.
    {
      nodeNo--;
      numNodes--;
      allNodesList.resize(numNodes);
      defaultInstance.nodeTm.resize(numNodes); // pose arrays shrink in lockstep with the node list
      authoredNodeTm.resize(numNodes);
      defaultInstance.poseMeta.resize(numNodes);
      continue;
    }

    // Node is kept: commit its mesh data into the resource-wide arrays.
    if (!tempVertices.empty())
    {
      const uint32_t prev = (uint32_t)stagingVerts.size();
      node.verticesOfs = prev;
      node.verticesCount = (uint32_t)tempVertices.size();
      stagingVerts.resize(prev + tempVertices.size());
      Point3_vec4 *dst = stagingVerts.data() + prev;
      for (int i = 0; i < tempVertices.size(); ++i)
      {
        dst[i] = tempVertices[i];
        dst[i].resv = 1.0f;
      }
    }
    if (!tempIndices.empty() && !tempVertices.empty())
    {
      const uint32_t prev = (uint32_t)stagingIndices.size();
      node.indicesOfs = prev;
      stagingIndices.resize(prev + tempIndices.size());
      uint32_t *dst = stagingIndices.data() + prev;
      // Drop faces with any out-of-range vertex index here, not at BLAS build: the exporter raw path
      // (collapseAndOptimize -> raw_indices_out) hands these straight to exp_collision, which indexes
      // m.vert[idx] unchecked. A signed cast of a negative index wraps to a huge unsigned, so the
      // unsigned compare catches both negatives and overflow.
      const unsigned vcount = (unsigned)tempVertices.size();
      uint32_t kept = 0, droppedFaces = 0;
      for (int i = 0; i + 2 < tempIndices.size(); i += 3) // rotate (0,1,2)->(0,2,1)
      {
        if ((unsigned)tempIndices[i] >= vcount || (unsigned)tempIndices[i + 1] >= vcount || (unsigned)tempIndices[i + 2] >= vcount)
        {
          ++droppedFaces;
          continue;
        }
        dst[kept + 0] = (uint32_t)tempIndices[i + 0];
        dst[kept + 2] = (uint32_t)tempIndices[i + 1];
        dst[kept + 1] = (uint32_t)tempIndices[i + 2];
        kept += 3;
      }
      const uint32_t trailing = (uint32_t)(tempIndices.size() % 3);
      if (droppedFaces || trailing)
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        resName = "unknown";
#endif
        if (droppedFaces)
          logerr("collision node <%s> of res <%s>: %u faces reference an out-of-range vertex; dropped", getNodeNameStr(node),
            resName.c_str(), droppedFaces);
        if (trailing)
          logerr("collision node <%s> of res <%s>: %u trailing indices (not a whole face); dropped", getNodeNameStr(node),
            resName.c_str(), trailing);
      }
      stagingIndices.resize(prev + kept);
      node.indicesCount = kept;
    }

    if (node.type == COLLISION_NODE_TYPE_BOX || node.type == COLLISION_NODE_TYPE_SPHERE)
      unbakePrimNode(node, authoredTm, defaultInstance.poseMeta[nodeNo]);
    if (node.type == COLLISION_NODE_TYPE_CAPSULE)
    {
      // node-local, no bake: T carries the placement (see the modern load path)
      Capsule c;
      if (DAGOR_UNLIKELY(node.boundingSphere.r < 0.f))
      {
        // Zero-vert marker: see the modern load path.
        c.set(Point3(0, 0, 0), Point3(0, 0, 0), 0.f);
        defaultInstance.poseMeta[(int)nodeNo].setTraceable(false);
      }
      else
        c.set(node.modelBBox);
      node.capsuleIndex = (uint16_t)capsules.size();
      capsules.push_back(c);
    }
    else if (node.type == COLLISION_NODE_TYPE_CONVEX && !tempConvexPlanes.empty())
    {
      size_t prevSize = convexPlanes.size();
      if (prevSize > eastl::numeric_limits<uint16_t>::max())
      {
        String resName;
#if _TARGET_STATIC_LIB
        get_game_resource_name(res_id, resName);
#else
        G_UNUSED(res_id);
        resName = "unknown";
#endif
        DAG_FATAL("Convex planes total %u exceeds uint16_t offset limit in node <%s> of res <%s>", (unsigned)prevSize,
          getNodeNameStr(node), resName.c_str());
      }
      node.planesOfs = (uint16_t)prevSize;
      node.planesCount = (uint16_t)tempConvexPlanes.size();
      convexPlanes.resize(prevSize + tempConvexPlanes.size());
      for (int i = 0; i < tempConvexPlanes.size(); ++i)
        convexPlanes[prevSize + i] = v_ldu(&tempConvexPlanes[i].n.x);
    }
  }

  vFullBBox = totalBBox;
  v_stu_bbox3(boundingBox, totalBBox);

  names.shrink_to_fit();
  capsules.shrink_to_fit();
  convexPlanes.shrink_to_fit();
  stagingVerts.shrink_to_fit();
  stagingIndices.shrink_to_fit();
  sortNodesList();
  rebuildNodesLL();

  // Exporter raw workspace stays external-raw (full-precision spans); runtime packs and drops the
  // staging (no grids exist on this path -- collapseAndOptimize builds them later -- so nothing is
  // stamped resident here).
  if (!raw_verts_out)
  {
    // No grids on this path (residency prediction sees empty blasNodeRanges), so every big mesh
    // node is chunk-eligible -- exactly what never-collapsed dedicated-server assets want.
    buildNodeBlasChunks(stagingVerts, stagingIndices);
    stampBlasResidentNodes();
  }

  _cb.endBlock();
}

void CollisionResource::recomputeTraceReuseFlagFromNodeSets()
{
  bool coll_and_trace_equals = true;
  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    const CollisionNode *node = &allNodesList[mi];
    if (node->checkBehaviorFlags(CollisionNode::TRACEABLE) != node->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE))
    {
      coll_and_trace_equals = false;
      break;
    }
  }
  // Assign (set AND clear), never OR-only: this is the authoritative gate for gridForCollidable below,
  // and a stale REUSE bit can arrive set (collisionFlags is read off disk; the bit is persisted by the
  // exporter). Leaving it set when the sets differ would skip gridForCollidable, and the per-node
  // fallback also skips collidable-only IDENT mesh nodes whenever a trace BLAS exists -- so those nodes
  // would get no collision coverage. Mirror the BLAS_TWO_SIDED set/clear.
  if (coll_and_trace_equals)
    collisionFlags |= COLLISION_RES_FLAG_REUSE_TRACE_FRT;
  else
    collisionFlags &= ~COLLISION_RES_FLAG_REUSE_TRACE_FRT;
}

void CollisionResource::collapseAndOptimize(const char *res_name, bool need_frt, bool frt_build_fast,
  dag::Vector<Point3_vec4> *raw_verts_out, dag::Vector<uint32_t> *raw_indices_out, dag::ConstSpan<Point3_vec4> in_raw_verts,
  dag::ConstSpan<uint32_t> in_raw_indices)
{
  if (collisionFlags & COLLISION_RES_FLAG_OPTIMIZED)
  {
    // debug("skip already optimized %p", this);
    return;
  }
  collisionFlags |= COLLISION_RES_FLAG_OPTIMIZED;
  // addMeshNode/addConvexNode publish nodes without touching the per-type lists, so a builder that
  // skipped rebuildNodesLL() would reach here unlinked and the optimize below would silently be a
  // no-op. Self-heal once; already-linked callers keep a valid head and never enter this branch (no
  // double rebuild), and a genuinely node-less resource still has INVALID_IDX after the rebuild.
  if (meshNodesHead == CollisionNode::INVALID_IDX)
    rebuildNodesLL();
  if (meshNodesHead == CollisionNode::INVALID_IDX) // || numMeshNodes < 2
  {
    // debug("CollisionResource: nothing to optimize %p", this);
    return;
  }

  // Materialize every mesh/convex node's verts into a raw staging workspace: phases A-C below
  // mutate and merge raw Point3_vec4 data, buildBLAS flattens from it, and the final storage is
  // rebuilt from it at the end (vert21-packed for the runtime, or handed to raw_verts_out for the
  // exporter). Sources: the runtime decodes the packed per-node chunks via iterateNodeVerts; the
  // exporter reads its full-precision raw_verts workspace directly (inputRaw below). The w lane is
  // forced to 1.0f to match the legacy raw layout (the exporter serializes Point3_vec4 verbatim).
  // Exporter input arrives full-precision in raw_verts_out (loadLegacyRawFormat wrote it there; node
  // verticesOfs still index it). Either way the resource never holds the raw verts -- they live in
  // raw_verts_out or the local staging.
  // Prefer an explicit raw input (createSingleMesh) over the exporter's raw_verts_out: both feed the
  // staging loop below the same way (skip the chunk decode), but only raw_verts_out also redirects the
  // tail to hand verts back instead of building runtime chunks.
  const Point3_vec4 *inputRaw = !in_raw_verts.empty() ? in_raw_verts.data() : (raw_verts_out ? raw_verts_out->data() : nullptr);
  const uint32_t *inputIdx = !in_raw_indices.empty() ? in_raw_indices.data() : (raw_indices_out ? raw_indices_out->data() : nullptr);
  dag::Vector<Point3_vec4> staging;
  dag::Vector<uint32_t> idxStaging;
  {
    size_t totalV = 0, totalI = 0;
    for (const CollisionNode &n : allNodesList)
      if ((n.type == COLLISION_NODE_TYPE_MESH || n.type == COLLISION_NODE_TYPE_CONVEX) && n.hasGeometry())
      {
        totalV += (uint32_t)n.verticesCount;
        totalI += n.indicesCount;
      }
    staging.reserve(totalV);
    idxStaging.reserve(totalI);
    for (CollisionNode &n : allNodesList)
    {
      if ((n.type != COLLISION_NODE_TYPE_MESH && n.type != COLLISION_NODE_TYPE_CONVEX) || !n.hasGeometry())
        continue;
      const uint32_t oldVOfs = n.verticesOfs;
      const uint32_t cnt = (uint32_t)n.verticesCount;
      const uint32_t vofs = (uint32_t)staging.size();
      staging.resize(vofs + cnt);
      Point3_vec4 *dst = staging.data() + vofs;
      if (inputRaw) // exporter: copy full-precision verts straight from raw_verts_out (w lane forced to 1)
        for (uint32_t i = 0; i < cnt; ++i)
          v_st(&dst[i].x, v_perm_xyzd(v_ld(&inputRaw[oldVOfs + i].x), V_C_ONE));
      else
        iterateNodeVerts((int)n.nodeIndex, [&](int i, vec4f v) { v_st(&dst[i].x, v_perm_xyzd(v, V_C_ONE)); });
      n.verticesOfs = vofs;
      // Faces (node-local, [0, verticesCount)): the exporter copies its full-precision index workspace;
      // the runtime re-materialises from the chunk leaf walk -- the same node-local order
      // iterateNodeVerts decoded the block above -- so phases A-C, the rebuild and the final buildBLAS
      // read/mutate verts+indices in lockstep. The resource keeps no index list either way.
      const uint32_t oldIOfs = n.indicesOfs;
      n.indicesOfs = (uint32_t)idxStaging.size();
      if (inputIdx)
        idxStaging.insert(idxStaging.end(), inputIdx + oldIOfs, inputIdx + oldIOfs + n.indicesCount);
      else
        walkNodeChunkLeavesForFaces(n, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
          idxStaging.push_back(i0);
          idxStaging.push_back(i1);
          idxStaging.push_back(i2);
        });
    }
  }

  // Phase A: bake the current default placement into staging geometry.
  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    CollisionNode *m = &allNodesList[mi];
    if (m->type != COLLISION_NODE_TYPE_CONVEX && m->type != COLLISION_NODE_TYPE_MESH)
      continue;
    if ((defaultInstance.poseMeta[m->nodeIndex].flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) == CollisionNode::IDENT ||
        m->indicesCount == 0)
      continue;
    // Singular authored placements remain hidden and unbaked.
    if (!defaultInstance.poseMeta[m->nodeIndex].isTraceable())
      continue;
    Point3_vec4 *vbase = staging.data() + m->verticesOfs;
    uint32_t *ibase = idxStaging.data() + m->indicesOfs;
    const uint32_t mVCount = (uint32_t)m->verticesCount;
    // The full plane inverse needs defined w lanes.
    mat44f nodeTm;
    v_mat44_make_from_43cu(nodeTm, defaultInstance.nodeTm[m->nodeIndex].array);
    bbox3f box;
    v_bbox3_init_empty(box);
    for (vec4f *__restrict verts = (vec4f *)(void *)vbase, *ve = verts + mVCount; verts != ve; ++verts)
    {
      *verts = v_mat44_mul_vec3p(nodeTm, *verts);
      v_bbox3_add_pt(box, *verts);
    }
    vec4f vSphereC = v_bbox3_center(box), sphereRad2 = v_zero();
    for (vec4f *__restrict verts = (vec4f *)(void *)vbase, *ve = verts + mVCount; verts != ve; ++verts)
      sphereRad2 = v_max(sphereRad2, v_length3_sq_x(v_sub(vSphereC, *verts)));

    mat44f N, TN;
    v_mat44_inverse(N, nodeTm);
    v_mat44_transpose(TN, N);
    plane3f *__restrict base = convexPlanes.data() + m->planesOfs;
    for (plane3f *__restrict planes = base, *pe = base + m->planesCount; planes != pe; ++planes)
      *planes = v_mat44_mul_vec4(TN, *planes);

    v_stu_bbox3(m->modelBBox, box);
    v_stu_p3(&m->boundingSphere.c.x, vSphereC);
    m->boundingSphere.r = v_extract_x(v_sqrt_x(sphereRad2));

    const float tmDet = v_extract_x(v_dot3_x(nodeTm.col0, v_cross3(nodeTm.col1, nodeTm.col2)));
    if (tmDet < 0.f) // swap indices order
      for (uint32_t i = 0, e = m->indicesCount; i + 2 < e; i += 3)
        eastl::swap(ibase[i + 0], ibase[i + 2]);
    m->flags = CollisionNode::IDENT | (m->flags & (~CollisionNode::TRANSLATE));
    m->flags = CollisionNode::ORTHONORMALIZED | (m->flags & (~CollisionNode::ORTHOUNIFORM));
    mat44f identTm;
    v_mat44_ident(identTm);
    setAuthoredNodeTm(m->nodeIndex, identTm, m->flags, 1.f);
  }

  // Phase B: bucket mesh nodes by (matId, isPhysCollidable) and build per-bucket merged geometry
  // into framemem temps. We do not mutate the staging verts / index staging here -- the rebuild pass
  // below produces fresh dense arrays from the existing slices plus the merged buckets.
  IMemAlloc *framemem = framemem_ptr();
  Tab<Tab<CollisionNode *>> meshNodesByMat(framemem);
  Tab<eastl::pair<PhysMat::MatID, bool>> matIndices(framemem);

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    CollisionNode *m = &allNodesList[mi];
    const bool isTraceable = m->checkBehaviorFlags(CollisionNode::TRACEABLE);
    const bool isPhysCollidable = m->checkBehaviorFlags(CollisionNode::PHYS_COLLIDABLE);
    // indicesCount guard: a degenerate-dropped node has no staging slice (the staging pass above skips
    // it), so bucketizing it would read past its absent slice at staging.data() + verticesOfs below.
    // Unbaked singular nodes cannot join an identity bucket.
    if (m->type == COLLISION_NODE_TYPE_MESH && isTraceable && m->hasGeometry() && defaultInstance.poseMeta[m->nodeIndex].isTraceable())
    {
      const eastl::pair<PhysMat::MatID, bool> matIndicesValue = eastl::make_pair(m->physMatId, isPhysCollidable);
      intptr_t bucket = find_value_idx(matIndices, matIndicesValue);
      if (bucket == -1)
      {
        Tab<CollisionNode *> nodes(framemem);
        bucket = meshNodesByMat.size();
        meshNodesByMat.push_back(nodes);
        matIndices.push_back(matIndicesValue);
      }
      meshNodesByMat[size_t(bucket)].push_back(m);
    }
  }

  // Per-bucket merged data, keyed by target nodeIndex (= nodes[0]->nodeIndex). The eventual rebuild
  // walks allNodesList; for each surviving node we look up a possibly-pending bucket merge here.
  struct BucketMerge
  {
    uint16_t targetNodeIndex;
    dag::Vector<Point3_vec4, framemem_allocator> verts;
    dag::Vector<uint16_t, framemem_allocator> indices;
  };
  dag::Vector<BucketMerge, framemem_allocator> bucketMerges;
  bucketMerges.reserve(meshNodesByMat.size());
  dag::RelocatableFixedVector<int, 256> nodesToRemove;
  nodesToRemove.reserve(allNodesList.size());

  for (const Tab<CollisionNode *> &nodes : meshNodesByMat)
  {
    CollisionNode *targetNode = nodes[0];
    uint64_t v_total = 0, i_total = 0;
    for (const CollisionNode *node : nodes)
    {
      // Bucketed nodes are mesh nodes added to meshNodesByMat only when type==MESH and traceable;
      // they always have valid mesh data (indicesCount > 0), so verticesCount is meaningful.
      v_total += (uint32_t)node->verticesCount;
      i_total += node->indicesCount;
    }
    if (v_total > 65530)
    {
      // Too large to merge into one 16-bit-indexed node. With per-node BLAS storage each node becomes its own
      // uint32 chunk (or goes grid-resident), so keep them all unmerged instead of dropping nodes[1..].
      debug("CollisionResource: bucket too large (%llu verts), keeping %d nodes unmerged %p res <%s>", (unsigned long long)v_total,
        (int)nodes.size(), this, res_name);
      continue;
    }
    if (v_total == 0)
      continue;

    BucketMerge bm;
    bm.targetNodeIndex = targetNode->nodeIndex;
    bm.verts.reserve((size_t)v_total);
    bm.indices.reserve((size_t)i_total);
    BBox3 bbox;
    uint16_t v_off = 0;
    for (CollisionNode *m : nodes)
    {
      const Point3_vec4 *srcVerts = staging.data() + m->verticesOfs;
      const uint32_t *srcIdx = idxStaging.data() + m->indicesOfs;
      const uint32_t mVCount = (uint32_t)m->verticesCount;
      for (uint32_t i = 0, e = mVCount; i < e; ++i)
      {
        bm.verts.push_back(srcVerts[i]);
        bbox += srcVerts[i];
      }
      for (uint32_t i = 0, e = m->indicesCount; i < e; ++i)
        bm.indices.push_back(uint16_t(srcIdx[i] + v_off));
      v_off = (uint16_t)(v_off + mVCount);
    }

    targetNode->modelBBox = bbox;
    targetNode->boundingSphere.c = bbox.center();
    float r2 = 0.f;
    for (const Point3_vec4 &v : bm.verts)
      inplace_max(r2, lengthSq(v - targetNode->boundingSphere.c));
    targetNode->boundingSphere.r = sqrtf(r2);
    targetNode->flags |= targetNode->IDENT;
    targetNode->flags &= ~targetNode->TRANSLATE;
    {
      mat44f identTm;
      v_mat44_ident(identTm);
      setAuthoredNodeTm(targetNode->nodeIndex, identTm, targetNode->flags, 1.f);
    }

    bucketMerges.push_back(eastl::move(bm));

    // The merged geometry now lives in nodes[0]; drop the now-redundant bucket members. Too-large buckets
    // took the early continue above and keep all their nodes (each becomes a per-node BLAS chunk).
    for (int i = 1; i < nodes.size(); ++i)
      nodesToRemove.push_back(nodes[i]->nodeIndex);
  }

  // Compact allNodesList: drop merged-away nodes. Surviving nodes keep their (now-stale) offsets;
  // they will be re-stamped during the rebuild pass below.
  Tab<CollisionNode> newAllNodes(tmpmem);
  newAllNodes.reserve(allNodesList.size());
  for (CollisionNode &node : allNodesList)
  {
    if (find_value_idx(nodesToRemove, node.nodeIndex) == -1)
    {
      node.insideOfNode = 0xffff;
      newAllNodes.push_back(node);
    }
  }
  newAllNodes.shrink_to_fit();
  allNodesList = eastl::move(newAllNodes);

  // Rebuild the staging verts / index staging in node order: for each surviving mesh/convex node,
  // append either the bucket-merged data (if the node is a merge target) or the existing slice from
  // the old pool. Stamp fresh offsets/counts on the node.
  dag::Vector<Point3_vec4> newStaging;
  dag::Vector<uint32_t> newIdxStaging;
  uint64_t totalV = 0, totalI = 0;
  for (CollisionNode &n : allNodesList)
  {
    if (n.hasGeometry()) // verticesCount means nothing for empty/non-mesh nodes
    {
      totalV += (uint32_t)n.verticesCount;
      totalI += n.indicesCount;
    }
  }
  for (const BucketMerge &bm : bucketMerges)
  {
    totalV += bm.verts.size();
    totalI += bm.indices.size();
  }
  newStaging.reserve((size_t)totalV);
  newIdxStaging.reserve((size_t)totalI);
  for (CollisionNode &n : allNodesList)
  {
    if (n.type != COLLISION_NODE_TYPE_MESH && n.type != COLLISION_NODE_TYPE_CONVEX)
      continue;
    const BucketMerge *bm = nullptr;
    for (const BucketMerge &candidate : bucketMerges)
      if (candidate.targetNodeIndex == n.nodeIndex)
      {
        bm = &candidate;
        break;
      }
    if (bm)
    {
      n.verticesOfs = (uint32_t)newStaging.size();
      n.verticesCount = (uint32_t)bm->verts.size();
      n.indicesOfs = (uint32_t)newIdxStaging.size();
      n.indicesCount = (uint32_t)bm->indices.size();
      newStaging.insert(newStaging.end(), bm->verts.begin(), bm->verts.end());
      newIdxStaging.insert(newIdxStaging.end(), bm->indices.begin(), bm->indices.end());
    }
    else if (n.hasGeometry())
    {
      const uint32_t newVOfs = (uint32_t)newStaging.size();
      const uint32_t newIOfs = (uint32_t)newIdxStaging.size();
      const Point3_vec4 *srcV = staging.data() + n.verticesOfs;
      const uint32_t *srcI = idxStaging.data() + n.indicesOfs;
      const uint32_t nVCount = (uint32_t)n.verticesCount;
      newStaging.insert(newStaging.end(), srcV, srcV + nVCount);
      newIdxStaging.insert(newIdxStaging.end(), srcI, srcI + n.indicesCount);
      n.verticesOfs = newVOfs;
      n.indicesOfs = newIOfs;
    }
  }
  staging = eastl::move(newStaging);
  idxStaging = eastl::move(newIdxStaging);
  staging.shrink_to_fit();
  idxStaging.shrink_to_fit();

  // Compact every nodeIndex-parallel array after bucket merging removes nodes.
  {
    const bool hasRelTms = (collisionFlags & COLLISION_RES_FLAG_HAS_REL_GEOM_NODE_ID) != 0;
    dag::Vector<TMatrix> compactedTm;
    dag::Vector<TMatrix> compactedAuthoredTm;
    dag::Vector<CollisionResourceInstance::PoseMeta> compactedMeta;
    compactedTm.reserve(allNodesList.size());
    compactedAuthoredTm.reserve(allNodesList.size());
    compactedMeta.reserve(allNodesList.size());
    Tab<TMatrix> compactedRelGeomNodeTms(tmpmem);
    if (hasRelTms)
      compactedRelGeomNodeTms.reserve(allNodesList.size());
    for (size_t i = 0; i < allNodesList.size(); i++)
    {
      const uint16_t oldIdx = allNodesList[i].nodeIndex;
      compactedTm.push_back(defaultInstance.nodeTm[oldIdx]);
      compactedAuthoredTm.push_back(authoredNodeTm[oldIdx]);
      compactedMeta.push_back(defaultInstance.poseMeta[oldIdx]);
      if (hasRelTms)
        compactedRelGeomNodeTms.push_back(relGeomNodeTms[oldIdx]);
      allNodesList[i].nodeIndex = (uint16_t)i;
    }
    defaultInstance.nodeTm = eastl::move(compactedTm);
    authoredNodeTm = eastl::move(compactedAuthoredTm);
    authoredNodeItm.clear(); // pre-compaction keying; the rebuildNodesLL below rebuilds it
    defaultInstance.poseMeta = eastl::move(compactedMeta);
    if (hasRelTms)
      relGeomNodeTms = eastl::move(compactedRelGeomNodeTms);
  }

  sortNodesList();
  rebuildNodesLL();

  gridForTraceable.reset();
  gridForCollidable.reset();

  // Decide whether TRACEABLE and PHYS_COLLIDABLE share the same node set so a single BLAS suffices.
  // This used to gate buildFRT (computed only when FRT was requested); the combined BLAS is now built
  // unconditionally below, so the decision must run regardless of need_frt -- otherwise an asset
  // exported with buildFRT=false never sets REUSE_TRACE_FRT and load builds two identical BLAS grids.
  recomputeTraceReuseFlagFromNodeSets();
  // Persist the BLAS cull mode so an optimized export restores it at load (BLAS is rebuilt, not
  // serialized; legacy HAS_*_FRT bits are stripped by the exporter). need_frt == "would have built a
  // two-sided CULL_BOTH FRT", so it is the two-sided marker -- the only on-disk cull signal new
  // exports carry; see the direct-load path in CollisionResource::load.
  if (need_frt)
    collisionFlags |= COLLISION_RES_FLAG_BLAS_TWO_SIDED;
  else
    collisionFlags &= ~COLLISION_RES_FLAG_BLAS_TWO_SIDED;
  // HAS_TRACE_FRT / HAS_COLL_FRT stay cleared -- new exports drop FRT blocks. The legacy exporter
  // (exp_collision.cpp) still tests these bits for old assets; that's preserved for compatibility.

  // Combined-per-behavior BLAS: the only runtime acceleration structure. Same gating as buildFRT
  // (IDENT mesh nodes, !SOLID, >= MIN_FACES_TO_CREATE_GRID). Built unconditionally (it accelerates
  // tracing regardless of need_frt), but its per-leaf cull mode mirrors the pre-BLAS behavior:
  // need_frt -> two-sided (CULL_BOTH) FRT, so trace two-sided; else the backface-culling per-node
  // path, so trace CullCCW. Preserves cull parity for need_frt=false assets instead of silently
  // making every BLAS asset two-sided.
  gridForTraceable.buildBLAS(this, make_span_const(staging), make_span_const(idxStaging), CollisionNode::TRACEABLE,
    /*two_sided*/ need_frt);
  if (!(collisionFlags & COLLISION_RES_FLAG_REUSE_TRACE_FRT))
    gridForCollidable.buildBLAS(this, make_span_const(staging), make_span_const(idxStaging), CollisionNode::PHYS_COLLIDABLE,
      /*two_sided*/ need_frt);
  G_UNUSED(frt_build_fast);

  if (raw_verts_out)
  {
    // Exporter raw workspace: hand the final raw verts + face indices to the caller. The export pipeline
    // re-reads and rewrites these spans after this (vert21 weld, Jolt validation, serialization) and
    // must see full-precision data so the shipped bits stay exact, so they are threaded through
    // explicitly rather than stored in the resource. No packing: the exporter object never traces.
    *raw_verts_out = eastl::move(staging);
    if (raw_indices_out)
      *raw_indices_out = eastl::move(idxStaging);
  }
  else
  {
    // Runtime: per-node BLAS chunks for big non-resident mesh nodes (needs the raw staging verts +
    // indices and the final grids), stamp grid membership + residency for nodes the grids absorbed,
    // pack the remainder as vert21 blocks, drop the staging (must run after BOTH buildBLAS calls,
    // same as the load() path).
    buildNodeBlasChunks(staging, idxStaging);
    stampBlasResidentNodes();
  }
}

// The SoA4 LeafRef encodes parent node offsets in 25 bits (32 MB) -- tighter than the 24-bit
// leaf-base gates, so a stackless tree in the 32..64 MB gap would pass those yet fail the SoA4
// conversion. Gating on stackless treeBytes is conservative: the converted tree is never larger.
static inline bool soa4_tree_fits_leaf_refs(int tree_bytes)
{
  return (int64_t)tree_bytes + (int64_t)soa4::LEAF_BYTES <= (int64_t)soa4::LEAF_ENTRY_OFS_MASK;
}

// Build a combined BLAS over every IDENT mesh node matching behavior_flag, flattening per-node
// geometry into one vertex+index stream fed to daBVH's SAH builder. A per-node leaf-order vertex
// renumber runs PER NODE only (never on the combined stream), preserving per-node vert21 contiguity so
// the trace dispatch recovers a leaf's source CollisionNode from its first vert21 index via
// blasNodeRanges. Contiguity is enforced by: (1) flattening verts node-by-node (each node a
// contiguous sub-range); (2) per-node QUAD_O_MAX over-spread dup at the tail of the same node's block;
// (3) confining all vertex reordering to per-node index spaces -- no combined-stream reorder runs
// (buildQuadPrims is triangle-order-independent), and a GLOBAL fetch-remap (which would shuffle verts
// across node boundaries) is never run.
//
// Per-CollisionNode storage (verticesOfs/indicesOfs) is NOT modified -- the caller's staging stays the source
// of truth for non-BLAS paths (capsule trace, FRT-empty fallbacks, intersection tests, public
// iterateNodeVerts). blasNodeRanges (sorted by verticesOfs, one entry per node)
// replaces the previous per-leaf blasLeafSrc table: the trace dispatch converts a leaf's first vert
// byte offset to a vert21 index and binary-searches to recover the source node. Subtri identity rides
// tri_ref's 2-bit sub-tri index (0..3); per-node face index is decoded on demand via getNodeFaceVertsByRef.
void CollisionResource::Grid::buildBLAS(CollisionResource *parent, dag::ConstSpan<Point3_vec4> raw_verts,
  dag::ConstSpan<uint32_t> raw_indices, uint8_t behavior_flag, bool two_sided)
{
  reset();

  // Cull mode the trace dispatch uses for this grid's BLAS. Set before the early-out gates so it is
  // well-defined even when no BLAS is built (the trace path gates on blasData.empty() either way).
  blasTwoSided = two_sided;

  if (!USE_TRACE_GRID || parent->meshNodesHead == CollisionNode::INVALID_IDX)
    return;

  // Per-node BLAS eligibility:
  //   - MESH only: CONVEX nodes are packed into their own per-node BLAS chunks, not the combined grid.
  //   - IDENT default pose only; non-IDENT nodes require per-node placement dispatch.
  //   - behavior_flag match + non-empty indices.
  auto isEligibleForBlas = [behavior_flag, parent](const CollisionNode *n) {
    return n->type == COLLISION_NODE_TYPE_MESH && n->checkBehaviorFlags(behavior_flag) &&
           (parent->getDefaultInstance().getPoseMeta(n->nodeIndex).flags & CollisionNode::IDENT) && n->indicesCount > 0;
  };

  bbox3f fullMeshBox;
  v_bbox3_init_empty(fullMeshBox);
  int totalIndices = 0;
  int totalVerts = 0;
  for (uint16_t mi = parent->meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = parent->allNodesList[mi].nextNode)
  {
    const CollisionNode *node = &parent->allNodesList[mi];
    if (!node->checkBehaviorFlags(behavior_flag))
      continue;
    if (!node->hasGeometry())
      continue; // degenerate-dropped node: no geometry to trace, and no reason to veto the grid below
    if (node->checkBehaviorFlags(CollisionNode::SOLID))
      return; // node requires trace without culling -- BLAS culls per-leaf; fall back to FRT/per-node path
    if (!isEligibleForBlas(node))
      continue;
    totalIndices += node->indicesCount;
    totalVerts += (int)node->verticesCount;
    v_bbox3_add_box(fullMeshBox, v_ldu_bbox3(node->modelBBox)); // IDENT -> node-local == resource-local
  }

  // Size gate identical to buildFRT (same minLeafSize / blockSize / xyzSum formula).
  const float minLeafSize = 0.25f;
  const int blockSize = 28;
  vec3f size = v_bbox3_size(fullMeshBox);
  vec3f checkMin = v_cmp_gt(size, v_splats(minLeafSize));
  int minTrue = v_count_true(checkMin);
  size = v_sel(V_C_ONE, size, checkMin);
  float vol = v_extract_x(v_hmul3(size));
  float facesCount = totalIndices / 3.f;
  float s = minTrue != 0 ? (powf(vol / (facesCount / blockSize), 1.f / minTrue) + 0.001f) // best leaf size
                         : v_extract_x(v_hmax3(size));
  vec3f vs = v_splats(max(s, minLeafSize));
  float xyzSum = v_extract_x(v_hadd3_x(v_div(size, vs)));
  if (facesCount < MIN_FACES_TO_CREATE_GRID || xyzSum <= MIN_WIDTH_TO_CREATE_GRID)
    return;

  // Flatten matching node geometry. Indices are rebased per node (node 0's verts at [0..vertCount0),
  // node 1's at [vertCount0..)). Each node is renumbered into leaf order in isolation and its QUAD_O_MAX dup runs
  // in the same loop, so the fetch reorder and dups stay inside the owning node's sub-range. CollisionNode
  // verticesOfs/indicesOfs are NOT modified -- the vert + index staging stays source of truth for the
  // per-node fallbacks; trace-time source-node lookup is via blasNodeRanges (filled below).
  dag::Vector<vec4f> allVerts;
  dag::Vector<unsigned> allIdx;
  // One source-node index per allVerts entry, filled in lockstep below. Constrains double-quad pairing
  // to a single source node so a leaf's first-vert -> node attribution (blas_src_node_for_leaf /
  // tri_ref) stays valid. Filled alongside the vertex append rather than zero-init + rescan.
  dag::Vector<uint32_t> vertGroup;
  // Headroom for QUAD_O_MAX dups -- near-zero on real assets after the leaf-order renumber (which often shrinks
  // the live set below totalVerts), so a comfortable upper bound.
  allVerts.reserve((size_t)totalVerts + (size_t)totalVerts / 16u);
  vertGroup.reserve((size_t)totalVerts + (size_t)totalVerts / 16u);
  allIdx.reserve((size_t)totalIndices);

  // Side table the trace dispatch binary-searches to recover a leaf's source node from its first
  // vert21 index (contiguity enforced by the flatten + per-node QUAD_O_MAX dup + per-node-only renumber).
  blasNodeRanges.reserve(parent->numMeshNodes);

  // Per-node renumber scratch, hoisted and reused. Default-allocator dag::Vector<vec4f> (as allVerts)
  // so the buffer has the 16-byte alignment v_ld / v_madd need; framemem avoided because these must
  // outlive the per-node framemem index scratch in declaration order.
  dag::Vector<vec4f> nodeVertsSrc, nodeVertsOpt;

  unsigned vertBase = 0;
  for (uint16_t mi = parent->meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = parent->allNodesList[mi].nextNode)
  {
    const CollisionNode *node = &parent->allNodesList[mi];
    if (!isEligibleForBlas(node))
      continue;
    const Point3_vec4 *nodeVerts = raw_verts.data() + node->verticesOfs;
    const uint32_t *nodeIdx = raw_indices.data() + node->indicesOfs; // build-time index staging (threaded in)
    const unsigned nodeVertCount = (unsigned)node->verticesCount;
    const unsigned nodeIdxCount = (unsigned)node->indicesCount;

    // Phase 1: renumber this node's verts IN ISOLATION into SAH-leaf order, then duplicate the residual
    // over-spread triangles, both purely inside the node's [0, nodeVertCount) block so its verts stay one
    // contiguous range (the per-node vert21 contiguity the BLAS <-> source-node mapping rests on).
    // leafOrderVertexFetch runs its own SAH triangle partition, so it tightens the QUAD_O_MAX windows
    // regardless of input index order -- no vertex-cache pre-pass needed. Without it an index-incoherent
    // node over-spreads and the post-dup block can blow past 65536.
    dag::Vector<unsigned, framemem_allocator> localIdx((size_t)nodeIdxCount);
    for (unsigned i = 0; i < nodeIdxCount; ++i)
      localIdx[i] = (unsigned)nodeIdx[i];
    // Defensive: a malformed asset can index past the node's vert block, which would drive an
    // out-of-bounds read/write in leafOrderVertexFetch. Leave the node out of the grid; the
    // non-resident pass (buildNodeBlasChunks -> buildOneNodeBlasChunk) then drops it.
    bool malformedIdx = false;
    for (unsigned i = 0; i < nodeIdxCount && !malformedIdx; ++i)
      malformedIdx = localIdx[i] >= nodeVertCount;
    if (malformedIdx)
    {
      logerr("collision node #%u: source index out of range (>= %u verts); excluded from grid BLAS", (unsigned)node->nodeIndex,
        nodeVertCount);
      continue;
    }
    nodeVertsSrc.resize(nodeVertCount);
    for (unsigned i = 0; i < nodeVertCount; ++i)
      nodeVertsSrc[i] = v_ld(&nodeVerts[i].x);
    // SAH-leaf-order renumber (drops unreferenced verts, keeps co-leaf verts adjacent) + shared
    // window-block over-spread dup, both per node so the node's verts stay one contiguous block.
    const unsigned nodeVertCountOpt =
      build_bvh::leafOrderVertexFetch(localIdx.data(), nodeIdxCount, nodeVertsSrc.data(), nodeVertCount, nodeVertsOpt);

    // Phase 2: append the reordered + dup'd verts (localIdx is now node-local into nodeVertsOpt).
    const unsigned nodeVertStart = (unsigned)allVerts.size();
    for (unsigned i = 0; i < nodeVertCountOpt; ++i)
    {
      allVerts.push_back(nodeVertsOpt[i]);
      vertGroup.push_back(node->nodeIndex); // same source node for this whole appended block
    }

    // Phase 4: rebase indices to combined-stream space and append. Count the faces that survive
    // buildQuadPrims' degenerate-drop (duplicate-index triangles): the combined builder drops exactly
    // these, quad-merging never drops a face, so the emitted count = distinct-index source faces. The
    // rebase is a constant per-node shift, so index equality (degeneracy) is identical in localIdx.
    uint32_t emittedFaces = 0;
    for (unsigned i = 0; i + 2 < nodeIdxCount; i += 3)
    {
      allIdx.push_back(vertBase + localIdx[i]);
      allIdx.push_back(vertBase + localIdx[i + 1]);
      allIdx.push_back(vertBase + localIdx[i + 2]);
      if (localIdx[i] != localIdx[i + 1] && localIdx[i + 1] != localIdx[i + 2] && localIdx[i] != localIdx[i + 2])
        ++emittedFaces;
    }

    // Phase 5: record this node's vert21-array range (srcNodeForLeaf maps a leaf's first vert21 index
    // back to a node). NodeRange offsets are uint32 and verticesCount is uint32, so a post-dup span
    // over 65536 traces and goes resident like any other node -- nothing special to gate.
    // Skip a fully-degenerate node (every triangle dropped): recording a zero-face range would mark it
    // grid-resident with indicesCount==0 in stampBlasResidentNodes and hide it from the per-node chunk
    // path. Leaving it out routes it to buildNodeBlasChunks, which logs and drops it like any degenerate
    // node. The orphaned verts already appended stay unreferenced (buildQuadPrims dropped the faces).
    if (emittedFaces > 0)
    {
      const unsigned postDupVertCount = (unsigned)allVerts.size() - nodeVertStart;
      NodeRange nr{};
      nr.verticesOfs = vertBase;
      nr.verticesEnd = vertBase + postDupVertCount;
      nr.facesCount = emittedFaces;
      nr.nodeIndex = node->nodeIndex;
      blasNodeRanges.push_back(nr);
    }

    vertBase = (unsigned)allVerts.size();
  }

  if (allIdx.size() < 3)
  {
    // Keep blasNodeRanges in sync with blasData: no triangles -> no side-table entries.
    blasNodeRanges.clear();
    return; // nothing to build
  }

  // No combined-stream reorder: buildQuadPrims pairs via an edge hash map (triangle order irrelevant)
  // and the SAH tree re-partitions, while a GLOBAL vertex-fetch renumber would shuffle verts across node
  // boundaries and break the per-node vert21 contiguity blasNodeRanges relies on. Vertex order is set
  // per node in the flatten loop above.

  const int faceCount = (int)allIdx.size() / 3;

  Tab<build_bvh::QuadPrim> prims;
  int qc = 0, sc = 0;
  build_bvh::buildQuadPrims(prims, qc, sc, allIdx.data(), faceCount, allVerts.data());
  if (prims.empty())
  {
    blasNodeRanges.clear(); // empty-grid invariant -- see comment above
    return;
  }

  // Pair quads into double-quad leaves (up to 4 tris/leaf), constrained to the same source node via
  // vertGroup (filled in lockstep with allVerts above) so a leaf never straddles two source nodes.
  dag::Vector<build_bvh::DoubleQuadPrim> dqPrims;
  build_bvh::buildDoubleQuadPrims(dqPrims, prims.data(), (int)prims.size(), allVerts.data(), vertGroup.data());

  dag::Vector<bbox3f> primBoxes(dqPrims.size());
  build_bvh::addDoubleQuadPrimitivesAABBList(primBoxes.data(), dqPrims.data(), (int)dqPrims.size(), allVerts.data());

  Tab<bbox3f> nodes;
  int maxDepth = 0;
  build_bvh::create_bvh_node_sah(nodes, primBoxes.data(), (uint32_t)dqPrims.size(), 4, maxDepth);

  // Quantization frame for both the BVH inner-node bboxes and the packed vert21 positions. safeSize
  // floors degenerate axes at blas_size_eps so the encoding stays well-defined for axis-flat meshes.
  blasBBox = fullMeshBox;
  vec3f safeSize = v_max(v_sub(fullMeshBox.bmax, fullMeshBox.bmin), v_splats(build_bvh::blas_size_eps));
  blasScale = v_div(v_splats(65535.f), safeSize);
  blasOfs = v_neg(v_mul(fullMeshBox.bmin, blasScale));
  blasInvScale = v_rcp(blasScale); // cached once; every vert21 decode path reads g.blasInvScale

  const int packedVertCnt = (int)allVerts.size();
  const int treeBytes = build_bvh::calcBLASTreeBytes((int)nodes.size(), (int)dqPrims.size());
  // The double-quad leaf stores each apex base as an unsigned 24-bit byte offset >> 2. If the
  // [tree][pad][vert21] span pushes a base past that range, writeDoubleQuad* would clamp it and the
  // leaf would trace against the wrong vertices. Abandon the grid BLAS (collision falls back to the
  // per-node path, gated on blasData.empty()) rather than emit corrupt geometry; pre-passed meshes
  // stay far under this (~64 MB/BLAS). vert21 starts at the 8-aligned blasVertsOfs() (padded
  // treeBytes), so the farthest leaf base is blasVertsOfs() + (vertCount-1)*8 -- gate on the padded
  // offset, not bare treeBytes, or a base sitting at the 24-bit edge would still clamp.
  const int64_t vertsBegin = (int64_t)((treeBytes + 7) & ~7); // stackless vert offset (== stkVertsOfs below)
  if (vertsBegin + (int64_t)(packedVertCnt - 1) * 8 > (int64_t)QUAD_BASE_BYTE_MAX)
  {
    logerr("collision grid BLAS span (tree %d B + %d verts) exceeds the unsigned 24-bit leaf base range; "
           "skipping BLAS for this resource (per-node trace fallback)",
      treeBytes, packedVertCnt);
    blasNodeRanges.clear(); // empty-grid invariant: blasData stays empty, so the caller's
                            // stampBlasResidentNodes marks no node grid-resident (matches the abandons above)
    return;
  }
  if (!soa4_tree_fits_leaf_refs(treeBytes))
  {
    logerr("collision grid BLAS tree (%d B) exceeds the SoA4 LeafRef 32 MB node offset range; "
           "skipping BLAS for this resource (per-node trace fallback)",
      treeBytes);
    blasNodeRanges.clear(); // empty-grid invariant, as the abandon above
    return;
  }
  const size_t vertBytes = (size_t)packedVertCnt * 8u;
  // vert21 stream padded to 8 for natural alignment of the 8-byte slots; not required for
  // correctness: the byte-offset -> index recoveries (MOC, and the leaf -> source-node mapping)
  // compute (apexByteOfs - blasVertsOfs()) / 8, so the stream base cancels and any 4-aligned base
  // works (the vert21 load is unaligned).
  const uint32_t stkVertsOfs = alignVert21StreamOfs((uint32_t)treeBytes);
  dag::Vector<uint8_t> stkBuf((size_t)stkVertsOfs + vertBytes, 0);

  int dataOffset = 0;
  build_bvh::writeDoubleQuadBVH2(stkBuf.data(), nodes.data(), dqPrims.data(), blasScale, blasOfs,
    /*vertDataOfs*/ (int)stkVertsOfs, 0, 0, dataOffset);
  G_ASSERTF(dataOffset == treeBytes, "Grid::buildBLAS: tree wrote %d bytes, expected %d", dataOffset, treeBytes);

  uint8_t *vertDst = stkBuf.data() + stkVertsOfs;
  for (int i = 0; i < packedVertCnt; ++i)
    build_bvh::packVert21(vertDst + i * 8, v_madd(allVerts[i], blasScale, blasOfs));

  const soa4::ConvertResult cr = soa4::buildFromStackless(stkBuf.data(), 0, treeBytes, (int)stkVertsOfs, (int)vertBytes, blasData);
  if (!cr.valid())
  {
    // Structurally impossible for a tree this builder just wrote (the converter round-trips 1:1);
    // if it ever fires, refuse the BLAS loudly rather than trace corrupt geometry.
    logerr("collision grid BLAS: SoA4 conversion failed (tree %d B, %d verts); skipping BLAS (per-node trace fallback)", treeBytes,
      packedVertCnt);
    blasData.clear();
    blasData.shrink_to_fit();
    blasNodeRanges.clear(); // empty-grid invariant, as the abandons above
    return;
  }
  blasTreeBytes = (uint32_t)cr.treeBytes;
  blasRootRef = cr.root;

  // blasNodeRanges (filled in the flatten loop, sorted by verticesOfs = insertion order) is the only
  // per-node side table kept: the trace dispatch binary-searches it to recover a leaf's source node
  // from the first vert21 index, and walkBlasResidentNodeLeavesForFaces uses it to filter leaves to a
  // node's [verticesOfs, verticesEnd) range. No per-leaf offset table -- the BLAS walk is the index.
}

// Find a node's NodeRange by nodeIndex. blasNodeRanges is small (~50) and sorted by verticesOfs, not
// nodeIndex, so this scans linearly.
static const CollisionResource::Grid::NodeRange *find_blas_range_for_node(const CollisionResource::Grid &g, uint16_t node_index)
{
  for (const auto &r : g.blasNodeRanges)
    if (r.nodeIndex == node_index)
      return &r;
  return nullptr;
}

// Pack one node's raw slice as an ownVerts21 block: [float bmin[3]][float invScale[3]][vert21 x N], and
// return the EXACT pack scale (the chunk header stores it for the bit-exact q-space trace transform).
// Per-node quantization frame over the slice (same 16.5 fixed-point encoding as the grid, see
// packVert21). Per node -- NOT a resource-level box: with a resource frame, a small Jolt-fed node
// inside a large resource would quantize on a grid orders of magnitude coarser than the
// per-node-referenced-bounds grid validateVerticesForJolt cleared at export, merging verts that
// Jolt's own quantization then rejects as degenerate.
vec3f CollisionResource::packOwnVerts21Block(uint8_t *block, const Point3_vec4 *verts, uint32_t count)
{
  bbox3f bb;
  v_bbox3_init_empty(bb);
  for (uint32_t i = 0; i < count; ++i)
    v_bbox3_add_pt(bb, v_ld(&verts[i].x));
  const vec3f safeSize = v_max(v_bbox3_size(bb), v_splats(build_bvh::blas_size_eps));
  const vec3f scale = v_div(v_splats(65535.f), safeSize);
  const vec3f qOfs = v_neg(v_mul(bb.bmin, scale));
  const vec3f invScale = v_rcp(scale); // exact IEEE division (v_div(1, x)), same as blasInvScale
  v_stu_p3((float *)block, bb.bmin);
  v_stu_p3((float *)block + 3, invScale);
  uint8_t *vDst = block + OWN_VERTS21_HEADER_BYTES;
  for (uint32_t i = 0; i < count; ++i)
    build_bvh::packVert21(vDst + (size_t)i * 8u, v_madd(v_ld(&verts[i].x), scale, qOfs));
  return scale;
}

// Owning-mode grid stamping, the shared tail of load / loadLegacyRawFormat / collapseAndOptimize.
//
// Stamps the GRID_TRACEABLE/GRID_PHYS membership flags from each grid's ranges (the trace dispatch
// tests the WALKED grid's flag to skip nodes the grid walk covers -- residency cannot answer that,
// being keyed off the AUTHORITATIVE grid, which can differ, e.g. a TRACEABLE|PHYS_COLLIDABLE node
// whose collidable grid failed the size gate). Then reinterprets verticesOfs/Count for resident
// nodes (isGridResident: the authoritative grid absorbed them) -- the "other" grid would never be
// looked up by iterateNodeFaces / getNodeFaceVerts / capsule fallback. Empty grids stamp nothing.
// Verts are NOT packed here: buildNodeBlasChunks already wrote every non-resident node's vert21
// block into nodeBlasData, and residents live in the grid. The resource keeps no source-face index
// list -- every node enumerates faces from its BLAS.
void CollisionResource::stampBlasResidentNodes()
{
  // Clear first, then re-derive in full: flags arrive from disk / raw dumps, and collapse rebuilds
  // the grids from scratch. Zero-span ranges (cannot happen; defensive) stamp nothing, so a set
  // membership bit always implies a valid resident reinterpretation below. AUTH_GRID_PHYS freezes
  // the authoritative-grid choice at this point: later behaviorFlags mutation (ECS node flag rules)
  // must not re-route the storage decode.
  for (CollisionNode &n : allNodesList)
  {
    n.flags &= ~(CollisionNode::ANY_GRID_RESIDENT | CollisionNode::AUTH_GRID_PHYS);
    if (nodeAuthGridIsPhys(n))
      n.flags |= CollisionNode::AUTH_GRID_PHYS;
  }
  for (const Grid::NodeRange &nr : gridForTraceable.blasNodeRanges)
    if (nr.nodeIndex < allNodesList.size() && nr.verticesEnd > nr.verticesOfs)
      allNodesList[nr.nodeIndex].flags |= CollisionNode::GRID_TRACEABLE;
  for (const Grid::NodeRange &nr : gridForCollidable.blasNodeRanges)
    if (nr.nodeIndex < allNodesList.size() && nr.verticesEnd > nr.verticesOfs)
      allNodesList[nr.nodeIndex].flags |= CollisionNode::GRID_PHYS;

  for (CollisionNode &n : allNodesList)
  {
    if (n.type != COLLISION_NODE_TYPE_MESH && n.type != COLLISION_NODE_TYPE_CONVEX)
      continue;
    if (!n.hasGeometry())
      continue;
    if (!isGridResident(n))
      continue; // authoritative grid did not absorb it; the per-node chunk stays the vert storage
    // Reinterpret from the same grid the resolver uses at runtime, keeping stamp and resolve in
    // sync. Values come straight from the current ranges, so a repeated stamp rewrites the same
    // data (idempotent by value). verticesCount is uint32, so a post-dup span over 65536 is fine.
    const Grid &g = getBlasGridForResidentNode(n);
    if (const Grid::NodeRange *nr = find_blas_range_for_node(g, n.nodeIndex))
    {
      n.verticesOfs = nr->verticesOfs;
      n.verticesCount = nr->verticesEnd - nr->verticesOfs;
      n.indicesCount = nr->facesCount * 3u; // emitted (post degenerate-drop) count, known at grid build
    }
  }

  // indicesCount is the emitted (post degenerate-drop) face count: the grid stamp above and
  // buildOneNodeBlasChunk both set it from the builder, so it already equals what iterateNodeFaces
  // emits. getNodeFaceCount() reads indicesCount/3 and trace materialises the same walk, so a stale
  // count would make those consumers over-read. In dev builds, walk and verify (curing on mismatch);
  // release trusts the build-time count and skips the per-node walk entirely.
#if DAGOR_DBGLEVEL > 0
  for (CollisionNode &n : allNodesList)
  {
    if (n.type != COLLISION_NODE_TYPE_MESH && n.type != COLLISION_NODE_TYPE_CONVEX)
      continue;
    if (!n.hasGeometry())
      continue;
    if (!isGridResident(n) && n.nodeBlasOfs == ~0u)
      continue; // no BLAS storage to walk (e.g. empty-staging reload) -- leave the count as-is
    uint32_t faces = 0;
    iterateNodeFaces((int)n.nodeIndex, [&](int, uint32_t, uint32_t, uint32_t) { ++faces; });
    if (n.indicesCount != faces * 3u)
    {
      logerr("collision node #%u: cached indicesCount %u != BLAS walk %u faces; curing", (unsigned)n.nodeIndex, n.indicesCount,
        faces * 3u);
      n.indicesCount = faces * 3u;
    }
  }
#endif
}

// ===== per-node BLAS chunks =====
// A quad-BLAS over a single node's triangles, stored as [NodeBlasChunkHeader][quad tree][the
// node's ownVerts21 block] in nodeBlasData, so leaf hits on big NON-grid-resident mesh/convex
// nodes (animated resources, capsule traces -- nothing the combined behavior grids cover) can
// descend a BVH instead of brute-forcing the whole triangle list. The chunked node's vert21 block
// lives inside the chunk INSTEAD of ownVerts21 (single storage; getPackedNodeVerts21 resolves the
// base through node.nodeBlasOfs), so the net memory cost is the tree alone.
//
// Drift-free by construction: the block is packed from the same raw staging slice (same
// packOwnVerts21Block frame math), and the tree is built IN THE BLOCK'S OWN q-SPACE -- writeQuadBVH2
// is fed the unpacked q values with scale=1/ofs=0, so packVert21(round(q)) == q exactly and inner-node
// boxes are exact integer bounds. The header stores the EXACT pack scale (not rcp(block invScale)) so
// the trace-time ray transform into q-space reproduces the build frame bit-for-bit.
void CollisionResource::buildNodeBlasChunks(dag::ConstSpan<Point3_vec4> staging, dag::ConstSpan<uint32_t> staging_indices)
{
  G_STATIC_ASSERT(sizeof(NodeBlasChunkHeader) == 24);
  // Runtime owning-mode loads only: chunks need the raw staging slices (node.verticesOfs/indicesOfs are
  // still staging element indices here). The exporter raw workspace and stampBlasResidentNodes never reach
  // this. The guard runs BEFORE the clear: a chunked node's vert block lives inside nodeBlasData, so
  // clearing without rebuilding would destroy live storage.
  if (staging.empty())
    return;
  nodeBlasData.clear();
  for (CollisionNode &n : allNodesList)
    n.nodeBlasOfs = ~0u;
  // Full rebuild relocates every chunk offset: bump the generation so any per-node BLAS tri_ref minted
  // against a prior layout is rejected, uniform with compactNodeBlasData() / bakeNodeTransform().
  ++nodeBlasBuildId;

  // Every non-grid-resident mesh/convex node gets a per-node BLAS chunk -- no size floor, so the
  // owning vertex storage is always a chunk (or a grid), never a bare ownVerts21 block.
  // 16-byte-aligned scratch (see the combined buildBLAS note on allocator choice).
  dag::Vector<vec4f> nodeVertsSrc, nodeVertsOpt;
  dag::Vector<Point3_vec4> packSrc;
  dag::Vector<vec4f> qVerts;
  dag::Vector<uint8_t> stkTmp, soaOut;

  for (uint16_t mi = meshNodesHead; mi != CollisionNode::INVALID_IDX; mi = allNodesList[mi].nextNode)
  {
    CollisionNode &node = allNodesList[mi];
    if (node.type != COLLISION_NODE_TYPE_MESH && node.type != COLLISION_NODE_TYPE_CONVEX)
      continue;
    // A degenerate-dropped node (indicesCount==0) keeps stale verticesOfs/verticesCount; building a chunk
    // would read past its (absent) staging slice in a mixed resource. It has no geometry -- skip it.
    if (!node.hasGeometry())
      continue;
    // Skip nodes the grids absorb (residents already have combined-BVH coverage; chunking them would
    // double their vert storage). Mirrors stampBlasResidentNodes' residency criterion; the AUTH bit
    // is not stamped yet, so derive the authoritative grid from behavior flags like the stamp will.
    if (find_blas_range_for_node(nodeAuthGridIsPhys(node) ? gridForCollidable : gridForTraceable, node.nodeIndex))
      continue;
    if (!buildOneNodeBlasChunk(node, staging.data() + node.verticesOfs, (unsigned)node.verticesCount,
          staging_indices.data() + node.indicesOfs, (unsigned)node.indicesCount, nodeVertsSrc, nodeVertsOpt, packSrc, qVerts, stkTmp,
          soaOut))
    {
      // Rejected chunk (reason logerr'd by the builder): drop its geometry -- no collision surface.
      node.indicesCount = 0;
    }
  }
  nodeBlasData.shrink_to_fit();
}

// Build a per-node BLAS chunk from the node's raw verts + indices: appends [NodeBlasChunkHeader]
// [quad tree][vert21 block] to nodeBlasData (the connectivity is baked into the tree; nodeIdx is read
// but not written back -- the SAH-leaf renumber + QUAD_O over-spread dup run on a local copy) and sets
// node.nodeBlasOfs/verticesOfs/verticesCount. Returns false (node left unchunked, reason logerr'd here)
// on a degenerate node (no quad prims), a malformed out-of-range index, or a chunk span past the 24-bit
// leaf base range; the post-dup vert count is otherwise uncapped. Scratch buffers are caller-provided
// for reuse across a node-list loop.
bool CollisionResource::buildOneNodeBlasChunk(CollisionNode &node, const Point3_vec4 *nodeVerts, unsigned nodeVertCount,
  const uint32_t *nodeIdx, unsigned nodeIdxCount, dag::Vector<vec4f> &nodeVertsSrc, dag::Vector<vec4f> &nodeVertsOpt,
  dag::Vector<Point3_vec4> &packSrc, dag::Vector<vec4f> &qVerts, dag::Vector<uint8_t> &stkTmp, dag::Vector<uint8_t> &soaOut)
{
  // SAH-leaf renumber + QUAD_O over-spread dup: everything stays inside this node's index space,
  // so the dup guard and the signed 13-bit quad-leaf offsets behave identically to the combined buildBLAS flatten.
  dag::Vector<unsigned, framemem_allocator> localIdx((size_t)nodeIdxCount);
  for (unsigned i = 0; i < nodeIdxCount; ++i)
    localIdx[i] = (unsigned)nodeIdx[i];
  // Defensive: a malformed asset can hold an index past the node's vert block; leafOrderVertexFetch
  // would read srcVerts and write its renumber table out of bounds. Drop the node.
  for (unsigned i = 0; i < nodeIdxCount; ++i)
    if (localIdx[i] >= nodeVertCount)
    {
      logerr("collision node <%s>#%u: source index out of range (>= %u verts); dropping", getNodeNameStr(node),
        (unsigned)node.nodeIndex, nodeVertCount);
      return false;
    }
  nodeVertsSrc.resize(nodeVertCount);
  for (unsigned i = 0; i < nodeVertCount; ++i)
    nodeVertsSrc[i] = v_ld(&nodeVerts[i].x);
  // SAH-leaf-order renumber + shared window-block over-spread dup (build_bvh): leafOrderVertexFetch
  // runs its own SAH partition (no vertex-cache pre-pass), matching vert order to the leaf grouping.
  const unsigned postDup =
    build_bvh::leafOrderVertexFetch(localIdx.data(), nodeIdxCount, nodeVertsSrc.data(), nodeVertCount, nodeVertsOpt);

  packSrc.resize(postDup);
  for (unsigned i = 0; i < postDup; ++i)
    v_st(&packSrc[i].x, nodeVertsOpt[i]);
  // No 65536 cap: the index staging is uint32 and the leaf encoding addresses each vert by absolute byte
  // offset (the signed 13-bit leaf field only bounds the dup'd intra-leaf spread, not the vert count), so a
  // heavily-dup'd chunk past 65536 verts writes its connectivity back without truncation.

  // Pack the block (it computes the per-node frame and returns the exact pack scale for the chunk
  // header -- bmin lives only in the block), then unpack q values for the tree build.
  dag::Vector<uint8_t, framemem_allocator> blockTmp;
  blockTmp.resize(OWN_VERTS21_HEADER_BYTES + (size_t)postDup * 8u);
  const vec3f scale = packOwnVerts21Block(blockTmp.data(), packSrc.data(), postDup);
  qVerts.resize(postDup);
  const uint8_t *q8 = blockTmp.data() + OWN_VERTS21_HEADER_BYTES;
  for (unsigned i = 0; i < postDup; ++i)
    qVerts[i] = RayData::unpackVert21(q8 + (size_t)i * 8u);

  Tab<build_bvh::QuadPrim> prims;
  int qc = 0, sc = 0;
  build_bvh::buildQuadPrims(prims, qc, sc, localIdx.data(), (int)(nodeIdxCount / 3u), qVerts.data());
  if (prims.empty())
  {
    logerr("collision node <%s>#%u: no buildable geometry (degenerate); dropping", getNodeNameStr(node), (unsigned)node.nodeIndex);
    return false;
  }
  // Emitted face count is known here (quad = 2 tris, single = 1; degenerate faces were dropped). Stamp
  // the node's cached count so consumers reading indicesCount/3 match the leaf walk without re-deriving.
  node.indicesCount = (uint32_t)(qc * 2 + sc) * 3u;
  // Pair quads into double-quad leaves (up to 4 tris/leaf). Single node, so pairing is unconstrained --
  // every vert belongs to this node, so no vert_group is passed (cf. the grid buildBLAS, which must).
  dag::Vector<build_bvh::DoubleQuadPrim> dqPrims;
  build_bvh::buildDoubleQuadPrims(dqPrims, prims.data(), (int)prims.size(), qVerts.data());
  dag::Vector<bbox3f> primBoxes(dqPrims.size());
  build_bvh::addDoubleQuadPrimitivesAABBList(primBoxes.data(), dqPrims.data(), (int)dqPrims.size(), qVerts.data());
  Tab<bbox3f> bvhNodes;
  int maxDepth = 0;
  build_bvh::create_bvh_node_sah(bvhNodes, primBoxes.data(), (uint32_t)dqPrims.size(), 4, maxDepth);
  const int treeBytes = build_bvh::calcBLASTreeBytes((int)bvhNodes.size(), (int)dqPrims.size());
  // Pad the tree region up to 8 before the vert21 block so the stream is 8-aligned (same invariant as
  // the grid's blasVertsOfs); ownVertsBlockPtr / the chunk trace recompute this from hdr->treeBytes.
  const uint32_t alignedTree = alignVert21StreamOfs((uint32_t)treeBytes);

  // Same unsigned 24-bit leaf-base gate as the grid BLAS / writeDoubleQuadBLAS: past QUAD_BASE_BYTE_MAX
  // packQuadA emits degenerate no-hit leaves, so traces would silently miss those triangles. This path
  // is the grid's oversized fallback, so it must reject the node rather than write a corrupt chunk.
  if ((int64_t)alignedTree + (int64_t)OWN_VERTS21_HEADER_BYTES + (int64_t)(postDup - 1) * 8 > (int64_t)QUAD_BASE_BYTE_MAX)
  {
    logerr("collision node <%s>#%u: per-node BLAS span (tree %d B + %u verts) exceeds the unsigned 24-bit leaf base range; dropping",
      getNodeNameStr(node), (unsigned)node.nodeIndex, treeBytes, postDup);
    return false;
  }
  if (!soa4_tree_fits_leaf_refs(treeBytes))
  {
    logerr("collision node <%s>#%u: per-node BLAS tree (%d B) exceeds the SoA4 LeafRef 32 MB node offset range; dropping",
      getNodeNameStr(node), (unsigned)node.nodeIndex, treeBytes);
    return false;
  }

  // The converter treats the whole ownVerts21 block (24 B header + vert21s) as its verbatim "vert
  // region": real leaf apexes land past the header, 8-aligned (the header is 24 B), so base
  // re-pointing stays exact.
  stkTmp.assign((size_t)alignedTree + blockTmp.size(), 0);
  int dataOffset = 0;
  // q-space identity frame; vert array (8 B vert21s) sits past the tree AND the 24 B block header,
  // both inside the region the trace walk addresses from the tree base.
  build_bvh::writeDoubleQuadBVH2(stkTmp.data(), bvhNodes.data(), dqPrims.data(), V_C_ONE, v_zero(),
    /*vertDataOfs*/ (int)alignedTree + (int)OWN_VERTS21_HEADER_BYTES, 0, 0, dataOffset);
  G_ASSERTF(dataOffset == treeBytes, "node BLAS chunk: tree wrote %d bytes, expected %d", dataOffset, treeBytes);
  memcpy(stkTmp.data() + alignedTree, blockTmp.data(), blockTmp.size());

  const soa4::ConvertResult cr = soa4::buildFromStackless(stkTmp.data(), 0, treeBytes, (int)alignedTree, (int)blockTmp.size(), soaOut);
  if (!cr.valid())
  {
    logerr("collision node <%s>#%u: SoA4 conversion failed (tree %d B); dropping", getNodeNameStr(node), (unsigned)node.nodeIndex,
      treeBytes);
    return false;
  }

  const uint32_t chunkOfs = (uint32_t)nodeBlasData.size();
  nodeBlasData.resize(chunkOfs + sizeof(NodeBlasChunkHeader) + soaOut.size(), 0);
  uint8_t *chunk = nodeBlasData.data() + chunkOfs;
  NodeBlasChunkHeader *hdr = (NodeBlasChunkHeader *)chunk;
  v_stu_p3(hdr->scale, scale);
  hdr->treeBytes = (uint32_t)cr.treeBytes;
  hdr->rootRef = cr.root;
  hdr->_resv = 0;
  memcpy(chunk + sizeof(NodeBlasChunkHeader), soaOut.data(), soaOut.size());

  // The reordered connectivity (localIdx) is baked into the chunk tree above; nodeIdx is not written
  // back (every caller discards it -- the chunk is the node's storage from here on).
  node.nodeBlasOfs = chunkOfs;
  node.verticesOfs = 0;
  node.verticesCount = (uint32_t)postDup;
  return true;
}
