// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>

#include <gameRes/dag_collisionResource.h>
#include <scene/dag_physMat.h>
#include <vecmath/dag_vecMath.h>

// Legacy (SW masked-occlusion-culling) occluder feeder. CollisionResources keep verts
// vert21-packed in per-node BLAS chunk blocks; faces are read from the chunk tree. Handled by
// consumer lifetime:
//  - withNodeMeshData feeds a *synchronous* consumer: materialises verts + indices into framemem
//    scratch for the call.
//  - addRasterizationTasks builds *async* tasks holding raw pointers that outlive the call:
//    each node feeds a RenderBlasSOA4 task over its resource-stable per-node BLAS chunk
//    (walk-and-emit, no caller index list).
void CollisionGeometryFeeder::withNodeMeshData(const CollisionResource &coll_res, int node_id, const NodeMeshConsumer &cb)
{
  // cb consumes the framemem pointers synchronously (NodeMeshConsumer contract: raw pointers valid only during the call).
  dag::Vector<Point3_vec4, framemem_allocator> matVerts;
  // 32-bit indices: a per-node BLAS chunk (heavy QUAD_O1 over-spread dup) can exceed 65536 verts. The
  // sole consumer (X-ray vertex cache) uploads the full node mesh into a 32-bit index buffer, so feed all
  // faces -- dropping triangles would render a partial damage mesh and trip its index-count assert.
  dag::Vector<uint32_t, framemem_allocator> matIdx;
  coll_res.materializeNodeMesh(node_id, matVerts, matIdx);
  if (matVerts.empty() || matIdx.empty())
    return;
  cb(matVerts.data(), (int)matVerts.size(), sizeof(Point3_vec4), matIdx.data(), (int)matIdx.size(), sizeof(uint32_t));
}

void CollisionGeometryFeeder::addRasterizationTasks(const CollisionResource &coll_res, mat44f_cref worldviewproj,
  eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &out_tasks, uint32_t triangles_partition, bool allow_convex)
{
  const auto allNodes = coll_res.getAllNodes();

  for (int ni = 0, ne = (int)allNodes.size(); ni < ne; ++ni)
  {
    const CollisionNode *node = coll_res.getNode(ni);
    if (!node || !node->hasGeometry())
      continue;
    if (!(node->type == COLLISION_NODE_TYPE_MESH || (allow_convex && node->type == COLLISION_NODE_TYPE_CONVEX)))
      continue;
    if (!node->checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;
    // The task covers the whole node, so one glass material (phys-collidable window) drops it:
    // missing an occluder is conservative, culling behind glass is not.
    if (coll_res.anyNodeMaterialPasses(*node, [](int m) { return IsPhysMatID_Valid(m) && PhysMat::getMaterial(m).lightTransparent; }))
      continue;
    // a mirrored/singular live pose hides the node from CPU traces; rasterizing it would be
    // wrong CULLING (flipped frustum), not conservative overdraw -- same gate as the SWRT feeder
    if (!coll_res.getDefaultInstance().isNodeTraceable(node->nodeIndex))
      continue;
    if (!coll_res.hasNodeBlas(ni))
      continue; // defensive: an occluder node with geometry always has a per-node chunk
    // The node's verts AND quad-BVH tree live in its resource-stable per-node BLAS chunk, which
    // satisfies the async task lifetime -- feed it to MOC RenderBlasSOA4. The decode frame -- and the node tm
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
    // Slice the node's triangles into triangles_partition RenderBlasSOA4 sub-jobs (triSkip/tri_count).
    // soa4Root routes to the SoA4 walker; vertOffset locates the vert21
    // stream past the 8-padded tree.
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
