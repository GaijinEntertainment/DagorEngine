// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>

#include <debug/dag_log.h>
#include <gameRes/dag_collisionResource.h>
#include <vecmath/dag_vecMath.h>

// Legacy (SW masked-occlusion-culling) occluder feeder. BLAS-resident nodes drop their
// ownVertices/ownIndices; verts live in the grid's vert21 array (8 B/vert). Handled by consumer
// lifetime:
//  - withNodeMeshData feeds a *synchronous* consumer: materialises resident verts/indices into
//    framemem scratch for the call.
//  - addRasterizationTasks builds *async* tasks holding a raw float pointer that outlives the call;
//    on-the-fly decode can't satisfy that without owning the buffer, so resident nodes are covered
//    by the whole-grid RenderBLAS task (MOC RenderBLAS reads the stable grid blasData directly) and
//    skipped in the raw-pointer per-node loop. A resident node with no covering task is dropped
//    with LOGERR_ONCE (conservative overdraw; does not happen for rendinst occluder content).
void CollisionGeometryFeeder::withNodeMeshData(const CollisionResource &coll_res, int node_id, const NodeMeshConsumer &cb)
{
  auto verts = coll_res.getNodeVertices(node_id);
  auto idxs = coll_res.getNodeIndices(node_id);
  if (!verts.empty())
  {
    cb(verts.data(), (int)verts.size(), sizeof(Point3_vec4), idxs.data(), (int)idxs.size(), sizeof(uint16_t));
    return;
  }
  // BLAS-resident node: ownVertices/ownIndices were dropped, so the getters return empty. cb
  // consumes the pointers synchronously (NodeMeshConsumer contract: raw pointers valid only during
  // the call, may reference transient decoded scratch), so materialise verts+indices from the grid's
  // vert21 array into framemem scratches and feed those. Without this, resident geometry (e.g.
  // damage-model armor in the X-ray hit-cam cache) produced zero-vertex entries and rendered nothing.
  const CollisionNode *n = coll_res.getNode(node_id);
  if (!n || !(n->flags & CollisionNode::BLAS_RESIDENT) || n->indicesCount == 0)
    return; // genuinely empty node
  dag::Vector<Point3_vec4, framemem_allocator> matVerts;
  matVerts.reserve((size_t)n->verticesCount + 1u);
  coll_res.iterateNodeVerts(node_id, [&](int, vec4f v) {
    Point3_vec4 p;
    v_st(&p.x, v);
    matVerts.push_back(p);
  });
  dag::Vector<uint16_t, framemem_allocator> matIdx;
  matIdx.reserve(n->indicesCount);
  coll_res.iterateNodeFaces(node_id, [&](int, uint16_t i0, uint16_t i1, uint16_t i2) {
    matIdx.push_back(i0);
    matIdx.push_back(i1);
    matIdx.push_back(i2);
  });
  if (matVerts.empty() || matIdx.empty())
    return;
  cb(matVerts.data(), (int)matVerts.size(), sizeof(Point3_vec4), matIdx.data(), (int)matIdx.size(), sizeof(uint16_t));
}

