// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>

#include <debug/dag_log.h>
#include <gameRes/dag_collisionResource.h>
#include <vecmath/dag_vecMath.h>
#include <EASTL/fixed_vector.h>

// Legacy (SW masked-occlusion-culling) occluder feeder. Owning CollisionResources keep verts
// vert21-packed: BLAS-resident nodes in the grid's vert21 array (8 B/vert), every other mesh/convex
// node as a per-node BLAS chunk block. Faces are read from the BLAS. Handled by consumer lifetime:
//  - withNodeMeshData feeds a *synchronous* consumer: materialises verts (+ resident indices) into
//    framemem scratch for the call.
//  - addRasterizationTasks builds *async* tasks holding raw pointers that outlive the call;
//    on-the-fly decode can't satisfy that without owning the buffer, so resident nodes are covered
//    by the whole-grid RenderBlasSOA4 task (MOC RenderBlasSOA4 reads the stable grid blasData directly) and
//    skipped in the per-node loop, while non-resident nodes feed a RenderBlasSOA4 task over their own
//    resource-stable per-node BLAS chunk (same walk-and-emit, no caller index list). A resident node
//    with no covering task is dropped with LOGERR_ONCE (conservative overdraw; does not happen for
//    rendinst occluder content).
void CollisionGeometryFeeder::withNodeMeshData(const CollisionResource &coll_res, int node_id, const NodeMeshConsumer &cb)
{
  // Owning resources keep verts vert21-packed (per-node BLAS chunk, or the grid for BLAS-resident
  // nodes), so materialise verts+indices via the dispatch-aware iterators into framemem scratches and
  // feed those. cb consumes the pointers synchronously (NodeMeshConsumer contract: raw pointers valid
  // only during the call, may reference transient decoded scratch).
  const CollisionNode *n = coll_res.getNode(node_id);
  if (!n || !n->hasGeometry())
    return; // genuinely empty node
  dag::Vector<Point3_vec4, framemem_allocator> matVerts;
  matVerts.reserve((size_t)n->verticesCount);
  coll_res.iterateNodeVerts(node_id, [&](int, vec4f v) {
    Point3_vec4 p;
    v_st(&p.x, v);
    matVerts.push_back(p);
  });
  // 32-bit indices: a per-node BLAS chunk (heavy QUAD_O1 over-spread dup) can exceed 65536 verts. The
  // sole consumer (X-ray vertex cache) uploads the full node mesh into a 32-bit index buffer, so feed all
  // faces -- dropping triangles would render a partial damage mesh and trip its index-count assert.
  dag::Vector<uint32_t, framemem_allocator> matIdx;
  matIdx.reserve(n->indicesCount);
  coll_res.iterateNodeFaces(node_id, [&](int, uint32_t i0, uint32_t i1, uint32_t i2) {
    matIdx.push_back(i0);
    matIdx.push_back(i1);
    matIdx.push_back(i2);
  });
  if (matVerts.empty() || matIdx.empty())
    return;
  cb(matVerts.data(), (int)matVerts.size(), sizeof(Point3_vec4), matIdx.data(), (int)matIdx.size(), sizeof(uint32_t));
}

