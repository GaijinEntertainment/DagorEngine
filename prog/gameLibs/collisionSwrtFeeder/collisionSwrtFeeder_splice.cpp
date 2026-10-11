// Copyright (C) Gaijin Games KFT.  All rights reserved.

// The RenderSWRT-free half of the chunk splice, so the collision unit suite can pin the byte math
// without linking the SWRT runtime; the feeder adds the box-resemblance score and registers the result.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>

#include <daBVH/dag_bvhBuild.h>
#include <daBVH/dag_bvhSerialization.h>
#include <daBVH/dag_bvhReencode.h>
#include <daBVH/dag_swBLAS_leaf.h>        // RayData::unpackVert21 (vert reencode)
#include <daBVH/dag_swBLAS_soa4Convert.h> // soa4::buildStackless (chunks store the SoA4 CPU layout)
#include <daSWRT/swBVH.h>
#include <daSWRT/swBLASBoxResemblance.h> // BlasBoxEncoding (the producer names its own byte format)
#include <debug/dag_assert.h>
#include <debug/dag_log.h>
#include <gameRes/dag_collisionResource.h>
#include <vecmath/dag_vecMath.h>
#include <daBVH/swBLASLeafDefs.hlsli>

// The filter crosses the conversion as a keep mask over the 6-bit leaf user values (the OWNING
// node's palette; one material per leaf): folded once, so the conversion's two passes agree.
// Every value resolves through getNodePhysMatId, the one home of the past-palette rule (it
// answers PHYSMAT_INVALID, as at trace time); each distinct id is asked once, PHYSMAT_INVALID included.
static uint64_t foldKeepMask(const CollisionResource &coll_res, int node_index, const CollisionGeometryFeeder::PhysMatFilter &mat_pred)
{
  static_assert(CollisionResource::MAX_NODE_PHYS_MATS <= 64, "the leaf user-value domain must fit the keep mask");
  if (!mat_pred)
    return ~0ull;
  uint64_t keepMask = 0;
  int invalidKept = -1; // PHYSMAT_INVALID's verdict, asked on first use
  for (uint32_t v = 0; v < CollisionResource::MAX_NODE_PHYS_MATS; ++v)
  {
    const int id = coll_res.getNodePhysMatId(node_index, (int)v);
    if (id == PHYSMAT_INVALID && invalidKept < 0)
      invalidKept = mat_pred((int16_t)PHYSMAT_INVALID) ? 1 : 0;
    const bool kept = id == PHYSMAT_INVALID ? invalidKept != 0 : mat_pred((int16_t)id);
    keepMask |= kept ? 1ull << v : 0ull;
  }
  return keepMask;
}

// A box model has no leaves: a mixed transparency keeps them, and so does a nonzero user value,
// unless box_user lets the box carry the one value all its faces share.
static bool spliceAsBox(const CollisionResource &coll_res, int node_index, const CollisionGeometryFeeder::PhysMatFilter &mat_pred,
  const CollisionGeometryFeeder::PhysMatLeafUser &leaf_user, bool box_user, daSWRT::BuiltBLAS &built)
{
  mat44f tm;
  v_mat44_make_from_43cu_unsafe(tm, coll_res.getNodeTm(node_index)[0]);
  const bool ident = coll_res.isIdentNode(node_index);
  vec4f verts[8];
  int vertCount = 0;
  coll_res.iterateNodeVerts(node_index, [&](int, vec4f v) { verts[vertCount++] = ident ? v : v_mat44_mul_vec3p(tm, v); });
  uint32_t indices[12 * 3];
  int indexCount = 0;
  uint8_t anyUser = 0, everyUser = 0xFF;
  coll_res.iterateNodeFacesWithLeafMaterial(node_index, [&](int, uint32_t i0, uint32_t i1, uint32_t i2, int mat) {
    if (mat_pred && !mat_pred((int16_t)mat))
      return;
    if (indexCount <= 12 * 3 - 3)
    {
      indices[indexCount + 0] = i0;
      indices[indexCount + 1] = i1;
      indices[indexCount + 2] = i2;
    }
    indexCount += 3;
    const uint8_t user = leaf_user ? leaf_user((int16_t)mat) & (QUAD_LEAF_USER_MASK | QUAD_LEAF_USER_TRANSPARENT) : 0;
    anyUser |= user;
    everyUser &= user;
  });
  const bool allTransparent = indexCount > 0 && (everyUser & QUAD_LEAF_USER_TRANSPARENT);
  const uint8_t value = anyUser & QUAD_LEAF_USER_MASK;
  const bool oneValue = box_user && value == (everyUser & QUAD_LEAF_USER_MASK);
  if ((value && !oneValue) || (!allTransparent && (anyUser & QUAD_LEAF_USER_TRANSPARENT)))
    return false;
  const bbox3f box = build_bvh::calcBox(verts, vertCount);
  if (!build_bvh::checkIfIsBox(indices, indexCount, verts, vertCount, box))
    return false;
  built.box = box;
  built.treeBytes = 0;
  built.lightTransparent = allTransparent;
  built.boxUser = value;
  return true;
}