void CollisionGeometryFeeder::addRasterizationTasks(const CollisionResource &coll_res, mat44f_cref worldviewproj,
  eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &out_tasks, uint32_t triangles_partition, bool allow_convex)
{
  const vec4f bmin = v_ld(&coll_res.boundingBox[0].x);
  const vec4f bmax = v_ldu(&coll_res.boundingBox[1].x);
  const auto allNodes = coll_res.getAllNodes();

  // BLAS-resident mesh occluders feed MOC RenderBLAS, which walks the combined-per-behavior vert21
  // quad-BVH directly. The grid's blasData is a stable (resource-lifetime) pointer, satisfying the
  // task's async lifetime -- which is why these nodes can't use the raw-float-pointer triangle path
  // (their ownVertices were dropped). The per-node loop below skips BLAS_RESIDENT nodes.
  // Gate: the combined BLAS can't exclude individual nodes, and a FLAG_TRANSPARENT occluder must not
  // write depth, so skip the BLAS task entirely if any covered node is transparent (dropping the
  // resident occluders -- conservative overdraw, never wrong culling; transparent collision is rare.
  // Non-resident covered nodes still rasterize through the raw loop below).
  const CollisionResource::Grid &occlGrid = coll_res.getBlasGrid(CollisionNode::TRACEABLE);
  bool blasTaskSubmitted = false;
  if (!occlGrid.blasData.empty())
  {
    bool anyTransparentResident = false;
    uint32_t blasTriCount = 0;
    for (const auto &nr : occlGrid.blasNodeRanges)
    {
      const CollisionNode *rn = coll_res.getNode(nr.nodeIndex);
      if (!rn)
        continue;
      if (rn->checkBehaviorFlags(CollisionNode::FLAG_TRANSPARENT))
        anyTransparentResident = true;
      blasTriCount += rn->indicesCount / 3u;
    }
    if (!anyTransparentResident && blasTriCount > 0)
    {
      blasTaskSubmitted = true;
      // rawToClip = worldviewproj * (raw21 -> resource-local). MOC fetches BLAS verts via
      // unpackVert21Raw -> [0..2097120] = 32 * box-space[0..65535], so the linear part is
      // (blasInvScale / 32) per axis and the translation is blasBBox.bmin (matches the runtime vert21
      // decode and dag_swTLAS_ray.h). The combined BLAS is axis-aligned (one quant frame for all
      // resident nodes), so there is no per-node rotation to fold in.
      const vec3f s = v_mul(occlGrid.blasInvScale, v_splats(1.0f / 32.0f));
      const float sx = v_extract_x(s), sy = v_extract_y(s), sz = v_extract_z(s);
      mat44f raw2local;
      raw2local.col0 = v_make_vec4f(sx, 0.f, 0.f, 0.f);
      raw2local.col1 = v_make_vec4f(0.f, sy, 0.f, 0.f);
      raw2local.col2 = v_make_vec4f(0.f, 0.f, sz, 0.f);
      raw2local.col3 = v_perm_xyzd(occlGrid.blasBBox.bmin, v_splats(1.0f));
      mat44f rawToClip;
      v_mat44_mul43(rawToClip, worldviewproj, raw2local);
      // Slice the BLAS triangle range into triangles_partition-sized RenderBLAS sub-jobs via
      // triSkip/triLimit so workers parallelize one resource's occluders with a bounded per-job index
      // cache. treeStart/treeEnd span the whole tree ([0, blasTreeBytes)); bmin/bmax are the
      // raw-vert21 extent ([0..2097120]) the rawToClip frustum test operates in.
      const uint32_t partition = triangles_partition ? triangles_partition : blasTriCount;
      for (uint32_t triStart = 0; triStart < blasTriCount; triStart += partition)
      {
        ParallelOcclusionRasterizer::RasterizationTaskData task;
        task.viewproj = rawToClip;
        task.bmin = v_zero();
        task.bmax = v_splats(2097120.f);
        task.blasData = occlGrid.blasData.data();
        task.treeStart = 0; //-V1048 intentional: whole-tree range [treeStart, treeEnd), paired with treeEnd below
        task.treeEnd = occlGrid.blasTreeBytes;
        task.triSkip = triStart;
        const uint32_t remaining = blasTriCount - triStart;
        task.tri_count = partition < remaining ? partition : remaining;
        out_tasks.emplace_back(task);
      }
    }
  }

  for (int ni = 0, ne = (int)allNodes.size(); ni < ne; ++ni)
  {
    const CollisionNode *node = coll_res.getNode(ni);
    if (!node || !node->indicesCount)
      continue;
    if (!(node->type == COLLISION_NODE_TYPE_MESH || (allow_convex && node->type == COLLISION_NODE_TYPE_CONVEX)))
      continue;
    if (!node->checkBehaviorFlags(CollisionNode::TRACEABLE) || node->checkBehaviorFlags(CollisionNode::FLAG_TRANSPARENT))
      continue;
    // Covered by the whole-grid RenderBLAS task above: nothing to emit here (emitting again would
    // double-rasterize). Membership in blasNodeRanges -- not BLAS_RESIDENT -- is the right key: a
    // node with post-dup vert span > 65536 keeps its NodeRange but stays non-resident (retains
    // ownVertices), yet its triangles are in the submitted task all the same.
    if (blasTaskSubmitted)
    {
      bool inBlas = false;
      for (const auto &nr : occlGrid.blasNodeRanges)
        if (nr.nodeIndex == node->nodeIndex)
        {
          inBlas = true;
          break;
        }
      if (inBlas)
        continue;
    }
    if (node->flags & CollisionNode::BLAS_RESIDENT)
    {
      // Resident node with NO covering RenderBLAS task: its raw verts were dropped at load and the
      // async task struct needs pointers that outlive this call, so the occluder is dropped --
      // conservative overdraw, never wrong culling. Occluders are rendinst traceable collision after
      // collapseAndOptimize, where the traceable grid is built and covers every resident node;
      // reaching here means unusual content (traceable grid not built while the node is resident in
      // gridForCollidable, or the task was dropped over a transparent covered node) -- shout once.
      LOGERR_ONCE("collision occluder: dropping BLAS-resident node <%s> with no covering RenderBLAS task",
        coll_res.getNodeName(node->nodeIndex));
      continue;
    }

    auto nodeVerts = coll_res.getNodeVertices(ni);
    auto nodeIdx = coll_res.getNodeIndices(ni);
    if (nodeVerts.empty())
      continue; // defensive (shouldn't trigger after the BLAS_RESIDENT skip above)
    const float *const vertsPtr = (const float *)nodeVerts.data();
    const uint16_t *const facesPtr = nodeIdx.data();
    const uint32_t faceCount = (uint32_t)nodeIdx.size() / 3;

    if ((node->flags & (CollisionNode::IDENT | CollisionNode::TRANSLATE)) == CollisionNode::IDENT)
    {
      for (uint32_t i = 0; i < faceCount; i += triangles_partition)
        out_tasks.emplace_back(ParallelOcclusionRasterizer::RasterizationTaskData{
          worldviewproj, bmin, bmax, vertsPtr, facesPtr + i * 3, min(faceCount - i, triangles_partition)});
    }
    else
    {
      mat44f nodeTm;
      v_mat44_make_from_43ca(nodeTm, coll_res.getNodeTm(ni)[0]);
      v_mat44_mul43(nodeTm, worldviewproj, nodeTm);
      alignas(16) BBox3 nodeBBox = coll_res.getNodeBBox(ni);
      const vec4f nbmin = v_ld(&nodeBBox[0].x);
      const vec4f nbmax = v_ldu(&nodeBBox[1].x);
      for (uint32_t i = 0; i < faceCount; i += triangles_partition)
        out_tasks.emplace_back(ParallelOcclusionRasterizer::RasterizationTaskData{
          nodeTm, nbmin, nbmax, vertsPtr, facesPtr + i * 3, min(faceCount - i, triangles_partition)});
    }
  }
}