void CollisionGeometryFeeder::addRasterizationTasks(const CollisionResource &coll_res, mat44f_cref worldviewproj,
  eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &out_tasks, uint32_t triangles_partition, bool allow_convex)
{
  const auto allNodes = coll_res.getAllNodes();

  // BLAS-resident mesh occluders feed MOC RenderBlasSOA4, which walks the combined-per-behavior vert21
  // quad-BVH directly. The grid's blasData is a stable (resource-lifetime) pointer, satisfying the
  // task's async lifetime -- which is why these nodes can't use the raw-float-pointer triangle path
  // (they have no ownVerts21 block). The per-node loop below skips grid-resident nodes.
  // Gate: the combined BLAS can't exclude individual nodes, and a FLAG_TRANSPARENT occluder must not
  // write depth, so skip the BLAS task entirely if any covered node is transparent (dropping the
  // resident occluders -- conservative overdraw, never wrong culling; transparent collision is rare.
  // Non-resident covered nodes still rasterize through the raw loop below).
  const CollisionResource::Grid &occlGrid = coll_res.getBlasGrid(CollisionNode::TRACEABLE);
  bool blasTaskSubmitted = false;
  // bind-grid bytes are valid only while every grid member holds its seed pose (same latch the
  // SWRT feeder gates on): a setNodeTm-posed member must not rasterize at its bind placement --
  // that is wrong CULLING, not just overdraw. Resident nodes then drop in the loop below.
  if (!occlGrid.blasData.empty() && coll_res.getDefaultInstance().isGridResidentPoseAtBind())
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
      // Slice the BLAS triangle range into triangles_partition-sized RenderBlasSOA4 sub-jobs via
      // triSkip/triLimit so workers parallelize one resource's occluders with a bounded per-job index
      // cache. soa4Root routes the job to the SoA4 walker; bmin/bmax are the raw-vert21 extent
      // ([0..2097120]) the rawToClip frustum test operates in.
      const uint32_t partition = triangles_partition ? triangles_partition : blasTriCount;
      for (uint32_t triStart = 0; triStart < blasTriCount; triStart += partition)
      {
        ParallelOcclusionRasterizer::RasterizationTaskData task;
        task.viewproj = rawToClip;
        task.bmin = v_zero();
        task.bmax = v_splats(2097120.f);
        task.blasData = occlGrid.blasData.data();
        task.vertOffset = occlGrid.blasVertsOfs();
        task.soa4Root = occlGrid.blasRootRef; // routes the job to RenderBlasSOA4 (grid trees are SoA4)
        task.triSkip = triStart;
        const uint32_t remaining = blasTriCount - triStart;
        task.tri_count = partition < remaining ? partition : remaining;
        out_tasks.emplace_back(task);
      }
    }
  }

  // Precompute BLAS-task membership as one bit per node (keyed by nodeIndex) so the per-node loop
  // tests O(1). blasNodeRanges is sorted by verticesOfs (not nodeIndex), so a per-node linear scan
  // would be O(nodes * ranges) every frame. Bit-packed in inline fixed_vector storage: a resource
  // with up to 256 nodes (almost all -- most have under 32) needs no allocation; framemem overflow
  // covers the rare larger one.
  eastl::fixed_vector<uint32_t, 8, true, framemem_allocator> coveredByBlasTask;
  const uint32_t nodeCount = (uint32_t)allNodes.size();
  if (blasTaskSubmitted)
  {
    coveredByBlasTask.resize((nodeCount + 31u) / 32u, 0);
    for (const auto &nr : occlGrid.blasNodeRanges)
      if (nr.nodeIndex < nodeCount)
        coveredByBlasTask[nr.nodeIndex >> 5] |= 1u << (nr.nodeIndex & 31u);
  }

  for (int ni = 0, ne = (int)allNodes.size(); ni < ne; ++ni)
  {
    const CollisionNode *node = coll_res.getNode(ni);
    if (!node || !node->hasGeometry())
      continue;
    if (!(node->type == COLLISION_NODE_TYPE_MESH || (allow_convex && node->type == COLLISION_NODE_TYPE_CONVEX)))
      continue;
    if (!node->checkBehaviorFlags(CollisionNode::TRACEABLE) || node->checkBehaviorFlags(CollisionNode::FLAG_TRANSPARENT))
      continue;
    // a mirrored/singular live pose hides the node from CPU traces; rasterizing it would be
    // wrong CULLING (flipped frustum), not conservative overdraw -- same gate as the SWRT feeder
    if (!coll_res.getDefaultInstance().isNodeTraceable(node->nodeIndex))
      continue;
    // Covered by the whole-grid RenderBlasSOA4 task above: nothing to emit here (emitting again would
    // double-rasterize). Membership in blasNodeRanges is the key: a node in this grid's ranges has
    // its triangles in the submitted task, whatever its post-dup vert span (no 65536 ceiling now).
    if (
      blasTaskSubmitted && node->nodeIndex < nodeCount && ((coveredByBlasTask[node->nodeIndex >> 5] >> (node->nodeIndex & 31u)) & 1u))
      continue;
    if (coll_res.isGridResident(*node))
    {
      // posed grid member (cleared latch): the bind BLAS bytes are stale and the raw verts were
      // dropped at load, so the occluder is dropped -- conservative overdraw, never wrong culling
      if (!coll_res.getDefaultInstance().isGridResidentPoseAtBind())
        continue;
      // Resident node with NO covering RenderBlasSOA4 task: its raw verts were dropped at load and the
      // async task struct needs pointers that outlive this call, so the occluder is dropped --
      // conservative overdraw, never wrong culling. Occluders are rendinst traceable collision after
      // collapseAndOptimize, where the traceable grid is built and covers every resident node;
      // reaching here means unusual content (traceable grid not built while the node is resident in
      // gridForCollidable, or the task was dropped over a transparent covered node) -- shout once.
      LOGERR_ONCE("collision occluder: dropping BLAS-resident node <%s> with no covering RenderBlasSOA4 task",
        coll_res.getNodeName(node->nodeIndex));
      continue;
    }
    if (!coll_res.hasNodeBlas(ni))
      continue; // defensive: a non-resident occluder with geometry always has a per-node chunk
    // Owning resource: the node's verts AND quad-BVH tree live in its resource-stable per-node BLAS
    // chunk, which satisfies the async task lifetime -- feed it to MOC RenderBlasSOA4 exactly like the
    // whole-grid task above (walk-and-emit; no caller index list). The decode frame -- and the node tm
    // for non-IDENT nodes -- folds into the task matrix: linear part invScale/32 per axis (MOC fetches
    // via unpackVert21Raw, [0..2097120] = 32 * box-space), translation = the block's bmin. The frame IS
    // the node-slice bbox, so the frustum pretest range [0, 2097120]^3 is exactly the node's bounds.
    const CollisionResource::NodeOccluderBlas chunk = coll_res.getNodeOccluderBlas(*node);
    const vec3f s = v_mul(chunk.invScale, v_splats(1.0f / 32.0f));
    const float sx = v_extract_x(s), sy = v_extract_y(s), sz = v_extract_z(s);
    mat44f raw2local;
    raw2local.col0 = v_make_vec4f(sx, 0.f, 0.f, 0.f);
    raw2local.col1 = v_make_vec4f(0.f, sy, 0.f, 0.f);
    raw2local.col2 = v_make_vec4f(0.f, 0.f, sz, 0.f);
    raw2local.col3 = v_perm_xyzd(chunk.bmin, v_splats(1.0f));
    mat44f rawToClip;
    if (coll_res.isIdentNode(ni))
      v_mat44_mul43(rawToClip, worldviewproj, raw2local);
    else
    {
      mat44f nodeTm;
      v_mat44_make_from_43ca(nodeTm, coll_res.getNodeTm(ni)[0]);
      v_mat44_mul43(nodeTm, worldviewproj, nodeTm);
      v_mat44_mul43(rawToClip, nodeTm, raw2local);
    }
    // Slice the node's triangles into triangles_partition RenderBlasSOA4 sub-jobs (triSkip/tri_count),
    // same as the whole-grid task. soa4Root routes to the SoA4 walker; vertOffset locates the vert21
    // stream past the tree and the 24 B block header.
    const uint32_t faceCount = node->indicesCount / 3u;
    const uint32_t partition = triangles_partition ? triangles_partition : faceCount;
    for (uint32_t triStart = 0; triStart < faceCount; triStart += partition)
    {
      ParallelOcclusionRasterizer::RasterizationTaskData task;
      task.viewproj = rawToClip;
      task.bmin = v_zero();
      task.bmax = v_splats(2097120.f);
      task.blasData = chunk.blasData;
      task.vertOffset = chunk.vertOffset;
      task.soa4Root = chunk.rootRef; // routes the job to RenderBlasSOA4 (chunk trees are SoA4)
      task.triSkip = triStart;
      const uint32_t remaining = faceCount - triStart;
      task.tri_count = partition < remaining ? partition : remaining;
      out_tasks.emplace_back(task);
    }
  }
}