bool CollisionGeometryFeeder::buildSwrtChunkSpliceBLAS(const CollisionResource &coll_res, dag::ConstSpan<int> node_indices,
  daSWRT::BuiltBLAS &built, ChunkSpliceInfo &out_info, const PhysMatFilter &mat_pred, const PhysMatLeafUser &leaf_user, bool box_user)
{
  // This producer reencodes the tree boxes below, so IT names the score decoder.
  out_info.boxEncoding = daSWRT::BlasBoxEncoding::Fp16;
  built.data.clear();
  if (node_indices.size() == 1 && coll_res.getNodeVertCount(node_indices[0]) == 8 &&
      spliceAsBox(coll_res, node_indices[0], mat_pred, leaf_user, box_user, built))
    return true;
  struct Chunk
  {
    dag::Vector<uint8_t> bytes;
    mat44f qToResource;
    int nodeIndex, treeBytes, vertsOfs, vertCount;
  };
  dag::Vector<Chunk> chunks;
  chunks.reserve(node_indices.size());
  v_bbox3_init_empty(built.box);
  int treeBytes = 0, vertCount = 0;
  for (const int nodeIndex : node_indices)
  {
    // Resolved HERE so a node of another resource is unrepresentable (the chunk offsets index
    // coll_res's own storage).
    const CollisionNode *node = coll_res.getNode(nodeIndex);
    if (!node || !coll_res.hasNodeBlas(nodeIndex))
      return false;
    const CollisionResource::NodeOccluderBlas src = coll_res.getNodeOccluderBlas(*node);
    const int nodeVerts = coll_res.getNodeVertCount(nodeIndex);
    Chunk &chunk = chunks.push_back();
    const soa4::StacklessResult rt = soa4::buildStackless(src.blasData, src.rootRef, (int)src.vertOffset, nodeVerts * 8, chunk.bytes,
      foldKeepMask(coll_res, nodeIndex, mat_pred));
    if (!rt.valid())
    {
      chunks.pop_back();
      if (rt.status == soa4::StacklessResult::Status::AllCarved)
        continue;
      // Structurally impossible for a validly built chunk (the conversion round-trips 1:1); degrade
      // loudly to "no SWRT model for this resource", never a corrupt upload -- filter or not.
      logerr("swrtFeeder: SoA4->stackless conversion failed for the node chunk; no SWRT model");
      return false;
    }
    G_ASSERT(rt.treeBytes > 0 && nodeVerts > 0);
    mat44f tm;
    v_mat44_make_from_43cu_unsafe(tm, coll_res.getNodeTm(nodeIndex)[0]);
    bbox3f frame;
    frame.bmin = src.bmin;
    frame.bmax = v_madd(src.invScale, v_splats(65535.f), src.bmin);
    if (!coll_res.isIdentNode(nodeIndex))
      v_bbox3_init(frame, tm, frame);
    v_bbox3_add_box(built.box, frame);
    chunk.qToResource.col0 = v_mul(tm.col0, v_splat_x(src.invScale));
    chunk.qToResource.col1 = v_mul(tm.col1, v_splat_y(src.invScale));
    chunk.qToResource.col2 = v_mul(tm.col2, v_splat_z(src.invScale));
    chunk.qToResource.col3 = v_mat44_mul_vec3p(tm, src.bmin);
    chunk.nodeIndex = nodeIndex;
    chunk.treeBytes = rt.treeBytes;
    chunk.vertsOfs = rt.vertsOfs; // == align8(treeBytes), the conversion's documented layout
    chunk.vertCount = nodeVerts;
    treeBytes += rt.treeBytes;
    vertCount += nodeVerts;
  }
  if (chunks.empty())
    return false; // the filter carved every leaf: legitimately no SWRT model

  built.vertsFp16 = true;
  const int vertsOfs = (treeBytes + 7) & ~7;
  if ((int64_t)vertsOfs + (int64_t)vertCount * 8 > QUAD_BASE_BYTE_MAX)
    return false;
  built.treeBytes = (uint32_t)treeBytes;
  built.data.resize_noinit(vertsOfs + vertCount * 8);
  memset(built.data.data() + treeBytes, 0, vertsOfs - treeBytes);

  // constraint: writeTLASLeaf decodes the model with this same floored extent.
  const vec4f halfU = v_mul(V_C_HALF, v_max(v_bbox3_size(built.box), v_splats(build_bvh::blas_size_eps)));
  const vec4f centerU = v_bbox3_center(built.box);
  uint8_t *dst = built.data.data();
  built.lightTransparent = static_cast<bool>(leaf_user);
  int treeAt = 0, vertAt = vertsOfs;
  for (const Chunk &c : chunks)
  {
    memcpy(dst + treeAt, c.bytes.data(), c.treeBytes);
    mat44f qToNorm;
    qToNorm.col0 = v_div(c.qToResource.col0, halfU);
    qToNorm.col1 = v_div(c.qToResource.col1, halfU);
    qToNorm.col2 = v_div(c.qToResource.col2, halfU);
    qToNorm.col3 = v_div(v_sub(c.qToResource.col3, centerU), halfU);
    const uint32_t baseDelta = (uint32_t)(vertAt - c.vertsOfs - treeAt) >> QUAD_BASE_ALIGN_SHIFT;
    uint8_t userOf[CollisionResource::MAX_NODE_PHYS_MATS];
    for (uint32_t v = 0; leaf_user && v < CollisionResource::MAX_NODE_PHYS_MATS; ++v)
      userOf[v] =
        leaf_user((int16_t)coll_res.getNodePhysMatId(c.nodeIndex, (int)v)) & (QUAD_LEAF_USER_MASK | QUAD_LEAF_USER_TRANSPARENT);
    for (int ofs = treeAt, end = treeAt + c.treeBytes; ofs < end;)
    {
      uint32_t encWord;
      memcpy(&encWord, dst + ofs + 12, sizeof(uint32_t));   // QUAD_LEAF_FLAG + quad/single encoding live at +12
      build_bvh::reencodeBoxNodeToFP16(dst + ofs, qToNorm); // box uint16 [0,65535] -> FP16 [-1,1]; preserves +12
      if (encWord & QUAD_LEAF_FLAG)
      {
        uint8_t *w1w3 = dst + ofs + BVH_BLAS_NODE_SIZE;
        uint32_t w[3];
        memcpy(w, w1w3, sizeof(w));
        w[0] += (w[0] & QUAD_BASE_MASK) ? baseDelta : 0u; // base 0: a no-hit leaf
        if (leaf_user)
        {
          const uint8_t u = userOf[w[2] >> QUAD_LEAF_USER_SHIFT];
          w[1] |= (u & QUAD_LEAF_USER_TRANSPARENT) ? QUAD_LEAF_TRANSPARENT_FLAG : 0u;
          w[2] = (w[2] & ~(QUAD_LEAF_USER_MASK << QUAD_LEAF_USER_SHIFT)) | (uint32_t)(u & QUAD_LEAF_USER_MASK) << QUAD_LEAF_USER_SHIFT;
          built.lightTransparent &= (u & QUAD_LEAF_USER_TRANSPARENT) != 0;
        }
        memcpy(w1w3, w, sizeof(w));
      }
      ofs += (encWord & QUAD_LEAF_FLAG) ? BVH_BLAS_LEAF_SIZE : BVH_BLAS_NODE_SIZE;
    }

    const uint8_t *srcVert = c.bytes.data() + c.vertsOfs;
    for (uint8_t *dstVert = dst + vertAt, *end = dstVert + c.vertCount * 8; dstVert < end; dstVert += 8, srcVert += 8)
      build_bvh::writeGpuBlasVert(dstVert, v_mat44_mul_vec3p(qToNorm, RayData::unpackVert21(srcVert)), /*fp16*/ true);
    treeAt += c.treeBytes;
    vertAt += c.vertCount * 8;
  }
  return true;
}
