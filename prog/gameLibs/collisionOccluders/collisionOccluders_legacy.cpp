// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <collisionGeometryFeeder/collisionGeometryFeeder.h>

#include <gameRes/dag_collisionResource.h>
#include <vecmath/dag_vecMath.h>

void CollisionGeometryFeeder::withNodeMeshData(const CollisionResource &coll_res, int node_id, const NodeMeshConsumer &cb)
{
  const CollisionNode *n = coll_res.getNode(node_id);
  if (!n)
    return;
  cb(n->vertices.data(), (int)n->vertices.size(), sizeof(Point3_vec4), n->indices.data(), (int)n->indices.size(), sizeof(uint16_t));
}

void CollisionGeometryFeeder::addRasterizationTasks(const CollisionResource &coll_res, mat44f_cref worldviewproj,
  eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &out_tasks, uint32_t triangles_partition, bool allow_convex)
{
  const vec4f bmin = v_ld(&coll_res.boundingBox[0].x);
  const vec4f bmax = v_ldu(&coll_res.boundingBox[1].x);
  const auto allNodes = coll_res.getAllNodes();
  for (int ni = 0, ne = (int)allNodes.size(); ni < ne; ++ni)
  {
    const CollisionNode *node = coll_res.getNode(ni);
    if (!node || !node->indices.size())
      continue;
    if (!(node->type == COLLISION_NODE_TYPE_MESH || (allow_convex && node->type == COLLISION_NODE_TYPE_CONVEX)))
      continue;
    if (!node->checkBehaviorFlags(CollisionNode::TRACEABLE))
      continue;

    const float *const vertsPtr = (const float *)node->vertices.data();
    const uint16_t *const facesPtr = node->indices.data();
    const uint32_t faceCount = (uint32_t)node->indices.size() / 3;

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
